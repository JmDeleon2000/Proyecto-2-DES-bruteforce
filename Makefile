.PHONY = all clean

CC = mpicc			

OMPFLAG = -fopenmp

naive: 
	${CC} ${OMPFLAG} -o naive.o bruteforceNaive.c
	${CC} ${OMPFLAG} -o dnc.o DNC.c
	${CC} ${OMPFLAG} -o sieve.o sieve.c


clean:
	rm -rvf *.o 