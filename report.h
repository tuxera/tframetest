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

#ifndef FRAMETEST_REPORT_H
#define FRAMETEST_REPORT_H

#include "tester.h"
#include "frametest.h"

extern void print_header_csv(const opts_t *opts);
extern void print_results_csv(const char *tcase, const opts_t *opts,
			      const test_result_t *res);
extern void print_results(const char *tcase, const opts_t *opts,
			  const test_result_t *res);

#endif
