//
//	Common memory manipulation routines
//

#ifndef MYMEMORY_H
#define MYMEMORY_H

#include "defs.h"
#include "types.h"  // u32

#ifndef __GNUC__
#pragma warning(disable:4101)
#endif

// These should only be used when 'string.h' cannot, ie undefined reference to memcpy error

/*
inline void* memcpy_aligned( void* dest, const void* src, size_t bytecount )
{
    // return __builtin_memcpy( dest, src, bytecount );
  
   // assumes ES==DS and multiple of 4 bytes !! #WARNING# This changes registers...
   u32 d0, d1, d2;
   ASMVOLATILE(
      "cld;"
//      ".align 16;"   // add this ??
      "rep movsl;"
      : "=&c"(d0), "=&S"(d1), "=&D"(d2)
      : "2"(dest), "1"(src), "0"(bytecount/4)
   );   
}
*/
#define memcpy_aligned(a,b,c) memcpy(a,b,c)

/**
 * OK, seems fixed now
 * 
 * @note API slightly different from standard memset: 2nd parameter is 32 bits i/o 8!
 */
static inline void* memset_aligned( void* dest, u32 value, size_t bytecount )
{
   u32 d0, d1;
   // assumes ES==DS !!
   ASMVOLATILE(
      "cld; rep stosl"      // Note the 'cld', needed? built_in does not do that
      : "=&c"(d0), "=&D"(d1)
      : "1"(dest), "a"(value), "0"(bytecount/4)
      : "memory"
   );
   return dest;
}
// #define memset_aligned(a,b,c) memset(a,b,c)

/**
 * This macro can be used to execute the next x bytes in code copied to the 
 * stack. Useful when the code memory is about to be released !
 *
 *	@Note the copied code should include a 'ret' instruction ! (could check that)
 * 
 * XX - This won't work in optimized code, the label gets 'optimized' (moved)!
 * 
 */
#define RUNONSTACK(codebytes) \
   u8 __codebuffer[codebytes]; \
   memcpy_aligned( __codebuffer, &&stackcode, codebytes ); \
   ASMVOLATILE( "jmp *%0" : : "r"(__codebuffer) : "memory" ); \
stackcode:




#endif
