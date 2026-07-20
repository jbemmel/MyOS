#ifndef ZIPCONTEXT_H
#define ZIPCONTEXT_H

#include "ZipHeader.h"  // E_RESULT
#include "types.h"

namespace MyOS {
namespace IO { class IStream; } 
namespace Zip {

using IO::IStream;

class HuffmanTree;

/**
 *	Contains the state variables needed during unzip
 */
class ZipContext
{    
   IStream* inputstream;		// optional, is used to fill buffer if present
   const buffer& inputbuffer;
   const buffer& winbuffer;

   u8* inptr;
   u8* outptr;

   // state data
   u32 crc;
   u32 wp, bk, bb;
        
   u8 getByte();

   // initialization routines
   static void initcrctable() ;

   E_RESULT inflateBlock() ;
   E_RESULT inflateStored() ;
   E_RESULT inflateFixed() ;
   E_RESULT inflateDynamic() ;

   E_RESULT inflateCodes( HuffmanTree*, HuffmanTree*, s32, s32) ;

   inline void decodedByte( u32& windowptr, u8 byte );
   void flushWindow( u32 w ) ;

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is

   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

    enum {  LBITS = 9,          /* bits in base literal/length lookup table */
            DBITS = 6,          /* bits in base distance lookup table */
    };

public:
	/**
	 *	Constructor, allocates an internal inputbuffer for efficiency
	 *	(a virtual getByte() call per byte is not)
	 */
	ZipContext( IStream& inputstream );

	/**
	 *	Constructor, assumes inputdata contains all zipped data
    *
    *  @param window32k - buffer, size must be power of 2
    *  @param treemem   - Start of memory to use for tree allocations
    *  @param eom       - End of tree memory
    */
   void constructor( const buffer& in, u8* treemem, u8* eom ); 

   inline ZipContext( const buffer& in, const buffer& window32k, u8* treemem, u8* eom )
   : inputbuffer(in), winbuffer(window32k)
   {
      constructor( in, treemem, eom ); 
   }

   /**
    * The routine that does all the work
    */
   E_RESULT inflate( buffer& result ) ;

   /**
    *  This method must be called before this class can be used
    */
   static void initialize() ;

   /**
    *  Used for memory allocation for decoding trees
    */
   inline HuffmanTree* allocate( size_t nodes ) ;
   // inline void free( void* memory ) { /* nothing */ }

private:
   u8* treemem;
   u8* curmemptr;
   u8* endOfMemory;
};

}} // namespace

#include "HuffmanTree.h"
#include "debug.h"

namespace MyOS { namespace Zip {

// inline functions
inline HuffmanTree*
ZipContext::allocate( size_t nodes )
{
   HuffmanTree* r = (HuffmanTree*) this->curmemptr;
   curmemptr += nodes*sizeof(HuffmanTree);

   // NOTE: This won't work during startup...   
   ASSERTION( curmemptr < endOfMemory, E_ERROR );
   return r;
}


}}	// namespace

#endif
