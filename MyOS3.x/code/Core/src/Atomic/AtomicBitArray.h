//
//	There is no such thing as a CMPXCHG(bit) !
//	So
//		Set:					Clear:
//		1)	BTS it				just BTR it
//		2)	if (old_value==0) done
//		3)	else {
//		4)		try next }


#ifndef ATOMICBITARRAY_H
#define ATOMICBITARRAY_H

#include "types.h"
#include "atomic32.h"

#include "debug.h"

namespace MyOS {

// for some stupid reason, these cannot be template members or they are optimized away ! (volatile unknown)

/**
 *  Atomically sets bit[i] in array, and returns the previous value
 *  @param array - The array to modify
 *  @param i     - The index_t of the bit to set
 *  @return True iff the bit was set
 */
static inline
bool BTS( void* array, u32 i )
{
    bool r;
    ASMVOLATILE(
        "bts %2, (%1)  \t\n"
        "setc %0       \t\n"
        : "=a"(r)
        : "r"(array), "0"(i)
        : "memory"  );
    return r;	// previous value
}

/**
 *  Atomically clears bit[i] in array, and returns the previous value
 *  @param array - The array to modify
 *  @param i     - The index_t of the bit to clear
 *  @return True iff the bit was set
 */
static inline  /* GCC says inlining fails... */
bool BTR( void *_this, u32 i )
{
    bool r;
    ASMVOLATILE(
        "btr %2, (%1)  \t\n"
        "setc %0       \t\n"
        : "=a"(r)
        : "r"(_this), "0"(i)
        : "memory" 
    );
    return r;	// previous value
}

/// Implements an atomic bitarray class
class AtomicBitArrayImpl
{
public:
    enum { E_FULL = 0xFFFFFFFF };

    // returns E_FULL if failed (full)
    static u32 getIndex( atomic32* data, u32 bitmask, volatile u32& hint );

    // returns the start of a range of sequential indices (max. 32)
    static u32 getMulti( 
      s8 amount, atomic32 *data, u32 bitmask, volatile u32& hint, u8 align=1 );
};


/**
 *  Template for different sizes of bitarrays, operations are atomic
 *  @param SIZE - #bits, must be a power of 2
 */
template <size_t SIZE, u32 BASE=0>
class AtomicBitArray
{
    atomic32 data[ (SIZE+31) / 32 ];	// multiple of 32 bits, rounded up
    volatile u32 hint;                 // index to start looking
    volatile ATOMIC32<s32> free;       // signed, it can drop below 0 (rarely)

public:
    typedef u32 index_t;
    enum { E_FULL = AtomicBitArrayImpl::E_FULL };

    inline AtomicBitArray() : hint(0), free(SIZE) {}

    // Clear all at once (not atomic of course)
    inline void clear() {
      memset_aligned( data, 0, (SIZE+31) / 8 );       
    }
    
    inline bool isClear() const {
      for (u32 i=0; i<(SIZE+31) / 32; ++i) {
         if (data[i]!=0) return false;  
      }
      return true;
    }

    /// Returns the number of bits set to <code> false </code>
    inline size_t getFree() const
        { s32 f = (s32) free; return (f>=0) ? f : 0;}

    // Returns true iff all entries are free (size==max)
    inline bool isEmpty() const { return free==SIZE; }

    // Returns the index of a bit set from false to true, E_FULL if failed (full)
    index_t getIndex();

    // Returns the start of a range of sequential indices (max. 32),
    // E_FULL if failed
    index_t getMulti( s8 amount, u8 align=1 );

    // Set bit[i] to false, i should be [0, SIZE[
    inline void releaseIndex( index_t i );

    // Sets bit[i]..bit[i+count-1] to false, i should be [0, SIZE[
    inline void releaseMulti( index_t i, s8 count ) {
      i -= BASE;
      register u32 freebits = ((1<<count)-1) << (i%32);
      ASSERTIONv( (i+count) < SIZE, E_CRITICAL, i );
      ASSERTIONv( (data[i/32] & freebits) == freebits, E_CRITICAL, freebits );
      data[i/32] &= ~freebits;
      *((u32*)&free) += count;  // note: result not needed -> no need for atomic      
    }      

    // Sets bit[i] to true (for initial provisioning), i = [0..SIZE[
    inline void markUsed( index_t i );
    
    // Returns the state of a bit
    inline bool isSet( index_t i ) const {
        // i -= BASE;
        return data[ i>>5 ] & (1<<(i&0x1F)); 
    }

    // returns the state of 32 bits in one go
    inline u32 areSet( index_t i ) const {
        // i -= BASE;
        return data[ i>>5 ];
    }
    
    inline bool lockIfFree();
    inline void release();
    
};

template <size_t SIZE, u32 BASE>
inline typename AtomicBitArray<SIZE,BASE>::index_t
AtomicBitArray<SIZE,BASE>::getIndex()
{
    // first make sure there is at least 1
    if ( (s32) free.ReadAdd( -1 ) >= (s32) 1) {
        index_t i =  AtomicBitArrayImpl::getIndex( data, SIZE-1, hint );
        if (i != E_FULL) {
            return i+BASE;
        }
    }
    
    ++free;
    return E_FULL;
}

// returns the start of a range of sequential indices (max. 32)
template <size_t SIZE, u32 BASE>
inline typename AtomicBitArray<SIZE,BASE>::index_t
AtomicBitArray<SIZE,BASE>::getMulti( s8 amount, u8 align /* =1 */ )
{
    if ( (s32) free.ReadAdd( -amount ) >= (s32) amount) {
        index_t i = AtomicBitArrayImpl::getMulti( amount, data, SIZE-1, hint, align );
        if (i != E_FULL) {
            return i+BASE;
        }
    }

   // restore count
   free += amount;
   // DPRINTK( "\ngetMulti->FULL, free restored with %d to %d", amount, (u32)free );    
   return E_FULL;
}


template <size_t SIZE, u32 BASE>
inline void
AtomicBitArray<SIZE,BASE>::releaseIndex( index_t i )
{
    // could spin while false, to cope with concurrent temporary 'clears' if any ?
    if (BTR( data, i-BASE )) {
        ++free;
    } else BUG( "releaseIndex on zero bit" );

    // could do hint=i, but that could also make things considerably worse...
}

/*
// i should be [0, SIZE[
template <u32 SIZE>
inline void
AtomicBitArray<SIZE>::releaseMulti( index_t i, u8 count )
{
   register u32 freebits = ((1<<count)-1) << (i%32);
   ASSERTIONv( (data[i/32] & freebits) == freebits, E_CRITICAL, freebits );
   data[i/32] &= ~freebits;
   (u32&)free += count;  // note: result not needed -> no need for atomic
}
*/

// for initial provisioning
template <u32 SIZE, u32 BASE>
inline void
AtomicBitArray<SIZE,BASE>::markUsed( index_t i )
{
    ASSERTION( (i-BASE) < SIZE, E_ERROR );
 
    // DPRINTK( "mark:%d free=%d |", i, (u32)free );
    if (BTS( data, i-BASE )==false) {
        --free;     // Note: need not be atomic
    } else ASSERTIONv( "\nbit not free!"==0, E_WARNING, i-BASE );
}

// for locking access to the bitarray when it is empty
template <u32 SIZE, u32 BASE>
inline bool
AtomicBitArray<SIZE,BASE>::lockIfFree( )
{
   return free.CmpXchg( SIZE, -1 );   // special value to mark lock
}

template <u32 SIZE, u32 BASE>
inline void
AtomicBitArray<SIZE,BASE>::release( )       // may only be called after lockIfFree returned true
{
   ASSERTION( free == -1, E_ERROR );   // There should be no more frees
   *((u32*) &free) = SIZE;
}

}  // namespace

#endif
