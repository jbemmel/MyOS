#ifndef BITBUDDYALLOCATOR_HPP
#define BITBUDDYALLOCATOR_HPP

#include "BitBuddyAllocator.h"
#include "mem.h"  // memset_aligned

#include "debug.h"
#include "MultiThreading/IThread.h"
 
/**
 *  Atomically complements bit[i] in array, and returns the previous value
 *  @param array - The array to modify
 *  @param i     - The index_t of the bit to clear
 *  @return True iff the bit was set
 */
static inline
bool BTC( void *_this, u32 i )
{
    bool r;
    ASMVOLATILE(
        "btc %2, (%1)  \t\n"
        "setc %0       \t\n"
        : "=a"(r)
        : "r"(_this), "0"(i)
        : "memory" );
    return r;  // previous value
}

namespace MyOS { namespace MM {

/**
 *  Order[0] -> 0
 *  Order[1] -> #(Order[0]) = 2^(MAX-MIN-1)     // -1 for buddy pair
 *  Order[2] -> #(Order[0]) + #(Order[1]) = (eg) 2^19 + 2^18 = (4-1)*2^18
 *  ...
 *  Order[n] -> 2^(MAX-MIN) - 2^(MAX-MIN-n)     // constant minus something
 *  
 *  Incremental calculation: O[0] -> O[1] : add 2^(MAX-MIN-o-1) each iteration
 */
#define BIT_START(o) ((1UL << (MAX_ORDER-O0_BITS)) - ((1UL << (MAX_ORDER-O0_BITS-o))))
#define DWORD_START(o) (BIT_START(o)>>5)

template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::BitBuddyAllocator()
: totalUnits(0), availableUnits(0), allocs(0), frees(0)
{
   // atomic32 constructor clears the 'array' member already
}   

template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::init()
{
   // L1-cache aligned (32 bytes on Pentium/PPro)
   ASSERTIONv( ((u32)array & 0x1F)==0, E_ERROR, (u32)array );   
   
   memset_aligned( (void*) bits, 0, (1UL << (MAX_ORDER-O0_BITS-3)) );
#ifdef DEBUG
   for (order_t o = 0; o<=(MAX_ORDER-O0_BITS); ++o) {
      ASSERTIONv( array[o]==0, E_CRITICAL, o );
   }          
#endif   
}   


template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
unit_t
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::allocate( order_t order ) 
{
   ++allocs;
   
   //DPRINTP( "\nAlloc o=%d", order );
   for (order_t o = order; o<=(MAX_ORDER-O0_BITS); ++o) {

      unit_t f = array[o].Replace(0);
      if (f) {                      // found a block
         while (--o>=order) {       // put back the pieces

            // choose lower half, put back upper (array[n] was 0 before...)
            u32 old = array[o].Replace( (f + (1<<(o+O0_BITS))) );
            if (RARELY(old)) {
               DPRINTK( "\n#4f#Parallel alloc failure: old=%x", old );
               
               // XX Bit expensive: free it by setting the bit
               register u32 bitindex = BIT_START(o) + (old >> (O0_BITS+o+1));
            
               // Check pairbit in map, set it if other buddy is not free
               if ( BTC( &((u32*)bits)[bitindex >> 5], (bitindex&31) ) != false ) {
                  BUG( "TODO: Parallel alloc old head, merge with free buddy" );
               }
            }
         }
         availableUnits -= (1UL << order);  // should be atomic
         // DPRINTP( "\nalloc -> %x", f );
         return f;
      }
   }

   // should do a second pass here, selecting a frame by scanning bits
   DPRINTK( "\nOUT OF MEMORY!! Total=%d available units=%d allocs=%d frees=%d", 
      totalUnits, availableUnits, allocs, frees );
   ASMVOLATILE( "cli" );

   u32 frag, dist;
   this->getMetrics(frag,dist);
   
   MultiThreading::Thread::currentThread().showThreadMem();
   
   while(1);
   
   return 0;
}

template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void 
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::showMRUUnits() {
   DPRINTK( "\nMAX_INDEX = %x", (1 << (MAX_ORDER-O0_BITS-5)) );
   for (int f=0; f<=(MAX_ORDER-O0_BITS); ++f) {
      DPRINTK( "\nMRU[%d]=%x size=0x%x units :", f, (u32) array[f], 1<<f );
   }
   //u32 o = 0;
   //if (sizeof(bits)<0x200) for (u32 b=0; b<sizeof(bits)/sizeof(u32); ++b) {
   //   if (b==DWORD_START(o)) { DPRINTK("|"); ++o; }    
   //   DPRINTK( "%x", ((u32*)bits)[b] );
   //}      
}

#ifdef OLD_FREE
template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::free( unit_t frame, order_t order ) {

   ++frees;   
   ASSERTION( frame & 1, E_ERROR ); // make sure 'present' bit is set

   // iterate over all orders but the top one
   for (register u32 o = order; o<(MAX_ORDER-O0_BITS); ++o) {

      // To allow parallel operations, temporarily remove head from list
      register unit_t head = array[o].Replace(0);
      if (head) {
   
         // first test if head & frame are buddies, if so merge them!
         u32 buddybit = (1UL<<(o+O0_BITS));
         if ( ( (frame^head) & buddybit ) != buddybit ) {
            // keep & adjust BIT_START, instead of recalculating ??
            register u32 bitindex = BIT_START(o) + (head >> (O0_BITS+o+1));
            
            // Check pairbit in map, set it if other buddy is not free
            if ( BTC( &((u32*)bits)[bitindex >> 5], (bitindex&31) ) == false ) {
            
               // Put frame in MRU list, bit is now set for head
               u32 old = array[o].Replace( frame );            
               if (RARELY(old)) { BUG( "\nParallel free failure!" ); }
            
               availableUnits += (1UL << order);  // should be atomic
               return;
            }
         } 
         frame = head & ~( buddybit );    // merge by clearing buddybit
      } else {
         // done, put 'frame' in array[o] (TODO: should be atomic...)
         u32 old = array[o].Replace( frame );
         if (RARELY(old)) { BUG( "\nParallel free(2) failure!" ); }
         availableUnits += (1UL << order);  // should be atomic
         return;
      }
   }
   
   // only comes here when max order is reached (SEGV in loop otherwise)
   (u32&) array[ MAX_ORDER-O0_BITS ] = 1;
   availableUnits = (1UL << (MAX_ORDER-O0_BITS));
}
#else

/**
 * Frees a unit previously allocated with 'allocate'
 * 
 * The strategy is to defer coalescing as much as possible. So when a free is
 * done that is a buddy of an entry in the MRU list, rather than merging them,
 * the buddypair bit is set, and the MRU entry is replaced with the entry
 * being freed (which is more likely MRU)
 */
template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::free( unit_t frame, order_t order ) {

   ++frees;   
   ASSERTION( frame & 1, E_ERROR ); // make sure 'present' bit is set
   ASSERTIONv( order <= (MAX_ORDER-O0_BITS), E_ERROR, order );

   // iterate over all orders but the top one
   for (register u32 o = order; o<=(MAX_ORDER-O0_BITS); ++o) {

      // To allow parallel operations, temporarily remove head from list
      register unit_t head = array[o].Replace(frame);
      if (head) {

         register u32 bitindex = BIT_START(o) + (head >> (O0_BITS+o+1));

         // Check pairbit in map; if it was set, clear and continue
         if ( BTC( &((u32*)bits)[bitindex >> 5], (bitindex&31) ) ) {
            u32 buddybit = (1UL<<(o+O0_BITS));
            frame = head & ~( buddybit );    // merge by clearing buddybit
            continue;
         }
      } 

      // done, 'frame' is now in array[o]
      availableUnits += (1UL << order);  // should be atomic
      return;
   }
   
   // only comes here when max order is reached (SEGV in loop otherwise)
   BUG( "\n###MAX_ORDER reached" );
   //(u32&) array[ MAX_ORDER-O0_BITS ] = 1;
   //availableUnits = (1UL << (MAX_ORDER-O0_BITS));
}
#endif

template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::compact() {
   // iterate over all orders but the top one
   for (register u32 o = 0; o<(MAX_ORDER-O0_BITS); ++o) {

      // To allow parallel operations, temporarily remove head from list
      register unit_t head = array[o].Replace(0);
      u32 o2 = o;
      while (head) {
         register u32 bitindex = BIT_START(o2) + (head >> (O0_BITS+o2+1));

         // Check pairbit in map, MRU entries are considered free
         if ( BTC( &((u32*)bits)[bitindex >> 5], (bitindex&31) ) ) {
            u32 buddybit = (1UL<<(o2+O0_BITS));
            head &= ~( buddybit );    // merge by clearing buddybit
            if (++o2 >= (MAX_ORDER-O0_BITS)) {
               (u32&) array[ MAX_ORDER-O0_BITS ] = head;
               return;
            }
         } else {
            // in bitmap now, no merge possible
            break;
         }
      }
   } 
}

template <u8 MAX_ORDER,u8 O0_BITS BITSDECL>
void
BitBuddyAllocator<MAX_ORDER,O0_BITS BITSVAR>::getMetrics(
   u32& fragmentation, u32& distribution
) {
   // create temporary table to make counting easier
   u8 bc[256];
   for (u32 i=0; i<256; ++i) {    // could do until /2
      u8 count = 0;
      for (u32 b=0x80; b; b>>=1) {
         count += ((i&b)!=0);
      }
      bc[i] = count;
      // bc[~i] = 8-count;
   }

   u32 ordercounts[ MAX_ORDER ];
   u32 total = 0, total_corrected = 0;
   order_t o;
   for (o=0; o<MAX_ORDER; ++o) {
      // note: this does not count the higher orders, where st=st+1
      u32 count = 0;
      u32 end = DWORD_START(o) + (1UL << (MAX_ORDER - O0_BITS - o - 6));
      if (end&0x80000000) {
         ordercounts[o] = 0xFFFFFFFF;
         break;
      }
      for (u32 i = DWORD_START(o); i < end; ++i) {
         for (u32 dw = ((u32*)bits)[ i ]; dw!=0; dw>>=8) count += bc[ dw&0xff ];
      }
      ordercounts[o] = count;
      DPRINTK( "\nCounted %d bits set at order %d", count, o );
      total += count;
      total_corrected += count << o;
   }
   DPRINTK( "\nLast dword in bits:%x", ((u32*)bits)[ (1 << (MAX_ORDER-O0_BITS-5) ) -1 ] );

   u32 avg = total_corrected / o, avgsum = 0;
   DPRINTK( "\navg = %d", avg );
   for (order_t o2=0; o2<o; ++o2) {
      s32 term = ordercounts[o2] - avg;
      avgsum += term * term;
   }   
   
   // in promilles
   fragmentation = (total * 1000) >> (MAX_ORDER-O0_BITS);
   distribution  = avgsum;
}

}} // namespace

#endif
