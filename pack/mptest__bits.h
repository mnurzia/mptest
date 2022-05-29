#ifndef MPTEST_BITS_H
#define MPTEST_BITS_H

#include "mptest_config.h"
#include "mptest__bits_api.h"

/* bit: exports */
#define MPTEST_INTERNAL extern

/* bit: hook_malloc */
#if MPTEST_USE_DYN_ALLOC
#endif

/* bit: debug */
#if MPTEST_DEBUG
#include <stdio.h>
#define MPTEST__ASSERT_UNREACHED() MPTEST_ASSERT(0)
#else
#define MPTEST__ASSERT_UNREACHED() (void)(0)
#endif

/* bit: char */
typedef char mptest_char;

/* bit: ds_string */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
typedef struct mptest__str {
    mptest_size _size_short; /* does not include \0 */
    mptest_size _alloc; /* does not include \0 */
    mptest_char* _data;
} mptest__str;

void mptest__str_init(mptest__str* str);
int mptest__str_init_s(mptest__str* str, const mptest_char* s);
int mptest__str_init_n(mptest__str* str, const mptest_char* chrs, mptest_size n);
int mptest__str_init_copy(mptest__str* str, const mptest__str* in);
void mptest__str_init_move(mptest__str* str, mptest__str* old);
void mptest__str_destroy(mptest__str* str);
mptest_size mptest__str_size(const mptest__str* str);
int mptest__str_cat(mptest__str* str, const mptest__str* other);
int mptest__str_cat_s(mptest__str* str, const mptest_char* s);
int mptest__str_cat_n(mptest__str* str, const mptest_char* chrs, mptest_size n);
int mptest__str_push(mptest__str* str, mptest_char chr);
int mptest__str_insert(mptest__str* str, mptest_size index, mptest_char chr);
const mptest_char* mptest__str_get_data(const mptest__str* str);
int mptest__str_cmp(const mptest__str* str_a, const mptest__str* str_b);
mptest_size mptest__str_slen(const mptest_char* chars);

#endif
#endif

/* bit: ds_string_view */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
typedef struct mptest__str_view {
    const mptest_char* _data;
    mptest_size _size;
} mptest__str_view;

void mptest__str_view_init(mptest__str_view* view, const mptest__str* other);
void mptest__str_view_init_s(mptest__str_view* view, const mptest_char* chars);
void mptest__str_view_init_n(mptest__str_view* view, const mptest_char* chars, mptest_size n);
void mptest__str_view_init_null(mptest__str_view* view);
mptest_size mptest__str_view_size(const mptest__str_view* view);
const mptest_char* mptest__str_view_get_data(const mptest__str_view* view);
int mptest__str_view_cmp(const mptest__str_view* a, const mptest__str_view* b);
#endif
#endif

/* bit: ds_vector */
#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
#define MPTEST_VEC_TYPE(T) \
    T ## _vec

#define MPTEST_VEC_IDENT(T, name) \
    T ## _vec_ ## name

#define MPTEST_VEC_IDENT_INTERNAL(T, name) \
    T ## _vec__ ## name

#if MPTEST_DEBUG

#define MPTEST_VEC_CHECK(vec) \
    do { \
        /* ensure size is not greater than allocation size */ \
        MPTEST_ASSERT(vec->_size <= vec->_alloc); \
        /* ensure that data is not null if size is greater than 0 */ \
        MPTEST_ASSERT(vec->_size ? vec->_data != NULL : 1); \
    } while (0)

#else

#define MPTEST_VEC_CHECK(vec) MPTEST_UNUSED(vec)

#endif

#define MPTEST_VEC_DECL(T) \
    typedef struct MPTEST_VEC_TYPE(T) { \
        mptest_size _size; \
        mptest_size _alloc; \
        T* _data; \
    } MPTEST_VEC_TYPE(T)

#define MPTEST__VEC_DECL_init(T) \
    void MPTEST_VEC_IDENT(T, init)(MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_init(T) \
    void MPTEST_VEC_IDENT(T, init)(MPTEST_VEC_TYPE(T)* vec) { \
        vec->_size = 0; \
        vec->_alloc = 0; \
        vec->_data = MPTEST_NULL; \
    } 

#define MPTEST__VEC_DECL_destroy(T) \
    void MPTEST_VEC_IDENT(T, destroy)(MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_destroy(T) \
    void MPTEST_VEC_IDENT(T, destroy)(MPTEST_VEC_TYPE(T)* vec) { \
        MPTEST_VEC_CHECK(vec); \
        if (vec->_data != MPTEST_NULL) { \
            MPTEST_FREE(vec->_data); \
        } \
    }

#define MPTEST__VEC_GROW_ONE(T, vec) \
    do { \
        vec->_size += 1; \
        if (vec->_size > vec->_alloc) { \
            if (vec->_data == MPTEST_NULL) { \
                vec->_alloc = 1; \
                vec->_data = (T*)MPTEST_MALLOC(sizeof(T) * vec->_alloc); \
                if (vec->_data == MPTEST_NULL) { \
                    return 1; \
                } \
            } else { \
                vec->_alloc *= 2; \
                vec->_data = (T*)MPTEST_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
                if (vec->_data == MPTEST_NULL) { \
                    return 1; \
                } \
            } \
        } \
    } while (0)

#define MPTEST__VEC_GROW(T, vec, n) \
    do { \
        vec->_size += n; \
        if (vec->_size > vec->_alloc) { \
            vec->_alloc = vec->_size + (vec->_size >> 1); \
            if (vec->_data == MPTEST_NULL) { \
                vec->_data = (T*)MPTEST_MALLOC(sizeof(T) * vec->_alloc); \
            } else { \
                vec->_data = (T*)MPTEST_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
            } \
            if (vec->_data == MPTEST_NULL) { \
                return 1; \
            } \
        } \
    } while (0)

#define MPTEST__VEC_SETSIZE(T, vec, n) \
    do { \
        if (vec->_alloc < n) { \
            vec->_alloc = n; \
            if (vec->_data == MPTEST_NULL) { \
                vec->_data = (T*)MPTEST_MALLOC(sizeof(T) * vec->_alloc); \
            } else { \
                vec->_data = (T*)MPTEST_REALLOC(vec->_data, sizeof(T) * vec->_alloc); \
            } \
            if (vec->_data == MPTEST_NULL) { \
                return 1; \
            } \
        } \
    } while (0)

#define MPTEST__VEC_DECL_push(T) \
    int MPTEST_VEC_IDENT(T, push)(MPTEST_VEC_TYPE(T)* vec, T elem)

#define MPTEST__VEC_IMPL_push(T) \
    int MPTEST_VEC_IDENT(T, push)(MPTEST_VEC_TYPE(T)* vec, T elem) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST__VEC_GROW_ONE(T, vec); \
        vec->_data[vec->_size - 1] = elem; \
        MPTEST_VEC_CHECK(vec); \
        return 0; \
    }

#if MPTEST_DEBUG

#define MPTEST_VEC_CHECK_POP(vec) \
    do { \
        /* ensure that there is an element to pop */ \
        MPTEST_ASSERT(vec->_size > 0); \
    } while (0)

#else

#define MPTEST_VEC_CHECK_POP(vec) MPTEST_UNUSED(vec)

#endif

#define MPTEST__VEC_DECL_pop(T) \
    T MPTEST_VEC_IDENT(T, pop)(MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_pop(T) \
    T MPTEST_VEC_IDENT(T, pop)(MPTEST_VEC_TYPE(T)* vec) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_POP(vec); \
        return vec->_data[--vec->_size]; \
    }

#define MPTEST__VEC_DECL_cat(T) \
    T MPTEST_VEC_IDENT(T, cat)(MPTEST_VEC_TYPE(T)* vec, MPTEST_VEC_TYPE(T)* other)

#define MPTEST__VEC_IMPL_cat(T) \
    int MPTEST_VEC_IDENT(T, cat)(MPTEST_VEC_TYPE(T)* vec, MPTEST_VEC_TYPE(T)* other) { \
        re_size i; \
        re_size old_size = vec->_size; \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK(other); \
        MPTEST__VEC_GROW(T, vec, other->_size); \
        for (i = 0; i < other->_size; i++) { \
            vec->_data[old_size + i] = other->_data[i]; \
        } \
        MPTEST_VEC_CHECK(vec); \
        return 0; \
    }

#define MPTEST__VEC_DECL_insert(T) \
    int MPTEST_VEC_IDENT(T, insert)(MPTEST_VEC_TYPE(T)* vec, mptest_size index, T elem)

#define MPTEST__VEC_IMPL_insert(T) \
    int MPTEST_VEC_IDENT(T, insert)(MPTEST_VEC_TYPE(T)* vec, mptest_size index, T elem) { \
        mptest_size i; \
        mptest_size old_size = vec->_size; \
        MPTEST_VEC_CHECK(vec); \
        MPTEST__VEC_GROW_ONE(T, vec); \
        if (old_size != 0) { \
            for (i = old_size; i >= index + 1; i--) { \
                vec->_data[i] = vec->_data[i - 1]; \
            } \
        } \
        vec->_data[index] = elem; \
        return 0; \
    }

#define MPTEST__VEC_DECL_peek(T) \
    T MPTEST_VEC_IDENT(T, peek)(const MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_peek(T) \
    T MPTEST_VEC_IDENT(T, peek)(const MPTEST_VEC_TYPE(T)* vec) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_POP(vec); \
        return vec->_data[vec->_size - 1]; \
    }

#define MPTEST__VEC_DECL_clear(T) \
    void MPTEST_VEC_IDENT(T, clear)(MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_clear(T) \
    void MPTEST_VEC_IDENT(T, clear)(MPTEST_VEC_TYPE(T)* vec) { \
        MPTEST_VEC_CHECK(vec); \
        vec->_size = 0; \
    }

#define MPTEST__VEC_DECL_size(T) \
    mptest_size MPTEST_VEC_IDENT(T, size)(const MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_size(T) \
    mptest_size MPTEST_VEC_IDENT(T, size)(const MPTEST_VEC_TYPE(T)* vec) { \
        return vec->_size; \
    }

#if MPTEST_DEBUG

#define MPTEST_VEC_CHECK_BOUNDS(vec, idx) \
    do { \
        /* ensure that idx is within bounds */ \
        MPTEST_ASSERT(idx < vec->_size); \
    } while (0)

#else

#define MPTEST_VEC_CHECK_BOUNDS(vec, idx) \
    do { \
        MPTEST_UNUSED(vec); \
        MPTEST_UNUSED(idx); \
    } while (0) 

#endif

#define MPTEST__VEC_DECL_get(T) \
    T MPTEST_VEC_IDENT(T, get)(const MPTEST_VEC_TYPE(T)* vec, mptest_size idx)

#define MPTEST__VEC_IMPL_get(T) \
    T MPTEST_VEC_IDENT(T, get)(const MPTEST_VEC_TYPE(T)* vec, mptest_size idx) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_BOUNDS(vec, idx); \
        return vec->_data[idx]; \
    }

#define MPTEST__VEC_DECL_getref(T) \
    T* MPTEST_VEC_IDENT(T, getref)(MPTEST_VEC_TYPE(T)* vec, mptest_size idx)

#define MPTEST__VEC_IMPL_getref(T) \
    T* MPTEST_VEC_IDENT(T, getref)(MPTEST_VEC_TYPE(T)* vec, mptest_size idx) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_BOUNDS(vec, idx); \
        return &vec->_data[idx]; \
    }

#define MPTEST__VEC_DECL_getcref(T) \
    const T* MPTEST_VEC_IDENT(T, getcref)(const MPTEST_VEC_TYPE(T)* vec, mptest_size idx)

#define MPTEST__VEC_IMPL_getcref(T) \
    const T* MPTEST_VEC_IDENT(T, getcref)(const MPTEST_VEC_TYPE(T)* vec, mptest_size idx) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_BOUNDS(vec, idx); \
        return &vec->_data[idx]; \
    }

#define MPTEST__VEC_DECL_set(T) \
    void MPTEST_VEC_IDENT(T, set)(MPTEST_VEC_TYPE(T)* vec, mptest_size idx, T elem)

#define MPTEST__VEC_IMPL_set(T) \
    void MPTEST_VEC_IDENT(T, set)(MPTEST_VEC_TYPE(T)* vec, mptest_size idx, T elem) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST_VEC_CHECK_BOUNDS(vec, idx); \
        vec->_data[idx] = elem; \
    }

#define MPTEST__VEC_DECL_capacity(T) \
    mptest_size MPTEST_VEC_IDENT(T, capacity)(MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_capacity(T) \
    mptest_size MPTEST_VEC_IDENT(T, capacity)(MPTEST_VEC_TYPE(T)* vec) { \
        return vec->_alloc; \
    }

#define MPTEST__VEC_DECL_get_data(T) \
    const T* MPTEST_VEC_IDENT(T, get_data)(const MPTEST_VEC_TYPE(T)* vec)

#define MPTEST__VEC_IMPL_get_data(T) \
    const T* MPTEST_VEC_IDENT(T, get_data)(const MPTEST_VEC_TYPE(T)* vec) { \
        return vec->_data; \
    }

#define MPTEST__VEC_DECL_move(T) \
    void MPTEST_VEC_IDENT(T, move)(MPTEST_VEC_TYPE(T)* vec, MPTEST_VEC_TYPE(T)* old);

#define MPTEST__VEC_IMPL_move(T) \
    void MPTEST_VEC_IDENT(T, move)(MPTEST_VEC_TYPE(T)* vec, MPTEST_VEC_TYPE(T)* old) { \
        MPTEST_VEC_CHECK(old); \
        *vec = *old; \
        MPTEST_VEC_IDENT(T, init)(old); \
    }

#define MPTEST__VEC_DECL_reserve(T) \
    int MPTEST_VEC_IDENT(T, reserve)(MPTEST_VEC_TYPE(T)* vec, mptest_size cap);

#define MPTEST__VEC_IMPL_reserve(T) \
    int MPTEST_VEC_IDENT(T, reserve)(MPTEST_VEC_TYPE(T)* vec, mptest_size cap) { \
        MPTEST_VEC_CHECK(vec); \
        MPTEST__VEC_SETSIZE(T, vec, cap); \
    }

#define MPTEST_VEC_DECL_FUNC(T, func) \
    MPTEST__VEC_DECL_ ## func (T)

#define MPTEST_VEC_IMPL_FUNC(T, func) \
    MPTEST__VEC_IMPL_ ## func (T)
#endif
#endif

#endif
