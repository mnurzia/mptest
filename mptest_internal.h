#ifndef MPTEST_INTERNAL_H
#define MPTEST_INTERNAL_H

#include "mptest_api.h"
#include "_cpack/internal.h"

#include <stdio.h>

MN_INTERNAL enum mptest__result mptest__state_do_run_test(struct mptest__state* state, mptest__test_func test_func);
MN_INTERNAL void mptest__state_print_indent(struct mptest__state* state);

#if MPTEST_USE_LONGJMP

MN_INTERNAL void mptest__longjmp_init(struct mptest__state* state);
MN_INTERNAL void mptest__longjmp_destroy(struct mptest__state* state);

#endif

#if MPTEST_USE_LEAKCHECK
    /* Number of guard bytes to put at the top of each block. */
    #define MPTEST__LEAKCHECK_GUARD_BYTES_COUNT 16

/* Flags kept for each block. */
enum mptest__leakcheck_block_flags
{
    /* The block was allocated with malloc(). */
    MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL = 1,
    /* The block was freed with free(). */
    MPTEST__LEAKCHECK_BLOCK_FLAG_FREED = 2,
    /* The block was the *input* of a reallocation with realloc(). */
    MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD = 4,
    /* The block was the *result* of a reallocation with realloc(). */
    MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW = 8
};

/* Header kept in memory before each allocation. */
struct mptest__leakcheck_header {
    /* Guard bytes (like a magic number, signifies proper allocation) */
    unsigned char guard_bytes[MPTEST__LEAKCHECK_GUARD_BYTES_COUNT];
    /* Block reference */
    struct mptest__leakcheck_block* block;
};

/* Structure that keeps track of a header's properties. */
struct mptest__leakcheck_block {
    /* Pointer to the header that exists right before the memory. */
    struct mptest__leakcheck_header* header;
    /* Size of block as passed to malloc() or realloc() */
    size_t block_size;
    /* Previous and next block records */
    struct mptest__leakcheck_block* prev;
    struct mptest__leakcheck_block* next;
    /* Realloc chain previous and next */
    struct mptest__leakcheck_block* realloc_prev;
    struct mptest__leakcheck_block* realloc_next;
    /* Flags (see `enum mptest__leakcheck_block_flags`) */
    enum mptest__leakcheck_block_flags flags;
    /* Source location where the malloc originated */
    const char* file;
    int         line;
};

    #define MPTEST__LEAKCHECK_HEADER_SIZEOF                                   \
        (sizeof(struct mptest__leakcheck_header))

MN_INTERNAL void mptest__leakcheck_init(struct mptest__state* state);
MN_INTERNAL void mptest__leakcheck_destroy(struct mptest__state* state);
MN_INTERNAL void mptest__leakcheck_reset(struct mptest__state* state);
MN_INTERNAL int mptest__leakcheck_has_leaks(struct mptest__state* state);
MN_INTERNAL int mptest__leakcheck_block_has_freeable(
    struct mptest__leakcheck_block* block);
#endif

#if MPTEST_USE_COLOR
    #define MPTEST__COLOR_PASS       "\x1b[1;32m" /* Pass messages */
    #define MPTEST__COLOR_FAIL       "\x1b[1;31m" /* Fail messages */
    #define MPTEST__COLOR_TEST_NAME  "\x1b[1;36m" /* Test names */
    #define MPTEST__COLOR_SUITE_NAME "\x1b[1;35m" /* Suite names */
    #define MPTEST__COLOR_EMPHASIS   "\x1b[1m"    /* Important numbers */
    #define MPTEST__COLOR_RESET      "\x1b[0m"    /* Regular text */
#else
    #define MPTEST__COLOR_PASS       ""
    #define MPTEST__COLOR_FAIL       ""
    #define MPTEST__COLOR_TEST_NAME  ""
    #define MPTEST__COLOR_SUITE_NAME ""
    #define MPTEST__COLOR_EMPHASIS   ""
    #define MPTEST__COLOR_RESET      ""
#endif

#if MPTEST_USE_TIME
MN_INTERNAL void mptest__time_init(struct mptest__state* state);
MN_INTERNAL void mptest__time_destroy(struct mptest__state* state);
#endif

#if MPTEST_USE_APARSE
MN_INTERNAL int mptest__aparse_init(struct mptest__state* state);
MN_INTERNAL void mptest__aparse_destroy(struct mptest__state* state);
MN_INTERNAL int mptest__aparse_match_test_name(struct mptest__state* state, const char* test_name);
MN_INTERNAL int mptest__aparse_match_suite_name(struct mptest__state* state, const char* suite_name);
#endif

#if MPTEST_USE_FUZZ
MN_INTERNAL void mptest__fuzz_init(struct mptest__state* state);
MN_INTERNAL enum mptest__result mptest__fuzz_run_test(struct mptest__state* state, mptest__test_func test_func);
MN_INTERNAL void mptest__fuzz_print(struct mptest__state* state);
#endif

#if MPTEST_USE_SYM
MN_INTERNAL void mptest__sym_dump(mptest_sym* sym, mn_int32 parent_ref, mn_int32 indent);
#endif

#endif
