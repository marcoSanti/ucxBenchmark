#Set path to the base directory of your UCX installation using setenv.sh

CC=gcc
CFLAGSD=-g -Wall -Wextra

ucxInclude=-L$(UCX_PATH)/lib -I$(UCX_PATH)/include -lucp -lucs -luct

benchmark: ucxHelper.o benchmark.o
	$(CC) benchmark.o ucxHelper.o $(ucxInclude) -o benchmark

benchmark1: ucxHelper.o benchmark1.o
	$(CC) benchmark1.o ucxHelper.o $(ucxInclude) -o benchmark1

debug:
	$(CC) $(CFLAGSD) test.c -c $(ucxInclude) -o test.o
	$(CC) $(CFLAGSD) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o
	$(CC) $(CFLAGSD) benchmark.c -c $(ucxInclude) -o benchmark.o
	$(CC) $(CFLAGSD) benchmark1.c -c $(ucxInclude) -o benchmark1.o
	$(CC) $(CFLAGSD) test.o ucxHelper.o $(ucxInclude) -o testDebug
	$(CC) $(CFLAGSD) benchmark.o ucxHelper.o $(ucxInclude) -o benchmarkDebug
	$(CC) $(CFLAGSD) benchmark1.o ucxHelper.o $(ucxInclude) -o benchmarkDebug1

benchmark1.o: benchmark1.c
	$(CC) benchmark1.c -c $(ucxInclude) -o benchmark1.o

benchmark.o: benchmark.c
	$(CC) benchmark.c -c $(ucxInclude) -o benchmark.o

test: test.o ucxHelper.o
	$(CC) test.o ucxHelper.o $(ucxInclude) -o test

test.o: test.c
	$(CC) test.c -c $(ucxInclude) -o test.o
 
ucxHelper.o: ucxHelper.c
	$(CC) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o



clean:
	rm -f *.o *.dat test testDebug benchmarkDebug ucxHelper benchmark benchmark1 benchmarkDebug1*~ 
