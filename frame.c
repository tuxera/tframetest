/*
 * This file is part of tframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "frame.h"

frame_t *frame_gen(const platform_t *platform, profile_t profile)
{
	frame_t *res = platform->calloc(1, sizeof(*res));

	if (!res)
		return NULL;

	res->profile = profile;
	res->size = profile_size(&profile);

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

frame_t *frame_from_file(const platform_t *platform, const char *fname,
			 size_t header_size)
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
		res->profile =
			profile_get_by_frame_size(header_size, res->size);

		if (res->profile.prof == PROF_INVALID) {
			res->profile.prof = PROF_CUSTOM;
			res->profile.name = "custom";
			res->profile.width = res->size;
			res->profile.bytes_per_pixel = 1;
			res->profile.height = 1;
			res->profile.header_size = 0;
		}

		if (platform->aligned_alloc(&res->data, ALIGN_SIZE,
					    res->size)) {
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
	if (!frame->size)
		return 0;
	memset(frame->data, val, frame->size);
	return frame->size;
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

		readcnt = platform->read(f, (char *)frame->data + res,
					 frame->size - res);
		if (readcnt == 0)
			break;
		res += readcnt;
	}
	return res;
}
