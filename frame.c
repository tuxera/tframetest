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

	if (posix_memalign(&res->data, ALIGN_SIZE, res->size)) {
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
	if (!res->size) {
		free(res);
		return NULL;
	}
	/* Round to direct I/O boundaries */
	if (res->size & 0xfff) {
		size_t extra = res->size & 0xfff;
		res->size += ALIGN_SIZE - extra;
	}
	res->profile.prof = PROF_CUSTOM;
	res->profile.name = "custom";
	res->profile.width = res->size;
	res->profile.bytes_per_pixel = 1;
	res->profile.height = 1;
	res->profile.header_size = 0;

	if (posix_memalign(&res->data, ALIGN_SIZE, res->size)) {
		free(res);
		return NULL;
	}
	if (!res->data) {
		free(res);
		return NULL;
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

#if 1
#if 0
	posix_fallocate(f, 0, frame->size);
#endif

	/* Avoid buffered writes if possible */
	return write(f, frame->data, frame->size);
#else
	return fwrite(frame->data, frame->size, 1, f);
#endif
}

size_t frame_read(int f, frame_t *frame)
{
	if (!f || !frame)
		return 0;

#if 1
	/* Avoid buffered reads */
	return read(f, frame->data, frame->size);
#else
	return fread(frame->data, frame->size, 1, f);
#endif
}
