class Node{
	public:
		Node(int,int,Node*);//value, level, daddy
		Node* getChild(int);
		void add(int);

		int val;
		int numChild;
		int level;
		char status;
		Node** children;
		Node* parent;
};

class Trie{
	public:
		Trie();
		void addPath(int*, int*);
		void print();
		void printNode(Node*);
		bool isExhausted();
		int* getPath();
	private:
		Node* explore(Node*);
		char status;
		Node* root;
};
