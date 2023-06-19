#include <linux/limits.h>
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <time.h>

#include "tester.h"

static inline uint64_t tester_time(void)
{
	struct timespec ts;
	uint64_t res;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			printf("daa\n");
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

size_t tester_frame_write(const char *path, frame_t *frame, size_t num)
{
	char name[PATH_MAX + 1];
	FILE *f;
	size_t res;

	snprintf(name, PATH_MAX, "%s/frame%.6lu.tst", path, num);
	name[PATH_MAX] = 0;

	f = fopen(name, "w+");
	if (!f)
		return 0;

	res = frame_write(f, frame);

	fclose(f);

	return res;
}

test_result_t tester_run_write(const char *path, frame_t *frame, size_t frames)
{
	test_result_t res = {0};
	size_t i;

	for (i = 0; i < frames; i++) {
		uint64_t start;

		start = tester_start();
		if (!tester_frame_write(path, frame, i)) {
			break;
		}
		res.time_taken_ns += tester_stop(start);
		++res.frames_written;
		res.bytes_written += frame->size;
	}
	return res;
}
