#if !defined(MN__MPTEST_API_H)
#define MN__MPTEST_API_H

#include "config.h"

/* bits/types/size */
typedef MN_SIZE_TYPE mn_size;

/* bits/util/cstd */
/* If __STDC__ is not defined, assume C89. */
#ifndef __STDC__
    #define MN__CSTD 1989
#else
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #define MN__CSTD 2011
        #elif __STDC_VERSION__ >= 199901L
            #define MN__CSTD 1999
        #else
            #define MN__CSTD 1989
        #endif
    #endif
#endif

/* bits/util/debug */
#if !MN_DEBUG
#if defined(NDEBUG)
#if NDEBUG == 0
#define MN_DEBUG 1
#else
#define MN_DEBUG 0
#endif
#endif
#endif

/* bits/util/exports */
#if !defined(MN__SPLIT_BUILD)
#if MN_STATIC
#define MN_API static
#else
#define MN_API extern
#endif
#else
#define MN_API extern
#endif

/* bits/util/null */
#define MN_NULL 0

/* bits/types/char */
#if !defined(MN_CHAR_TYPE)
#define MN_CHAR_TYPE char
#endif

typedef MN_CHAR_TYPE mn_char;

#if MPTEST_USE_SYM
/* bits/types/fixed/int32 */
#if !defined(MN_INT32_TYPE)
#if MN__CSTD >= 1999
#include <stdint.h>
#define MN_INT32_TYPE int32_t
#else
#define MN_INT32_TYPE signed int
#endif
#endif

typedef MN_INT32_TYPE mn_int32;
#endif /* MPTEST_USE_SYM */

#if MPTEST_USE_APARSE
/* aparse */
#include <stddef.h>
#define APARSE_ERROR_NONE 0
#define APARSE_ERROR_NOMEM -1
#define APARSE_ERROR_PARSE -2
#define APARSE_ERROR_OUT -3
#define APARSE_ERROR_INVALID -4
#define APARSE_ERROR_SHOULD_EXIT -5
#define APARSE_SHOULD_EXIT APARSE_ERROR_SHOULD_EXIT

/* Syntax validity table: */
/* Where `O` is the option in question, `s` is a subargument to `O`, `P` is
 * another option (could be the same option), and `o` is the long option form
 * of `O`. */
/* |  args       | -O  | -O=s | -OP | -Os | -O s | --o | --o=s | --o s |
 * | ----------- | --- | ---- | --- | --- | ---- | --- | ----- | ----- |
 * | 2+          |     |      |     |     | *    |     |       | *     |
 * | 1           |     | *    |     | *   | *    |     | *     | *     |
 * | 0           | *   |      | *   |     |      | *   |       |       |
 * | <0_OR_1>    | *   | *    | *   | *   | *    | *   | *     | *     |
 * | <0_OR_1_EQ> | *   | *    | *   |     |      |     | *     |       |
 * | <0_OR_MORE> | *   | *    | *   | *   | *    | *   | *     | *     |
 * | <1_OR_MORE> |     | *    |     | *   | *    |     | *     | *     | */
typedef enum aparse_nargs
{
    /* Parse either zero or 1 subarguments. */
    APARSE_NARGS_0_OR_1 = -1, /* Like regex '?' */
    /* Parse either zero or 1 subarguments, but only allow using '='. */
    APARSE_NARGS_0_OR_1_EQ = -2, /* Like regex '?' */
    /* Parse zero or more subarguments. */
    APARSE_NARGS_0_OR_MORE = -3, /* Like regex '*' */
    /* Parse one or more subarguments. */
    APARSE_NARGS_1_OR_MORE = -4 /* Like regex '+' */
} aparse_nargs;

typedef int aparse_error;
typedef struct aparse__state aparse__state;

typedef struct aparse_state {
    aparse__state* state;
} aparse_state;

typedef aparse_error (*aparse_out_cb)(void* user, const char* buf, mn_size buf_size);
typedef aparse_error (*aparse_custom_cb)(void* user, aparse_state* state, int sub_arg_idx, const char* text, mn_size text_size);

MN_API aparse_error aparse_init(aparse_state* state);
MN_API void aparse_set_out_cb(aparse_state* state, aparse_out_cb out_cb, void* user);
MN_API void aparse_destroy(aparse_state* state);

MN_API aparse_error aparse_add_opt(aparse_state* state, char short_opt, const char* long_opt);
MN_API aparse_error aparse_add_pos(aparse_state* state, const char* name);
MN_API aparse_error aparse_add_sub(aparse_state* state);

MN_API void aparse_arg_help(aparse_state* state, const char* help_text);
MN_API void aparse_arg_metavar(aparse_state* state, const char* metavar);

MN_API void aparse_arg_type_bool(aparse_state* state, int* out);
MN_API void aparse_arg_type_str(aparse_state* state, const char** out, mn_size* out_size);
MN_API void aparse_arg_type_help(aparse_state* state);
MN_API void aparse_arg_type_version(aparse_state* state);
MN_API void aparse_arg_type_custom(aparse_state* state, aparse_custom_cb cb, void* user, aparse_nargs nargs);

/* MN_API void aparse_arg_int_multi(aparse_state* state, int** out, mn_size* out_size) */
/* MN_API void aparse_arg_str_multi(aparse_state* state, const char** out, mn_size** size_out, mn_size* num) */

MN_API int aparse_sub_add_cmd(aparse_state* state, const char* name, aparse_state** subcmd);

MN_API aparse_error aparse_parse(aparse_state* state, int argc, const char* const* argv);
#endif /* MPTEST_USE_APARSE */

#endif /* MN__MPTEST_API_H */
