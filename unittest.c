#include <stdio.h>

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

	printf("Done.\n");
	return 0;
}
