#include "types.h"

// Taken from Linux
extern u32 /*__attribute__((weak))*/ __div64_32(u64 *n, u32 base);

// Copied from Linux div64.h
/* The unnecessary pointer compare is there
 * to check for type safety (n must be 64bit)
 */
#define do_div(n,base) ({                      \
    u32 __base = (base);                       \
    u32 __rem;                                 \
    (void)(((typeof((n)) *)0) == ((u64 *)0));  \
    if (((n) >> 32) == 0) {                    \
        __rem = (u32)(n) % __base;             \
        (n) = (u32)(n) / __base;               \
    } else                                     \
        __rem = __div64_32(&(n), __base);      \
        __rem;                                 \
    })
