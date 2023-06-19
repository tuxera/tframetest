#ifndef FRAMETEST_PROFILES_H
#define FRAMETEST_PROFILES_H

#include <stddef.h>

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
profile_t profile_get_by_name(const char *name);
profile_t profile_get_by_type(enum ProfileType prof);
profile_t profile_get_by_index(size_t idx);

#endif
