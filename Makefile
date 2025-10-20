MAJOR=3025
MINOR=1
PATCH=1
CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=$(MAJOR) -DMINOR=$(MINOR) -DPATCH=$(PATCH)
LDFLAGS+=-pthread
HEADERS := $(wildcard *.h)
BUILD_FOLDER=$(PWD)/build
SOURCES=profile.c frame.c tester.c histogram.c report.c platform.c timing.c
TEST_SOURCES=$(wildcard tests/test_*.c)
OBJECTS=$(addprefix $(BUILD_FOLDER)/,$(SOURCES:.c=.o))
ALL_FILES=$(SOURCES) $(HEADERS) $(TEST_SOURCES)

all: $(BUILD_FOLDER) $(BUILD_FOLDER)/tframetest

release: $(BUILD_FOLDER) $(BUILD_FOLDER)/tframetest
	strip $(BUILD_FOLDER)/tframetest

$(BUILD_FOLDER)/tframetest: $(BUILD_FOLDER)/frametest.o $(BUILD_FOLDER)/libtframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/libtframetest.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_FOLDER)/%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_FOLDER):
	install -d $(BUILD_FOLDER)

test:
	make -C tests

dist:
	git archive --prefix="tframetest-$(MAJOR).$(MINOR).$(PATCH)/" HEAD | gzip -9 > "tframetest-$(MAJOR).$(MINOR).$(PATCH).tar.gz"

win:
	BUILD_FOLDER="$(BUILD_FOLDER)/win" ./build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

win64:
	BUILD_FOLDER="$(BUILD_FOLDER)/win64" CROSS=x86_64-w64-mingw32- ./build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

coverage:
	CFLAGS="-O0 -fprofile-arcs -ftest-coverage" LDFLAGS="-lgcov" make -C tests test BUILD_FOLDER="$(BUILD_FOLDER)/tests-coverage"
	lcov --capture --directory "$(BUILD_FOLDER)/tests-coverage" --output-file "$(BUILD_FOLDER)/test-coverage.info" -exclude "$(shell realpath "$(PWD)/tests")/*" --exclude "/usr/*"
	genhtml "$(BUILD_FOLDER)/test-coverage.info" --output-directory "$(BUILD_FOLDER)/coverage-report"

format:
	clang-format -i $(ALL_FILES)

clean:
	make -C tests clean
	rm -rf $(BUILD_FOLDER)
	rm -f *.o tframetest tframetest.exe libtframetest.a *.gcno *.gcda *.gcov

.PHONY: all clean release test dist win win64 coverage format
