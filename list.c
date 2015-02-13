#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<dlfcn.h>
#include<stdbool.h>

void print();

struct node{
	pthread_t me;
	int id;
	bool alive;
	struct node* next;
};

int newID;
struct node* head;
pthread_mutex_t m;

bool contains(pthread_t tid){
	struct node* t=head;
	while(t!=NULL && t->me!=tid)
		t=t->next;
	if(t!=NULL)
		return true;
	return false;
}

void add(pthread_t tid){
	if(head==NULL){
		head=(struct node*)malloc(sizeof(struct node));
		head->me=tid;
		head->id=newID++;
		head->alive=true;
		head->next=NULL;
		return;
	}
	if(contains(tid)){
		return;
	}
	struct node* t=(struct node*)malloc(sizeof(struct node));
	t->me=tid;
	t->alive=true;
	t->id=newID++;
	t->next=head;
	head=t;
}




int hash(pthread_t tid){
	struct node* t=head;
	while(t!=NULL){
		if(t->me==tid && t->alive){
			return t->id;
		}
		t=t->next;
	}
	return -1;
}

void print(){
	struct node* t=head;
	while(t!=NULL){
		printf("%d %d\n",t->id,(int)t->me);
		t=t->next;
	}
	fprintf(stdout,"\n");
}

void list_init(){
	pthread_mutex_init(&m,NULL);
}


//for testing
/*
int main(){
	list_init();
	add(pthread_self());
	add(pthread_self()+4);
	add(pthread_self());
	add(pthread_self()+7);
	//print();

	//bool* b=malloc(sizeof(bool));
	fprintf(stdout,"%d\n",(int)sizeof(pthread_t));
}
*/
