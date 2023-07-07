#ifndef FRAMETEST_TESTER_H
#define FRAMETEST_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "frametest.h"
#include "frame.h"
#include "platform.h"

typedef struct testset_t {
	const char *path;
	frame_t *frame;

	size_t frames_cnt;
	size_t thread_cnt;

	uint64_t frames_written;
	uint64_t time_taken_ns;
} testset_t;

typedef enum test_mode_t {
	TEST_MODE_NORM = 0,
	TEST_MODE_REVERSE,
	TEST_MODE_RANDOM,
} test_mode_t;

uint64_t tester_start(void);
uint64_t tester_stop(uint64_t);
test_result_t tester_run_write(const platform_t *platform, const char *path,
		frame_t *frame, size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode);
test_result_t tester_run_read(const platform_t *platform, const char *path,
		frame_t *frame, size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode);
frame_t *tester_get_frame_read(const platform_t *platform, const char *path);

static inline void result_free(test_result_t *res)
{
	if (!res)
		return;
	if (res->completion)
		free(res->completion);
	res->completion = NULL;
}

static inline int test_result_aggregate(test_result_t *dst, test_result_t *src)
{
	test_completion_t *tmp;
	size_t frm;

	if (!dst || !src)
		return 1;

	frm = (dst->frames_written + src->frames_written);
	if (frm && src->frames_written && src->completion) {
		tmp = realloc(dst->completion, sizeof(*tmp) * frm);
		if (tmp) {
			memcpy(tmp + dst->frames_written,
					src->completion,
					sizeof(*tmp) * src->frames_written);
			dst->completion = tmp;
		}
	}

	dst->frames_written += src->frames_written;
	dst->bytes_written += src->bytes_written;
	dst->time_taken_ns += src->time_taken_ns;

	return 0;
}

#endif
