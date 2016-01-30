####################################################################### 
# File :       Makefile 
#######################################################################

# compiler:
CC = gcc
GFLAGS=

# keep track of dependencies automatically
.KEEP_STATE:

# lib
LIBS = -lpthread
#unix LIBS = -lpthread -lsocket -lnsl-llibstdc++

all:
	$(CC) -c *.cpp
	$(CC) -o coor Coor.o common.o $(LIBS)
	$(CC) -o pa3 pa3_main.o common.o $(LIBS)
clean:
	rv *.o