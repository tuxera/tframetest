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

#include <string.h>
#include "profile.h"

/*
 * All the "cmp" profiles tries to match similar profile in "frametest" which
 * uses 4:3 ratio for 2k and 4k with 32 bit colors.
 */
static profile_t profiles[] = {
	{ "invalid", PROF_INVALID, 0, 0, 0, 0 },
	{ "SD-32bit-cmp", PROF_SD, 720, 480, 4, 0 },
	{ "SD-24bit", PROF_SD, 720, 480, 3, 0 },
	{ "FULLHD-32bit-cmp", PROF_HD, 1920, 1080, 4, 0 },
	{ "HD-24bit", PROF_HD, 1280, 720, 3, 0 },
	{ "FULLHD-24bit", PROF_FULLHD, 1920, 1080, 3, 0 },
	{ "2K-32bit-cmp", PROF_2K, 2048, 1556, 4, 0 },
	{ "2K-24bit", PROF_2K, 2048, 1080, 3, 0 },
	{ "4K-32bit-cmp", PROF_4K, 4096, 3112, 4, 0 },
	{ "4K-24bit", PROF_4K, 3840, 2160, 3, 0 },
	{ "4K-16bit", PROF_4K, 3840, 2160, 2, 0 },
	{ "4K-32bit", PROF_4K, 3840, 2160, 4, 0 },
	{ "8K-24bit", PROF_8K, 7680, 4320, 3, 0 },
	{ "empty", PROF_CUSTOM, 0, 0, 0, 0 },
};
static size_t profile_cnt = sizeof(profiles) / sizeof(profiles[0]);

size_t profile_size(const profile_t *profile)
{
	size_t size;

	size = profile->width * profile->height * profile->bytes_per_pixel;
	size += profile->header_size;
	/* Round to direct I/O boundaries */
	if (size & 0xfff) {
		size_t extra = size & 0xfff;
		size += ALIGN_SIZE - extra;
	}

	return size;
}

size_t profile_count(void)
{
	return profile_cnt;
}

profile_t profile_get_by_name(const char *name)
{
	if (!name)
		return profiles[0];

	for (size_t i = 1; i < profile_cnt; i++) {
		if (strcmp(profiles[i].name, name) == 0)
			return profiles[i];
	}

	return profiles[0];
}

profile_t profile_get_by_type(enum ProfileType prof)
{
	for (size_t i = 1; i < profile_cnt; i++) {
		if (profiles[i].prof == prof)
			return profiles[i];
	}

	return profiles[0];
}

profile_t profile_get_by_index(size_t idx)
{
	if (idx < profile_cnt)
		return profiles[idx];

	return profiles[0];
}

profile_t profile_get_by_frame_size(size_t header_size, size_t size)
{
	for (size_t i = 1; i < profile_cnt; i++) {
		size_t prof_size = profile_size(&profiles[i]);
		prof_size += header_size;

		if (prof_size == size)
			return profiles[i];
	}

	return profiles[0];
}
