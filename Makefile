#Set path to the base directory of your UCX installation using setenv.sh

CC=gcc
CFLAGSD=-g -Wall -Wextra

ucxInclude=-L$(UCX_PATH)/lib -I$(UCX_PATH)/include -lucp -lucs -luct

benchmark: ucxHelper.o benchmark.o
	$(CC) benchmark.o ucxHelper.o $(ucxInclude) -o benchmark

hostdevice: ucxHelper.o hostdevice.o
	$(CC) hostdevice.o ucxHelper.o $(ucxInclude) -o hostdevice

debug:
	$(CC) $(CFLAGSD) test.c -c $(ucxInclude) -o test.o
	$(CC) $(CFLAGSD) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o
	$(CC) $(CFLAGSD) benchmark.c -c $(ucxInclude) -o benchmark.o
	$(CC) $(CFLAGSD) hostdevice.c -c $(ucxInclude) -o hostdevice.o
	$(CC) $(CFLAGSD) test.o ucxHelper.o $(ucxInclude) -o testDebug
	$(CC) $(CFLAGSD) benchmark.o ucxHelper.o $(ucxInclude) -o benchmarkDebug
	$(CC) $(CFLAGSD) hostdevice.o ucxHelper.o $(ucxInclude) -o hostdeviceDebug

hostdevice.o: hostdevice.c
	$(CC) hostdevice.c -c $(ucxInclude) -o hostdevice.o

benchmark.o: benchmark.c
	$(CC) benchmark.c -c $(ucxInclude) -o benchmark.o

test: test.o ucxHelper.o
	$(CC) test.o ucxHelper.o $(ucxInclude) -o test

test.o: test.c
	$(CC) test.c -c $(ucxInclude) -o test.o
 
ucxHelper.o: ucxHelper.c
	$(CC) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o



clean:
	rm -f *.o *.dat test testDebug benchmarkDebug ucxHelper benchmark hostdevice hostdeviceDebug benchmarkDebug1  
