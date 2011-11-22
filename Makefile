CONFIG = -Wall
all:
#	gcc sockutil.c -c
	gcc ${CONFIG} -c dispatcher.c
#	gcc ${CONFIG} -c uclient.c
#	gcc ${CONFIG} -c uclient2.c
	gcc ${CONFIG} -c service2.c
	gcc ${CONFIG} -c service4.c
	gcc ${CONFIG} -c stm.c
	gcc -lpthread dispatcher.o -o dispatcher
	gcc -lpthread service2.o -o service2
	gcc -lpthread service4.o -o service4
	gcc -lpthread stm.o -o stm
#	gcc sockutil.o uclient.o -o uclient
#	gcc sockutil.o uclient2.o -o uclient2
