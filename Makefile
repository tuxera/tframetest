CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors
LDFLAGS+=-pthread
HEADERS := $(wildcard *.h)
DATE=$(shell date +%Y%m%d-%H%M%S)

all: tframetest

tframetest: frametest.o libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

libtframetest.a: profile.o frame.o tester.o histogram.o report.o platform.o
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

test: unittest
	./unittest

unittest: unittest.o test_frame.o test_profile.o test_tester.o test_platform.o test_histogram.o libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

coverage:
	CFLAGS="-O0 -fprofile-arcs -ftest-coverage" LDFLAGS="-lgcov" make -C . clean test
	find -name "*.gcda" | while read f; do bn=$$(basename "$$f" .gcda); gcov "$$bn.c"; done;
	mkdir -p "coverage-$(DATE)"
	gcovr --html-details "coverage-$(DATE)/coverage.html"
	zip -r "coverage-$(DATE).zip" "coverage-$(DATE)"

clean:
	rm -f *.o tframetest tframetest.exe libtframetest.a unittest *.gcno *.gcda *.gcov

.PHONY: all clean
