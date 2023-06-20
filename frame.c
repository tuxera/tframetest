#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "frame.h"


frame_t *frame_gen(profile_t profile)
{
	frame_t *res = calloc(1, sizeof(*res));

	if (!res)
		return NULL;

	res->profile = profile;
	res->size = profile.width * profile.height * profile.bytes_per_pixel;
	res->size += profile.header_size;

	res->data = malloc(res->size);
	if (!res->data) {
		free(res);
		return NULL;
	}
	(void)frame_fill(res, 't');

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

size_t frame_write(FILE *f, frame_t *frame)
{
	if (!f || !frame)
		return 0;

#if 1
	posix_fallocate(fileno(f), 0, frame->size);

	/* Avoid buffered writes if possible */
	return write(fileno(f), frame->data, frame->size);
#else
	return fwrite(frame->data, frame->size, 1, f);
#endif
}

size_t frame_read(FILE *f, frame_t *frame)
{
	if (!f || !frame)
		return 0;
#if 1
	/* Avoid buffered reads */
	return read(fileno(f), frame->data, frame->size);
#else
	return fread(frame->data, frame->size, 1, f);
#endif
}
