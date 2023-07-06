#ifdef __linux__
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif

#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "tester.h"

static inline uint64_t tester_time(void)
{
	struct timespec ts;
	uint64_t res;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return 0;
		}
	}
	res = (uint64_t)ts.tv_sec * SEC_IN_NS;
	res += ts.tv_nsec;

	return res;
}

uint64_t tester_start(void)
{
	return tester_time();
}

uint64_t tester_stop(uint64_t start)
{
	return tester_time() - start;
}

static inline size_t tester_frame_write(const platform_t *platform,
		test_result_t *res, const char *path, frame_t *frame,
		size_t num, test_completion_t *comp)
{
	char name[PATH_MAX + 1];
	uint64_t start;
	size_t ret;
	platform_handle_t f;

	snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
	name[PATH_MAX] = 0;

	f = platform->open(name, PLATFORM_OPEN_CREATE
			| PLATFORM_OPEN_WRITE
			| PLATFORM_OPEN_DIRECT, 0666);
	if (f <= 0)
		return 0;
	comp->open = tester_start();

	start = tester_start();
	ret = frame_write(platform, f, frame);
	res->write_time_taken_ns += tester_stop(start);
	comp->io = tester_start();

	platform->close(f);
	comp->close = tester_start();

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

static inline size_t tester_frame_read(const platform_t *platform,
		test_result_t *res, const char *path, frame_t *frame,
		size_t num, test_completion_t *comp)
{
	char name[PATH_MAX + 1];
	uint64_t start;
	size_t ret;
	platform_handle_t f;

	snprintf(name, PATH_MAX, "%s/frame%.6zu.tst", path, num);
	name[PATH_MAX] = 0;

	f = platform->open(name, PLATFORM_OPEN_READ
			| PLATFORM_OPEN_DIRECT, 0666);
	if (f <= 0)
		return 0;
	comp->open = tester_start();

	start = tester_start();
	ret = frame_read(platform, f, frame);
	res->write_time_taken_ns += tester_stop(start);
	comp->io = tester_start();

	platform->close(f);
	comp->close = tester_start();

	/* Faking the output! */
	if (!ret && !frame->size)
		return 1;
	return ret;
}

frame_t *tester_get_frame_read(const platform_t *platform, const char *path)
{
	char name[PATH_MAX + 1];

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, 0UL);
	name[PATH_MAX] = 0;

	return frame_from_file(platform, name);
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
		frame_t *frame, size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode)
{
	test_result_t res = {0};
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	budget = fps ? (SEC_IN_NS / fps) : 0;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq)
			return res;

		for (i = 0; i < frames; i++)
			seq[i] = start_frame + i;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < end_frame; i++) {
		uint64_t frame_start = tester_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_write(platform, &res, path, frame, frame_idx,
				&res.completion[i - start_frame]))
			break;
		res.completion[i - start_frame].frame = tester_start();
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = tester_stop(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = tester_stop(frame_start);
			}
		}
	}
	if (seq)
		platform->free(seq);
	return res;
}

test_result_t tester_run_read(const platform_t *platform, const char *path,
		frame_t *frame, size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode)
{
	test_result_t res = {0};
	size_t i;
	size_t budget;
	size_t end_frame;
	size_t *seq = NULL;

	budget = fps ? (SEC_IN_NS / fps) : 0;

	res.completion = platform->calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	if (mode == TEST_RANDOM) {
		seq = platform->malloc(sizeof(*seq) * frames);
		if (!seq)
			return res;

		for (i = 0; i < frames; i++)
			seq[i] = i + start_frame;
		shuffle_array(seq, frames);
	}

	for (i = start_frame; i < start_frame + frames; i++) {
		uint64_t frame_start = tester_start();
		size_t frame_idx;

		res.completion[i - start_frame].start = frame_start;
		switch (mode) {
		case TEST_REVERSE:
			frame_idx = end_frame - i + start_frame - 1;
			break;
		case TEST_RANDOM:
			frame_idx = seq[i - start_frame];
			break;
		case TEST_NORM:
		default:
			frame_idx = i;
			break;
		}
		if (!tester_frame_read(platform, &res, path, frame, frame_idx,
				&res.completion[i - start_frame]))
			return res;
		res.completion[i - start_frame].frame = tester_start();
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = tester_stop(frame_start);

			while (frame_elapsed < budget) {
				platform->usleep(100);
				frame_elapsed = tester_stop(frame_start);
			}
		}
	}
	if (seq)
		platform->free(seq);
	return res;
}
