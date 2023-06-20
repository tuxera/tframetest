#ifndef FRAMETEST_FRAME_H
#define FRAMETEST_FRAME_H

#include <stddef.h>
#include <stdio.h>
#include "profile.h"

typedef struct frame_t {
	profile_t profile;
	size_t size;
	void *data;
} frame_t;

frame_t *frame_gen(profile_t profile);
void frame_destroy(frame_t *frame);
size_t frame_fill(frame_t *frame, char val);

size_t frame_write(int f, frame_t *frame);
size_t frame_read(int f, frame_t *frame);

#endif
