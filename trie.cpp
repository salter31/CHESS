#include<stdio.h>
#include<stdlib.h>

#include "trie.h"

//trie status
#define exhausted 2
#define stuck 1
#define well 0

//node statuses
#define done 2
#define explored 1
#define unexplored 0


Node::Node(int value,int depth, Node* p){
	level=depth;
	children=NULL;
	status=unexplored;
	val=value;
	numChild=0;
	parent=p;
}

Node* Node::getChild(int val){
	Node* t;
	for(int i=0;i<numChild;i++){
		t=children[i];
		if(t->val==val)
			return t;
	}
	return NULL;
}

void Node::add(int val){
	if(getChild(val)!=NULL)
		return;

	++numChild;
	children=(Node**)realloc(children,sizeof(Node*)*numChild);
	Node* infant=new Node(val,level+1,this);
	children[numChild-1]=infant;
}


Trie::Trie(){
	root= new Node(-1,-1,NULL);
	status=!explored;
}

void Trie::addPath(int* path,int* opts){//opts==NULL -> no new opts
	Node* n=root;
	int i=0;
	while(path!=NULL && path[i]!=-1){
		if(n->getChild(path[i])!=NULL){
			n=n->getChild(path[i]);
		}else{
			n->add(path[i]);
			n=n->getChild(path[i]);
		}
		++i;
	}

	if(opts==NULL){//no new options/path is explored
		n->status=done;
		while(n!=NULL){
			for(i=0;i<n->numChild;i++){
				if(n->children[i]->status!=done)
					return;
			}
			n->status=done;
			n=n->parent;
		}
	}else{//add opts to n
		status = well;
		i=0;
		while(opts[i]!=-1)
			n->add(opts[i++]);
	}
}

void Trie::printNode(Node* n){
	if(n->numChild==0){//leaf. print back to root
		Node* t=n;
		while(t!=root){
			printf("%d(%d)<-",t->val,t->status);
			t=t->parent;
		}
		printf("begin\n");
	}else{
		for(int i=0;i<n->numChild;i++){
			printNode(n->children[i]);
		}
	}
}

void Trie::print(){//# of paths printed == # of leaves
	if(root->numChild==0){
		printf("Empty\n");
		return;
	}

	if(root->status==done)
		printf("Explored:\n");
	else
		printf("Incomplete\n");
	for(int i=0;i<root->numChild;i++){
		printNode(root->children[i]);
	}
}


bool Trie::isExhausted(){
	if(root->status==done)
		return true;
	return false;
}

//-1 to end path
//null to rep no new paths
int* Trie::getPath(){
	int* path=NULL;
	Node* winner=NULL;
	if(root->numChild==0){
		if(root->status==explored){
			return NULL;
		}
		root->status=explored;
		path=(int*)malloc(sizeof(int));
		*path=-1;
		return path;
	}else{
		for(int i=0;i<root->numChild;i++){
			winner=explore(root->children[i]);
			if(winner!=NULL)
				break;
		}
	}
	if(winner==NULL){//no new paths
		return NULL;
	}else{
		path=(int*)malloc(sizeof(int)*(winner->level+2));
		path[winner->level+1]=-1;
		int i=winner->level;
		while(winner!=root){
			winner->status=explored;
			path[i]=winner->val;
			winner=winner->parent;
			--i;
		}
	}
	return path;
}

Node* Trie::explore(Node* n){
	Node* t;
	if(n->status==unexplored){
		n->status=explored;
		return n;
	}
	for(int i=0;i<n->numChild;i++){
		t=explore(n->children[i]);
		if(t!=NULL)
			return t;
	}
	return NULL;
}

void printPath(int* nums){//for testing
	printf("Path: ");
	if(nums==NULL){
		printf("\n");
		return;
	}
	int i=0;
	while(nums[i]!=-1){
		printf("%d->",nums[i++]);
	}
	printf("(-1)\n");
}



//for testing
/*
int main(int argc, char** args){
	Trie t;
	//t.print();
	//printf("\n");

	int* path=(int*)malloc(10);
	int* exp;
	int* opts=(int*)malloc(10);
	opts[0]=1;
	opts[1]=2;
	opts[2]=-1;


	t.addPath(NULL,opts);
	while((exp=t.getPath())!=NULL){}
//		printPath(exp);

//	printf("\nnext adding phase\n");

	path[0]=1;
	path[1]=-1;


	opts[2]=3;
	opts[3]=-1;
	t.addPath(path,opts);

	path[0]=2;
	path[1]=-1;
	t.addPath(path,opts);

	while((exp=t.getPath())!=NULL){}
//		printPath(exp);

//	printf("\nFILLING IT UP\n");
	path[0]=1;
	path[2]=-1;
	path[1]=1;
	t.addPath(path,NULL);

	path[1]=2;
	t.addPath(path,NULL);

	path[1]=3;
	t.addPath(path,NULL);

	//t.print();

	path[0]=2;
	path[1]=1;
	t.addPath(path,NULL);
//	printf("a\n");
	path[1]=2;
	t.addPath(path,NULL);

//	printf("b\n");
	path[1]=3;
	t.addPath(path,NULL);

//	printf("c\n");








	//t.addPath(NULL,opts);

	/*
	path=t.getPath();//path is empty
	printPath(path);//end

	t.addPath(path,opts);//add opts 1,2
	path=t.getPath();//path 1
	printPath(path);//path 1->end

	path=t.getPath();//path 2
	printPath(path);//path 2 -> end

	path=t.getPath();
	t.addPath(path,NULL);


	//t.print();

	path[0]=1;
	path[1]=2;
	path[2]=3;
	path[3]=4;
	path[4]=-1;
	//t.addPath(path,opts);


	path[0]=5;
	path[1]=6;
	path[2]=7;
	path[3]=8;
	path[4]=-1;
	//t.addPath(path,NULL);

	path[0]=5;
	path[1]=6;
	path[2]=7;
	path[3]=9;
	path[4]=-1;
	//t.addPath(path,NULL);



	t.print();
	int j=0;
	while(!t.isExhausted()){
		printf("\niteration: %d\n",j++);
		t.addPath(t.getPath(),NULL);
		t.print();
	}

	printf("\n");
	//currently works for back to back getpaths on blank trie
	//t.getPath();
	//t.getPath();
	t.print();
	if(t.isExhausted())
		printf("done\n");
	else
	printf("not done\n");
}
*/
