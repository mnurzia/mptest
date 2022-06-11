#if !defined(MN__MPTEST_CONFIG_H)
#define MN__MPTEST_CONFIG_H

/* Set to 1 in order to define setjmp(), longjmp(), and jmp_buf replacements. */
#if !defined(MN_USE_CUSTOM_LONGJMP)
#include <setjmp.h>
#define MN_SETJMP setjmp
#define MN_LONGJMP longjmp
#define MN_JMP_BUF jmp_buf
#endif

/* bits/hooks/malloc */
/* Set to 1 in order to define malloc(), free(), and realloc() replacements. */
#if !defined(MN_USE_CUSTOM_ALLOCATOR)
#include <stdlib.h>
#define MN_MALLOC malloc
#define MN_REALLOC realloc
#define MN_FREE free
#endif

/* bits/types/size */
/* desc */
/* cppreference */
#if !defined(MN_SIZE_TYPE)
#include <stdlib.h>
#define MN_SIZE_TYPE size_t
#endif

/* bits/util/debug */
/* Set to 1 in order to override the setting of the NDEBUG variable. */
#if !defined(MN_DEBUG)
#define MN_DEBUG 0
#endif

/* bits/hooks/assert */
/* desc */
/* cppreference */
#if !defined(MN_ASSERT)
#include <assert.h>
#define MN_ASSERT assert
#endif

/* bits/util/exports */
/* Set to 1 in order to define all symbols with static linkage (local to the
 * including source file) as opposed to external linkage. */
#if !defined(MN_STATIC)
#define MN_STATIC 0
#endif

/* bits/types/char */
/* desc */
/* cppreference */
#if !defined(MN_CHAR_TYPE)
#define MN_CHAR_TYPE char
#endif

#if MPTEST_USE_SYM
/* bits/types/fixed/int32 */
/* desc */
/* cppreference */
#if !defined(MN_INT32_TYPE)
#endif
#endif /* MPTEST_USE_SYM */

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_DYN_ALLOC)
#define MPTEST_USE_DYN_ALLOC 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_LONGJMP)
#define MPTEST_USE_LONGJMP 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_SYM)
#define MPTEST_USE_SYM 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_LEAKCHECK)
#define MPTEST_USE_LEAKCHECK 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_COLOR)
#define MPTEST_USE_COLOR 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_TIME)
#define MPTEST_USE_TIME 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_APARSE)
#define MPTEST_USE_APARSE 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_USE_FUZZ)
#define MPTEST_USE_FUZZ 1
#endif

/* mptest */
/* Help text */
#if !defined(MPTEST_DETECT_UNCAUGHT_ASSERTS)
#define MPTEST_DETECT_UNCAUGHT_ASSERTS 1
#endif

#endif /* MN__MPTEST_CONFIG_H */
