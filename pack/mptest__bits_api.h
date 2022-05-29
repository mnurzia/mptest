#ifndef MPTEST_BITS_API_H
#define MPTEST_BITS_API_H

#include "mptest_config.h"

/* bit: version */
#define MPTEST_VERSION_MAJOR 0
#define MPTEST_VERSION_MINOR 0
#define MPTEST_VERSION_PATCH 1
#define MPTEST_VERSION_STRING "0.0.1"

/* bit: exports */
#if MPTEST_STATIC
    #define MPTEST_API static
#else
    #define MPTEST_API extern
#endif

/* bit: cstd */
/* If __STDC__ is not defined, assume C89. */
#ifndef __STDC__
  #define MPTEST__CSTD 1989
#else
  #if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 201112L
      #define MPTEST__CSTD 2011
    #elif __STDC_VERSION__ >= 199901L
      #define MPTEST__CSTD 1999
    #else
      #define MPTEST__CSTD 1989
    #endif
  #endif
#endif

/* bit: hook_malloc */
#if MPTEST_USE_DYN_ALLOC
#if !MPTEST_USE_CUSTOM_MALLOC
#include <stdlib.h>
#define MPTEST_MALLOC malloc
#define MPTEST_REALLOC realloc
#define MPTEST_FREE free
#else
#if !defined(MPTEST_MALLOC) || !defined(MPTEST_REALLOC) || !defined(MPTEST_FREE)
#error In order to use MPTEST_USE_CUSTOM_MALLOC you must provide defnitions for MPTEST_MALLOC, MPTEST_REALLOC and MPTEST_FREE.
#endif
#endif
#endif

/* bit: hook_longjmp */
#if !MPTEST_USE_CUSTOM_LONGJMP
#include <setjmp.h>
#define MPTEST_SETJMP setjmp
#define MPTEST_LONGJMP longjmp
#define MPTEST_JMP_BUF jmp_buf
#else
#if !defined(MPTEST_SETJMP) || !defined(MPTEST_LONGJMP) || !defined(MPTEST_JMP_BUF)
#error In order to use MPTEST_USE_CUSTOM_LONGJMP you must provide defnitions for MPTEST_SETJMP, MPTEST_LONGJMP and MPTEST_JMP_BUF.
#endif
#endif

/* bit: hook_assert */
#if !MPTEST_USE_CUSTOM_ASSERT
#include <assert.h>
#define MPTEST_ASSERT assert
#else
#if !defined(MPTEST_ASSERT)
#error In order to use MPTEST_USE_CUSTOM_ASSERT you must provide a definition for MPTEST_ASSERT.
#endif
#endif

/* bit: inttypes */
#if MPTEST__CSTD >= 1999
#include <stdint.h>
typedef uint8_t mptest_uint8;
typedef int8_t mptest_int8;
typedef uint16_t mptest_uint16;
typedef int16_t mptest_int16;
typedef uint32_t mptest_uint32;
typedef int32_t mptest_int32;
#else
typedef unsigned char mptest_uint8;
typedef signed char mptest_int8;
typedef unsigned short mptest_uint16;
typedef signed short mptest_int16;
typedef unsigned int mptest_uint32;
typedef signed int mptest_int32;
#endif

/* bit: null */
#define MPTEST_NULL 0

/* bit: sizet */
#if !MPTEST_USE_CUSTOM_SIZE_TYPE
#include <stddef.h>
#define MPTEST_SIZE_TYPE size_t
#else
#if !defined(MPTEST_SIZE_TYPE)
#error In order to use MPTEST_USE_CUSTOM_SIZE_TYPE you must provide a definition for MPTEST_SIZE_TYPE.
#endif
#endif
typedef MPTEST_SIZE_TYPE mptest_size;

/* bit: unused */
#define MPTEST__UNUSED(i) (void)(i)

/* bit: ds_string */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
#endif
#endif

/* bit: ds_string_view */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
#endif
#endif

/* bit: ds_vector */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
#endif
#endif

#endif
