class Stack{
	public:
		Stack(int*,int);
		int pop();
	private:
		int* arr;
		int top;
		int max;
};

class Path{
	public:
		Path();
		int explore();
		void push(int[],int);
		Stack toStack();
		void print();
	private:
		int** sched;
		int* optsLen;
		int top;
		int MAX_SIZE;
		bool down;
		int cur;
};
