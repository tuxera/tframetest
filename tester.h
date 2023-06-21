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
	uint64_t write_time_taken_ns;
	uint64_t time_taken_ns;
} test_result_t;

uint64_t tester_start(void);
uint64_t tester_stop(uint64_t);
test_result_t tester_run_write(const char *path, frame_t *frame,
		size_t start_frame, size_t frames);
test_result_t tester_run_read(const char *path, frame_t *frame,
		size_t start_frame, size_t frames);
frame_t *tester_get_frame_read(const char *path);

static inline int test_result_aggregate(test_result_t *dst, test_result_t *src)
{
	if (!dst || !src)
		return 1;

	dst->frames_written += src->frames_written;
	dst->bytes_written += src->bytes_written;
	dst->time_taken_ns += src->time_taken_ns;

	return 0;
}

#endif
