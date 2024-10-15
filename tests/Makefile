CFLAGS+=-std=c99 -O0 -g -Wall -Werror -Wpedantic -pedantic-errors -I. -I..
TESTS=frame histogram profile tester
BUILD_FOLDER:=$(PWD)/build/tests
TEST_BINS=$(addprefix $(BUILD_FOLDER)/test_,$(TESTS))
OBJECTS=$(addsuffix .o,$(TEST_BINS))
DATE=$(shell date +%Y%m%d-%H%M%S)
LDFLAGS+=-Wl,--wrap=printf -Wl,--wrap=puts -Wl,--wrap=putchar

all: test

test: build_tests run_tests

build_tests: $(BUILD_FOLDER) $(BUILD_FOLDER)/test_platform.o $(OBJECTS)

$(BUILD_FOLDER):
	install -d $(BUILD_FOLDER)

$(BUILD_FOLDER)/test_%: $(BUILD_FOLDER)/test_platform.o $(BUILD_FOLDER)/test_%.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/test_%.o: test_%.c ../%.c ../%.h test_platform.c
	$(CC) -c $(CFLAGS) -o $@ $<

run_tests: $(TEST_BINS)
	@for tst in $(TESTS); do \
		"$(BUILD_FOLDER)/test_$${tst}"; \
	done

clean:
	rm -f *.o *.gcno *.gcda *.gcov $(TEST_BINS)

.PHONY: all test build_tests run_tests
