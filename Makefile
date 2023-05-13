.PHONY = all clean

CC = mpicc			

OMPFLAG = -fopenmp

compileall: 
	${CC} ${OMPFLAG} -o naive.o bruteforceNaive.c
	${CC} ${OMPFLAG} -o dnc.o DNC.c
	${CC} ${OMPFLAG} -o dncomp.o DNComp.c
	${CC} ${OMPFLAG} -o sieve.o sieve.c


testDNComp:
	mpirun --app DNCompApp > DNCompresult1.txt
	mpirun --app DNCompApp > DNCompresult2.txt
	mpirun --app DNCompApp > DNCompresult3.txt
	mpirun --app DNCompApp > DNCompresult4.txt
	mpirun --app DNCompApp > DNCompresult5.txt
	mpirun --app DNCompApp > DNCompresult6.txt
	mpirun --app DNCompApp > DNCompresult7.txt
	mpirun --app DNCompApp > DNCompresult8.txt
	mpirun --app DNCompApp > DNCompresult9.txt
	mpirun --app DNCompApp > DNCompresult10.txt


testDNC:
	mpirun --app DNCApp > DNCresult1.txt
	mpirun --app DNCApp > DNCresult2.txt
	mpirun --app DNCApp > DNCresult3.txt
	mpirun --app DNCApp > DNCresult4.txt
	mpirun --app DNCApp > DNCresult5.txt
	mpirun --app DNCApp > DNCresult6.txt
	mpirun --app DNCApp > DNCresult7.txt
	mpirun --app DNCApp > DNCresult8.txt
	mpirun --app DNCApp > DNCresult9.txt
	mpirun --app DNCApp > DNCresult10.txt


testNaive:
	mpirun --app naiveApp > naiveresult1.txt
	mpirun --app naiveApp > naiveresult2.txt
	mpirun --app naiveApp > naiveresult3.txt
	mpirun --app naiveApp > naiveresult4.txt
	mpirun --app naiveApp > naiveresult5.txt
	mpirun --app naiveApp > naiveresult6.txt
	mpirun --app naiveApp > naiveresult7.txt
	mpirun --app naiveApp > naiveresult8.txt
	mpirun --app naiveApp > naiveresult9.txt
	mpirun --app naiveApp > naiveresult10.txt

testSieve:
	mpirun --app sieveApp > sieveresult1.txt
	mpirun --app sieveApp > sieveresult2.txt
	mpirun --app sieveApp > sieveresult3.txt
	mpirun --app sieveApp > sieveresult4.txt
	mpirun --app sieveApp > sieveresult5.txt
	mpirun --app sieveApp > sieveresult6.txt
	mpirun --app sieveApp > sieveresult7.txt
	mpirun --app sieveApp > sieveresult8.txt
	mpirun --app sieveApp > sieveresult9.txt
	mpirun --app sieveApp > sieveresult10.txt

clean:
	rm -rvf *.o 