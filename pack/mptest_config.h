#ifndef MPTEST_CONFIG_H
#define MPTEST_CONFIG_H

/* config: USE_DYN_ALLOC */
/* Set MPTEST_USE_DYN_ALLOC to 1 in order to allow the usage of malloc() and
 * free() inside mptest. Mptest can run without these functions, but certain
 * features are disabled. */
#ifndef MPTEST_USE_DYN_ALLOC
#define MPTEST_USE_DYN_ALLOC 1
#endif

/* config: USE_LONGJMP */
/* Set MPTEST_USE_LONGJMP to 1 in order to use runtime assert checking
 * facilities. Must be enabled to use heap profiling. */
#ifndef MPTEST_USE_LONGJMP
#define MPTEST_USE_LONGJMP 1
#endif

/* config: USE_SYM */
/* Set MPTEST_USE_SYM to 1 in order to use s-expression data structures for
 * testing. MPTEST_USE_DYN_ALLOC must be set. */
#ifndef MPTEST_USE_SYM
#define MPTEST_USE_SYM 1
#endif

/* config: USE_LEAKCHECK */
/* Set MPTEST_USE_LEAKCHECK to 1 in order to use runtime leak checking
 * facilities. MPTEST_USE_LONGJMP must be set. */
#if MPTEST_USE_LONGJMP
#if MPTEST_USE_DYN_ALLOC
#ifndef MPTEST_USE_LEAKCHECK
#define MPTEST_USE_LEAKCHECK 1
#endif
#endif
#endif

/* config: USE_COLOR */
/* Set MPTEST_USE_COLOR to 1 if you want ANSI-colored output. */
#ifndef MPTEST_USE_COLOR
#define MPTEST_USE_COLOR 1
#endif

/* config: USE_TIME */
/* Set MPTEST_USE_TIME to 1 if you want to time tests or suites. */
#ifndef MPTEST_USE_TIME
#define MPTEST_USE_TIME 1
#endif

/* config: USE_APARSE */
/* Set MPTEST_USE_APARSE to 1 if you want to build a command line version of
 * the test program. */
#if MPTEST_USE_DYN_ALLOC
#ifndef MPTEST_USE_APARSE
#define MPTEST_USE_APARSE 0
#endif
#endif

/* config: USE_FUZZ */
/* Set MPTEST_USE_FUZZ to 1 if you want to include fuzzing and test shuffling
 * support. */
#ifndef MPTEST_USE_FUZZ
#define MPTEST_USE_FUZZ 1
#endif

/* config: STATIC */
/* Whether or not API definitions should be defined as static linkage (local to
 * the including source file), as opposed to external linkage. */
#ifndef MPTEST_STATIC
#define MPTEST_STATIC 0
#endif

/* config: USE_CUSTOM_MALLOC */
/* Set MPTEST_USE_CUSTOM_MALLOC to 1 in order to use your own definitions for
 * malloc(), realloc() and free(). If MPTEST_USE_CUSTOM_MALLOC is set, you must
 * also define MPTEST_MALLOC, MPTEST_REALLOC and MPTEST_FREE. Otherwise,
 * <stdlib.h> is included and standard versions of malloc(), realloc() and
 * free() are used. */
#ifndef MPTEST_USE_CUSTOM_MALLOC
#define MPTEST_USE_CUSTOM_MALLOC 0
#endif

/* config: MALLOC */
/* a malloc() function. Performs a memory allocation. */
/* See https://en.cppreference.com/w/c/memory/malloc for more information. */
#if MPTEST_USE_CUSTOM_MALLOC
#ifndef MPTEST_MALLOC
#define MPTEST_MALLOC MY_MALLOC
#endif
#endif

/* config: REALLOC */
/* a realloc() function. Performs a memory reallocation. */
/* See https://en.cppreference.com/w/c/memory/realloc for more information. */
#if MPTEST_USE_CUSTOM_MALLOC
#ifndef MPTEST_REALLOC
#define MPTEST_REALLOC MY_REALLOC
#endif
#endif

/* config: FREE */
/* a free() function. Returns memory back to the operating system. */
/* See https://en.cppreference.com/w/c/memory/free for more information. */
#if MPTEST_USE_CUSTOM_MALLOC
#ifndef MPTEST_FREE
#define MPTEST_FREE MY_FREE
#endif
#endif

/* config: USE_CUSTOM_LONGJMP */
/* Set MPTEST_USE_CUSTOM_LONGJMP to 1 in order to use your own definitions for
 * setjmp(), longjmp() and jmp_buf. If MPTEST_USE_CUSTOM_LONGJMP is set, you
 * must also define MPTEST_SETJMP, MPTEST_LONGJMP and MPTEST_JMP_BUF.
 * Otherwise, <setjmp.h> is included and standard versions of setjmp(),
 * longjmp() and jmp_buf are used. */
#ifndef MPTEST_USE_CUSTOM_LONGJMP
#define MPTEST_USE_CUSTOM_LONGJMP 0
#endif

/* config: SETJMP */
/* setjmp() macro or function. Sets a nonlocal jump context. */
/* See https://en.cppreference.com/w/c/program/setjmp for more information. */
#if MPTEST_USE_CUSTOM_LONGJMP
#ifndef MPTEST_SETJMP
#define MPTEST_SETJMP MY_SETJMP
#endif
#endif

/* config: LONGJMP */
/* longjmp() macro or function. Jumps to a saved jmp_buf context. */
/* See https://en.cppreference.com/w/c/program/longjmp for more information. */
#if MPTEST_USE_CUSTOM_LONGJMP
#ifndef MPTEST_LONGJMP
#define MPTEST_LONGJMP MY_LONGJMP
#endif
#endif

/* config: JMP_BUF */
/* jmp_buf type. Holds a context saved by setjmp(). */
/* See https://en.cppreference.com/w/c/program/jmp_buf for more information. */
#if MPTEST_USE_CUSTOM_LONGJMP
#ifndef MPTEST_JMP_BUF
#define MPTEST_JMP_BUF my_jmp_buf
#endif
#endif

/* config: USE_CUSTOM_ASSERT */
/* Set MPTEST_USE_CUSTOM_ASSERT to 1 in order to use your own definition for
 * assert(). If MPTEST_USE_CUSTOM_ASSERT is set, you must also define
 * MPTEST_ASSERT. Otherwise, <assert.h> is included and a standard version of
 * assert() is used. */
#ifndef MPTEST_USE_CUSTOM_ASSERT
#define MPTEST_USE_CUSTOM_ASSERT 0
#endif

/* config: ASSERT */
/* an assert() macro. Expects a single argument, which is a expression that
 * evaluates to either 1 or 0. */
/* See https://en.cppreference.com/w/c/error/assert for more information. */
#if MPTEST_USE_CUSTOM_ASSERT
#ifndef MPTEST_ASSERT
#define MPTEST_ASSERT MY_ASSERT
#endif
#endif

/* config: USE_CUSTOM_SIZE_TYPE */
/* Set MPTEST_USE_CUSTOM_SIZE_TYPE to 1 in order to use your own definition for
 * size_t. If MPTEST_USE_CUSTOM_SIZE_TYPE is set, you must also define
 * MPTEST_SIZE_TYPE. Otherwise, <stddef.h> is included and a standard version
 * of size_t is used. */
#ifndef MPTEST_USE_CUSTOM_SIZE_TYPE
#define MPTEST_USE_CUSTOM_SIZE_TYPE 0
#endif

/* config: SIZE_TYPE */
/* a type that can store any size. */
/* See https://en.cppreference.com/w/c/types/size_t for more information. */
#if MPTEST_USE_CUSTOM_SIZE_TYPE
#ifndef MPTEST_SIZE_TYPE
#define MPTEST_SIZE_TYPE my_size_t
#endif
#endif

/* config: DEBUG */
/* Set MPTEST_DEBUG to 1 if you want to enable debug asserts,  bounds checks,
 * and more runtime diagnostics. */
#ifndef MPTEST_DEBUG
#define MPTEST_DEBUG 1
#endif

#endif
