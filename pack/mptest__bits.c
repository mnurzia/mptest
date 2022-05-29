
#include "mptest__bits.h"

int mptest__uint8_check[sizeof(mptest_uint8)==1];
int mptest__int8_check[sizeof(mptest_int8)==1];
int mptest__uint16_check[sizeof(mptest_uint16)==2];
int mptest__int16_check[sizeof(mptest_int16)==2];
int mptest__uint32_check[sizeof(mptest_uint32)==4];
int mptest__int32_check[sizeof(mptest_int32)==4];

#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
/* Maximum size, without null terminator */
#define MPTEST__STR_SHORT_SIZE_MAX (((sizeof(mptest__str) - sizeof(mptest_size)) / (sizeof(mptest_char)) - 1))

#define MPTEST__STR_GET_SHORT(str) !((str)->_size_short & 1)
#define MPTEST__STR_SET_SHORT(str, short) \
    do { \
        mptest_size temp = short; \
        (str)->_size_short &= ~((mptest_size)1); \
        (str)->_size_short |= !temp; \
    } while (0)
#define MPTEST__STR_GET_SIZE(str) ((str)->_size_short >> 1)
#define MPTEST__STR_SET_SIZE(str, size) \
    do { \
        mptest_size temp = size; \
        (str)->_size_short &= 1; \
        (str)->_size_short |= temp << 1; \
    } while (0)
#define MPTEST__STR_DATA(str) (MPTEST__STR_GET_SHORT(str) ? ((mptest_char*)&((str)->_alloc)) : (str)->_data)

/* Round up to multiple of 32 */
#define MPTEST__STR_ROUND_ALLOC(alloc) \
    (((alloc + 1) + 32) & (~((mptest_size)32)))

#if MPTEST_DEBUG

#define MPTEST__STR_CHECK(str) \
    do { \
        if (MPTEST__STR_GET_SHORT(str)) { \
            /* If string is short, the size must always be less than */ \
            /* MPTEST__STR_SHORT_SIZE_MAX. */ \
            MPTEST_ASSERT(MPTEST__STR_GET_SIZE(str) <= MPTEST__STR_SHORT_SIZE_MAX); \
        } else { \
            /* If string is long, the size can still be less, but the other */ \
            /* fields must be valid. */ \
            /* Ensure there is enough space */ \
            MPTEST_ASSERT((str)->_alloc >= MPTEST__STR_GET_SIZE(str)); \
            /* Ensure that the _data field isn't NULL if the size is 0 */ \
            if (MPTEST__STR_GET_SIZE(str) > 0) { \
                MPTEST_ASSERT((str)->_data != MPTEST_NULL); \
            } \
            /* Ensure that if _alloc is 0 then _data is NULL */ \
            if ((str)->_alloc == 0) { \
                MPTEST_ASSERT((str)->_data == MPTEST_NULL); \
            } \
        } \
        /* Ensure that there is a null-terminator */ \
        MPTEST_ASSERT(MPTEST__STR_DATA(str)[MPTEST__STR_GET_SIZE(str)] == '\0'); \
    } while (0)

#else

#define MPTEST__STR_CHECK(str) MPTEST_UNUSED(str)

#endif

void mptest__str_init(mptest__str* str) {
    str->_size_short = 0;
    MPTEST__STR_DATA(str)[0] = '\0';
}

void mptest__str_destroy(mptest__str* str) {
    if (!MPTEST__STR_GET_SHORT(str)) {
        if (str->_data != MPTEST_NULL) {
            MPTEST_FREE(str->_data);
        }
    }
}

mptest_size mptest__str_size(const mptest__str* str) {
    return MPTEST__STR_GET_SIZE(str);
}

MPTEST_INTERNAL int mptest__str_grow(mptest__str* str, mptest_size new_size) {
    mptest_size old_size = MPTEST__STR_GET_SIZE(str);
    MPTEST__STR_CHECK(str);
    if (MPTEST__STR_GET_SHORT(str)) {
        if (new_size <= MPTEST__STR_SHORT_SIZE_MAX) {
            /* Can still be a short str */
            MPTEST__STR_SET_SIZE(str, new_size);
        } else {
            /* Needs allocation */
            mptest_size new_alloc = 
                MPTEST__STR_ROUND_ALLOC(new_size + (new_size >> 1));
            mptest_char* new_data = (mptest_char*)MPTEST_MALLOC(sizeof(mptest_char) * (new_alloc + 1));
            mptest_size i;
            if (new_data == MPTEST_NULL) {
                return 1;
            }
            /* Copy data from old string */
            for (i = 0; i < old_size; i++) {
                new_data[i] = MPTEST__STR_DATA(str)[i];
            }
            /* Fill in the remaining fields */
            MPTEST__STR_SET_SHORT(str, 0);
            MPTEST__STR_SET_SIZE(str, new_size);
            str->_data = new_data;
            str->_alloc = new_alloc;
        }
    } else {
        if (new_size > str->_alloc) {
            /* Needs allocation */
            mptest_size new_alloc = 
                MPTEST__STR_ROUND_ALLOC(new_size + (new_size >> 1));
            mptest_char* new_data;
            if (str->_alloc == 0) {
                new_data = \
                    (mptest_char*)MPTEST_MALLOC(sizeof(mptest_char) * (new_alloc + 1));
            } else {
                new_data = \
                    (mptest_char*)MPTEST_REALLOC(
                        str->_data, sizeof(mptest_char) * (new_alloc + 1));
            }
            if (new_data == MPTEST_NULL) {
                return 1;
            }
            str->_data = new_data;
            str->_alloc = new_alloc;
        }
        MPTEST__STR_SET_SIZE(str, new_size);
    }
    /* Null terminate */
    MPTEST__STR_DATA(str)[MPTEST__STR_GET_SIZE(str)] = '\0';
    MPTEST__STR_CHECK(str);
    return 0;
}

int mptest__str_push(mptest__str* str, mptest_char chr) {
    int err = 0;
    mptest_size old_size = MPTEST__STR_GET_SIZE(str);
    if ((err = mptest__str_grow(str, old_size + 1))) {
        return err;
    }
    MPTEST__STR_DATA(str)[old_size] = chr;
    MPTEST__STR_CHECK(str);
    return err;
}

mptest_size mptest__str_slen(const mptest_char* s) {
    mptest_size out = 0;
    while (*(s++)) {
        out++;
    }
    return out;
}

int mptest__str_init_s(mptest__str* str, const mptest_char* s) {
    int err = 0;
    mptest_size i;
    mptest_size sz = mptest__str_slen(s);
    mptest__str_init(str);
    if ((err = mptest__str_grow(str, sz))) {
        return err;
    }
    for (i = 0; i < sz; i++) {
        MPTEST__STR_DATA(str)[i] = s[i];
    }
    return err;
}

int mptest__str_init_n(mptest__str* str, const mptest_char* chrs, mptest_size n) {
    int err = 0;
    mptest_size i;
    mptest__str_init(str);
    if ((err = mptest__str_grow(str, n))) {
        return err;
    }
    for (i = 0; i < n; i++) {
        MPTEST__STR_DATA(str)[i] = chrs[i];
    }
    return err;
}

int mptest__str_cat(mptest__str* str, const mptest__str* other) {
    int err = 0;
    mptest_size i;
    mptest_size n = MPTEST__STR_GET_SIZE(other);
    mptest_size old_size = MPTEST__STR_GET_SIZE(str);
    if ((err = mptest__str_grow(str, old_size + n))) {
        return err;
    }
    /* Copy data */
    for (i = 0; i < n; i++) {
        MPTEST__STR_DATA(str)[old_size + i] = MPTEST__STR_DATA(other)[i];
    }
    MPTEST__STR_CHECK(str);
    return err;
}

int mptest__str_cat_n(mptest__str* str, const mptest_char* chrs, mptest_size n) {
    int err = 0;
    mptest_size i;
    mptest_size old_size = MPTEST__STR_GET_SIZE(str);
    if ((err = mptest__str_grow(str, old_size + n))) {
        return err;
    }
    /* Copy data */
    for (i = 0; i < n; i++) {
        MPTEST__STR_DATA(str)[old_size + i] = chrs[i];
    }
    MPTEST__STR_CHECK(str);
    return err;
}

int mptest__str_cat_s(mptest__str* str, const mptest_char* chrs) {
    mptest_size chrs_size = mptest__str_slen(chrs);
    return mptest__str_cat_n(str, chrs, chrs_size);
}

int mptest__str_insert(mptest__str* str, mptest_size index, mptest_char chr) {
    int err = 0;
    mptest_size i;
    mptest_size old_size = MPTEST__STR_GET_SIZE(str);
    /* bounds check */
    MPTEST_ASSERT(index <= MPTEST__STR_GET_SIZE(str));
    if ((err = mptest__str_grow(str, old_size + 1))) {
        return err;
    }
    /* Shift data */
    if (old_size != 0) {
        for (i = old_size; i >= index + 1; i--) {
            MPTEST__STR_DATA(str)[i] = MPTEST__STR_DATA(str)[i - 1];
        }
    }
    MPTEST__STR_DATA(str)[index] = chr;
    MPTEST__STR_CHECK(str);
    return err;
}

const mptest_char* mptest__str_get_data(const mptest__str* str) {
    return MPTEST__STR_DATA(str);
}

int mptest__str_init_copy(mptest__str* str, const mptest__str* in) {
    mptest_size i;
    int err = 0;
    mptest__str_init(str);
    if ((err = mptest__str_grow(str, mptest__str_size(in)))) {
        return err;
    }
    for (i = 0; i < mptest__str_size(str); i++) {
        MPTEST__STR_DATA(str)[i] = MPTEST__STR_DATA(in)[i];
    }
    return err;
}

void mptest__str_init_move(mptest__str* str, mptest__str* old) {
    MPTEST__STR_CHECK(old);
    *str = *old;
    mptest__str_init(old);
}

int mptest__str_cmp(const mptest__str* str_a, const mptest__str* str_b) {
    mptest_size a_len = mptest__str_size(str_a);
    mptest_size b_len = mptest__str_size(str_b);
    const mptest_char* a_data = mptest__str_get_data(str_a);
    const mptest_char* b_data = mptest__str_get_data(str_b);
    mptest_size i;
    if (a_len < b_len) {
        return -1;
    } else if (a_len > b_len) {
        return 1;
    }
    for (i = 0; i < a_len; i++) {
        if (a_data[i] != b_data[i]) {
            if (a_data[i] < b_data[i]) {
                return -1;
            } else {
                return 1;
            }
        }
    }
    return 0;
}

#endif
#endif

#if MPTEST_USE_SYM
#if MPTEST_USE_DYN_ALLOC
void mptest__str_view_init(mptest__str_view* view, const mptest__str* other) {
    view->_size = mptest__str_size(other);
    view->_data = mptest__str_get_data(other);
}

void mptest__str_view_init_s(mptest__str_view* view, const mptest_char* chars) {
    view->_size = mptest__str_slen(chars);
    view->_data = chars;
}

void mptest__str_view_init_n(mptest__str_view* view, const mptest_char* chars, mptest_size n) {
    view->_size = n;
    view->_data = chars;
}

void mptest__str_view_init_null(mptest__str_view* view) {
    view->_size = 0;
    view->_data = MPTEST_NULL;
}

mptest_size mptest__str_view_size(const mptest__str_view* view) {
    return view->_size;
}

const mptest_char* mptest__str_view_get_data(const mptest__str_view* view) {
    return view->_data;
}

int mptest__str_view_cmp(const mptest__str_view* view_a, const mptest__str_view* view_b) {
    mptest_size a_len = mptest__str_view_size(view_a);
    mptest_size b_len = mptest__str_view_size(view_b);
    const mptest_char* a_data = mptest__str_view_get_data(view_a);
    const mptest_char* b_data = mptest__str_view_get_data(view_b);
    mptest_size i;
    if (a_len < b_len) {
        return -1;
    } else if (a_len > b_len) {
        return 1;
    }
    for (i = 0; i < a_len; i++) {
        if (a_data[i] != b_data[i]) {
            if (a_data[i] < b_data[i]) {
                return -1;
            } else {
                return 1;
            }
        }
    }
    return 0;
}
#endif
#endif
