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

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
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
	res->time_taken_ns += tester_stop(start);

	close(f);

	return ret;
}

test_result_t tester_run_write(const char *path, frame_t *frame,
		size_t start_frame, size_t frames)
{
	test_result_t res = {0};
	size_t i;

	for (i = start_frame; i < start_frame + frames; i++) {

		if (!tester_frame_write(&res, path, frame, i)) {
			break;
		}
		++res.frames_written;
		res.bytes_written += frame->size;
	}
	return res;
}

test_result_t tester_run_read(const char *path, frame_t *frame,
		size_t start_frame, size_t frames)
{
	test_result_t res = {0};
	size_t i;

	for (i = start_frame; i < start_frame + frames; i++) {

		// TODO
#if 0
		if (!tester_frame_write(&res, path, frame, i)) {
			break;
		}
#endif
		++res.frames_written;
		res.bytes_written += frame->size;
	}
	return res;
}
