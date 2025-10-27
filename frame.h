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
frame_t *frame_from_file(const platform_t *platform, const char *fname,
			 size_t header_size);

void frame_destroy(const platform_t *platform, frame_t *frame);
size_t frame_fill(frame_t *frame, char val);

size_t frame_write(const platform_t *platform, platform_handle_t f,
		   frame_t *frame);
size_t frame_read(const platform_t *platform, platform_handle_t f,
		  frame_t *frame);

#endif
