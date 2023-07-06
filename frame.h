#ifndef FRAMETEST_FRAME_H
#define FRAMETEST_FRAME_H

#include <stddef.h>
#include <stdio.h>
#include "profile.h"
#include "platform.h"

typedef struct frame_t {
	profile_t profile;
	size_t size;
	void *data;
} frame_t;

frame_t *frame_gen(const platform_t *platform, profile_t profile);
frame_t *frame_from_file(const platform_t *platform, const char *fname);

void frame_destroy(const platform_t *platform, frame_t *frame);
size_t frame_fill(frame_t *frame, char val);

size_t frame_write(const platform_t *platform, platform_handle_t f,
		frame_t *frame);
size_t frame_read(const platform_t *platform, platform_handle_t f,
		frame_t *frame);

#endif
