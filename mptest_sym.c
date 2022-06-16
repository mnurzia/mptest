#include "mptest_internal.h"

#if MPTEST_USE_SYM

typedef enum mptest__sym_type {
  MPTEST__SYM_TYPE_EXPR,
  MPTEST__SYM_TYPE_ATOM_STRING,
  MPTEST__SYM_TYPE_ATOM_NUMBER
} mptest__sym_type;

typedef union mptest__sym_data {
  mn__str str;
  mn_int32 num;
} mptest__sym_data;

typedef struct mptest__sym_tree {
  mptest__sym_type type;
  mn_int32 first_child_ref;
  mn_int32 next_sibling_ref;
  mptest__sym_data data;
} mptest__sym_tree;

void mptest__sym_tree_init(mptest__sym_tree* tree, mptest__sym_type type)
{
  tree->type = type;
  tree->first_child_ref = MPTEST__SYM_NONE;
  tree->next_sibling_ref = MPTEST__SYM_NONE;
}

void mptest__sym_tree_destroy(mptest__sym_tree* tree)
{
  if (tree->type == MPTEST__SYM_TYPE_ATOM_STRING) {
    mn__str_destroy(&tree->data.str);
  }
}

MN__VEC_DECL(mptest__sym_tree);
MN__VEC_IMPL_FUNC(mptest__sym_tree, init)
MN__VEC_IMPL_FUNC(mptest__sym_tree, destroy)
MN__VEC_IMPL_FUNC(mptest__sym_tree, push)
MN__VEC_IMPL_FUNC(mptest__sym_tree, size)
MN__VEC_IMPL_FUNC(mptest__sym_tree, getref)
MN__VEC_IMPL_FUNC(mptest__sym_tree, getcref)

struct mptest_sym {
  mptest__sym_tree_vec tree_storage;
};

void mptest__sym_init(mptest_sym* sym)
{
  mptest__sym_tree_vec_init(&sym->tree_storage);
}

void mptest__sym_destroy(mptest_sym* sym)
{
  mn_size i;
  for (i = 0; i < mptest__sym_tree_vec_size(&sym->tree_storage); i++) {
    mptest__sym_tree_destroy(
        mptest__sym_tree_vec_getref(&sym->tree_storage, i));
  }
  mptest__sym_tree_vec_destroy(&sym->tree_storage);
}

MN_INTERNAL mptest__sym_tree* mptest__sym_get(mptest_sym* sym, mn_int32 ref)
{
  MN_ASSERT(ref != MPTEST__SYM_NONE);
  return mptest__sym_tree_vec_getref(&sym->tree_storage, (mn_size)ref);
}

MN_INTERNAL const mptest__sym_tree*
mptest__sym_getcref(const mptest_sym* sym, mn_int32 ref)
{
  MN_ASSERT(ref != MPTEST__SYM_NONE);
  return mptest__sym_tree_vec_getcref(&sym->tree_storage, (mn_size)ref);
}

MN_INTERNAL int mptest__sym_new(
    mptest_sym* sym, mn_int32 parent_ref, mn_int32 prev_sibling_ref,
    mptest__sym_tree new_tree, mn_int32* new_ref)
{
  int err = 0;
  mn_int32 next_ref = (mn_int32)mptest__sym_tree_vec_size(&sym->tree_storage);
  if ((err = mptest__sym_tree_vec_push(&sym->tree_storage, new_tree))) {
    return err;
  }
  *new_ref = next_ref;
  if (parent_ref != MPTEST__SYM_NONE) {
    if (prev_sibling_ref == MPTEST__SYM_NONE) {
      mptest__sym_tree* parent = mptest__sym_get(sym, parent_ref);
      parent->first_child_ref = *new_ref;
    } else {
      mptest__sym_tree* sibling = mptest__sym_get(sym, prev_sibling_ref);
      sibling->next_sibling_ref = *new_ref;
    }
  }
  return err;
}

MN__VEC_DECL(mptest_sym_build);
MN__VEC_IMPL_FUNC(mptest_sym_build, init)
MN__VEC_IMPL_FUNC(mptest_sym_build, destroy)
MN__VEC_IMPL_FUNC(mptest_sym_build, push)
MN__VEC_IMPL_FUNC(mptest_sym_build, pop)
MN__VEC_IMPL_FUNC(mptest_sym_build, getref)

#define MPTEST__SYM_PARSE_ERROR -1

typedef struct mptest__sym_parse {
  const char* str;
  mn_size str_pos;
  mn_size str_size;
  mptest_sym_build_vec stk;
  mn_size stk_ptr;
  const char* error;
  mn__str build_str;
} mptest__sym_parse;

void mptest__sym_parse_init(mptest__sym_parse* parse, const mn__str_view in)
{
  parse->str = mn__str_view_get_data(&in);
  parse->str_pos = 0;
  parse->str_size = mn__str_view_size(&in);
  mptest_sym_build_vec_init(&parse->stk);
  parse->stk_ptr = 0;
  parse->error = MN_NULL;
  mn__str_init(&parse->build_str);
}

void mptest__sym_parse_destroy(mptest__sym_parse* parse)
{
  mptest_sym_build_vec_destroy(&parse->stk);
  mn__str_destroy(&parse->build_str);
}

int mptest__sym_parse_next(mptest__sym_parse* parse)
{
  MN_ASSERT(parse->str_pos <= parse->str_size);
  if (parse->str_pos == parse->str_size) {
    parse->str_pos++;
    return -1;
  } else {
    return parse->str[parse->str_pos++];
  }
}

MN_INTERNAL int mptest__sym_parse_expr_begin(
    mptest__sym_parse* parse, mptest_sym_build* base_build)
{
  int err = 0;
  mptest_sym_build next_build;
  if (parse->stk_ptr == 0) {
    if ((err = mptest_sym_build_expr(base_build, &next_build))) {
      return err;
    }
  } else {
    if ((err = mptest_sym_build_expr(
             mptest_sym_build_vec_getref(&parse->stk, parse->stk_ptr - 1),
             &next_build))) {
      return err;
    }
  }
  if ((err = mptest_sym_build_vec_push(&parse->stk, next_build))) {
    return err;
  }
  parse->stk_ptr++;
  return err;
}

MN_INTERNAL int mptest__sym_parse_expr_end(mptest__sym_parse* parse)
{
  if (parse->stk_ptr == 0) {
    parse->error = "unmatched ')'";
    return MPTEST__SYM_PARSE_ERROR;
  }
  parse->stk_ptr--;
  mptest_sym_build_vec_pop(&parse->stk);
  return 0;
}

MN_INTERNAL int mptest__sym_parse_isblank(int ch)
{
  return (ch == '\n') || (ch == '\t') || (ch == '\r') || (ch == ' ');
}

MN_INTERNAL int mptest__sym_parse_ispunc(int ch)
{
  return (ch == '(') || (ch == ')');
}

MN_INTERNAL int mptest__sym_parse_hexdig(int ch)
{
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    return -1;
  }
}

MN_INTERNAL int mptest__sym_parse_esc(mptest__sym_parse* parse)
{
  int ch = mptest__sym_parse_next(parse);
  if (ch == -1) {
    parse->error = "unfinished escape sequence";
    return MPTEST__SYM_PARSE_ERROR;
  } else if (ch == 't') {
    return '\t';
  } else if (ch == 'r') {
    return '\r';
  } else if (ch == '0') {
    return '\0';
  } else if (ch == 'n') {
    return '\n';
  } else if (ch == 'x') {
    int out_ch = 0;
    int dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid hex escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch = dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid hex escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch *= 16;
    out_ch += dig;
    return out_ch;
  } else if (ch == 'u') {
    int out_ch = 0;
    int dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid unicode escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch = dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid unicode escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch *= 16;
    out_ch += dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid unicode escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch *= 16;
    out_ch += dig;
    ch = mptest__sym_parse_next(parse);
    if ((dig = mptest__sym_parse_hexdig(ch)) == -1) {
      parse->error = "invalid unicode escape";
      return MPTEST__SYM_PARSE_ERROR;
    }
    out_ch *= 16;
    out_ch += dig;
    return out_ch;
  } else if (ch == '"') {
    return '"';
  } else {
    parse->error = "invalid escape character";
    return MPTEST__SYM_PARSE_ERROR;
  }
}

MN_INTERNAL int mptest__sym_parse_gen_utf8(int codep, unsigned char* out_buf)
{
  if (codep <= 0x7F) {
    out_buf[0] = codep & 0x7F;
    return 1;
  } else if (codep <= 0x07FF) {
    out_buf[0] = (unsigned char)(((codep >> 6) & 0x1F) | 0xC0);
    out_buf[1] = (unsigned char)(((codep >> 0) & 0x3F) | 0x80);
    return 2;
  } else if (codep <= 0xFFFF) {
    out_buf[0] = (unsigned char)(((codep >> 12) & 0x0F) | 0xE0);
    out_buf[1] = (unsigned char)(((codep >> 6) & 0x3F) | 0x80);
    out_buf[2] = (unsigned char)(((codep >> 0) & 0x3F) | 0x80);
    return 3;
  } else if (codep <= 0x10FFFF) {
    out_buf[0] = (unsigned char)(((codep >> 18) & 0x07) | 0xF0);
    out_buf[1] = (unsigned char)(((codep >> 12) & 0x3F) | 0x80);
    out_buf[2] = (unsigned char)(((codep >> 6) & 0x3F) | 0x80);
    out_buf[3] = (unsigned char)(((codep >> 0) & 0x3F) | 0x80);
    return 4;
  } else {
    return -1;
  }
}

MN_INTERNAL int mptest__sym_parse_do(
    mptest_sym_build* build_in, const mn__str_view in, const char** err_msg,
    mn_size* err_pos)
{
  int err = 0;
  mptest__sym_parse parse;
  mptest__sym_parse_init(&parse, in);
  while (1) {
    int ch = mptest__sym_parse_next(&parse);
    mptest_sym_build* build;
    if (parse.stk_ptr) {
      build = mptest_sym_build_vec_getref(&parse.stk, parse.stk_ptr - 1);
    } else {
      build = build_in;
    }
    mn__str_clear(&parse.build_str);
    if (ch == -1) {
      break;
    } else if (ch == '(') {
      /* ( | Begin expression */
      if ((err = mptest__sym_parse_expr_begin(&parse, build_in))) {
        goto error;
      }
    } else if (ch == ')') {
      /* ) | End expression */
      if ((err = mptest__sym_parse_expr_end(&parse))) {
        goto error;
      }
    } else if (mptest__sym_parse_isblank(ch)) {
      /* <\s> | Whitespace */
      continue;
    } else if (ch == '0') {
      /* 0 | Start of hex literal, or just a zero */
      mn_int32 num = 0;
      mn_size saved_pos = parse.str_pos;
      int n = 0;
      ch = mptest__sym_parse_next(&parse);
      if (mptest__sym_parse_ispunc(ch) || mptest__sym_parse_isblank(ch) ||
          ch == -1) {
        /* 0 | Just a zero */
        parse.str_pos = saved_pos;
        if ((err = mptest_sym_build_num(build, 0))) {
          goto error;
        }
        continue;
      }
      if (ch != 'x') {
        parse.error = "invalid hex literal";
        err = MPTEST__SYM_PARSE_ERROR;
        goto error;
      }
      while (1) {
        int dig;
        saved_pos = parse.str_pos;
        ch = mptest__sym_parse_next(&parse);
        if (n == 8) {
          parse.error = "hex literal too long";
          err = MPTEST__SYM_PARSE_ERROR;
          goto error;
        }
        if ((dig = mptest__sym_parse_hexdig(ch)) != -1) {
          /* [0-9A-Fa-f] | Hex digit */
          num *= 16;
          num += dig;
        } else {
          if ((mptest__sym_parse_isblank(ch) || mptest__sym_parse_ispunc(ch) ||
               ch == -1) &&
              n != 0) {
            /* [\s)] | Continue */
            parse.str_pos = saved_pos;
            if ((err = mptest_sym_build_num(build, num))) {
              goto error;
            }
            break;
          } else {
            /* Everything else | Error */
            parse.error = "expected hex digits for hex literal";
            err = MPTEST__SYM_PARSE_ERROR;
            goto error;
          }
        }
        n++;
      }
    } else if (ch > '0' && ch <= '9') {
      /* 1-9 | Integer literal */
      mn_int32 num = ch - '0';
      mn_size saved_pos;
      int n = 1;
      while (1) {
        saved_pos = parse.str_pos;
        ch = mptest__sym_parse_next(&parse);
        if (n == 10) {
          parse.error = "decimal literal too long";
          err = MPTEST__SYM_PARSE_ERROR;
          goto error;
        }
        if (ch >= '0' && ch <= '9') {
          num *= 10;
          num += ch - '0';
        } else {
          if (mptest__sym_parse_isblank(ch) || mptest__sym_parse_ispunc(ch) ||
              ch == -1) {
            parse.str_pos = saved_pos;
            if ((err = mptest_sym_build_num(build, num))) {
              goto error;
            }
            break;
          } else {
            parse.error = "expected decimal digits for decimal literal";
            err = MPTEST__SYM_PARSE_ERROR;
            goto error;
          }
        }
        n++;
      }
    } else if (ch == '"') {
      /* " | String starter */
      while (1) {
        ch = mptest__sym_parse_next(&parse);
        if (ch == -1) {
          /* "<EOF> | Invalid, error */
          parse.error = "unfinished string literal";
          err = MPTEST__SYM_PARSE_ERROR;
          goto error;
        } else if (ch == '\\') {
          /* "\ | Escape sequence starter */
          int rune = mptest__sym_parse_esc(&parse);
          unsigned char ubuf[8];
          int ubuf_sz = 0;
          if (rune < 0) {
            err = rune;
            goto error;
          }
          if ((ubuf_sz = mptest__sym_parse_gen_utf8(rune, ubuf)) < 0) {
            parse.error = "invalid UTF-8 character number";
            err = MPTEST__SYM_PARSE_ERROR;
            goto error;
          }
          if ((err = mn__str_cat_n(
                   &parse.build_str, (char*)ubuf, (mn_size)ubuf_sz))) {
            goto error;
          }
        } else if (ch == '"') {
          /* "" | Finish string */
          if ((err = mptest_sym_build_str(
                   build, (const char*)mn__str_get_data(&parse.build_str),
                   mn__str_size(&parse.build_str)))) {
            goto error;
          }
          break;
        } else {
          /* <*> | Add to string */
          char ch_char = (char)ch;
          if ((err = mn__str_cat_n(&parse.build_str, &ch_char, 1))) {
            goto error;
          }
        }
      }
    } else if (ch == '\'') {
      /* ' | Char starter */
      int rune;
      unsigned char ubuf[8];
      int ubuf_sz = 0;
      ch = mptest__sym_parse_next(&parse);
      if (ch == -1) {
        parse.error = "expected character for character literal";
        err = MPTEST__SYM_PARSE_ERROR;
        goto error;
      } else if (ch == '\\') {
        rune = mptest__sym_parse_esc(&parse);
      } else {
        rune = ch;
      }
      if (rune < 0) {
        err = rune;
        goto error;
      }
      if ((ubuf_sz = mptest__sym_parse_gen_utf8(rune, ubuf)) < 0) {
        parse.error = "invalid UTF-8 character number";
        err = MPTEST__SYM_PARSE_ERROR;
        goto error;
      }
      ch = mptest__sym_parse_next(&parse);
      if (ch == -1 || ch != '\'') {
        parse.error = "expected ' to close character literal";
        err = MPTEST__SYM_PARSE_ERROR;
        goto error;
      }
      if ((err = mptest_sym_build_num(build, rune))) {
        goto error;
      }
    } else {
      /* <*> | Add to atom */
      char ch_char = (char)ch;
      if ((err = mn__str_cat_n(&parse.build_str, &ch_char, 1))) {
        goto error;
      }
      while (1) {
        mn_size saved_pos = parse.str_pos;
        ch = mptest__sym_parse_next(&parse);
        if (mptest__sym_parse_isblank(ch) || mptest__sym_parse_ispunc(ch) ||
            ch == -1) {
          /* [\s()<EOF>] | Build string */
          if ((err = mptest_sym_build_str(
                   build, (const char*)mn__str_get_data(&parse.build_str),
                   mn__str_size(&parse.build_str)))) {
            goto error;
          }
          parse.str_pos = saved_pos;
          break;
        } else {
          /* <*> | Add to string */
          ch_char = (char)ch;
          if ((err = mn__str_cat_n(&parse.build_str, &ch_char, 1))) {
            goto error;
          }
        }
      }
    }
  }
  if (parse.stk_ptr) {
    parse.error = "unmatched '('";
    err = MPTEST__SYM_PARSE_ERROR;
    goto error;
  }
error:
  *err_pos = parse.str_pos;
  *err_msg = parse.error;
  mptest__sym_parse_destroy(&parse);
  return err;
}

MN_INTERNAL void
mptest__sym_dump(mptest_sym* sym, mn_int32 parent_ref, mn_int32 indent)
{
  mptest__sym_tree* tree;
  mn_int32 child_ref;
  mn_int32 i;
  if (parent_ref == MPTEST__SYM_NONE) {
    return;
  }
  tree = mptest__sym_get(sym, parent_ref);
  if (tree->first_child_ref == MPTEST__SYM_NONE) {
    if (tree->type == MPTEST__SYM_TYPE_ATOM_NUMBER) {
      printf("%i", tree->data.num);
    } else if (tree->type == MPTEST__SYM_TYPE_ATOM_STRING) {
      printf("%s", (const char*)mn__str_get_data(&tree->data.str));
    } else if (tree->type == MPTEST__SYM_TYPE_EXPR) {
      printf("()");
    }
  } else {
    printf("\n");
    for (i = 0; i < indent; i++) {
      printf("  ");
    }
    printf("(");
    child_ref = tree->first_child_ref;
    while (child_ref != MPTEST__SYM_NONE) {
      mptest__sym_tree* child = mptest__sym_get(sym, child_ref);
      mptest__sym_dump(sym, child_ref, indent + 1);
      child_ref = child->next_sibling_ref;
      if (child_ref != MPTEST__SYM_NONE) {
        printf(" ");
      }
    }
    printf(")");
  }
}

MN_INTERNAL int mptest__sym_equals(
    mptest_sym* sym, mptest_sym* other, mn_int32 sym_ref, mn_int32 other_ref)
{
  mptest__sym_tree* parent_tree;
  mptest__sym_tree* other_tree;
  if ((sym_ref == other_ref) && sym_ref == MPTEST__SYM_NONE) {
    return 1;
  } else if (sym_ref == MPTEST__SYM_NONE || other_ref == MPTEST__SYM_NONE) {
    return 0;
  }
  parent_tree = mptest__sym_get(sym, sym_ref);
  other_tree = mptest__sym_get(other, other_ref);
  if (parent_tree->type != other_tree->type) {
    return 0;
  } else {
    if (parent_tree->type == MPTEST__SYM_TYPE_ATOM_NUMBER) {
      if (parent_tree->data.num != other_tree->data.num) {
        return 0;
      }
      return 1;
    } else if (parent_tree->type == MPTEST__SYM_TYPE_ATOM_STRING) {
      if (mn__str_cmp(&parent_tree->data.str, &other_tree->data.str) != 0) {
        return 0;
      }
      return 1;
    } else if (parent_tree->type == MPTEST__SYM_TYPE_EXPR) {
      mn_int32 parent_child_ref = parent_tree->first_child_ref;
      mn_int32 other_child_ref = other_tree->first_child_ref;
      mptest__sym_tree* parent_child;
      mptest__sym_tree* other_child;
      while (parent_child_ref != MPTEST__SYM_NONE &&
             other_child_ref != MPTEST__SYM_NONE) {
        if (!mptest__sym_equals(
                sym, other, parent_child_ref, other_child_ref)) {
          return 0;
        }
        parent_child = mptest__sym_get(sym, parent_child_ref);
        other_child = mptest__sym_get(other, other_child_ref);
        parent_child_ref = parent_child->next_sibling_ref;
        other_child_ref = other_child->next_sibling_ref;
      }
      return parent_child_ref == other_child_ref;
    }
    return 0;
  }
}

MN_API void mptest_sym_build_init(
    mptest_sym_build* build, mptest_sym* sym, mn_int32 parent_ref,
    mn_int32 prev_child_ref)
{
  build->sym = sym;
  build->parent_ref = parent_ref;
  build->prev_child_ref = prev_child_ref;
}

MN_API void mptest_sym_build_destroy(mptest_sym_build* build)
{
  MN__UNUSED(build);
}

MN_API int mptest_sym_build_expr(mptest_sym_build* build, mptest_sym_build* sub)
{
  mptest__sym_tree new_tree;
  mn_int32 new_child_ref;
  int err = 0;
  mptest__sym_tree_init(&new_tree, MPTEST__SYM_TYPE_EXPR);
  if ((err = mptest__sym_new(
           build->sym, build->parent_ref, build->prev_child_ref, new_tree,
           &new_child_ref))) {
    return err;
  }
  build->prev_child_ref = new_child_ref;
  mptest_sym_build_init(sub, build->sym, new_child_ref, MPTEST__SYM_NONE);
  return err;
}

MN_API int
mptest_sym_build_str(mptest_sym_build* build, const char* str, mn_size str_size)
{
  mptest__sym_tree new_tree;
  mn_int32 new_child_ref;
  int err = 0;
  mptest__sym_tree_init(&new_tree, MPTEST__SYM_TYPE_ATOM_STRING);
  if ((err =
           mn__str_init_n(&new_tree.data.str, (const mn_char*)str, str_size))) {
    return err;
  }
  if ((err = mptest__sym_new(
           build->sym, build->parent_ref, build->prev_child_ref, new_tree,
           &new_child_ref))) {
    return err;
  }
  build->prev_child_ref = new_child_ref;
  return err;
}

MN_API int mptest_sym_build_cstr(mptest_sym_build* build, const char* cstr)
{
  return mptest_sym_build_str(build, cstr, mn__str_slen((const mn_char*)cstr));
}

MN_API int mptest_sym_build_num(mptest_sym_build* build, mn_int32 num)
{
  mptest__sym_tree new_tree;
  mn_int32 new_child_ref;
  int err = 0;
  mptest__sym_tree_init(&new_tree, MPTEST__SYM_TYPE_ATOM_NUMBER);
  new_tree.data.num = num;
  if ((err = mptest__sym_new(
           build->sym, build->parent_ref, build->prev_child_ref, new_tree,
           &new_child_ref))) {
    return err;
  }
  build->prev_child_ref = new_child_ref;
  return err;
}

MN_API int mptest_sym_build_type(mptest_sym_build* build, const char* type)
{
  mptest_sym_build new;
  int err = 0;
  if ((err = mptest_sym_build_expr(build, &new))) {
    return err;
  }
  if ((err = mptest_sym_build_cstr(&new, type))) {
    return err;
  }
  *build = new;
  return err;
}

MN_API void mptest_sym_walk_init(
    mptest_sym_walk* walk, const mptest_sym* sym, mn_int32 parent_ref,
    mn_int32 prev_child_ref)
{
  walk->sym = sym;
  walk->parent_ref = parent_ref;
  walk->prev_child_ref = prev_child_ref;
}

MN_API int
mptest__sym_walk_peeknext(mptest_sym_walk* walk, mn_int32* out_child_ref)
{
  const mptest__sym_tree* prev;
  mn_int32 child_ref;
  if (walk->parent_ref == MPTEST__SYM_NONE) {
    if (!mptest__sym_tree_vec_size(&walk->sym->tree_storage)) {
      return SYM_EMPTY;
    }
    child_ref = 0;
  } else if (walk->prev_child_ref == MPTEST__SYM_NONE) {
    prev = mptest__sym_getcref(walk->sym, walk->parent_ref);
    child_ref = prev->first_child_ref;
  } else {
    prev = mptest__sym_getcref(walk->sym, walk->prev_child_ref);
    child_ref = prev->next_sibling_ref;
  }
  if (child_ref == MPTEST__SYM_NONE) {
    return SYM_NO_MORE;
  }
  *out_child_ref = child_ref;
  return 0;
}

MN_API int
mptest__sym_walk_getnext(mptest_sym_walk* walk, mn_int32* out_child_ref)
{
  int err = 0;
  if ((err = mptest__sym_walk_peeknext(walk, out_child_ref))) {
    return err;
  }
  walk->prev_child_ref = *out_child_ref;
  return err;
}

MN_API int mptest_sym_walk_getexpr(mptest_sym_walk* walk, mptest_sym_walk* sub)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_getnext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  if (child->type != MPTEST__SYM_TYPE_EXPR) {
    return SYM_WRONG_TYPE;
  } else {
    mptest_sym_walk_init(sub, walk->sym, child_ref, MPTEST__SYM_NONE);
    return 0;
  }
}

MN_API int mptest_sym_walk_getstr(
    mptest_sym_walk* walk, const char** str, mn_size* str_size)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_getnext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  if (child->type != MPTEST__SYM_TYPE_ATOM_STRING) {
    return SYM_WRONG_TYPE;
  } else {
    *str = (const char*)mn__str_get_data(&child->data.str);
    *str_size = mn__str_size(&child->data.str);
    return 0;
  }
}

MN_API int mptest_sym_walk_getnum(mptest_sym_walk* walk, mn_int32* num)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_getnext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  if (child->type != MPTEST__SYM_TYPE_ATOM_NUMBER) {
    return SYM_WRONG_TYPE;
  } else {
    *num = child->data.num;
    return 0;
  }
}

MN_API int
mptest_sym_walk_checktype(mptest_sym_walk* walk, const char* expected_type)
{
  const char* str;
  int err = 0;
  mn_size str_size;
  mn_size expected_size = mn__str_slen(expected_type);
  mn__str_view expected, actual;
  mn__str_view_init_n(&expected, expected_type, expected_size);
  if ((err = mptest_sym_walk_getstr(walk, &str, &str_size))) {
    return err;
  }
  mn__str_view_init_n(&actual, str, str_size);
  if (mn__str_view_cmp(&expected, &actual) != 0) {
    return SYM_WRONG_TYPE;
  }
  return 0;
}

MN_API int mptest_sym_walk_hasmore(mptest_sym_walk* walk)
{
  const mptest__sym_tree* prev;
  if (walk->parent_ref == MPTEST__SYM_NONE) {
    if (mptest__sym_tree_vec_size(&walk->sym->tree_storage) == 0) {
      return 0;
    } else {
      return 1;
    }
  } else if (walk->prev_child_ref == MPTEST__SYM_NONE) {
    prev = mptest__sym_getcref(walk->sym, walk->parent_ref);
    if (prev->first_child_ref == MPTEST__SYM_NONE) {
      return 0;
    } else {
      return 1;
    }
  } else {
    prev = mptest__sym_getcref(walk->sym, walk->prev_child_ref);
    if (prev->next_sibling_ref == MPTEST__SYM_NONE) {
      return 0;
    } else {
      return 1;
    }
  }
}

MN_API int mptest_sym_walk_peekstr(mptest_sym_walk* walk)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_peeknext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  return child->type == MPTEST__SYM_TYPE_ATOM_STRING;
}

MN_API int mptest_sym_walk_peekexpr(mptest_sym_walk* walk)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_peeknext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  return child->type == MPTEST__SYM_TYPE_EXPR;
}

MN_API int mptest_sym_walk_peeknum(mptest_sym_walk* walk)
{
  int err = 0;
  const mptest__sym_tree* child;
  mn_int32 child_ref;
  if ((err = mptest__sym_walk_peeknext(walk, &child_ref))) {
    return err;
  }
  child = mptest__sym_getcref(walk->sym, child_ref);
  return child->type == MPTEST__SYM_TYPE_ATOM_NUMBER;
}

MN_API int mptest__sym_check_init(
    mptest_sym_build* build_out, const char* str, const char* file, int line,
    const char* msg)
{
  int err = 0;
  mptest_sym* sym_actual = MN_NULL;
  mptest_sym* sym_expected = MN_NULL;
  mn__str_view in_str_view;
  const char* err_msg;
  mn_size err_pos;
  mptest_sym_build parse_build;
  sym_actual = (mptest_sym*)MN_MALLOC(sizeof(mptest_sym));
  if (sym_actual == MN_NULL) {
    err = 1;
    goto error;
  }
  mptest__sym_init(sym_actual);
  sym_expected = (mptest_sym*)MN_MALLOC(sizeof(mptest_sym));
  if (sym_expected == MN_NULL) {
    err = 1;
    goto error;
  }
  mptest__sym_init(sym_expected);
  mptest_sym_build_init(
      build_out, sym_actual, MPTEST__SYM_NONE, MPTEST__SYM_NONE);
  mn__str_view_init_n(&in_str_view, str, mn__str_slen(str));
  mptest_sym_build_init(
      &parse_build, sym_expected, MPTEST__SYM_NONE, MPTEST__SYM_NONE);
  if ((err = mptest__sym_parse_do(
           &parse_build, in_str_view, &err_msg, &err_pos))) {
    goto error;
  }
  mptest__state_g.fail_data.sym_fail_data.sym_actual = sym_actual;
  mptest__state_g.fail_data.sym_fail_data.sym_expected = sym_expected;
  return err;
error:
  if (sym_actual != MN_NULL) {
    mptest__sym_destroy(sym_actual);
    MN_FREE(sym_actual);
  }
  if (sym_expected != MN_NULL) {
    mptest__sym_destroy(sym_expected);
    MN_FREE(sym_expected);
  }
  if (err == MPTEST__SYM_PARSE_ERROR) { /* parse error */
    mptest__state_g.fail_reason = MPTEST__FAIL_REASON_SYM_SYNTAX;
    mptest__state_g.fail_file = file;
    mptest__state_g.fail_line = line;
    mptest__state_g.fail_msg = msg;
    mptest__state_g.fail_data.sym_syntax_error_data.err_msg = err_msg;
    mptest__state_g.fail_data.sym_syntax_error_data.err_pos = err_pos;
  } else if (err == -1) { /* no mem */
    mptest__state_g.fail_reason = MPTEST__FAIL_REASON_NOMEM;
    mptest__state_g.fail_file = file;
    mptest__state_g.fail_line = line;
    mptest__state_g.fail_msg = msg;
  }
  return err;
}

MN_API int mptest__sym_check(const char* file, int line, const char* msg)
{
  if (!mptest__sym_equals(
          mptest__state_g.fail_data.sym_fail_data.sym_actual,
          mptest__state_g.fail_data.sym_fail_data.sym_expected, 0, 0)) {
    mptest__state_g.fail_reason = MPTEST__FAIL_REASON_SYM_INEQUALITY;
    mptest__state_g.fail_file = file;
    mptest__state_g.fail_line = line;
    mptest__state_g.fail_msg = msg;
    return 1;
  } else {
    return 0;
  }
}

MN_API void mptest__sym_check_destroy()
{
  mptest__sym_destroy(mptest__state_g.fail_data.sym_fail_data.sym_actual);
  mptest__sym_destroy(mptest__state_g.fail_data.sym_fail_data.sym_expected);
  MN_FREE(mptest__state_g.fail_data.sym_fail_data.sym_actual);
  MN_FREE(mptest__state_g.fail_data.sym_fail_data.sym_expected);
}

MN_API int mptest__sym_make_init(
    mptest_sym_build* build_out, mptest_sym_walk* walk_out, const char* str,
    const char* file, int line, const char* msg)
{
  mn__str_view in_str_view;
  const char* err_msg;
  mn_size err_pos;
  int err = 0;
  mptest_sym* sym_out;
  sym_out = (mptest_sym*)MN_MALLOC(sizeof(mptest_sym));
  if (sym_out == MN_NULL) {
    err = 1;
    goto error;
  }
  mptest__sym_init(sym_out);
  mptest_sym_build_init(build_out, sym_out, MPTEST__SYM_NONE, MPTEST__SYM_NONE);
  mn__str_view_init_n(&in_str_view, str, mn__str_slen(str));
  if ((err =
           mptest__sym_parse_do(build_out, in_str_view, &err_msg, &err_pos))) {
    goto error;
  }
  mptest_sym_walk_init(walk_out, sym_out, MPTEST__SYM_NONE, MPTEST__SYM_NONE);
  return err;
error:
  if (sym_out) {
    mptest__sym_destroy(sym_out);
    MN_FREE(sym_out);
  }
  if (err == MPTEST__SYM_PARSE_ERROR) { /* parse error */
    mptest__state_g.fail_reason = MPTEST__FAIL_REASON_SYM_SYNTAX;
    mptest__state_g.fail_file = file;
    mptest__state_g.fail_line = line;
    mptest__state_g.fail_msg = msg;
    mptest__state_g.fail_data.sym_syntax_error_data.err_msg = err_msg;
    mptest__state_g.fail_data.sym_syntax_error_data.err_pos = err_pos;
  } else if (err == -1) { /* no mem */
    mptest__state_g.fail_reason = MPTEST__FAIL_REASON_NOMEM;
    mptest__state_g.fail_file = file;
    mptest__state_g.fail_line = line;
    mptest__state_g.fail_msg = msg;
  }
  return err;
}

MN_API void mptest__sym_make_destroy(mptest_sym_build* build)
{
  mptest__sym_destroy(build->sym);
  MN_FREE(build->sym);
}

#endif
