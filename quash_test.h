/**
 * @file quash_test.h
 *
 * This file holds useful macros for testing and debugging.
 */

/// Set this to 1 to enable debugging mode or 0 to disable debugging mode
#define TEST_Q (0)

#if TEST_Q == 1
/// Print debug information
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
/// Add debug exclusive code
#  define IFDEBUG(x) x
#else
/// Does not print debug information
#  define PDEBUG(fmt, ...)
/// Does not add debug exclusive code
#  define IFDEBUG(x)
#endif
