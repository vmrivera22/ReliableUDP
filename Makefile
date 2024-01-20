CFLAGS = -g -Wall
CC=g++ $(CFLAGS)
LDFLAGS= -pthread -lpthread 

DDIR = ./bin
all		:	myclient myserver myclient.o myserver.o
.PHONY	:	all
myclient	:	myclient.o
	$(CC) -o $(DDIR)/myclient $(LDFLAGS) myclient.o $(LDFLAGS)
myclient.o	:	./src/myclient.cpp
	$(CC) -c ./src/myclient.cpp
myserver	:	myserver.o
	$(CC) -o $(DDIR)/myserver $(LDFLAGS) myserver.o $(LDFLAGS)
myserver.o	:	./src/myserver.cpp
	$(CC) -c ./src/myserver.cpp
clean	:
	rm -rf $(DDIR)/myclient $(DDIR)/myserver myclient.o myserver.o infer-out
infer	:	clean
	infer-capture -- make
	infer-analyze -- make