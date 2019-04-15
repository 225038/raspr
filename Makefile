CC=clang
CFLAGS=-std=c99 -Wall -pedantic
LFLAGS=-L. -lruntime

all: export build

export:
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/aleksandra/raspr/";
    LD_PRELOAD=/home/aleksandra/raspr/libruntime.so ./a.out –p 2 10 20;

build:
	$(CC) $(CFLAGS) *.c $(LFLAGS)

logsRemove:
	rm -r *.log

clean:
	killall -9 a.out