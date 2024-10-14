CFLAGS+=-std=c99 -O0 -g -Wall -Werror -Wpedantic -pedantic-errors -I. -I..
TESTS=frame histogram profile tester
BUILD_FOLDER=$(PWD)/build/tests
TEST_BINS=$(addprefix $(BUILD_FOLDER)/test_,$(TESTS))
OBJECTS=$(addsuffix .o,$(TEST_BINS))
DATE=$(shell date +%Y%m%d-%H%M%S)

all: test

test: build_tests run_tests

build_tests: $(BUILD_FOLDER) $(BUILD_FOLDER)/test_platform.o $(OBJECTS)

$(BUILD_FOLDER):
	install -d $(BUILD_FOLDER)

$(BUILD_FOLDER)/test_%: $(BUILD_FOLDER)/test_platform.o $(BUILD_FOLDER)/test_%.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

run_tests: $(TEST_BINS)
	@for tst in $(TESTS); do \
		"$(BUILD_FOLDER)/test_$${tst}"; \
	done

coverage:
	CFLAGS="-O0 -fprofile-arcs -ftest-coverage" LDFLAGS="-lgcov" make -C . clean test
	find -name "*.gcda" | while read f; do bn=$$(basename "$$f" .gcda); gcov "$$bn.c"; done;
	mkdir -p "coverage-$(DATE)"
	gcovr --html-details "coverage-$(DATE)/coverage.html"
	zip -r "../coverage-$(DATE).zip" "coverage-$(DATE)"

clean:
	rm -f *.o *.gcno *.gcda *.gcov $(TEST_BINS)

.PHONY: all test build_tests run_tests
