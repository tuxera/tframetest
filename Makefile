#CFLAGS=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -Wsequence-point -Wstrict-overflow=5 -Wnull-dereference
CFLAGS=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors
HEADERS := $(wildcard *.h)

all: frametest

frametest: frametest.o frame.o profile.o tester.o

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o frametest
