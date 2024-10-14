MAJOR=3023
MINOR=8
PATCH=1
CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=$(MAJOR) -DMINOR=$(MINOR) -DPATCH=$(PATCH)
LDFLAGS+=-pthread
HEADERS := $(wildcard *.h)
DATE=$(shell date +%Y%m%d-%H%M%S)

all: tframetest

release: tframetest
	strip tframetest

tframetest: frametest.o libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

libtframetest.a: profile.o frame.o tester.o histogram.o report.o platform.o timing.o
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

test:
	make -C tests

dist:
	git archive --prefix="tframetest-$(MAJOR).$(MINOR).$(PATCH)/" HEAD | gzip -9 > "tframetest-$(MAJOR).$(MINOR).$(PATCH).tar.gz"

win:
	./build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

win64:
	CROSS=x86_64-w64-mingw32- ./build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

coverage:
	make -C tests coverage

clean:
	make -C tests clean
	rm -f *.o tframetest tframetest.exe libtframetest.a *.gcno *.gcda *.gcov

.PHONY: all clean release test dist win win64 coverage
