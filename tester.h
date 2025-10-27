/*
 * This file is part of tframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FRAMETEST_TESTER_H
#define FRAMETEST_TESTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "frametest.h"
#include "frame.h"
#include "platform.h"
#include "timing.h"

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

typedef enum test_files_t {
	TEST_FILES_MULTIPLE = 0,
	TEST_FILES_SINGLE = 1,
} test_files_t;

test_result_t tester_run_write(const platform_t *platform, const char *path,
			       frame_t *frame, size_t start_frame,
			       size_t frames, size_t fps, test_mode_t mode,
			       test_files_t files);
test_result_t tester_run_read(const platform_t *platform, const char *path,
			      frame_t *frame, size_t start_frame, size_t frames,
			      size_t fps, test_mode_t mode, test_files_t files);
frame_t *tester_get_frame_read(const platform_t *platform, const char *path,
			       size_t header_size);

static inline void result_free(const platform_t *platform, test_result_t *res)
{
	if (!res)
		return;
	if (res->completion)
		platform->free(res->completion);
	res->completion = NULL;
}

static inline int test_result_aggregate(test_result_t *dst,
					const test_result_t *src)
{
	test_completion_t *tmp;
	size_t frm;

	if (!dst || !src)
		return 1;

	frm = (dst->frames_written + src->frames_written);
	if (frm && src->frames_written && src->completion) {
		tmp = realloc(dst->completion, sizeof(*tmp) * frm);
		if (tmp) {
			memcpy(tmp + dst->frames_written, src->completion,
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
