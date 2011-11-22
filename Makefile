CONFIG = -Wall -DDEBUG
all:
	gcc ${CONFIG} -c dispatcher.c
	gcc ${CONFIG} -c service2.c
	gcc ${CONFIG} -c service4.c
	gcc ${CONFIG} -c stm.c
	gcc -lpthread dispatcher.o -o dispatcher
	gcc -lpthread service2.o -o service2
	gcc -lpthread service4.o -o service4
	gcc -lpthread stm.o -o stm

