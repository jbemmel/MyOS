/**
 * Option 1: An array of MAX_ORDER MRU frames, with a bitarray for each order to keep track
 * of free (bit==1) frames. For 4GB of physical memory, total size is only 132Kb
 * (linear memory, it could perhaps even be paged..)
 *
 * Deallocation works best if it is done in reverse allocation order (ends up with a clean
 * allocator)
 *
 * Alternatives:
 * - keep more than 1 MRU entry
 * - do better determination of MRU by maintaining & using age bits
 * - select other than leftmost part of a larger-order block
 * - Use 1 bit per *pair* of buddies, instead of 1 bit per buddy
 * - Use a 'carving' allocator that will allocate e.g. 20 bytes as 32 - 8 - 4
 *   (with a min limit) ?
 *
 * I intend to have a low-priority thread periodically scan the MRU heads, and fill them if they are
 * empty.
 *
 * For updating the MRU bits in mapped pages, the best period may be determined dynamically:
 * record how many pages retire with each sweep, and adjust the period such that on average,
 * 1/N pages retire (where N is a policy parameter)
 *
 * Reason: If the thread runs too often, we cannot distinguish old pages from new ones anymore
 *         since they are all considered old
 *
 *         If it runs too little, idem but then all are young
 *
 * 11/1/2004 I've got the buddypair-bit solution working, only problem is that
 *           after a while 10 threads continously allocating memory will 
 *           make it reach a point where there are no more entries on the MRU
 *           list, and lots of bits set in the bitmaps. Problem is: there is
 *           no telling which buddy of each pair is free!
 * 
 *           I can think of a solution to put a magic marker as dword[0] in a
 *           free page, and check for that when MRU is exhausted
 * 
 *           Alternatively, a backup MRU stack can delay the point where the
 *           problem occurs, but is it sufficient?
 * 
 *           I can always switch back to the bit-per-buddy solution...
 * 
 *           On the other hand, the problem might be that some pages don't get
 *           freed. The 10 threads allocate 1000+2000+2000 bytes, then sleep,
 *           and then release the memory again. This should not exhaust memory
 * 
 *           0:52 -> I found the leak! PBT was allocating a 'spare' frame
 *           each time, I changed 'atomic32::Replace' but forgot to update
 *           that call... Now it runs much longer :)
 */
#ifndef BITBUDDYALLOCATOR_H
#define BITBUDDYALLOCATOR_H

// For mapping to a fixed position in memory
#ifdef BBA_FIXED_MEM
# define BITSDECL ,u32 bits
# define BITSVAR ,bits
#else
# define BITSDECL
# define BITSVAR
#endif
 
#include "types.h"
#include "Atomic/atomic32.h"

namespace MyOS { namespace MM {

typedef u32 unit_t;

/**
 * Template to implement a buddy allocator with bitmaps
 * @param MAX_ORDER: Log2 of the max unitsize represented, 32 = 2^32 = 4GB
 * @param O0_BITS: Log2 of the bits in the smallest allocatable unit, 12 for pages
 * 
 */
template <u8 MAX_ORDER=32, u8 O0_BITS=12 BITSDECL=0>
class BitBuddyAllocator {

   typedef s8 order_t;

   // MRU, this should be L1-cache aligned (32 byte)
   // For pages, contains frames *with* their paging bits intact   
   atomic32 array[ MAX_ORDER-O0_BITS + 1];

   // counts of total and available units
   u32 totalUnits, availableUnits;

   // statistics
   u32 allocs, frees;
   
   // In deployment, this should be allocated from separate pages such that it 
   // is 4K aligned
   //
   // Note: Using the buddypair-optimization: 1 bit per *pair* of buddies
   // So in total 2^(MAX-MIN-1)+2^(MAX-MIN-2)+...+1 bits == 2^(MAX-MIN), -5 
   // for addressing dwords
   //
#ifndef BBA_FIXED_MEM   
   u32 bits[ (1 << (MAX_ORDER-O0_BITS-5) )   ]; // 2^5=32 bits per entry
#else   
   /** @return Number of 4K-pages needed to map the bitmap */
   public: inline static u32 getPageCount() { 
      return (1 << (MAX_ORDER-O0_BITS-15)); 
   }
#endif

public:

   // Make template parameters available (right way?)
   enum { 
      E_MAX_ORDER = MAX_ORDER,
      E_O0_BITS   = O0_BITS,
#ifdef BBA_FIXED_MEM
      E_MAPPING = bits
#endif      
   };

   // Default constructor
   BitBuddyAllocator();

   // Clears memory used by bitmaps, must be separate from constructor
   void init();

   /**
    * The allocator starts out empty, it must be populated using
    * sequential 'free' calls
    * 
    * This method must then be called after all 'free' calls are made
    */
   inline void initDone( ) {
       totalUnits = availableUnits; 
   }

   /**
    * Allocates 2^order of 2^O0_BITS bits of storage
    * 
    * @param order: Order of allocation, [0..MAX_ORDER]
    */
   unit_t allocate( order_t order );

   /**
    * Frees memory previously allocated using 'alloc'
    * Note: Best results seem obtained when freeing in _reverse_ allocation
    * order
    * 
    * @param frame: frame *including age bits*, averaged if >1 page
    */
   void free( unit_t block, order_t order );

   inline size_t getAvailableUnits() const { return availableUnits; }
   inline size_t getTotalUnits() const { return totalUnits; }

   void showMRUUnits();

   void compact();
   
   /**
    * This methods returns metrics on the performance of the allocator
    * 
    * Fragmentation is measured by counting the set bits in the bitmap;
    * Worst case, all bits are set (1 buddy allocated per pair)
    * 
    * Distribution is measured by summing the set bits per order,
    * compensating for blocksize, and calculating the sum of mean square
    * Worst case, all buddies of order 0 are half-allocated, and none
    * of the other orders. Ideally, order[MAX-1] has 1 bit, [MAX-2] 2, etc
    * 
    */
   void getMetrics( u32& fragmentation, u32& distribution );
};

}} // namespace

#endif
