.PHONY = all clean

CC = mpicc			

OMPFLAG = -fopenmp

naive: 
	${CC} ${OMPFLAG} -o naive.o bruteforceNaive.c


clean:
	rm -rvf *.o 