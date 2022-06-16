#include "internal.h"

/* bits/types/char */
MN__STATIC_ASSERT(mn__char_is_one_byte, sizeof(mn_char) == 1);

/* bits/container/str */
/* Maximum size, without null terminator */
#define MN__STR_SHORT_SIZE_MAX                                                \
    (((sizeof(mn__str) - sizeof(mn_size)) / (sizeof(mn_char)) - 1))

#define MN__STR_GET_SHORT(str) !((str)->_size_short & 1)
#define MN__STR_SET_SHORT(str, short)                                         \
    do {                                                                      \
        mn_size temp = short;                                                 \
        (str)->_size_short &= ~((mn_size)1);                                  \
        (str)->_size_short |= !temp;                                          \
    } while (0)
#define MN__STR_GET_SIZE(str) ((str)->_size_short >> 1)
#define MN__STR_SET_SIZE(str, size)                                           \
    do {                                                                      \
        mn_size temp = size;                                                  \
        (str)->_size_short &= 1;                                              \
        (str)->_size_short |= temp << 1;                                      \
    } while (0)
#define MN__STR_DATA(str)                                                     \
    (MN__STR_GET_SHORT(str) ? ((mn_char*)&((str)->_alloc)) : (str)->_data)

/* Round up to multiple of 32 */
#define MN__STR_ROUND_ALLOC(alloc) (((alloc + 1) + 32) & (~((mn_size)32)))

#if MN_DEBUG

#define MN__STR_CHECK(str)                                                    \
    do {                                                                      \
        if (MN__STR_GET_SHORT(str)) {                                         \
            /* If string is short, the size must always be less than */       \
            /* MN__STR_SHORT_SIZE_MAX. */                                     \
            MN_ASSERT(MN__STR_GET_SIZE(str) <= MN__STR_SHORT_SIZE_MAX);       \
        } else {                                                              \
            /* If string is long, the size can still be less, but the other   \
             */                                                               \
            /* fields must be valid. */                                       \
            /* Ensure there is enough space */                                \
            MN_ASSERT((str)->_alloc >= MN__STR_GET_SIZE(str));                \
            /* Ensure that the _data field isn't NULL if the size is 0 */     \
            if (MN__STR_GET_SIZE(str) > 0) {                                  \
                MN_ASSERT((str)->_data != MN_NULL);                           \
            }                                                                 \
            /* Ensure that if _alloc is 0 then _data is NULL */               \
            if ((str)->_alloc == 0) {                                         \
                MN_ASSERT((str)->_data == MN_NULL);                           \
            }                                                                 \
        }                                                                     \
        /* Ensure that there is a null-terminator */                          \
        MN_ASSERT(MN__STR_DATA(str)[MN__STR_GET_SIZE(str)] == '\0');          \
    } while (0)

#else

#define MN__STR_CHECK(str) MN__UNUSED(str)

#endif

void mn__str_init(mn__str* str) {
    str->_size_short = 0;
    MN__STR_DATA(str)[0] = '\0';
}

void mn__str_destroy(mn__str* str) {
    if (!MN__STR_GET_SHORT(str)) {
        if (str->_data != MN_NULL) {
            MN_FREE(str->_data);
        }
    }
}

mn_size mn__str_size(const mn__str* str) { return MN__STR_GET_SIZE(str); }

MN_INTERNAL int mn__str_grow(mn__str* str, mn_size new_size) {
    mn_size old_size = MN__STR_GET_SIZE(str);
    MN__STR_CHECK(str);
    if (MN__STR_GET_SHORT(str)) {
        if (new_size <= MN__STR_SHORT_SIZE_MAX) {
            /* Can still be a short str */
            MN__STR_SET_SIZE(str, new_size);
        } else {
            /* Needs allocation */
            mn_size new_alloc =
              MN__STR_ROUND_ALLOC(new_size + (new_size >> 1));
            mn_char* new_data =
              (mn_char*)MN_MALLOC(sizeof(mn_char) * (new_alloc + 1));
            mn_size i;
            if (new_data == MN_NULL) {
                return -1;
            }
            /* Copy data from old string */
            for (i = 0; i < old_size; i++) {
                new_data[i] = MN__STR_DATA(str)[i];
            }
            /* Fill in the remaining fields */
            MN__STR_SET_SHORT(str, 0);
            MN__STR_SET_SIZE(str, new_size);
            str->_data = new_data;
            str->_alloc = new_alloc;
        }
    } else {
        if (new_size > str->_alloc) {
            /* Needs allocation */
            mn_size new_alloc =
              MN__STR_ROUND_ALLOC(new_size + (new_size >> 1));
            mn_char* new_data;
            if (str->_alloc == 0) {
                new_data =
                  (mn_char*)MN_MALLOC(sizeof(mn_char) * (new_alloc + 1));
            } else {
                new_data = (mn_char*)MN_REALLOC(
                  str->_data, sizeof(mn_char) * (new_alloc + 1));
            }
            if (new_data == MN_NULL) {
                return -1;
            }
            str->_data = new_data;
            str->_alloc = new_alloc;
        }
        MN__STR_SET_SIZE(str, new_size);
    }
    /* Null terminate */
    MN__STR_DATA(str)[MN__STR_GET_SIZE(str)] = '\0';
    MN__STR_CHECK(str);
    return 0;
}

int mn__str_push(mn__str* str, mn_char chr) {
    int err = 0;
    mn_size old_size = MN__STR_GET_SIZE(str);
    if ((err = mn__str_grow(str, old_size + 1))) {
        return err;
    }
    MN__STR_DATA(str)[old_size] = chr;
    MN__STR_CHECK(str);
    return err;
}

mn_size mn__str_slen(const mn_char* s) {
    mn_size out = 0;
    while (*(s++)) {
        out++;
    }
    return out;
}

int mn__str_init_s(mn__str* str, const mn_char* s) {
    int err = 0;
    mn_size i;
    mn_size sz = mn__str_slen(s);
    mn__str_init(str);
    if ((err = mn__str_grow(str, sz))) {
        return err;
    }
    for (i = 0; i < sz; i++) {
        MN__STR_DATA(str)[i] = s[i];
    }
    return err;
}

int mn__str_init_n(mn__str* str, const mn_char* chrs, mn_size n) {
    int err = 0;
    mn_size i;
    mn__str_init(str);
    if ((err = mn__str_grow(str, n))) {
        return err;
    }
    for (i = 0; i < n; i++) {
        MN__STR_DATA(str)[i] = chrs[i];
    }
    return err;
}

int mn__str_cat(mn__str* str, const mn__str* other) {
    int err = 0;
    mn_size i;
    mn_size n = MN__STR_GET_SIZE(other);
    mn_size old_size = MN__STR_GET_SIZE(str);
    if ((err = mn__str_grow(str, old_size + n))) {
        return err;
    }
    /* Copy data */
    for (i = 0; i < n; i++) {
        MN__STR_DATA(str)[old_size + i] = MN__STR_DATA(other)[i];
    }
    MN__STR_CHECK(str);
    return err;
}

int mn__str_cat_n(mn__str* str, const mn_char* chrs, mn_size n) {
    int err = 0;
    mn_size i;
    mn_size old_size = MN__STR_GET_SIZE(str);
    if ((err = mn__str_grow(str, old_size + n))) {
        return err;
    }
    /* Copy data */
    for (i = 0; i < n; i++) {
        MN__STR_DATA(str)[old_size + i] = chrs[i];
    }
    MN__STR_CHECK(str);
    return err;
}

int mn__str_cat_s(mn__str* str, const mn_char* chrs) {
    mn_size chrs_size = mn__str_slen(chrs);
    return mn__str_cat_n(str, chrs, chrs_size);
}

int mn__str_insert(mn__str* str, mn_size index, mn_char chr) {
    int err = 0;
    mn_size i;
    mn_size old_size = MN__STR_GET_SIZE(str);
    /* bounds check */
    MN_ASSERT(index <= MN__STR_GET_SIZE(str));
    if ((err = mn__str_grow(str, old_size + 1))) {
        return err;
    }
    /* Shift data */
    if (old_size != 0) {
        for (i = old_size; i >= index + 1; i--) {
            MN__STR_DATA(str)[i] = MN__STR_DATA(str)[i - 1];
        }
    }
    MN__STR_DATA(str)[index] = chr;
    MN__STR_CHECK(str);
    return err;
}

const mn_char* mn__str_get_data(const mn__str* str) {
    return MN__STR_DATA(str);
}

int mn__str_init_copy(mn__str* str, const mn__str* in) {
    mn_size i;
    int err = 0;
    mn__str_init(str);
    if ((err = mn__str_grow(str, mn__str_size(in)))) {
        return err;
    }
    for (i = 0; i < mn__str_size(str); i++) {
        MN__STR_DATA(str)[i] = MN__STR_DATA(in)[i];
    }
    return err;
}

void mn__str_init_move(mn__str* str, mn__str* old) {
    MN__STR_CHECK(old);
    *str = *old;
    mn__str_init(old);
}

int mn__str_cmp(const mn__str* str_a, const mn__str* str_b) {
    mn_size a_len = mn__str_size(str_a);
    mn_size b_len = mn__str_size(str_b);
    const mn_char* a_data = mn__str_get_data(str_a);
    const mn_char* b_data = mn__str_get_data(str_b);
    mn_size i;
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

void mn__str_clear(mn__str* str) {
    MN__STR_SET_SIZE(str, 0);
    MN__STR_DATA(str)[0] = '\0';
}

#if MPTEST_USE_DYN_ALLOC
#if MPTEST_USE_SYM
/* bits/container/str_view */
void mn__str_view_init(mn__str_view* view, const mn__str* other) {
    view->_size = mn__str_size(other);
    view->_data = mn__str_get_data(other);
}

void mn__str_view_init_s(mn__str_view* view, const mn_char* chars) {
    view->_size = mn__str_slen(chars);
    view->_data = chars;
}

void mn__str_view_init_n(mn__str_view* view, const mn_char* chars, mn_size n) {
    view->_size = n;
    view->_data = chars;
}

void mn__str_view_init_null(mn__str_view* view) {
    view->_size = 0;
    view->_data = MN_NULL;
}

mn_size mn__str_view_size(const mn__str_view* view) {
    return view->_size;
}

const mn_char* mn__str_view_get_data(const mn__str_view* view) {
    return view->_data;
}

int mn__str_view_cmp(const mn__str_view* view_a, const mn__str_view* view_b) {
    mn_size a_len = mn__str_view_size(view_a);
    mn_size b_len = mn__str_view_size(view_b);
    const mn_char* a_data = mn__str_view_get_data(view_a);
    const mn_char* b_data = mn__str_view_get_data(view_b);
    mn_size i;
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
#endif /* MPTEST_USE_DYN_ALLOC */
#endif /* MPTEST_USE_SYM */

#if MPTEST_USE_SYM
/* bits/types/fixed/int32 */
/* If this fails, you need to define MN_INT32_TYPE to a signed integer type
 * that is 32 bits wide. */
MN__STATIC_ASSERT(mn__int32_is_4_bytes, sizeof(mn_int32) == 4);
#endif /* MPTEST_USE_SYM */

#if MPTEST_USE_APARSE
/* bits/util/ntstr/cmp_n */
MN_INTERNAL int mn__scmp_n(const char* a, mn_size a_size, const char* b)
{
    mn_size a_pos = 0;
    while (1) {
        if (a_pos == a_size) {
            if (*b != '\0') {
                return 0;
            } else {
                /* *b equals '\0' or '=' */
                return 1;
            }
        }
        if (*b == '\0' || a[a_pos] != *b) {
            /* b ended first or a and b do not match */
            return 0;
        }
        a_pos++;
        b++;
    }
    return 0;
}
#endif /* MPTEST_USE_APARSE */

/* bits/util/ntstr/len */
MN_INTERNAL mn_size mn__slen(const mn_char* s) {
    mn_size sz = 0;
    while (*s) {
        sz++;
        s++;
    }
    return sz;
}

#if MPTEST_USE_APARSE
/* aparse */
MN_API aparse_error aparse_init(aparse_state* state) {
    state->state = (aparse__state*)MN_MALLOC(sizeof(aparse__state));
    if (state->state == MN_NULL) {
        return APARSE_ERROR_NOMEM;
    }
    aparse__state_init(state->state);
    state->state->root = MN_MALLOC(sizeof(aparse__state_root));
        if (state->state->root == MN_NULL) {
        return APARSE_ERROR_NOMEM;
    }
    state->state->root->out_buf_ptr = 0;
    state->state->root->prog_name = MN_NULL;
    state->state->root->prog_name_size = MN_NULL;
    state->state->is_root = 1;
    return APARSE_ERROR_NONE;
}

MN_API void aparse_destroy(aparse_state* state) {
    aparse__state_destroy(state->state);
    if (state->state != MN_NULL) {
        MN_FREE(state->state);
    }
}

MN_API void aparse_set_out_cb(aparse_state* state, aparse_out_cb out_cb, void* user) {
    aparse__state_set_out_cb(state->state, out_cb, user);
}

MN_API aparse_error aparse_add_opt(aparse_state* state, char short_opt, const char* long_opt) {
    aparse__state_check_before_add(state->state);
    return aparse__state_add_opt(state->state, short_opt, long_opt);
}

MN_API aparse_error aparse_add_pos(aparse_state* state, const char* name) {
    aparse__state_check_before_add(state->state);
    return aparse__state_add_pos(state->state, name);
}

MN_API aparse_error aparse_add_sub(aparse_state* state) {
    aparse__state_check_before_add(state->state);
    return aparse__state_add_sub(state->state);
}

MN_API void aparse_arg_help(aparse_state* state, const char* help_text) {
    aparse__state_check_before_modify(state->state);
    state->state->tail->help = help_text;
    if (help_text != MN_NULL) {
        state->state->tail->help_size = mn__slen(help_text);
    }
}

MN_API void aparse_arg_metavar(aparse_state* state, const char* metavar) {
    aparse__state_check_before_modify(state->state);
    state->state->tail->metavar = metavar;
    if (metavar != MN_NULL) {
        state->state->tail->metavar_size = mn__slen(metavar);
    }
}

MN_API void aparse_arg_type_bool(aparse_state* state, int* out) {
    aparse__state_check_before_set_type(state->state);
    aparse__arg_bool_init(state->state->tail, out);
}

MN_API void aparse_arg_type_str(aparse_state* state, const char** out, mn_size* out_size) {
    aparse__state_check_before_set_type(state->state);
    aparse__arg_str_init(state->state->tail, out, out_size);
}

MN_API void aparse_arg_type_help(aparse_state* state) {
    aparse__state_check_before_set_type(state->state);
    aparse__arg_help_init(state->state->tail);
}

MN_API void aparse_arg_type_version(aparse_state* state) {
    aparse__state_check_before_set_type(state->state);
    aparse__arg_version_init(state->state->tail);
}

MN_API void aparse_arg_type_custom(aparse_state* state, aparse_custom_cb cb, void* user, aparse_nargs nargs) {
    aparse__state_check_before_set_type(state->state);
    aparse__arg_custom_init(state->state->tail, cb, user, nargs);
}

MN_API aparse_error aparse_parse(aparse_state* state, int argc, const char* const* argv) {
    aparse_error err = APARSE_ERROR_NONE;
    if (argc == 0) {
        return APARSE_ERROR_INVALID;
    } else {
        state->state->root->prog_name = argv[0];
        state->state->root->prog_name_size = mn__slen(state->state->root->prog_name);
        err = aparse__parse_argv(state->state, argc - 1, argv + 1);
        if (err == APARSE_ERROR_PARSE) {
            if ((err = aparse__state_flush(state->state))) {
                return err;
            }
            return APARSE_ERROR_PARSE;
        } else if (err == APARSE_ERROR_SHOULD_EXIT) {
            if ((err = aparse__state_flush(state->state))) {
                return err;
            }
            return APARSE_ERROR_SHOULD_EXIT;
        } else {
            return err;
        }
    }
}
#endif /* MPTEST_USE_APARSE */

#if MPTEST_USE_APARSE
/* aparse */
MN_INTERNAL void aparse__arg_init(aparse__arg* arg) {
    arg->type = 0;
    arg->help = MN_NULL;
    arg->metavar = MN_NULL;
    arg->callback = MN_NULL;
    arg->callback_data = MN_NULL;
    arg->callback_data_2.plain = MN_NULL;
    arg->nargs = 0;
    arg->required = 0;
    arg->was_specified = 0;
    arg->next = MN_NULL;
}

MN_INTERNAL void aparse__arg_destroy(aparse__arg* arg) {
    if (arg->destroy != MN_NULL) {
        arg->destroy(arg);
    }
}

MN_INTERNAL void aparse__arg_bool_destroy(aparse__arg* arg);
MN_INTERNAL aparse_error aparse__arg_bool_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size);

MN_INTERNAL void aparse__arg_bool_init(aparse__arg* arg, int* out) {
    arg->nargs = APARSE_NARGS_0_OR_1_EQ;
    arg->callback = aparse__arg_bool_cb;
    arg->callback_data = (void*)out;
    arg->destroy = aparse__arg_bool_destroy;
}

MN_INTERNAL void aparse__arg_bool_destroy(aparse__arg* arg) {
    MN__UNUSED(arg);
}

MN_INTERNAL aparse_error aparse__arg_bool_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size) {
    aparse_error err = APARSE_ERROR_NONE;
    int* out = (int*)arg->callback_data;
    MN__UNUSED(state);
    MN__UNUSED(sub_arg_idx);
    if (text == MN_NULL) {
        *out = 1;
        return APARSE_ERROR_NONE;
    } else if (text_size == 1 && *text == '0') {
        *out = 0;
        return APARSE_ERROR_NONE;
    } else if (text_size == 1 && *text == '1') {
        *out = 1;
        return APARSE_ERROR_NONE;
    } else {
        if ((err = aparse__error_begin_arg(state, arg))) {
            return err;
        }
        if ((err = aparse__state_out_s(state, "invalid value for boolean flag: "))) {
            return err;
        }
        if ((err = aparse__error_quote(state, text, text_size))) {
            return err;
        }
        if ((err = aparse__state_out(state, '\n'))) {
            return err;
        }
        return APARSE_ERROR_PARSE;
    }
}

MN_INTERNAL void aparse__arg_str_destroy(aparse__arg* arg);
MN_INTERNAL aparse_error aparse__arg_str_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size);

MN_INTERNAL void aparse__arg_str_init(aparse__arg* arg, const char** out, mn_size* out_size) {
    MN_ASSERT(out != MN_NULL);
    arg->nargs = 1;
    arg->callback = aparse__arg_str_cb;
    arg->callback_data = (void*)out;
    arg->callback_data_2.plain = (void*)out_size;
    arg->destroy = aparse__arg_str_destroy;
}

MN_INTERNAL void aparse__arg_str_destroy(aparse__arg* arg) {
    MN__UNUSED(arg);
}

MN_INTERNAL aparse_error aparse__arg_str_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size) {
    const char** out = (const char**)arg->callback_data;
    mn_size* out_size = (mn_size*)arg->callback_data_2.plain;
    MN_ASSERT(text != MN_NULL);
    MN__UNUSED(state);
    MN__UNUSED(sub_arg_idx);
    *out = text;
    if (out_size) {
        *out_size = text_size;
    }
    return APARSE_ERROR_NONE;
}

MN_INTERNAL void aparse__arg_help_destroy(aparse__arg* arg);
MN_INTERNAL aparse_error aparse__arg_help_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size);

MN_INTERNAL void aparse__arg_help_init(aparse__arg* arg) {
    arg->nargs = 0;
    arg->callback = aparse__arg_help_cb;
    arg->destroy = aparse__arg_help_destroy;
}

MN_INTERNAL void aparse__arg_help_destroy(aparse__arg* arg) {
    MN__UNUSED(arg);
}

MN_INTERNAL aparse_error aparse__arg_help_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size) {
    aparse_error err = APARSE_ERROR_NONE;
    MN__UNUSED(arg);
    MN__UNUSED(sub_arg_idx);
    MN__UNUSED(text);
    MN__UNUSED(text_size);
    if ((err = aparse__error_usage(state))) {
        return err;
    }
    {
        int has_printed_header = 0;
        aparse__arg* cur = state->head;
        while (cur) {
            if (cur->type != APARSE__ARG_TYPE_POSITIONAL) {
                cur = cur->next;
                continue;
            }
            if (!has_printed_header) {
                if ((err = aparse__state_out_s(state, "\npositional arguments:\n"))) {
                    return err;
                }
                has_printed_header = 1;
            }
            if ((err = aparse__state_out_s(state, "  "))) {
                return err;
            }
            if (cur->metavar == MN_NULL) {
                if ((err = aparse__state_out_n(state, cur->contents.pos.name, cur->contents.pos.name_size))) {
                    return err;
                }
            } else {
                if ((err = aparse__state_out_n(state, cur->metavar, cur->metavar_size))) {
                    return err;
                }
            }
            if ((err = aparse__state_out(state, '\n'))) {
                return err;
            }
            if (cur->help != MN_NULL) {
                if ((err = aparse__state_out_s(state, "    "))) {
                    return err;
                }
                if ((err = aparse__state_out_n(state, cur->help, cur->help_size))) {
                    return err;
                }
                if ((err = aparse__state_out(state, '\n'))) {
                    return err;
                }
            }
            cur = cur->next;
        }
    }
    {
        int has_printed_header = 0;
        aparse__arg* cur = state->head;
        while (cur) {
            if (cur->type != APARSE__ARG_TYPE_OPTIONAL) {
                cur = cur->next;
                continue;
            }
            if (!has_printed_header) {
                if ((err = aparse__state_out_s(state, "\noptional arguments:\n"))) {
                    return err;
                }
                has_printed_header = 1;
            }
            if ((err = aparse__state_out_s(state, "  "))) {
                return err;
            }
            if (cur->contents.opt.short_opt != '\0') {
                if ((err = aparse__error_print_short_opt(state, cur))) {
                    return err;
                }
                if (cur->nargs != APARSE_NARGS_0_OR_1_EQ &&
                    cur->nargs != 0) {
                    if ((err = aparse__state_out(state, ' '))) {
                        return err;
                    }
                }
                if ((err = aparse__error_print_sub_args(state, cur))) {
                    return err;
                }
            }
            if (cur->contents.opt.long_opt != MN_NULL) {
                if (cur->contents.opt.short_opt != '\0') {
                    if ((err = aparse__state_out_s(state, ", "))) {
                        return err;
                    }
                }
                if ((err = aparse__error_print_long_opt(state, cur))) {
                    return err;
                }
                if (cur->nargs != APARSE_NARGS_0_OR_1_EQ &&
                    cur->nargs != 0) {
                    if ((err = aparse__state_out(state, ' '))) {
                        return err;
                    }
                }
                if ((err = aparse__error_print_sub_args(state, cur))) {
                    return err;
                }
            }
            if ((err = aparse__state_out(state, '\n'))) {
                return err;
            }
            if (cur->help != MN_NULL) {
                if ((err = aparse__state_out_s(state, "    "))) {
                    return err;
                }
                if ((err = aparse__state_out_n(state, cur->help, cur->help_size))) {
                    return err;
                }
                if ((err = aparse__state_out(state, '\n'))) {
                    return err;
                }
            }
            cur = cur->next;
        }
    }
    return APARSE_SHOULD_EXIT;
}

MN_INTERNAL void aparse__arg_version_destroy(aparse__arg* arg);
MN_INTERNAL aparse_error aparse__arg_version_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size);

MN_INTERNAL void aparse__arg_version_init(aparse__arg* arg) {
    arg->nargs = 0;
    arg->callback = aparse__arg_version_cb;
    arg->destroy = aparse__arg_version_destroy;
}

MN_INTERNAL void aparse__arg_version_destroy(aparse__arg* arg) {
    MN__UNUSED(arg);
}

MN_INTERNAL aparse_error aparse__arg_version_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size) {
    aparse_error err = APARSE_ERROR_NONE;
    MN__UNUSED(arg);
    MN__UNUSED(sub_arg_idx);
    MN__UNUSED(text);
    MN__UNUSED(text_size);
    /* TODO: print version */
    if ((err = aparse__state_out_s(state, "version\n"))) {
        return err;
    }
    return APARSE_SHOULD_EXIT;
}

MN_INTERNAL void aparse__arg_custom_destroy(aparse__arg* arg);
MN_INTERNAL aparse_error aparse__arg_custom_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size);

MN_INTERNAL void aparse__arg_custom_init(aparse__arg* arg, aparse_custom_cb cb, void* user, aparse_nargs nargs) {
    arg->nargs = nargs;
    arg->callback = aparse__arg_custom_cb;
    arg->callback_data = (void*)user;
    arg->callback_data_2.custom_cb = cb;
    arg->destroy = aparse__arg_custom_destroy;
}

MN_INTERNAL void aparse__arg_custom_destroy(aparse__arg* arg) {
    MN__UNUSED(arg);
}

MN_INTERNAL aparse_error aparse__arg_custom_cb(aparse__arg* arg, aparse__state* state, mn_size sub_arg_idx, const char* text, mn_size text_size) {
    aparse_custom_cb cb = (aparse_custom_cb)arg->callback_data_2.custom_cb;
    aparse_state state_;
    state_.state = state;
    return cb(arg->callback_data, &state_, (int)sub_arg_idx, text, text_size);
}

MN_INTERNAL void aparse__arg_sub_destroy(aparse__arg* arg);

MN_INTERNAL void aparse__arg_sub_init(aparse__arg* arg) {
    arg->type = APARSE__ARG_TYPE_SUBCOMMAND;
    arg->contents.sub.head = MN_NULL;
    arg->contents.sub.tail = MN_NULL;
    arg->destroy = aparse__arg_sub_destroy;
}

MN_INTERNAL void aparse__arg_sub_destroy(aparse__arg* arg) {
    aparse__sub* sub = arg->contents.sub.head;
    MN_ASSERT(arg->type == APARSE__ARG_TYPE_SUBCOMMAND);
    while (sub) {
        aparse__sub* prev = sub;
        aparse__state_destroy(&prev->subparser);
        sub = prev->next;
        MN_FREE(prev);
    }
}
#endif /* MPTEST_USE_APARSE */

#if MPTEST_USE_APARSE
/* aparse */
MN_INTERNAL aparse_error aparse__error_begin_progname(aparse__state* state) {
    aparse_error err = APARSE_ERROR_NONE;
    if ((err = aparse__state_out_n(state, state->root->prog_name, state->root->prog_name_size))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, ": "))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_begin(aparse__state* state) {
    aparse_error err = APARSE_ERROR_NONE;
    if ((err = aparse__error_begin_progname(state))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, "error: "))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_print_short_opt(aparse__state* state, const aparse__arg* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    MN_ASSERT(arg->contents.opt.short_opt);
    if ((err = aparse__state_out(state, '-'))) {
        return err;
    }
    if ((err = aparse__state_out(state, arg->contents.opt.short_opt))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_print_long_opt(aparse__state* state, const aparse__arg* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    MN_ASSERT(arg->contents.opt.long_opt);
    if ((err = aparse__state_out_s(state, "--"))) {
        return err;
    }
    if ((err = aparse__state_out_n(state, arg->contents.opt.long_opt, arg->contents.opt.long_opt_size))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_begin_opt(aparse__state* state, const aparse__arg* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    MN_ASSERT(arg->type == APARSE__ARG_TYPE_OPTIONAL);
    if ((err = aparse__error_begin(state))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, "option "))) {
        return err;
    }
    if (arg->contents.opt.short_opt != '\0') {
        if ((err = aparse__error_print_short_opt(state, arg))) {
            return err;
        }
    }
    if (arg->contents.opt.long_opt != MN_NULL) {
        if (arg->contents.opt.short_opt != '\0') {
            if ((err = aparse__state_out_s(state, ", "))) {
                return err;
            }
        }
        if ((err = aparse__error_print_long_opt(state, arg))) {
            return err;
        }
    }
    if ((err = aparse__state_out_s(state, ": "))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_begin_pos(aparse__state* state, const aparse__arg* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    MN_ASSERT(arg->type == APARSE__ARG_TYPE_POSITIONAL);
    if ((err = aparse__error_begin(state))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, "argument "))) {
        return err;
    }
    if ((err = aparse__state_out_n(state, arg->contents.pos.name, arg->contents.pos.name_size))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, ": "))) {
        return err;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_begin_arg(aparse__state* state, const aparse__arg* arg) {
    if (arg->type == APARSE__ARG_TYPE_OPTIONAL) {
        return aparse__error_begin_opt(state, arg);
    } else {
        return aparse__error_begin_pos(state, arg);
    }
}

MN_INTERNAL aparse_error aparse__error_unrecognized_arg(aparse__state* state, const char* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    if ((err = aparse__error_begin(state))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, "unrecognized argument: "))) {
        return err;
    }
    if ((err = aparse__state_out_s(state, arg))) {
        return err;
    }
    if ((err = aparse__state_out(state, '\n'))) {
        return err;
    }
    return err;
}

MN_INTERNAL char aparse__hexdig(unsigned char c) {
    if (c < 10) {
        return '0' + (char)c;
    } else {
        return 'a' + ((char)c - 10);
    }
}

MN_INTERNAL aparse_error aparse__error_quote(aparse__state* state, const char* text, mn_size text_size) {
    aparse_error err = APARSE_ERROR_NONE;
    mn_size i;
    if ((err = aparse__state_out(state, '"'))) {
        return err;
    }
    for (i = 0; i < text_size; i++) {
        char c = text[i];
        if (c < ' ') {
            if ((err = aparse__state_out(state, '\\'))) {
                return err;
            }
            if ((err = aparse__state_out(state, 'x'))) {
                return err;
            }
            if ((err = aparse__state_out(state, aparse__hexdig((c >> 4) & 0xF)))) {
                return err;
            }
            if ((err = aparse__state_out(state, aparse__hexdig(c & 0xF)))) {
                return err;
            }
        } else {
            if ((err = aparse__state_out(state, c))) {
                return err;
            }
        }
    }
    if ((err = aparse__state_out(state, '"'))) {
        return err;
    }
    return err;
}

int aparse__error_can_coalesce_in_usage(const aparse__arg* arg) {
    if (arg->type != APARSE__ARG_TYPE_OPTIONAL) {
        return 0;
    }
    if (arg->required) {
        return 0;
    }
    if (arg->contents.opt.short_opt == '\0') {
        return 0;
    }
    if ((arg->nargs != APARSE_NARGS_0_OR_1_EQ) && (arg->nargs != 0)) {
        return 0;
    }
    return 1;
}

MN_INTERNAL aparse_error aparse__error_print_sub_args(aparse__state* state, const aparse__arg* arg) {
    aparse_error err = APARSE_ERROR_NONE;
    const char* var;
    mn_size var_size;
    if (arg->metavar != MN_NULL) {
        var = arg->metavar;
        var_size = arg->metavar_size;
    } else if (arg->type == APARSE__ARG_TYPE_POSITIONAL) {
        var = arg->contents.pos.name;
        var_size = arg->contents.pos.name_size;
    } else {
        var = "ARG";
        var_size = 3;
    }
    if (arg->nargs == APARSE_NARGS_1_OR_MORE) {
        if ((err = aparse__state_out_n(state, var, var_size))) {
            return err;
        }
        if ((err = aparse__state_out_s(state, " ["))) {
            return err;
        }
        if ((err = aparse__state_out_n(state, var, var_size))) {
            return err;
        }
        if ((err = aparse__state_out_s(state, " ...]]"))) {
            return err;
        }
    } else if (arg->nargs == APARSE_NARGS_0_OR_MORE) {
        if ((err = aparse__state_out(state, '['))) {
            return err;
        }
        if ((err = aparse__state_out_n(state, var, var_size))) {
            return err;
        }
        if ((err = aparse__state_out_s(state, " ["))) {
            return err;
        }
        if ((err = aparse__state_out_n(state, var, var_size))) {
            return err;
        }
        if ((err = aparse__state_out_s(state, " ...]]"))) {
            return err;
        }
    } else if (arg->nargs == APARSE_NARGS_0_OR_1_EQ) {
        /* pass */
    } else if (arg->nargs == APARSE_NARGS_0_OR_1) {
        if ((err = aparse__state_out(state, '['))) {
            return err;
        }
        if ((err = aparse__state_out_n(state, var, var_size))) {
            return err;
        }
        if ((err = aparse__state_out(state, ']'))) {
            return err;
        }
    } else if (arg->nargs > 0) {
        int i;
        for (i = 0; i < arg->nargs; i++) {
            if (i) {
                if ((err = aparse__state_out(state, ' '))) {
                    return err;
                }
            }
            if ((err = aparse__state_out_n(state, var, var_size))) {
                return err;
            }
        }
    }
    return err;
}

MN_INTERNAL aparse_error aparse__error_usage(aparse__state* state) {
    aparse_error err = APARSE_ERROR_NONE;
    const aparse__arg* cur = state->head;
    int has_printed = 0;
    if ((err = aparse__state_out_s(state, "usage: "))) {
        return err;
    }
    if ((err = aparse__state_out_n(state, state->root->prog_name, state->root->prog_name_size))) {
        return err;
    }
    /* print coalesced args */
    while (cur) {
        if (aparse__error_can_coalesce_in_usage(cur)) {
            if (!has_printed) {
                if ((err = aparse__state_out_s(state, " [-"))) {
                    return err;
                }
                has_printed = 1;
            }
            if ((err = aparse__state_out(state, cur->contents.opt.short_opt))) {
                return err;
            }
        }
        cur = cur->next;
    }
    if (has_printed) {
        if ((err = aparse__state_out(state, ']'))) {
            return err;
        }
    }
    /* print other args */
    cur = state->head;
    while (cur) {
        if (!aparse__error_can_coalesce_in_usage(cur)) {
            if ((err = aparse__state_out(state, ' '))) {
                return err;
            }
            if (!cur->required) {
                if ((err = aparse__state_out(state, '['))) {
                    return err;
                }
            }
            if (cur->type == APARSE__ARG_TYPE_OPTIONAL) {
                if (cur->contents.opt.short_opt) {
                    if ((err = aparse__error_print_short_opt(state, cur))) {
                        return err;
                    }
                } else if (cur->contents.opt.long_opt) {
                    if ((err = aparse__error_print_long_opt(state, cur))) {
                        return err;
                    }
                }
                if (cur->nargs != APARSE_NARGS_0_OR_1_EQ &&
                    cur->nargs != 0) {
                    if ((err = aparse__state_out(state, ' '))) {
                        return err;
                    }
                }
            }
            if ((err = aparse__error_print_sub_args(state, cur))) {
                return err;
            }
            if (!cur->required) {
                if ((err = aparse__state_out(state, ']'))) {
                    return err;
                }
            }
        }
        cur = cur->next;
    }
    if ((err = aparse__state_out(state, '\n'))) {
        return err;
    }
    return err;
}

#if 0
MN_INTERNAL void aparse__error_print_opt_name(aparse_state* state,
    const struct aparse__arg*                   opt)
{
    (void)(state);
    if (opt->short_opt && !opt->long_opt) {
        /* Only short option was specified */
        fprintf(stderr, "-%c", opt->short_opt);
    } else if (opt->short_opt && opt->long_opt) {
        /* Both short option and long option were specified */
        fprintf(stderr, "-%c/--%s", opt->short_opt, opt->long_opt);
    } else if (opt->long_opt) {
        /* Only long option was specified */
        fprintf(stderr, "--%s", opt->long_opt);
    }
}

MN_INTERNAL void aparse__error_print_usage_coalesce_short_args(aparse_state* state)
{
    /* Print optional short options without arguments */
    size_t i;
    int    has_printed_short_opt_no_arg = 0;
    for (i = 0; i < state->args_size; i++) {
        const struct aparse__arg* current_opt = &state->args[i];
        /* Filter out positional options */
        if (current_opt->type == APARSE__ARG_TYPE_POSITIONAL) {
            continue;
        }
        /* Filter out required options */
        if (current_opt->required) {
            continue;
        }
        /* Filter out options with no short option */
        if (!current_opt->short_opt) {
            continue;
        }
        /* Filter out options with nargs that don't coalesce */
        if (!aparse__nargs_can_coalesce(current_opt->nargs)) {
            continue;
        }
        /* Print the initial '[-' */
        if (!has_printed_short_opt_no_arg) {
            has_printed_short_opt_no_arg = 1;
            fprintf(stderr, " [-");
        }
        fprintf(stderr, "%c", current_opt->short_opt);
    }
    if (has_printed_short_opt_no_arg) {
        fprintf(stderr, "]");
    }
}

MN_INTERNAL void aparse__error_print_usage_arg(aparse_state* state,
    const struct aparse__arg*                    current_arg)
{
    const char* metavar = "ARG";
    (void)(state);
    if (current_arg->metavar) {
        metavar = current_arg->metavar;
    }
    /* Optional arguments are encased in [] */
    if (!current_arg->required) {
        fprintf(stderr, "[");
    }
    /* Print option name */
    if (current_arg->type == APARSE__ARG_TYPE_OPTIONAL) {
        if (current_arg->short_opt) {
            fprintf(stderr, "-%c", current_arg->short_opt);
        } else {
            fprintf(stderr, "--%s", current_arg->long_opt);
        }
        /* Space separates the option name from the arguments */
        if (!aparse__nargs_can_coalesce(current_arg->nargs)) {
            fprintf(stderr, " ");
        }
    }
    if (current_arg->nargs == APARSE_NARGS_0_OR_1) {
        fprintf(stderr, "[%s]", metavar);
    } else if (current_arg->nargs == APARSE_NARGS_0_OR_MORE) {
        fprintf(stderr, "[%s ...]", metavar);
    } else if (current_arg->nargs == APARSE_NARGS_1_OR_MORE) {
        fprintf(stderr, "%s [%s ...]", metavar, metavar);
    } else {
        int j = (int)current_arg->nargs;
        for (j = 0; j < current_arg->nargs; j++) {
            if (j) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "%s", metavar);
        }
    }
    if (!current_arg->required) {
        fprintf(stderr, "]");
    }
}

MN_INTERNAL void aparse__error_print_usage(aparse_state* state)
{
    size_t                    i;
    const struct aparse__arg* current_arg;
    if (state->argc == 0) {
        fprintf(stderr, "usage:");
    } else {
        fprintf(stderr, "usage: %s", state->argv[0]);
    }
    /* Print optional short options with no arguments first */
    aparse__error_print_usage_coalesce_short_args(state);
    /* Print other optional options */
    for (i = 0; i < state->args_size; i++) {
        current_arg = &state->args[i];
        /* Filter out positionals */
        if (current_arg->type == APARSE__ARG_TYPE_POSITIONAL) {
            continue;
        }
        /* Filter out required options */
        if (current_arg->required) {
            continue;
        }
        /* Filter out options we printed in coalesce */
        if (aparse__nargs_can_coalesce(current_arg->nargs)) {
            continue;
        }
        fprintf(stderr, " ");
        aparse__error_print_usage_arg(state, current_arg);
    }
    /* Print mandatory options */
    for (i = 0; i < state->args_size; i++) {
        current_arg = &state->args[i];
        /* Filter out positionals */
        if (current_arg->type == APARSE__ARG_TYPE_POSITIONAL) {
            continue;
        }
        /* Filter out optional options */
        if (!current_arg->required) {
            continue;
        }
        fprintf(stderr, " ");
        aparse__error_print_usage_arg(state, current_arg);
    }
    /* Print mandatory positionals */
    for (i = 0; i < state->args_size; i++) {
        current_arg = &state->args[i];
        /* Filter out optionals */
        if (current_arg->type == APARSE__ARG_TYPE_OPTIONAL) {
            continue;
        }
        /* Filter out optional positionals */
        if (!current_arg->required) {
            continue;
        }
        fprintf(stderr, " ");
        aparse__error_print_usage_arg(state, current_arg);
    }
    /* Print optional positionals */
    for (i = 0; i < state->args_size; i++) {
        current_arg = &state->args[i];
        /* Filter out optionals */
        if (current_arg->type == APARSE__ARG_TYPE_OPTIONAL) {
            continue;
        }
        /* Filter out mandatory positionals */
        if (current_arg->required) {
            continue;
        }
        fprintf(stderr, " ");
        aparse__error_print_usage_arg(state, current_arg);
    }
    fprintf(stderr, "\n");
}

MN_INTERNAL void aparse__error_begin(aparse_state* state)
{
    aparse__error_print_usage(state);
    if (state->argc == 0) {
        fprintf(stderr, "error: ");
    } else {
        fprintf(stderr, "%s: error: ", state->argv[0]);
    }
}

MN_INTERNAL void aparse__error_end(aparse_state* state)
{
    MN__UNUSED(state);
    fprintf(stderr, "\n");
}

MN_INTERNAL void aparse__error_arg_begin(aparse_state* state)
{
    MN__UNUSED(state);
}

MN_INTERNAL void aparse__error_arg_end(aparse_state* state)
{
    MN__UNUSED(state);
}

#endif
#endif /* MPTEST_USE_APARSE */

#if MPTEST_USE_APARSE
/* aparse */
#include <stdio.h>

MN_INTERNAL aparse_error aparse__state_default_out_cb(void* user, const char* buf, mn_size buf_size) {
    MN__UNUSED(user);
    if (fwrite(buf, buf_size, 1, stdout) != 1) {
        return APARSE_ERROR_OUT;
    }
    return APARSE_ERROR_NONE;
}

MN_INTERNAL void aparse__state_init(aparse__state* state) {
    state->head = MN_NULL;
    state->tail = MN_NULL;
    state->help = MN_NULL;
    state->help_size = 0;
    state->out_cb = aparse__state_default_out_cb;
    state->user = MN_NULL;
    state->root = MN_NULL;
    state->is_root = 0;
}

#if 0

MN_INTERNAL void aparse__state_init_from(aparse__state* state, aparse__state* other) {
    aparse__state_init(state);
    state->out_cb = other->out_cb;
    state->user = other->user;
    state->root = other->root;
}

#endif

MN_INTERNAL void aparse__state_destroy(aparse__state* state) {
    aparse__arg* arg = state->head;
    while (arg) {
        aparse__arg* prev = arg;
        arg = arg->next;
        aparse__arg_destroy(prev);
        MN_FREE(prev);
    }
    if (state->is_root) {
        if (state->root != MN_NULL) {
            MN_FREE(state->root);
        }
    }
}

MN_INTERNAL void aparse__state_set_out_cb(aparse__state* state, aparse_out_cb out_cb, void* user) {
    state->out_cb = out_cb;
    state->user = user;
}

MN_INTERNAL void aparse__state_reset(aparse__state* state) {
    aparse__arg* cur = state->head;
    while (cur) {
        cur->was_specified = 0;
        if (cur->type == APARSE__ARG_TYPE_SUBCOMMAND) {
            aparse__sub* sub = cur->contents.sub.head;
            while (sub) {
                aparse__state_reset(&sub->subparser);
                sub = sub->next;
            }
        }
        cur = cur->next;
    }
}

MN_INTERNAL aparse_error aparse__state_arg(aparse__state* state) {
    aparse__arg* arg = (aparse__arg*)MN_MALLOC(sizeof(aparse__arg));
    if (arg == MN_NULL) {
        return APARSE_ERROR_NOMEM;
    }
    aparse__arg_init(arg);
    if (state->head == MN_NULL) {
        state->head = arg;
        state->tail = arg;
    } else {
        state->tail->next = arg;
        state->tail = arg;
    }
    return APARSE_ERROR_NONE;
}

MN_INTERNAL void aparse__state_check_before_add(aparse__state* state) {
    /* for release builds */
    MN__UNUSED(state);

    /* If this fails, you forgot to specifiy a type for the previous argument. */
    MN_ASSERT(MN__IMPLIES(state->tail != MN_NULL, state->tail->callback != MN_NULL));
}

MN_INTERNAL void aparse__state_check_before_modify(aparse__state* state) {
    /* for release builds */
    MN__UNUSED(state);

    /* If this fails, you forgot to call add_opt() or add_pos(). */
    MN_ASSERT(state->tail != MN_NULL);
}

MN_INTERNAL void aparse__state_check_before_set_type(aparse__state* state) {
    /* for release builds */
    MN__UNUSED(state);

    /* If this fails, you forgot to call add_opt() or add_pos(). */
    MN_ASSERT(state->tail != MN_NULL);

    /* If this fails, you are trying to set the argument type of a subcommand. */
    MN_ASSERT(state->tail->type != APARSE__ARG_TYPE_SUBCOMMAND);

    /* If this fails, you called arg_xxx() twice. */
    MN_ASSERT(state->tail->callback == MN_NULL);
}

MN_INTERNAL aparse_error aparse__state_add_opt(aparse__state* state, char short_opt, const char* long_opt) {
    aparse_error err = APARSE_ERROR_NONE;
    /* If either of these fail, you specified both short_opt and long_opt as
     * NULL. For a positional argument, use aparse__add_pos. */
    MN_ASSERT(MN__IMPLIES(short_opt == '\0', long_opt != MN_NULL));
    MN_ASSERT(MN__IMPLIES(long_opt == MN_NULL, short_opt != '\0'));
    if ((err = aparse__state_arg(state))) {
        return err;
    }
    state->tail->type = APARSE__ARG_TYPE_OPTIONAL;
    state->tail->contents.opt.short_opt = short_opt;
    state->tail->contents.opt.long_opt = long_opt;
    if (long_opt != MN_NULL) {
        state->tail->contents.opt.long_opt_size = mn__slen(long_opt);
    } else {
        state->tail->contents.opt.long_opt_size = 0;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__state_add_pos(aparse__state* state, const char* name) {
    aparse_error err = APARSE_ERROR_NONE;
    if ((err = aparse__state_arg(state))) {
        return err;
    }
    state->tail->type = APARSE__ARG_TYPE_POSITIONAL;
    state->tail->contents.pos.name = name;
    state->tail->contents.pos.name_size = mn__slen(name);
    return err;
}

MN_INTERNAL aparse_error aparse__state_add_sub(aparse__state* state) {
    aparse_error err = APARSE_ERROR_NONE;
    if ((err = aparse__state_arg(state))) {
        return err;
    }
    aparse__arg_sub_init(state->tail);
    return err;
}

#if 0
MN_INTERNAL aparse_error aparse__state_sub_add_cmd(aparse__state* state, const char* name, aparse__state** subcmd) {
    aparse__sub* sub = (aparse__sub*)MN_MALLOC(sizeof(aparse__sub));
    MN_ASSERT(state->tail->type == APARSE__ARG_TYPE_SUBCOMMAND);
    MN_ASSERT(name != MN_NULL);
    if (sub == MN_NULL) {
        return APARSE_ERROR_NOMEM;
    }
    sub->name = name;
    sub->name_size = mn__slen(name);
    aparse__state_init_from(&sub->subparser, state);
    sub->next = MN_NULL;
    if (state->tail->contents.sub.head == MN_NULL) {
        state->tail->contents.sub.head = sub;
        state->tail->contents.sub.tail = sub;
    } else {
        state->tail->contents.sub.tail->next = sub;
        state->tail->contents.sub.tail = sub;
    }
    *subcmd = &sub->subparser;
    return 0;
}

#endif


MN_INTERNAL aparse_error aparse__state_flush(aparse__state* state) {
    aparse_error err = APARSE_ERROR_NONE;
    if (state->root->out_buf_ptr) {
        if ((err = state->out_cb(state->user, state->root->out_buf, state->root->out_buf_ptr))) {
            return err;
        }
        state->root->out_buf_ptr = 0;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__state_out(aparse__state* state, char out) {
    aparse_error err = APARSE_ERROR_NONE;
    if (state->root->out_buf_ptr == APARSE__STATE_OUT_BUF_SIZE) {
        if ((err = aparse__state_flush(state))) {
            return err;
        }
    }
    state->root->out_buf[state->root->out_buf_ptr++] = out;
    return err;
}

MN_INTERNAL aparse_error aparse__state_out_s(aparse__state* state, const char* s) {
    aparse_error err = APARSE_ERROR_NONE;
    while (*s) {
        if ((err = aparse__state_out(state, *s))) {
            return err;
        }
        s++;
    }
    return err;
}

MN_INTERNAL aparse_error aparse__state_out_n(aparse__state* state, const char* s, mn_size n) {
    aparse_error err = APARSE_ERROR_NONE;
    mn_size i;
    for (i = 0; i < n; i++) {
        if ((err = aparse__state_out(state, s[i]))) {
            return err;
        }
    }
    return err;
}
#endif /* MPTEST_USE_APARSE */

#if MPTEST_USE_APARSE
/* aparse */
/* accepts an lvalue */
#define APARSE__NEXT_POSITIONAL(n) \
    while ((n) != MN_NULL && (n)->type != APARSE__ARG_TYPE_POSITIONAL) { \
        (n) = (n)->next; \
    }

int aparse__is_positional(const char* arg_text) {
    return (arg_text[0] == '\0') /* empty string */
            || (arg_text[0] == '-' && arg_text[1] == '\0') /* just a dash */
            || (arg_text[0] == '-' && arg_text[1] == '-' && arg_text[2] == '\0') /* two dashes */
            || (arg_text[0] != '-'); /* all positionals */
}

/* Returns NULL if the option does not match. */
const char* aparse__arg_match_long_opt(const struct aparse__arg* opt,
    const char* arg_without_dashes)
{
    mn_size a_pos = 0;
    const char* a_str = opt->contents.opt.long_opt;
    const char* b = arg_without_dashes;
    while (1) {
        if (a_pos == opt->contents.opt.long_opt_size) {
            if (*b != '\0' && *b != '=') {
                return NULL;
            } else {
                /* *b equals '\0' or '=' */
                return b;
            }
        }
        if (*b == '\0' || a_str[a_pos] != *b) {
            /* b ended first or a and b do not match */
            return NULL;
        }
        a_pos++;
        b++;
    }
    return NULL;
}

MN_API aparse_error aparse__parse_argv(aparse__state* state, int argc, const char* const* argv) {
    aparse_error err = APARSE_ERROR_NONE;
    int argc_idx = 0;
    aparse__arg* next_positional = state->head;
    mn_size arg_text_size;
    APARSE__NEXT_POSITIONAL(next_positional);
    aparse__state_reset(state);
    while (argc_idx < argc) {
        const char* arg_text = argv[argc_idx++];
        if (aparse__is_positional(arg_text)) {
            if (next_positional == MN_NULL) {
                if ((err = aparse__error_unrecognized_arg(state, arg_text))) {
                    return err;
                }
                return APARSE_ERROR_PARSE;
            }
            arg_text_size = mn__slen((const mn_char*)arg_text);
            if ((err = next_positional->callback(next_positional, state, 0, arg_text, arg_text_size))) {
                return err;
            }
            APARSE__NEXT_POSITIONAL(next_positional);
        } else {
            int is_long = 0;
            const char* arg_end;
            if (arg_text[0] == '-' && arg_text[1] != '-') {
                arg_end = arg_text + 1;
            } else {
                arg_end = arg_text + 2;
                is_long = 1;
            }
            do {
                aparse__arg* arg = state->head;
                int has_text_left = 0;
                if (!is_long) {
                    char short_opt = *(arg_end++);
                    while (1) {
                        if (arg == MN_NULL) {
                            break;
                        }
                        if (arg->type == APARSE__ARG_TYPE_OPTIONAL) {
                            if (arg->contents.opt.short_opt == short_opt) {
                                break;
                            }
                        }
                        arg = arg->next;
                    }
                    if (arg == MN_NULL) {
                        if ((err = aparse__error_unrecognized_arg(state, arg_text))) {
                            return err;
                        }
                        return APARSE_ERROR_PARSE;
                    }
                    has_text_left = *arg_end != '\0';
                } else {
                    while (1) {
                        if (arg == MN_NULL) {
                            break;
                        }
                        if (arg->type == APARSE__ARG_TYPE_OPTIONAL) {
                            if (arg->contents.opt.long_opt != MN_NULL) {
                                mn_size opt_pos = 0;
                                const char* opt_ptr = arg->contents.opt.long_opt;
                                const char* arg_ptr = arg_end;
                                int found = 0;
                                while (1) {
                                    if (opt_pos == arg->contents.opt.long_opt_size) {
                                        if (*arg_ptr != '\0' && *arg_ptr != '=') {
                                            break;
                                        } else {
                                            /* *b equals '\0' or '=' */
                                            arg_end = arg_ptr;
                                            found = 1;
                                            break;
                                        }
                                    }
                                    if (*arg_ptr == '\0' || opt_ptr[opt_pos] != *arg_ptr) {
                                        /* b ended first or a and b do not match */
                                        break;
                                    }
                                    opt_pos++;
                                    arg_ptr++;
                                }
                                if (found) {
                                    break;
                                }
                            }
                        }
                        arg = arg->next;
                    }
                    if (arg == MN_NULL) {
                        if ((err = aparse__error_unrecognized_arg(state, arg_text))) {
                            return err;
                        }
                        return APARSE_ERROR_PARSE;
                    }
                }
                if (*arg_end == '=') {
                    /* use equals as argument */
                    if (arg->nargs == 0
                        || arg->nargs > 1) {
                        if ((err = aparse__error_begin_arg(state, arg))) {
                            return err;
                        }
                        if ((err = aparse__state_out_s(state, "cannot parse '='\n"))) {
                            return err;
                        }
                        return APARSE_ERROR_PARSE;
                    } else  {
                        arg_end++;
                        if ((err = arg->callback(arg, state, 0, arg_end, mn__slen(arg_end)))) {
                            return err;
                        }
                    }
                    break;
                } else if (has_text_left) {
                    /* use rest of arg as argument */
                    if (arg->nargs > 1) {
                        if ((err = aparse__error_begin_arg(state, arg))) {
                            return err;
                        }
                        if ((err = aparse__state_out_s(state, "cannot parse '"))) {
                            return err;
                        }
                        if ((err = aparse__state_out_s(state, arg_end))) {
                            return err;
                        }
                        if ((err = aparse__state_out(state, '\n'))) {
                            return err;
                        }
                        return APARSE_ERROR_PARSE;
                    } else if (arg->nargs != APARSE_NARGS_0_OR_1_EQ &&
                        arg->nargs != 0) {
                        if ((err = arg->callback(arg, state, 0, arg_end, mn__slen(arg_end)))) {
                            return err;
                        }
                        break;
                    } else {
                        if ((err = arg->callback(arg, state, 0, MN_NULL, 0))) {
                            return err;
                        }
                        /* fallthrough, continue parsing short options */
                    }
                } else if (argc_idx == argc || !aparse__is_positional(argv[argc_idx])) {
                    if (arg->nargs == APARSE_NARGS_1_OR_MORE
                        || arg->nargs == 1
                        || arg->nargs > 1) {
                        if ((err = aparse__error_begin_arg(state, arg))) {
                            return err;
                        }
                        if ((err = aparse__state_out_s(state, "expected an argument\n"))) {
                            return err;
                        }
                        return APARSE_ERROR_PARSE;
                    } else if (arg->nargs == APARSE_NARGS_0_OR_1_EQ
                            || arg->nargs == 0) {
                        if ((err = arg->callback(arg, state, 0, MN_NULL, 0))) {
                            return err;
                        }
                        /* fallthrough */  
                    } else {
                        if ((err = arg->callback(arg, state, 0, MN_NULL, 0))) {
                            return err;
                        }
                    }
                    break;
                } else {
                    if (arg->nargs == APARSE_NARGS_0_OR_1
                        || arg->nargs == 1) {
                        arg_text = argv[argc_idx++];
                        arg_text_size = mn__slen(arg_text);
                        if ((err = arg->callback(arg, state, 0, arg_text, arg_text_size))) {
                            return err;
                        }
                    } else if (arg->nargs == APARSE_NARGS_0_OR_1_EQ
                            || arg->nargs == 0) {
                        if ((err = arg->callback(arg, state, 0, MN_NULL, 0))) {
                            return err;
                        }
                    } else {
                        mn_size sub_arg_idx = 0;
                        while (argc_idx < argc) {
                            arg_text = argv[argc_idx++];
                            arg_text_size = mn__slen(arg_text);
                            if ((err = arg->callback(arg, state, sub_arg_idx++, arg_text, arg_text_size))) {
                                return err;
                            }
                            if ((int)sub_arg_idx == arg->nargs) {
                                break;
                            }
                        }
                        if ((int)sub_arg_idx != arg->nargs) {
                            if ((err = aparse__error_begin_arg(state, arg))) {
                                return err;
                            }
                            if ((err = aparse__state_out_s(state, "expected an argument\n"))) {
                                return err;
                            }
                            return APARSE_ERROR_PARSE;
                        }
                    }
                    break;
                }
            } while (!is_long);
        }
    }
    return err;
}

#undef APARSE__NEXT_POSITIONAL
#endif /* MPTEST_USE_APARSE */

