#include<stdio.h>
#include<stdlib.h>

class Q{
	public:
		Q(int*,int);
		int deq();
	private:
		int* arr;
		int curr;
		int len;
};

Q::Q(int* nums,int size){
	if(size==0){
		arr=NULL;
		curr=len=0;
		return;
	}
	len=size;
	arr=nums;
	curr=0;
}

int Q::deq(){
	if(curr==len)
		return -1;
	return arr[curr++];
}

class Path{
	public:
		Path();
		int swap();
		void push(int[],int);
		Q toQ();
		void print();
	private:
		int** sched;
		int* optsLen;
		int top;
		int MAX_SIZE;
		int curr;
		bool down;
};

Path::Path(){
	MAX_SIZE=10;
	sched=(int**)malloc(sizeof(int*)*MAX_SIZE);
	optsLen=(int*)malloc(sizeof(int*)*MAX_SIZE);
	top=-1;
	curr=0;
}

void Path::push(int opts[], int len){
	if(top==MAX_SIZE){
		MAX_SIZE*=2;
		sched=(int**)realloc(sched,sizeof(int*)*MAX_SIZE);
		optsLen=(int*)realloc(optsLen,sizeof(int*)*MAX_SIZE);
	}
	sched[top+1]=opts;
	optsLen[++top]=len;
}

int Path::swap(){
	if(top==-1)
		return -1;
	if(curr<optsLen[top]){//normal swap
		return sched[top][curr++];
	}else{
		curr=0;
		--top;
	}
}

Q Path::toQ(){
	int* ret=(int*)malloc(sizeof(int)*(top+1));

	for(int i=top;i>=0;i--){
		ret[i]=*(sched[i]);
	}
	return Q (ret,top+1);
}

void Path::print(){
	for(int i=top;i>=0;i--){
		for(int j=0;j<optsLen[i];j++){
			printf("%d ",sched[i][j]);
		}
		printf("\n");
	}
}


//testing
/*
int main(){
	int* arr=(int*)malloc(sizeof(int)*5);
	arr[0]=2;
	arr[1]=5;
	arr[2]=9;

	int* arr2=(int*)malloc(sizeof(int)*5);
	arr2[0]=1;
	arr2[1]=2;
	arr2[2]=3;

	int arr3=7;
	int arr4=8;
	Path p;
	p.push(arr,3);
	//p.print();
	p.push(arr2,3);
	p.push(&arr3,1);
	p.push(&arr4,1);
	p.push(arr2,3);
//	p.toStack();
	//p.print();
	int t,n;
	while((t=p.explore())!=-1){
		printf("\nPATH\n");
		p.print();
		//printf("\n");
		printf("level %d\n",t);
	//	Stack st;
		Q q = p.toQ();
		printf("Q(top->bottom)\n");
		while((n=q.deq())!=-1){
			printf("%d ",n);
		}
		printf("\n");
	}
}

*/
