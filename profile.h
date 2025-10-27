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

#ifndef FRAMETEST_PROFILES_H
#define FRAMETEST_PROFILES_H

#include <stddef.h>

#define ALIGN_SIZE 4096

enum ProfileType {
	PROF_INVALID,
	PROF_CUSTOM,
	PROF_SD,
	PROF_HD,
	PROF_FULLHD,
	PROF_2K,
	PROF_4K,
	PROF_8K,
};

typedef struct profile_t {
	const char *name;
	enum ProfileType prof;
	size_t width;
	size_t height;
	size_t bytes_per_pixel;
	size_t header_size;
} profile_t;

size_t profile_count(void);
size_t profile_size(const profile_t *profile);
profile_t profile_get_by_name(const char *name);
profile_t profile_get_by_type(enum ProfileType prof);
profile_t profile_get_by_index(size_t idx);
profile_t profile_get_by_frame_size(size_t header_size, size_t size);

#endif
