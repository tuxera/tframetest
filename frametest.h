#ifndef FRAMETEST_FRAMETEST_H
#define FRAMETEST_FRAMETEST_H

#include "profile.h"
#include "frame.h"

#define SEC_IN_NS 1000000000UL
#define SEC_IN_MS (SEC_IN_NS / 1000UL)

enum TestMode {
	TEST_WRITE = 1 << 0,
	TEST_READ  = 1 << 1,
	TEST_EMPTY = 1 << 2,
};

typedef struct opts_t {
	enum TestMode mode;

	enum ProfileType prof;
	size_t write_size;
	profile_t profile;

	frame_t *frm;
	const char *path;

	size_t threads;
	size_t frames;
	size_t fps;
	size_t header_size;

	unsigned int reverse : 1;
	unsigned int random : 1;
	unsigned int csv : 1;
	unsigned int no_csv_header : 1;
	unsigned int times : 1;
	unsigned int frametimes : 1;
	unsigned int histogram : 1;
} opts_t;

typedef struct test_completion_t {
	uint64_t start;
	uint64_t open;
	uint64_t io;
	uint64_t close;
	uint64_t frame;
} test_completion_t;

typedef struct test_result_t {
	uint64_t frames_written;
	uint64_t bytes_written;
	uint64_t time_taken_ns;
	test_completion_t *completion;
} test_result_t;

#endif
