#ifndef FRAMETEST_TESTER_H
#define FRAMETEST_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include "frame.h"

#define SEC_IN_NS 1000000000UL

typedef struct testset_t {
	const char *path;
	frame_t *frame;

	size_t frames_cnt;
	size_t thread_cnt;

	uint64_t frames_written;
	uint64_t time_taken_ns;
} testset_t;

typedef struct test_result_t {
	uint64_t frames_written;
	uint64_t bytes_written;
	uint64_t time_taken_ns;
} test_result_t;

uint64_t tester_start(void);
uint64_t tester_stop(uint64_t);
test_result_t tester_run_write(const char *path, frame_t *frame, size_t frames);

#endif