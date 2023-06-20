CFLAGS=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors
LDFLAGS+=-pthread
HEADERS := $(wildcard *.h)

all: tframetest

tframetest: profile.o frame.o tester.o frametest.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o tframetest

.PHONY: all clean
