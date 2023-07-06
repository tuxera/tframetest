CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors
LDFLAGS+=-pthread
HEADERS := $(wildcard *.h)

all: tframetest

tframetest: frametest.o libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

libtframetest.a: profile.o frame.o tester.o histogram.o report.o platform.o
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

test: unittest
	./unittest

unittest: unittest.o test_frame.o test_profile.o test_tester.o test_platform.o libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o tframetest tframetest.exe libtframetest.a unittest

.PHONY: all clean
