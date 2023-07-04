#include <linux/limits.h>
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
/* For O_DIRECT */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

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

size_t tester_frame_write(test_result_t *res, const char *path, frame_t *frame,
		size_t num)
{
	char name[PATH_MAX + 1];
	uint64_t start;
	size_t ret;
	int f;

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, num);
	name[PATH_MAX] = 0;

	f = open(name, O_CREAT | O_DIRECT | O_WRONLY, 0666);
	if (f <= 0)
		return 0;

	start = tester_start();
	ret = frame_write(f, frame);
	res->write_time_taken_ns += tester_stop(start);

	close(f);

	return ret;
}

size_t tester_frame_read(test_result_t *res, const char *path, frame_t *frame,
		size_t num)
{
	char name[PATH_MAX + 1];
	uint64_t start;
	size_t ret;
	int f;

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, num);
	name[PATH_MAX] = 0;

	f = open(name, O_DIRECT | O_RDONLY);
	if (f <= 0)
		return 0;

	start = tester_start();
	ret = frame_read(f, frame);
	res->write_time_taken_ns += tester_stop(start);

	close(f);

	return ret;
}

frame_t *tester_get_frame_read(const char *path)
{
	char name[PATH_MAX + 1];

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, 0UL);
	name[PATH_MAX] = 0;

	return frame_from_file(name);
}

test_result_t tester_run_write(const char *path, frame_t *frame,
		size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode)
{
	test_result_t res = {0};
	size_t i;
	size_t budget;
	size_t end_frame;

	budget = fps ? (SEC_IN_NS / fps) : 0;

	res.completion = calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	for (i = start_frame; i < end_frame; i++) {
		uint64_t frame_start = tester_start();

		switch (mode) {
		case TEST_REVERSE:
			if (!tester_frame_write(&res, path, frame,
					end_frame - i + start_frame - 1))
				return res;
			break;
		case TEST_NORM:
			if (!tester_frame_write(&res, path, frame, i))
				return res;
			break;
		}
		res.completion[i - start_frame] = tester_stop(frame_start);
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = tester_stop(frame_start);

			while (frame_elapsed < budget) {
				usleep(100);
				frame_elapsed = tester_stop(frame_start);
			}
		}
	}
	return res;
}

test_result_t tester_run_read(const char *path, frame_t *frame,
		size_t start_frame, size_t frames, size_t fps,
		test_mode_t mode)
{
	test_result_t res = {0};
	size_t i;
	size_t budget;
	size_t end_frame;

	budget = fps ? (SEC_IN_NS / fps) : 0;

	res.completion = calloc(frames, sizeof(*res.completion));
	if (!res.completion)
		return res;

	budget = fps ? (SEC_IN_NS / fps) : 0;
	end_frame = start_frame + frames;

	for (i = start_frame; i < start_frame + frames; i++) {
		uint64_t frame_start = tester_start();

		switch (mode) {
		case TEST_REVERSE:
			if (!tester_frame_read(&res, path, frame,
					end_frame - i + start_frame - 1))
				return res;
			break;
		case TEST_NORM:
			if (!tester_frame_read(&res, path, frame, i))
				return res;
			break;
		}
		res.completion[i - start_frame] = tester_stop(frame_start);
		++res.frames_written;
		res.bytes_written += frame->size;
		/* If fps limit is enabled loop until frame budget is gone */
		if (fps && budget) {
			uint64_t frame_elapsed = tester_stop(frame_start);

			while (frame_elapsed < budget) {
				usleep(100);
				frame_elapsed = tester_stop(frame_start);
			}
		}
	}
	return res;
}
