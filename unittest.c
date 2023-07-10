#include <stdio.h>
#include "unittest.h"

INIT_ASSERT()

#define RUN_TEST(NAME)\
	extern int test_ ## NAME (void);\
	printf("Testing %s...\n", #NAME);\
	if (test_ ## NAME ()) { return 1;}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	RUN_TEST(frame);
	RUN_TEST(profile);
	RUN_TEST(tester);
	RUN_TEST(histogram);

	printf("Done.\n");
	return 0;
}
