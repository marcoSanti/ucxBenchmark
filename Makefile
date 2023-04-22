#Set path to the base directory of your UCX installation using setenv.sh

CC=gcc
CFLAGSD=-g -Wall -Wextra

ucxInclude=-L$(UCX_PATH)/lib -I$(UCX_PATH)/include -lucp -lucs -luct

debug:
	$(CC) $(CFLAGSD) test.c -c $(ucxInclude) -o test.o
	$(CC) $(CFLAGSD) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o
	$(CC) $(CFLAGSD) test.o ucxHelper.o $(ucxInclude) -o test


test: test.o ucxHelper.o
	$(CC) test.o ucxHelper.o $(ucxInclude) -o test

test.o: test.c
	$(CC) test.c -c $(ucxInclude) -o test.o
 
ucxHelper.o: ucxHelper.c
	$(CC) ucxHelper.c -c $(ucxInclude) -o ucxHelper.o



clean:
	rm -f *.o test ucxHelper *~ 
