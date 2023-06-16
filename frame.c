#include <stdlib.h>
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

	return res;
}

size_t frame_write(FILE *f, frame_t *frame)
{
	if (!f || !frame)
		return 0;
	return fwrite(frame->data, frame->size, 1, f);
}
