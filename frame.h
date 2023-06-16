#ifndef FRAMETEST_FRAME_H
#define FRAMETEST_FRAME_H

#include <stddef.h>
#include "profile.h"

typedef struct frame_t {
	profile_t profile;
	size_t size;
	void *data;
} frame_t;

frame_t *frame_gen(profile_t profile);

#endif
