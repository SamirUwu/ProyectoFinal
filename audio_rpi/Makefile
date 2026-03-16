CC=gcc
CFLAGS=-Iinclude -Wall -O2
SRC=$(wildcard src/*.c)
OUT=audio_engine

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) -lm -lasound

clean:
	rm -f $(OUT)
