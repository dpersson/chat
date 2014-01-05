CC	= gcc

all:
	$(CC) -Wall -g main.c -o main `pkg-config --libs --cflags libev`

clean:
	rm -rf *.o main