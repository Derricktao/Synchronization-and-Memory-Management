
libuthread.a: queue.o sem.o thread.o tps.o
	ar rcs libuthread.a queue.o sem.o thread.o tps.o

queue.o: queue.c queue.h
	gcc -g -c queue.c

tps.o: tps.c tps.h
	gcc -g -c tps.c

sem.o: sem.c sem.h
	gcc -g -c sem.c
	
	#Already have thread.o, don't need to create
clean:
	rm -r libuthread.a tps.o queue.o sem.o

