BIN = bin

CC = g++
CFLAGS = -O3 -Wall -I/usr/include -static

LIBS = -lm 

PROGS = $(BIN)/online_code

OBJS = $(BIN)/encoder.o $(BIN)/decoder.o $(BIN)/common.o

all: $(PROGS)

$(BIN)/online_code: main.cpp $(OBJS)
	$(CC) $(CFLAGS) -g -o $(BIN)/online_code main.cpp $(OBJS) $(LIBS)

$(BIN)/encoder.o: encoder.cpp codec.h
	$(CC) -c $(CFLAGS) encoder.cpp -o $(BIN)/encoder.o

$(BIN)/decoder.o: decoder.cpp codec.h
	$(CC) -c $(CFLAGS) decoder.cpp -o $(BIN)/decoder.o

$(BIN)/common.o: common.cpp codec.h
	$(CC) -c $(CFLAGS) common.cpp -o $(BIN)/common.o


clean:
	/bin/rm -f $(OBJS) $(PROGS)

