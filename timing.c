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

#ifdef __linux__
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif
#include <time.h>

#include "frametest.h"
#include "timing.h"

uint64_t timing_time(void)
{
	struct timespec ts;
	uint64_t res;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return 0;
		}
	}
	res = (uint64_t)ts.tv_sec * SEC_IN_NS;
	res += ts.tv_nsec;

	return res;
}
