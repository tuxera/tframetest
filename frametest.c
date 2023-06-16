/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include "profile.h"
#include "frame.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	profile_t prof;
	frame_t *frm;

	prof = profile_get_by_name("2K-24bit");

	frm = frame_gen(prof);
	if (!frm) {
		fprintf(stderr, "Can't allocate frame\n");
		return 1;
	}

	printf("Profile name: %s\n", frm->profile.name);
	{
		FILE *tmp = fopen("frame1", "w+");
		if (!tmp) {
			fprintf(stderr, "Can't create frame file\n");
			return 1;
		}
		if (!frame_write(tmp, frm)) {
			fprintf(stderr, "Can't write to file\n");
			return 1;
		}
		fclose(tmp);
	}

	return 0;
}
