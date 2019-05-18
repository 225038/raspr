CC=clang
CFLAGS=-std=c99 -Wall -pedantic
LFLAGS=-L. -lruntime

all: export build

export:
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/aleksandra/Документы/распределенки/4/raspr/pa4/libruntime.so";

    LD_PRELOAD=/home/workspace/study/raspr/libruntime.so ./a.out –p 2 10 20;

build:
	$(CC) $(CFLAGS) *.c $(LFLAGS)

logsRemove:
	rm -r *.log

clean:
	killall -9 a.out
