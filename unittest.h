#ifndef FRAMETEST_UNITTEST_H
#define FRAMETEST_UNITTEST_H

#define TEST_INIT() unsigned long ok = 0; unsigned long tests = 0;
#define TEST(X) ++tests; if (!test_ ## X ()) ++ok; else { \
	printf(" FAILED: %s\n", #X); return 1; }
#define TEST_END() return tests == ok ? 0 : 1;

#define TEST_ASSERT(X) if (!(X)) { \
	printf(" ASSERT: %s @%s:%u\n", #X, __FILE__, __LINE__); return 1; }
#define TEST_ASSERT_EQ(X, Y) if ((X) != (Y)) {\
	printf(" ASSERT: %s == %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }
#define TEST_ASSERT_NE(X, Y) if ((X) == (Y)) {\
	printf(" ASSERT: %s != %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }
#define TEST_ASSERT_EQ_STR(X, Y) if (strcmp(X, Y)) {\
	printf(" ASSERT: %s == %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }

#endif
