#ifndef HUFFMANTREE_H
#define HUFFMANTREE_H

#include "types.h"

namespace MyOS { namespace Zip {

class ZipContext;

struct TreeParams {
    u8* b;  /* code lengths in bits (all assumed <= BMAX) JVB: changed to uch */
    u32 n;  /* number of codes (assumed <= N_MAX) */
    u32 s;  /* number of simple-valued codes (0..s-1) */
    const u16* d; /* list of base values for non-simple codes */
    const u8* e;  /* list of extra bits for non-simple codes */

    TreeParams( u8* cl, u32 nc, u32 sv, const u16* base, const u8* ex )
        : b(cl), n(nc), s(sv), d(base), e(ex) {}
};

// Represents a single node in the huffman code tree
class HuffmanTree
{
/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */    

public:    
  union {
    u16 n;              /* literal, length base, or distance base */
    HuffmanTree* t;     /* pointer to next level of table */
  } v;
  u8 e;                /* number of extra bits or operation */
  u8 b;

public:    
    static int build( ZipContext& context, TreeParams& params, 
        HuffmanTree** result, s32& maxlookup ) UNCOMPRESSEDSECTION;

    inline u8 getE() const { return e; }
    inline u8 getB() const { return b; }
    inline u16 getN() const { return v.n; }
    inline HuffmanTree* getT() const { return v.t; }

    // void free( ZipContext& ctxt ) UNCOMPRESSEDSECTION;

private:
    HuffmanTree() {}
    void operator delete( void* );
} PACKED;

}}	// namespace

#endif
