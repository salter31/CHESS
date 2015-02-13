all: chess.so sample sample2 sample3 sample4 head

sample: sample1.c
	gcc -o sample1 -g sample1.c -lpthread -lrt

sample2: sample2.c
	gcc -o sample2 -g sample2.c -lpthread -lrt 

sample3: sample3.c
	gcc -o sample3 -g sample3.c -lpthread -lrt 

sample4: sample4.c
	gcc -o sample4 -g sample4.c -lpthread -lrt 

head: trie.cpp head.cpp
	mpiCC -w -o head -g $^

chess.so: list.c chess.cpp 
	mpiCC -o $@ -Wall -shared -g -O0 -D_GNU_SOURCE $^ -fPIC -lpthread -ldl  

clean:
	rm sample1 sample2 sample3 sample4 head chess.so
