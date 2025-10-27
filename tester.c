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

#ifdef __linux__
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif
#include <limits.h>
#include <stdlib.h>

#include "tester.h"
#include "timing.h"

static inline size_t tester_frame_write(const platform_t *platform,
					const char *path, frame_t *frame,
					size_t num, test_files_t files,
					test_completion_t *comp)
{
	char name[PATH_MAX + 1];
	size_t ret;
	platform_handle_t f;

	switch (files) {
	case TEST_FILES_MULTIPLE:
		snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
		name[PATH_MAX] = 0;
		break;
	case TEST_FILES_SINGLE:
		snprintf(name, PATH_MAX, "%s", path);
		name[PATH_MAX] = 0;
		break;
	default:
		return 1;
	}

	f = platform->open(name,
			   PLATFORM_OPEN_CREATE | PLATFORM_OPEN_WRITE |
				   PLATFORM_OPEN_DIRECT,
			   0666);

	if (f <= 0)
		return 1;

	if (files == TEST_FILES_SINGLE) {
		long pos =
			platform->seek(f, num * frame->size, PLATFORM_SEEK_SET);
		if (pos < 0)
			return 1;
	}

	comp->open = timing_start();

	ret = frame_write(platform, f, frame);
	comp->io = timing_start();

	platform->close(f);
	comp->close = timing_start();

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

static inline size_t tester_frame_read(const platform_t *platform,
				       const char *path, frame_t *frame,
				       size_t num, test_files_t files,
				       test_completion_t *comp)
{
	char name[PATH_MAX + 1];
	size_t ret;
	platform_handle_t f;

	switch (files) {
	case TEST_FILES_MULTIPLE:
		snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
		name[PATH_MAX] = 0;
		break;
	case TEST_FILES_SINGLE:
		snprintf(name, PATH_MAX, "%s", path);
		name[PATH_MAX] = 0;
		break;
	default:
		return 1;
	}

	f = platform->open(name, PLATFORM_OPEN_READ | PLATFORM_OPEN_DIRECT,
			   0666);
	if (f <= 0)
		return 0;

	if (files == TEST_FILES_SINGLE) {
		long pos =
			platform->seek(f, num * frame->size, PLATFORM_SEEK_SET);
		if (pos < 0)
			return 1;
	}

	comp->open = timing_start();

	ret = frame_read(platform, f, frame);
	comp->io = timing_start();

	platform->close(f);
	comp->close = timing_start();

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

frame_t *tester_get_frame_read(const platform_t *platform, const char *path,
			       size_t frame_size)
{
	char name[PATH_MAX + 1];

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, 0UL);
	name[PATH_MAX] = 0;

	return frame_from_file(platform, name, frame_size);
}

static inline void shuffle_array(size_t *arr, size_t size)
{
	size_t i;

	if (!arr || size <= 1)
		return;

	for (i = size - 1; i >= 1; i--) {
		size_t j = rand() % size;
		size_t tmp;

		tmp = arr[j];
		arr[j] = arr[i];
		arr[i] = tmp;
	}
}

test_result_t tester_run_write(const platform_t *platform, const char *path,
			       frame_t *frame, size_t start_frame,
			       size_t frames, size_t fps, test_mode_t mode,
			       test_files_t files)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq)
			return res;

		for (i = 0; i < frames; i++)
			seq[i] = start_frame + i;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < end_frame; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_write(platform, path, frame, frame_idx, files,
					&res.completion[i - start_frame]))
			break;
		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}
	if (seq)
		platform->free(seq);
	return res;
}

test_result_t tester_run_read(const platform_t *platform, const char *path,
			      frame_t *frame, size_t start_frame, size_t frames,
			      size_t fps, test_mode_t mode, test_files_t files)
{
	test_result_t res = { 0 };
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_MODE_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq)
			return res;

		for (i = 0; i < frames; i++)
			seq[i] = i + start_frame;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < start_frame + frames; i++) {
		uint64_t frame_start = timing_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_MODE_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_MODE_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_MODE_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_read(platform, path, frame, frame_idx, files,
				       &res.completion[i - start_frame]))
			return res;
		res.completion[i - start_frame].frame = timing_start();
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = timing_elapsed(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = timing_elapsed(frame_start);
			}
		}
	}
	if (seq)
		platform->free(seq);
	return res;
}
