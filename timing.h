/*
 * This file is part of tframetest.
 *
 * Copyright (c) 2023 Tuxera Inc.
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

#ifndef FRAMETEST_TIMING_H
#define FRAMETEST_TIMING_H

#include <stdint.h>

uint64_t timing_time(void);

static inline uint64_t timing_start(void)
{
	return timing_time();
}

static inline uint64_t timing_elapsed(uint64_t start)
{
	return timing_time() - start;
}

#endif
