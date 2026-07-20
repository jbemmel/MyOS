// Collection of simple types

#ifndef MYOSTYPES_H
#define MYOSTYPES_H

#include "defs.h"   // PACKED

/// common constants
enum
{
	_1KB = 0x000400,
	_4KB  = 4 * _1KB,
   _32KB = 32 * _1KB,
   _64KB = 64 * _1KB,
   _128KB = 128 * _1KB,
   _256KB = 256 * _1KB,
   _512KB = 512 * _1KB,   
	_1MB = 0x100000,
	_4MB = 0x400000
};


// signed types
typedef int s32;
typedef short s16;
typedef signed char s8;

// unsigned variants
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int size_t;

// u64 is normally defined by GCC in <stdint.h>, but I can't find it
#ifdef __GNUC__
   typedef unsigned long long u64;
   typedef long long int s64;
#elif defined(_MSC_VER)
   typedef unsigned __int64 u64;
   typedef __int64 s64;
#endif

#define INT_MAX         ((int)(~0U>>1))
#define INT_MIN         (-INT_MAX - 1)

typedef const char* string_t;

/// Macro variants used for constants
#define HTONS16(val)  ((u16) ( \
    (((u16) (val) & (u16) 0x00ffU) << 8) | \
    (((u16) (val) & (u16) 0xff00U) >> 8)))
#define HTONS32(val) ((u32) ( \
    (((u32) (val) & (u32) 0x000000ffU) << 24) | \
    (((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
    (((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
    (((u32) (val) & (u32) 0xff000000U) >> 24)))

//
// Simple helper functions, can also be used elsewhere
//
inline u16 htons16( u16 d )
{
#ifdef __GNUC__
   if (__builtin_constant_p(d)) {
      return HTONS16(d);
   } else {
      __asm__ ( "xchgb %h0, %b0" : "=q"(d) : "0"(d) );
     return d;
   }
#else
	return HTONS16(d);
#endif
}

inline u32 htons32( u32 d )
{
#ifdef __GNUC__
   if (__builtin_constant_p(d)) {
      return HTONS32(d);
   } else {  
      __asm__ ( "bswap %0" : "=r"(d) : "0"(d) );   // 'const' but gcc ignores it
     return d;
   }
#else
	return HTONS32(d);
#endif
}


// network order types
struct nw16_t {

private:
    u16 value;

public:
   // No constructor, so they can be used in unions
   //inline nw16_t() {}
   //inline explicit nw16_t( u16 v  ) : value( htons16(v) ) {}
   inline u16 hostorder() const        { return htons16(value); }
   inline u16 netorder() const         { return value; }

   inline bool operator ==( const nw16_t o ) const { return value==o.value; }

   inline nw16_t& operator =( u16 v ) {
      value = htons16(v);
      return *this; 
   }

   inline void setNetOrder( u16 valInNetworkOrder ) {
      value = valInNetworkOrder;
   }

} PACKED;

struct nw32_t {
private:
    u32 value;

public:
   // No constructor, so they can be used in unions
   //inline nw32_t() {}
   //inline explicit nw32_t( u32 v  ) : value( htons32(v) ) {}
   inline u32 hostorder() const    { return htons32(value); }
   inline u32 netorder() const      { return value; } 

   inline nw32_t& operator =( u32 v ) {
      value = htons32(v);
      return *this; 
   }

	inline void setValueNetorder( u32 inNetOrder ) { value = inNetOrder; }

	inline bool operator ==( const nw32_t o ) const { return value==o.value; }

	// Avoid swapping	
	inline void clear() { value=0; }
} PACKED;

inline static int min( int a, int b ) { return (a<=b) ? a : b; }
inline static int max( int a, int b ) { return (a>=b) ? a : b; }  

#endif
