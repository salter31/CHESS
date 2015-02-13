#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mpi.h"
#include "trie.h"
#include <errno.h>

#define MASTER 0
#define busy 1

#define KILLALL -2
#define EMPTY -1

void printPath(int*);

const char* fname="tempF\0";
bool errorFound=false;
int pathsExplored=0;
char* host=strdup("127.0.0.1");
int numtasks,taskid;

int shouldWait;

MPI_Status status;


int fExists(){
	if(access(fname, F_OK)!=-1)
		return 1;
	return 0;
}


void writePort(int p){
	int j;
	while(fExists());//wait for break. possibly issue here
	MPI_Send(&j,1,MPI_INT,MASTER,1,MPI_COMM_WORLD);

	while(fExists());
	FILE* f=fopen(fname,"w");
	fprintf(f,"%d\n",p);
	fclose(f);
}

void notifyEnd(){//sends termination signal to all threads
	//Isend(buf,count,type,dest,tag,comm,MPI_Request* r)

	MPI_Request *r=(MPI_Request*)malloc(sizeof(MPI_Request));
	int b=KILLALL;

	for(int i=1;i<numtasks;i++){
		MPI_Isend(&b,1,MPI_INT,i,1,MPI_COMM_WORLD,r);
	}
}




void masterCode(){//makes a trie and sends paths to worker procs
	Trie t;//trie of path executions
	int availableProcs = numtasks-1;//num of worker procs
	int p = 1;//current proc to assign a task to
	char stats[availableProcs];//book keeping of busy procs
	for(int i=0;i<availableProcs;i++)
		stats[i]=!busy;

	int pathLen;
	int* path;

	int assignPhase=1;
	int pe=0;

	int done=true;

	while(!t.isExhausted()){//trie isnt explored
		done=true;
		fprintf(stdout,"\n\n		ASSIGNING PHASE %d\n",assignPhase++);

		while(availableProcs > 0 && (path=t.getPath())!=NULL){
			//trie doesnt need new info and there are free procs
			//updates path

			done=false;

			++pe;
			pathLen=0;

			while(path[pathLen++]!=-1);//determine pathLen

			fprintf(stdout,"M: pathLen %d\nM: ",pathLen);
			printPath(path);
			printf("\n");

			if(pathLen==0)
				pathLen=1;

			MPI_Send(&pathLen,1, MPI_INT,p,1,MPI_COMM_WORLD);//send worker pathLen
			MPI_Send(path,pathLen,MPI_INT,p,1,MPI_COMM_WORLD);//send path

			int j;
			MPI_Recv(&j,1,MPI_INT,p,1,MPI_COMM_WORLD,&status);
			while(!fExists());


			availableProcs--;
			stats[p-1]=busy;
			p=p%(numtasks-1)+1;
		}

		if(path==NULL && done){
			notifyEnd();
			return;
		}


		fprintf(stdout,"\n\n		RECEIVING PHASE\n");
		int i;
		for(i=1; i<numtasks; i++){
			if(stats[i-1] == busy){
				int optsLen,* opts;

				//rec pathLen
				MPI_Recv(&pathLen,1,MPI_INT,i,1,MPI_COMM_WORLD,&status);

				//check for error
				if(pathLen==KILLALL){
					notifyEnd();
					MPI_Finalize();
					exit(1);
					return;
				}

				path=(int*)malloc(sizeof(int)*(pathLen+1));

				//rec path
				MPI_Recv(path,pathLen,MPI_INT,i,1,MPI_COMM_WORLD,&status);
				path[pathLen]=-1;


				//rec optsLen
				MPI_Recv(&optsLen,1,MPI_INT,i,1,MPI_COMM_WORLD,&status);
				opts=(int*)malloc(sizeof(int)*(optsLen+1));

				//rec opts
				MPI_Recv(opts,optsLen,MPI_INT,i,1,MPI_COMM_WORLD,&status);
				opts[optsLen]=-1;

				t.addPath(path,opts);//update the trie

				//update status and available Procs
				stats[i-1] = !busy;
				++availableProcs;
			}
		}
	}
	notifyEnd();
}


void workerCode(int argc, char** argv){
	int pathLen,optsLen, port, me, optval=1;
	int* path, *opts;


	struct sockaddr_in clientAddr;
	int alen;
	alen= sizeof(clientAddr);

	int chess;

	while(true){
		MPI_Recv(&pathLen,1,MPI_INT,MASTER,1,MPI_COMM_WORLD,&status);

		if(pathLen==KILLALL)
			return;

		if(pathLen>0){
			path=(int*)malloc(sizeof(int)*(pathLen+1));//-1 terminating symbol
			MPI_Recv(path,pathLen,MPI_INT,MASTER,1,MPI_COMM_WORLD,&status);
		}

		path[pathLen]=-1;

		me =  socket(PF_INET, SOCK_STREAM, 0);

		struct sockaddr_in serverIPAddress;
		memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
		serverIPAddress.sin_family = AF_INET;
		serverIPAddress.sin_addr.s_addr = INADDR_ANY;
		serverIPAddress.sin_port = 0;

		setsockopt(me, SOL_SOCKET, SO_REUSEADDR,(char*)&optval, sizeof(int));

		bind(me,(struct sockaddr*)&serverIPAddress, sizeof(serverIPAddress));
		listen(me, 1);

		struct sockaddr_in s;
		int slen;
		getsockname(me,(struct sockaddr*)&s,(socklen_t*)&slen);
		port=s.sin_port;
		writePort(port);

		int pid;
		int stat;
		pid=fork();
		if(pid==0){
			char* varAss=(char*)malloc(strlen(getenv("MYLOC"))+12);
			sprintf(varAss,"LD_PRELOAD=%s",getenv("MYLOC"));
			putenv(varAss);
			execvp(argv[1],argv+1);
			printf("failed to create new proc\n%s\n",strerror(errno));
			exit(errno);
			return;
		}else{
			//accept chess connection
			chess = accept(me, (struct sockaddr*)&clientAddr,(socklen_t*)&alen);

			write(chess,&pathLen,sizeof(int));//send pathLen
			write(chess,path,pathLen*sizeof(int));//send path



			recv(chess,&optsLen,sizeof(int),0);//receive optsLen
			if(optsLen==0)
				optsLen=1;
			opts=(int*)malloc(sizeof(int)*(optsLen+1));
			recv(chess,opts,sizeof(int)*optsLen,0);//receive opts

			close(chess);
			close(me);

			waitpid(pid,&stat,0);
			if(WIFEXITED(stat)){
				fprintf(stdout,"successful run\n");
				//normal exit
				MPI_Send(&pathLen, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
				MPI_Send(path, pathLen, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
				MPI_Send(&optsLen, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
				MPI_Send(opts, optsLen, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
			}else{

				int bad=KILLALL;
				MPI_Send(&bad, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
				printf("\n\n\nError Found\nBuggy Schedule: ");
				printPath(path);
				printf("\n\n\n");
				return;
			}
		}
	}
	notifyEnd();
}


int main( int argc, char ** argv ){
	if(argc <= 1){
		printf("usage: ./test.sh possiblyBuggyProg arg1 arg2 ....\n");
		exit(0);
	}

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

	if(taskid==MASTER){
		masterCode();
	}else{
		fprintf(stdout,"worker %d\n",taskid);
		workerCode(argc, argv);
	}
	MPI_Finalize();
	if(taskid==MASTER)
		printf("No errors found\n");
}

