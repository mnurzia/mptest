#if !defined(MN__MPTEST_INTERNAL_H)
#define MN__MPTEST_INTERNAL_H

#include "api.h"

/* bits/math/implies */
#define MN__IMPLIES(a, b) (!(a) || b)

/* bits/util/exports */
#if !defined(MN__SPLIT_BUILD)
#define MN_INTERNAL static
#else
#define MN_INTERNAL extern
#endif

#define MN_INTERNAL_DATA static

/* bits/util/preproc/token_paste */
#define MN__PASTE_0(a, b) a ## b
#define MN__PASTE(a, b) MN__PASTE_0(a, b)

/* bits/util/static_assert */
#define MN__STATIC_ASSERT(name, expr) char MN__PASTE(mn__, name)[(expr)==1]

/* bits/container/str */
typedef struct mn__str {
    mn_size _size_short; /* does not include \0 */
    mn_size _alloc; /* does not include \0 */
    mn_char* _data;
} mn__str;

void mn__str_init(mn__str* str);
int mn__str_init_s(mn__str* str, const mn_char* s);
int mn__str_init_n(mn__str* str, const mn_char* chrs, mn_size n);
int mn__str_init_copy(mn__str* str, const mn__str* in);
void mn__str_init_move(mn__str* str, mn__str* old);
void mn__str_destroy(mn__str* str);
mn_size mn__str_size(const mn__str* str);
int mn__str_cat(mn__str* str, const mn__str* other);
int mn__str_cat_s(mn__str* str, const mn_char* s);
int mn__str_cat_n(mn__str* str, const mn_char* chrs, mn_size n);
int mn__str_push(mn__str* str, mn_char chr);
int mn__str_insert(mn__str* str, mn_size index, mn_char chr);
const mn_char* mn__str_get_data(const mn__str* str);
int mn__str_cmp(const mn__str* str_a, const mn__str* str_b);
mn_size mn__str_slen(const mn_char* chars);

#if MPTEST_USE_DYN_ALLOC
#if MPTEST_USE_SYM
/* bits/container/str_view */
typedef struct mn__str_view {
    const mn_char* _data;
    mn_size _size;
} mn__str_view;

void mn__str_view_init(mn__str_view* view, const mn__str* other);
void mn__str_view_init_s(mn__str_view* view, const mn_char* chars);
void mn__str_view_init_n(mn__str_view* view, const mn_char* chars, mn_size n);
void mn__str_view_init_null(mn__str_view* view);
mn_size mn__str_view_size(const mn__str_view* view);
const mn_char* mn__str_view_get_data(const mn__str_view* view);
int mn__str_view_cmp(const mn__str_view* a, const mn__str_view* b);
#endif /* MPTEST_USE_DYN_ALLOC */
#endif /* MPTEST_USE_SYM */

#if MPTEST_USE_APARSE
/* bits/util/ntstr/cmp_n */
MN_INTERNAL mn__scmp_n(const char* a, mn_size a_size, const char* b);
#endif /* MPTEST_USE_APARSE */

/* bits/util/ntstr/len */
MN_INTERNAL mn_size mn__slen(const mn_char* s);

/* bits/util/unused */
#define MN__UNUSED(x) ((void)(x))

#if MPTEST_USE_APARSE
/* aparse */
#include <stdio.h>
typedef struct aparse__arg aparse__arg;

typedef struct aparse__arg_opt {
    char short_opt;
    const char* long_opt;
    mn_size long_opt_size;
} aparse__arg_opt;

typedef struct aparse__sub aparse__sub;

typedef struct aparse__arg_sub {
    aparse__sub* head;
    aparse__sub* tail;
} aparse__arg_sub;

typedef struct aparse__arg_pos {
    const char* name;
    mn_size name_size;
} aparse__arg_pos;

typedef union aparse__arg_contents {
    aparse__arg_opt opt;
    aparse__arg_sub sub;
    aparse__arg_pos pos;
} aparse__arg_contents;

enum aparse__arg_type
{
    /* Optional argument (-o, --o) */
    APARSE__ARG_TYPE_OPTIONAL,
    /* Positional argument */
    APARSE__ARG_TYPE_POSITIONAL,
    /* Subcommand argument */
    APARSE__ARG_TYPE_SUBCOMMAND
};

typedef aparse_error (*aparse__arg_parse_cb)(aparse__arg* arg, aparse__state* state,  mn_size sub_arg_idx, const char* text, mn_size text_size);
typedef void (*aparse__arg_destroy_cb)(aparse__arg* arg);

typedef union aparse__arg_callback_data_2 {
    void* plain;
    aparse_custom_cb custom_cb;
} aparse__arg_callback_data_2;

struct aparse__arg {
    enum aparse__arg_type type;
    aparse__arg_contents contents;
    const char* help;
    mn_size help_size;
    const char* metavar;
    mn_size metavar_size;
    aparse_nargs nargs;
    int required;
    int was_specified;
    aparse__arg* next;
    aparse__arg_parse_cb callback;
    aparse__arg_destroy_cb destroy;
    void* callback_data;
    aparse__arg_callback_data_2 callback_data_2;
};

#define APARSE__STATE_OUT_BUF_SIZE 128

typedef struct aparse__state_root {
    char out_buf[APARSE__STATE_OUT_BUF_SIZE];
    mn_size out_buf_ptr;
    const char* prog_name;
    mn_size prog_name_size;
} aparse__state_root;

struct aparse__state {
    aparse__arg* head;
    aparse__arg* tail;
    const char* help;
    mn_size help_size;
    aparse_out_cb out_cb;
    void* user;
    aparse__state_root* root;
    int is_root;
};

struct aparse__sub {
    const char* name;
    mn_size name_size;
    aparse__state subparser;
    aparse__sub* next;
};

MN_INTERNAL void aparse__arg_init(aparse__arg* arg);
MN_INTERNAL void aparse__arg_destroy(aparse__arg* arg);
MN_INTERNAL void aparse__state_init_from(aparse__state* state, aparse__state* other);
MN_INTERNAL void aparse__state_init(aparse__state* state);
MN_INTERNAL void aparse__state_destroy(aparse__state* state);
MN_INTERNAL void aparse__state_set_out_cb(aparse__state* state, aparse_out_cb out_cb, void* user);
MN_INTERNAL void aparse__state_reset(aparse__state* state);
MN_INTERNAL aparse_error aparse__state_add_opt(aparse__state* state, char short_opt, const char* long_opt);
MN_INTERNAL aparse_error aparse__state_add_pos(aparse__state* state, const char* name);
MN_INTERNAL aparse_error aparse__state_add_sub(aparse__state* state);

MN_INTERNAL void aparse__state_check_before_add(aparse__state* state);
MN_INTERNAL void aparse__state_check_before_modify(aparse__state* state);
MN_INTERNAL void aparse__state_check_before_set_type(aparse__state* state);
MN_INTERNAL aparse_error aparse__state_flush(aparse__state* state);
MN_INTERNAL aparse_error aparse__state_out(aparse__state* state, char out);
MN_INTERNAL aparse_error aparse__state_out_s(aparse__state* state, const char* s);
MN_INTERNAL aparse_error aparse__state_out_n(aparse__state* state, const char* s, mn_size n);

MN_INTERNAL void aparse__arg_bool_init(aparse__arg* arg, int* out);
MN_INTERNAL void aparse__arg_str_init(aparse__arg* arg, const char** out, mn_size* out_size);
MN_INTERNAL void aparse__arg_help_init(aparse__arg* arg);
MN_INTERNAL void aparse__arg_version_init(aparse__arg* arg);
MN_INTERNAL void aparse__arg_custom_init(aparse__arg* arg, aparse_custom_cb cb, void* user, aparse_nargs nargs);
MN_INTERNAL void aparse__arg_sub_init(aparse__arg* arg);

MN_API aparse_error aparse__parse_argv(aparse__state* state, int argc, const char* const* argv);

MN_INTERNAL aparse_error aparse__error_begin(aparse__state* state);
MN_INTERNAL aparse_error aparse__error_begin_arg(aparse__state* state, const aparse__arg* arg);
MN_INTERNAL aparse_error aparse__error_unrecognized_arg(aparse__state* state, const char* arg);
MN_INTERNAL aparse_error aparse__error_quote(aparse__state* state, const char* text, mn_size text_size);
#endif /* MPTEST_USE_APARSE */

#if MPTEST_USE_DYN_ALLOC
#if MPTEST_USE_SYM
/* bits/container/vec */
#define MN__VEC_TYPE(T) \
    MN__PASTE(T, _vec)

#define MN__VEC_IDENT(T, name) \
    MN__PASTE(T, MN__PASTE(_vec_, name))

#define MN__VEC_IDENT_INTERNAL(T, name) \
    MN__PASTE(T, MN__PASTE(_vec__, name))

#define MN__VEC_DECL_FUNC(T, func) \
    MN__PASTE(MN__VEC_DECL_, func)(T)

#define MN__VEC_IMPL_FUNC(T, func) \
    MN__PASTE(MN__VEC_IMPL_, func)(T)

#if MN_DEBUG

#define MN__VEC_CHECK(vec) \
    do { \
        /* ensure size is not greater than allocation size */ \
        MN_ASSERT(vec->_size <= vec->_alloc); \
        /* ensure that data is not null if size is greater than 0 */ \
        MN_ASSERT(vec->_size ? vec->_data != MN_NULL : 1); \
    } while (0)

#else

#define MN__VEC_CHECK(vec) MN__UNUSED(vec)

#endif

#define MN__VEC_DECL(T) \
    typedef struct MN__VEC_TYPE(T) { \
        mn_size _size; \
        mn_size _alloc; \
        T* _data; \
    } MN__VEC_TYPE(T)

#define MN__VEC_DECL_init(T) \
    void MN__VEC_IDENT(T, init)(MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_init(T) \
    void MN__VEC_IDENT(T, init)(MN__VEC_TYPE(T)* vec) { \
        vec->_size = 0; \
        vec->_alloc = 0; \
        vec->_data = MN_NULL; \
    } 

#define MN__VEC_DECL_destroy(T) \
    void MN__VEC_IDENT(T, destroy)(MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_destroy(T) \
    void MN__VEC_IDENT(T, destroy)(MN__VEC_TYPE(T)* vec) { \
        MN__VEC_CHECK(vec); \
        if (vec->_data != MN_NULL) { \
            MN_FREE(vec->_data); \
        } \
    }

#define MN__VEC_GROW_ONE(T, vec) \
    do { \
        vec->_size += 1; \
        if (vec->_size > vec->_alloc) { \
            if (vec->_data == MN_NULL) { \
                vec->_alloc = 1; \
                vec->_data = (T*)MN_MALLOC(sizeof(T) * vec->_alloc); \
                if (vec->_data == MN_NULL) { \
                    return -1; \
                } \
            } else { \
                vec->_alloc *= 2; \
                vec->_data = (T*)MN_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
                if (vec->_data == MN_NULL) { \
                    return -1; \
                } \
            } \
        } \
    } while (0)

#define MN__VEC_GROW(T, vec, n) \
    do { \
        vec->_size += n; \
        if (vec->_size > vec->_alloc) { \
            vec->_alloc = vec->_size + (vec->_size >> 1); \
            if (vec->_data == MN_NULL) { \
                vec->_data = (T*)MN_MALLOC(sizeof(T) * vec->_alloc); \
            } else { \
                vec->_data = (T*)MN_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
            } \
            if (vec->_data == MN_NULL) { \
                return -1; \
            } \
        } \
    } while (0)

#define MN__VEC_SETSIZE(T, vec, n) \
    do { \
        if (vec->_alloc < n) { \
            vec->_alloc = n; \
            if (vec->_data == MN_NULL) { \
                vec->_data = (T*)MN_MALLOC(sizeof(T) * vec->_alloc); \
            } else { \
                vec->_data = (T*)MN_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
            } \
            if (vec->_data == MN_NULL) { \
                return -1; \
            } \
        } \
    } while (0)

#define MN__VEC_DECL_push(T) \
    int MN__VEC_IDENT(T, push)(MN__VEC_TYPE(T)* vec, T elem)

#define MN__VEC_IMPL_push(T) \
    int MN__VEC_IDENT(T, push)(MN__VEC_TYPE(T)* vec, T elem) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_GROW_ONE(T, vec); \
        vec->_data[vec->_size - 1] = elem; \
        MN__VEC_CHECK(vec); \
        return 0; \
    }

#if MN_DEBUG

#define MN__VEC_CHECK_POP(vec) \
    do { \
        /* ensure that there is an element to pop */ \
        MN_ASSERT(vec->_size > 0); \
    } while (0)

#else

#define MN__VEC_CHECK_POP(vec) MN__UNUSED(vec)

#endif

#define MN__VEC_DECL_pop(T) \
    T MN__VEC_IDENT(T, pop)(MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_pop(T) \
    T MN__VEC_IDENT(T, pop)(MN__VEC_TYPE(T)* vec) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_POP(vec); \
        return vec->_data[--vec->_size]; \
    }

#define MN__VEC_DECL_cat(T) \
    T MN__VEC_IDENT(T, cat)(MN__VEC_TYPE(T)* vec, MN__VEC_TYPE(T)* other)

#define MN__VEC_IMPL_cat(T) \
    int MN__VEC_IDENT(T, cat)(MN__VEC_TYPE(T)* vec, MN__VEC_TYPE(T)* other) { \
        re_size i; \
        re_size old_size = vec->_size; \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK(other); \
        MN__VEC_GROW(T, vec, other->_size); \
        for (i = 0; i < other->_size; i++) { \
            vec->_data[old_size + i] = other->_data[i]; \
        } \
        MN__VEC_CHECK(vec); \
        return 0; \
    }

#define MN__VEC_DECL_insert(T) \
    int MN__VEC_IDENT(T, insert)(MN__VEC_TYPE(T)* vec, mn_size index, T elem)

#define MN__VEC_IMPL_insert(T) \
    int MN__VEC_IDENT(T, insert)(MN__VEC_TYPE(T)* vec, mn_size index, T elem) { \
        mn_size i; \
        mn_size old_size = vec->_size; \
        MN__VEC_CHECK(vec); \
        MN__VEC_GROW_ONE(T, vec); \
        if (old_size != 0) { \
            for (i = old_size; i >= index + 1; i--) { \
                vec->_data[i] = vec->_data[i - 1]; \
            } \
        } \
        vec->_data[index] = elem; \
        return 0; \
    }

#define MN__VEC_DECL_peek(T) \
    T MN__VEC_IDENT(T, peek)(const MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_peek(T) \
    T MN__VEC_IDENT(T, peek)(const MN__VEC_TYPE(T)* vec) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_POP(vec); \
        return vec->_data[vec->_size - 1]; \
    }

#define MN__VEC_DECL_clear(T) \
    void MN__VEC_IDENT(T, clear)(MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_clear(T) \
    void MN__VEC_IDENT(T, clear)(MN__VEC_TYPE(T)* vec) { \
        MN__VEC_CHECK(vec); \
        vec->_size = 0; \
    }

#define MN__VEC_DECL_size(T) \
    mn_size MN__VEC_IDENT(T, size)(const MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_size(T) \
    mn_size MN__VEC_IDENT(T, size)(const MN__VEC_TYPE(T)* vec) { \
        return vec->_size; \
    }

#if MN_DEBUG

#define MN__VEC_CHECK_BOUNDS(vec, idx) \
    do { \
        /* ensure that idx is within bounds */ \
        MN_ASSERT(idx < vec->_size); \
    } while (0)

#else

#define MN__VEC_CHECK_BOUNDS(vec, idx) \
    do { \
        MN__UNUSED(vec); \
        MN__UNUSED(idx); \
    } while (0) 

#endif

#define MN__VEC_DECL_get(T) \
    T MN__VEC_IDENT(T, get)(const MN__VEC_TYPE(T)* vec, mn_size idx)

#define MN__VEC_IMPL_get(T) \
    T MN__VEC_IDENT(T, get)(const MN__VEC_TYPE(T)* vec, mn_size idx) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_BOUNDS(vec, idx); \
        return vec->_data[idx]; \
    }

#define MN__VEC_DECL_getref(T) \
    T* MN__VEC_IDENT(T, getref)(MN__VEC_TYPE(T)* vec, mn_size idx)

#define MN__VEC_IMPL_getref(T) \
    T* MN__VEC_IDENT(T, getref)(MN__VEC_TYPE(T)* vec, mn_size idx) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_BOUNDS(vec, idx); \
        return &vec->_data[idx]; \
    }

#define MN__VEC_DECL_getcref(T) \
    const T* MN__VEC_IDENT(T, getcref)(const MN__VEC_TYPE(T)* vec, mn_size idx)

#define MN__VEC_IMPL_getcref(T) \
    const T* MN__VEC_IDENT(T, getcref)(const MN__VEC_TYPE(T)* vec, mn_size idx) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_BOUNDS(vec, idx); \
        return &vec->_data[idx]; \
    }

#define MN__VEC_DECL_set(T) \
    void MN__VEC_IDENT(T, set)(MN__VEC_TYPE(T)* vec, mn_size idx, T elem)

#define MN__VEC_IMPL_set(T) \
    void MN__VEC_IDENT(T, set)(MN__VEC_TYPE(T)* vec, mn_size idx, T elem) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_CHECK_BOUNDS(vec, idx); \
        vec->_data[idx] = elem; \
    }

#define MN__VEC_DECL_capacity(T) \
    mn_size MN__VEC_IDENT(T, capacity)(MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_capacity(T) \
    mn_size MN__VEC_IDENT(T, capacity)(MN__VEC_TYPE(T)* vec) { \
        return vec->_alloc; \
    }

#define MN__VEC_DECL_get_data(T) \
    const T* MN__VEC_IDENT(T, get_data)(const MN__VEC_TYPE(T)* vec)

#define MN__VEC_IMPL_get_data(T) \
    const T* MN__VEC_IDENT(T, get_data)(const MN__VEC_TYPE(T)* vec) { \
        return vec->_data; \
    }

#define MN__VEC_DECL_move(T) \
    void MN__VEC_IDENT(T, move)(MN__VEC_TYPE(T)* vec, MN__VEC_TYPE(T)* old);

#define MN__VEC_IMPL_move(T) \
    void MN__VEC_IDENT(T, move)(MN__VEC_TYPE(T)* vec, MN__VEC_TYPE(T)* old) { \
        MN__VEC_CHECK(old); \
        *vec = *old; \
        MN__VEC_IDENT(T, init)(old); \
    }

#define MN__VEC_DECL_reserve(T) \
    int MN__VEC_IDENT(T, reserve)(MN__VEC_TYPE(T)* vec, mn_size cap);

#define MN__VEC_IMPL_reserve(T) \
    int MN__VEC_IDENT(T, reserve)(MN__VEC_TYPE(T)* vec, mn_size cap) { \
        MN__VEC_CHECK(vec); \
        MN__VEC_SETSIZE(T, vec, cap); \
        return 0; \
    }
#endif /* MPTEST_USE_DYN_ALLOC */
#endif /* MPTEST_USE_SYM */

#endif /* MN__MPTEST_INTERNAL_H */
