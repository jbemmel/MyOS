#include "AtomicBitArray.h"
#include "debug.h"

namespace MyOS {

u32
AtomicBitArrayImpl::getIndex( atomic32 *data, u32 bitmask, volatile u32 &hint )
{
 
   // make a local copy of hint
   register u32 localhint = hint;
   
   // bitmask == SIZE-1
   for (s32 c = bitmask; c>=0; --c) {
      if ( BTS( data, localhint ) == false )  // found a free entry
      {
         hint = (localhint+1) & bitmask;
         return localhint;
      }
      localhint = (localhint+1) & bitmask;
   }  
   return E_FULL;
}


// returns the start of a range of sequential indices (max. 32)
u32 
AtomicBitArrayImpl::getMulti( 
   s8 amount, atomic32* data, u32 bitmask, volatile u32& hint, u8 align )
{
   ASSERTION( amount <= 32, E_ERROR );

   // PRINTF( "\ngetMulti amount=%u mask=%x", amount, bitmask );
   // ASSERTION( amount>1, E_WARNING );  // use getIndex for 1

   // start at 'hint' to the end, then again until hint. This may fail!

   // note: skips last (padded) element
	for ( u32 localhint = ((hint+31) & bitmask) >> 5, c = bitmask>>5; c>0; --c )
	{
		register u32 bits = data[ localhint ].Replace( E_FULL );
      if (bits != E_FULL) {
         // PRINTF( "..found bits %x", bits );

         // shift does not works for 32 bits, 1<<32==1 since the count is 0..31
         for (u32 mask = 0xFFFFFFFFUL >> (32-amount), i=0; (s32) i <= (s8) (32-amount); i+=align, mask<<=1) {

            // check if bits are free
            if ((bits&mask)==0) {
               // DPRINTK( "\nfound %u #%X Xfirst %x putting back %x", (localhint << 5) + i, amount, bits, bits|mask );
               *((u32*) &data[ localhint ]) = (bits | mask);   // put back unused bits
               hint = localhint;
               return (localhint << 5) + i;              // convert to bit index
            }
         }
         (u32&) data[ localhint ] = bits;
      }
      localhint = (localhint+1) & (bitmask>>5);
   }
   
   return E_FULL;
}

}  // namespace
