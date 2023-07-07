#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "frame.h"

#define ALIGN_SIZE 4096

frame_t *frame_gen(const platform_t *platform, profile_t profile)
{
	frame_t *res = platform->calloc(1, sizeof(*res));

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

	if (platform->aligned_alloc(&res->data, ALIGN_SIZE, res->size)) {
		platform->free(res);
		return NULL;
	}
	if (!res->data) {
		platform->free(res);
		return NULL;
	}
	(void)frame_fill(res, 't');

	return res;
}

frame_t *frame_from_file(const platform_t *platform, const char *fname)
{
	platform_stat_t st;
	frame_t *res;

	if (platform->stat(fname, &st))
		return NULL;

	res = platform->calloc(1, sizeof(*res));
	if (!res)
		return NULL;

	res->size = st.size;
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

		if (platform->aligned_alloc(&res->data, ALIGN_SIZE, res->size)) {
			platform->free(res);
			return NULL;
		}
		if (!res->data) {
			platform->free(res);
			return NULL;
		}
	}

	return res;
}

void frame_destroy(const platform_t *platform, frame_t *frame)
{
	if (!frame)
		return;
	if (frame->data)
		platform->free(frame->data);
	platform->free(frame);
}

size_t frame_fill(frame_t *frame, char val)
{
	size_t i;

	for (i = 0; i < frame->size; i++)
		((char*)frame->data)[i] = val;
	return i;
}

size_t frame_write(const platform_t *platform, platform_handle_t f,
		frame_t *frame)
{
	if (!f || !frame)
		return 0;
	if (!frame->size)
		return 0;

#if 0
	posix_fallocate(f, 0, frame->size);
#endif

	/* Avoid buffered writes if possible */
	return platform->write(f, frame->data, frame->size);
}

size_t frame_read(const platform_t *platform, platform_handle_t f,
		frame_t *frame)
{
	size_t res = 0;

	if (!f || !frame)
		return 0;

	res = 0;
	while (res < frame->size) {
		size_t readcnt;

		readcnt = platform->read(f, (char*)frame->data + res,
				frame->size - res);
		if (readcnt == 0)
			break;
		res += readcnt;
	}
	return res;
}
