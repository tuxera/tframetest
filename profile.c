#include <string.h>
#include "profile.h"

static profile_t profiles[] = {
	{ "invalid", PROF_INVALID, 0, 0, 0, 0 },
	{ "SD-24bit", PROF_SD, 720, 480, 3, 0 },
	{ "HD-24bit", PROF_HD, 1280, 720, 3, 0},
	{ "FULLHD-24bit", PROF_FULLHD, 1920, 1080, 3, 0},
	{ "2K-24bit", PROF_2K, 2048, 1080, 3, 0},
	{ "4K-16bit", PROF_4K, 3840, 2160, 2, 0},
	{ "4K-24bit", PROF_4K, 3840, 2160, 3, 0},
	{ "4K-24bit", PROF_4K, 3840, 2160, 4, 0},
	{ "4K-24bit-hdr", PROF_4K, 3840, 2160, 4, 128},
	{ "8K-24bit", PROF_8K, 7680, 4320, 3, 0},
};
static size_t profile_cnt = sizeof(profiles) / sizeof(profiles[0]);

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
