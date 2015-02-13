#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mpi.h"

//pthread functions we will wrap
int (*original_pthread_create)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*) = NULL;
int (*original_pthread_join)(pthread_t, void**) = NULL;
int (*original_pthread_mutex_lock)(pthread_mutex_t*) = NULL;
int (*original_pthread_mutex_unlock)(pthread_mutex_t*) = NULL;


//used to going from pthread_t's to manually assigned thread ids
void add(pthread_t);
int hash(pthread_t);//pthread_t to thread id
void list_init();//makes empty list
void print();

void cprintPath(int*);


//allows us to use the original pthread functions
static void initialize_original_functions();

struct Thread_Arg {
	void* (*start_routine)(void*);
	void* arg;
};

#define mutex pthread_mutex_t

int NUM_THREADS=1;
pthread_mutex_t GL;
pthread_mutex_t helper;
bool* flags; //desire to get lock

char* status;//1 = waiting to run
//2 = waiting on a mutex
//3 = waiting on a thread
//4 = ded

long* blocks;//points to the lock each thread can be waiting for
int running=-1;//running thread
bool main=true;
bool locks=true;

const char* fname="tempF\0";

void cprintPath(int* nums,int pathLen){
	printf("CHESS Path: ");
	for(int i=0;i<pathLen;i++)
		printf("%d->",nums[i++]);
	printf("(-1)\n");
}

int readPort(){
	FILE* f=fopen(fname,"r");
	int p;
	fscanf(f,"%d\n",&p);
	//fprintf(stdout,"CHESS read port %d\n",p);
	fclose(f);
	unlink(fname);
	return p;
}

int* path;
int curLevel;
int pathLen;
int sock;

void schedInit(){//get sched info
	//printf("	first CHESS call\n");
	curLevel=0;
	pathLen=0;
	path=NULL;
	int port = readPort();
	//fprintf(stdout,"CHESS post readPort %d\n",port);
	sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	//fprintf(stdout,"CHESS post socket\n");
	//remove(fname);//possible double delete here
	struct sockaddr_in addr;
	memset((char*)&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=port;

	if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0)
		printf("couldnt connect\n");
	//else
	//	printf("connected\n");
	recv(sock,&pathLen,sizeof(int),0);
	//fprintf(stdout,"CHESS rec pathLen %d\n",pathLen);
	path=(int*)malloc(sizeof(int)*(pathLen+1));
	path[pathLen]=-1;
	recv(sock,path,pathLen*sizeof(int),0);
	//int i=0;
	//printf("CHESS REC ");
	//while(path[i]!=-1)
	//	printf("%d->",path[i++]);
	//printf("(-1)\n");
	//printf("CHESS %d\n",r);
	//cprintPath(path,pathLen);

}


//debugging info
void printInfo(){
	int i;
	fprintf(stdout,"	status: ");
	for(i=0;i<NUM_THREADS;i++){
		fprintf(stdout, "%d ",status[i]);
	}
	fprintf(stdout,"\n	blocks: ");
	for(i=0;i<NUM_THREADS;i++){
		fprintf(stdout, "%ld ",blocks[i]);
	}
	fprintf(stdout,"\n 	running: %d\n",running);
}

void unBlock(pthread_mutex_t* p){
	for(int i=0;i<NUM_THREADS;i++){
		if((pthread_mutex_t*)blocks[i]==p){
			blocks[i]=0;
			status[i]=1;
		}
	}
}

void unBlockThread(pthread_t p){
	for(int i =0;i<NUM_THREADS;i++){
		if(status[i]==3 && (pthread_t)blocks[i]==p){
			status[i]=2;
		}
	}
}

int stepCount;

//nextThread
int stepDown(){
	int i,j;
	j=0;
	if(path!=NULL && *path==-1){
		int optsLen=0;
		int opt=0;
		int* opts;
		for(i=0;i<NUM_THREADS;i++){
			if(status[i]==1){
				++optsLen;
			}
		}
		if(optsLen>0){
			opts=(int*)malloc(sizeof(int)*optsLen);

			for(i=0;i<NUM_THREADS;i++){
				if(status[i]==1){
					opts[opt++]=i;
				}
			}

			fprintf(stdout,"\n");
			send(sock,&optsLen,sizeof(int),0);
			send(sock,opts,optsLen*sizeof(int),0);
		}else{
			send(sock,&optsLen,sizeof(int),0);
			opts=(int*)malloc(sizeof(int));
			*opts=-1;
			send(sock,opts,sizeof(int),0);
		}
		path=NULL;
	}else if(path!=NULL){
		running=*(path++);
		return running;
	}


	if(running==-1){
		running=0;
		return 1;
	}

	j=0;
	for(i=0;j<NUM_THREADS;i=(i+1)%NUM_THREADS){
		++j;
		if(status[i]==1){
			running=i;
			return i;
		}
	}

	j=0;
	for(i=0;j<NUM_THREADS;i=(i+1)%NUM_THREADS){
		++j;
		if(status[i]<=2){
			running=i;
			return i;
		}
	}

	j=0;
	//scans threads waiting for a lock/thread to end
	for(i=0;j<NUM_THREADS;i=(i+1)%NUM_THREADS){
		++j;
		if(status[i]<=3){
			running=i;
			status[i]=1;//updates status
			return i;
		}
	}
	//possibly check for deadlock detection
	return running;
}


void lock(){
	int me=hash(pthread_self());
	while(true){
		original_pthread_mutex_lock(&GL);
		if(running==me){
			return;
		}else{
			original_pthread_mutex_unlock(&GL);
		}
	}
}

void unlock(){
	stepDown();//pick next thread
	original_pthread_mutex_unlock(&GL);//release global lock
	//winner nominates itself to start running
}

static void* thread_main(void *arg){
	if(locks){//there is a chance this does nothing
		pthread_mutex_init(&GL,NULL);
		pthread_mutex_init(&helper,NULL);
		locks=false;
	}

	while(hash(pthread_self())==-1){}
	lock();


	void* v;
	struct Thread_Arg thread_arg = *(struct Thread_Arg*)arg;
	free(arg);
	v = thread_arg.start_routine(thread_arg.arg);


	status[hash(pthread_self())]=4;//set status to ded
	blocks[hash(pthread_self())]=0;

	unBlockThread(pthread_self());
	unlock();//release global
	return v;
}


extern "C" int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){
	initialize_original_functions();
	struct Thread_Arg *thread_arg = (struct Thread_Arg*)malloc(sizeof(struct Thread_Arg));
	thread_arg->start_routine = start_routine;
	thread_arg->arg = arg;
	++NUM_THREADS;

	if(main){//beginning of concurrent code
		if(locks){//initiate locks
			pthread_mutex_init(&GL,NULL);
			pthread_mutex_init(&helper,NULL);
			locks=false;
		}
		schedInit();
		stepCount=0;
		list_init();
		add(pthread_self());
		status=(char*)malloc(sizeof(char)*NUM_THREADS);
		blocks=(long*)malloc(sizeof(long*)*NUM_THREADS);
		for(int i =0; i<NUM_THREADS; i++){
			status[i]=1;
			blocks[i]=0;
		}
		main=false;
		stepDown();
		int ret = original_pthread_create(thread, attr, thread_main, thread_arg);

		add(*thread);
		lock();
		return ret;
	}
	//2+ threads
	int ret= original_pthread_create(thread, attr, thread_main, thread_arg);
	status=(char*)realloc(status,sizeof(char)*NUM_THREADS);
	status[NUM_THREADS-1]=1;

	blocks=(long*)realloc(blocks,sizeof(long*)*NUM_THREADS);
	blocks[NUM_THREADS-1]=0;

	add(*thread);

	return ret;
}

extern "C" int pthread_join(pthread_t joinee, void **retval){
	initialize_original_functions();
	status[hash(pthread_self())]=3;//waiting for thread
	blocks[hash(pthread_self())]=joinee;
	unlock();//give it up
	int ret=original_pthread_join(joinee, retval);

	status[hash(pthread_self())]=1;//continue executing
	lock();
	return ret;
}

extern "C" int pthread_mutex_lock(pthread_mutex_t *mutex){
	initialize_original_functions();

	int ret;
	if((ret=pthread_mutex_trylock(mutex))!=0){//held by another thread
		status[hash(pthread_self())]=2;//waiting for a mutex
		blocks[hash(pthread_self())]=(long)mutex;
		unlock();
		while(true){
			lock();
			if((ret=pthread_mutex_trylock(mutex))==0)//mutex is free
				break;//mutex obtained, continue execution race free
			unlock();
		}
		status[hash(pthread_self())]=1;
		blocks[hash(pthread_self())]=0;

		return ret;
	}

	status[hash(pthread_self())]=1;
	blocks[hash(pthread_self())]=0;
	return ret;
}

extern "C" int pthread_mutex_unlock(pthread_mutex_t *mutex){
	initialize_original_functions();
	int ret=original_pthread_mutex_unlock(mutex);
	//unblock
	unBlock(mutex);
	unlock();
	lock();

	return ret;
}

extern "C" int sched_yield(void){
	unlock();
	lock();
	return 0;
}

static void initialize_original_functions(){
	static bool initialized = false;
	if (!initialized) {
		initialized = true;

		original_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*))dlsym(RTLD_NEXT, "pthread_create");
		
		original_pthread_join = (int (*)(pthread_t, void**))dlsym(RTLD_NEXT, "pthread_join");
		original_pthread_mutex_lock = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_lock");
		original_pthread_mutex_unlock = (int (*)(pthread_mutex_t*))dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	}
}
