#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "frame.h"

#define ALIGN_SIZE 4096


int platform_aligned_alloc(void **res, size_t align, size_t size)
{
#ifdef _WIN32
	void *tmp;

	tmp = _aligned_malloc(size, align);
	if (!tmp)
		return 1;
	*res = tmp;

	return 0;
#else
	return posix_memalign(res, align, size);
#endif
}

frame_t *frame_gen(profile_t profile)
{
	frame_t *res = calloc(1, sizeof(*res));

	if (!res)
		return NULL;

	res->profile = profile;
	res->size = profile.width * profile.height * profile.bytes_per_pixel;
	res->size += profile.header_size;
	/* Round to direct I/O boundaries */
	if (res->size & 0xfff) {
		size_t extra = res->size & 0xfff;
		res->size += ALIGN_SIZE - extra;
	}

	if (platform_aligned_alloc(&res->data, ALIGN_SIZE, res->size)) {
		free(res);
		return NULL;
	}
	if (!res->data) {
		free(res);
		return NULL;
	}
	(void)frame_fill(res, 't');

	return res;
}

frame_t *frame_from_file(const char *fname)
{
	struct stat st;
	frame_t *res;

	if (stat(fname, &st)) {
		return NULL;
	}

	res = calloc(1, sizeof(*res));
	if (!res)
		return NULL;

	res->size = st.st_size;
	/* Round to direct I/O boundaries */
	if (res->size & 0xfff) {
		size_t extra = res->size & 0xfff;
		res->size += ALIGN_SIZE - extra;
	}
	if (!res->size)
		res->profile = profile_get_by_name("empty");
	else {
		res->profile.prof = PROF_CUSTOM;
		res->profile.name = "custom";
		res->profile.width = res->size;
		res->profile.bytes_per_pixel = 1;
		res->profile.height = 1;
		res->profile.header_size = 0;

		if (platform_aligned_alloc(&res->data, ALIGN_SIZE, res->size)) {
			free(res);
			return NULL;
		}
		if (!res->data) {
			free(res);
			return NULL;
		}
	}

	return res;
}

void frame_destroy(frame_t *frame)
{
	if (!frame)
		return;
	if (frame->data)
		free(frame->data);
	free(frame);
}

size_t frame_fill(frame_t *frame, char val)
{
	size_t i;

	for (i = 0; i < frame->size; i++)
		((char*)frame->data)[i] = val;
	return i;
}

size_t frame_write(int f, frame_t *frame)
{
	if (!f || !frame)
		return 0;
	if (!frame->size)
		return 0;

#if 0
	posix_fallocate(f, 0, frame->size);
#endif

	/* Avoid buffered writes if possible */
	return write(f, frame->data, frame->size);
}

size_t frame_read(int f, frame_t *frame)
{
	size_t res = 0;

	if (!f || !frame)
		return 0;

	res = 0;
	while (res < frame->size) {
		size_t readcnt;

		readcnt = read(f, (char*)frame->data + res, frame->size - res);
		if (readcnt == 0)
			break;
		res += readcnt;
	}
	return res;
}
