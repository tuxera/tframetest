CFLAGS=-O2 -Wall -Werror
HEADERS := $(wildcard *.h)

all: frametest

frametest: frametest.o frame.o profile.o

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o frametest
