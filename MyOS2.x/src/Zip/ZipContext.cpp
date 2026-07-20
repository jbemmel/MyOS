/**
 * NOTE: Some of these routines require quite some stack space!
 */

#include "ZipContext.h"
#include "HuffmanTree.h"
#include "string.h"
#include "buffer.h"

#ifdef TEST
   #include <stdio.h>
   #define SECTION(X)
   #define PRINTF(S,args...) printf(S,##args)
   #define ASSERT(X) if (!(X)) { PRINTF( "Assertion failed at %d:" #X, __LINE__ ); __asm__( "int $3" ); }
   #define ASSERTION(X,V)
#else
   
#endif

/// GCC 2.95.3 has a problem with const data in other segments
#if (__GNUC__ > 2 || __GNUC_MINOR__ > 95)
#  define _CONST_ const
#else
#  define _CONST_
#endif

// #define INTERCEPT ASMVOLATILE( "int $3" )
#define INTERCEPT

namespace MyOS { namespace Zip {

#define NEXTBYTE()  (*inptr++)
#define NEEDBITS(n) {while(k<(n)){b|=((u32)NEXTBYTE())<<k;k+=8;}}
#define DUMPBITS(n) {b>>=(n);k-=(n);}

/**
 *  Note: This ends up in .bss which is cleared after boot
 *  Therefore, the ZIP interface implementation must re-init it!
 */
static u32 crc_32_tab[256];

void
ZipContext::constructor( const buffer& inputdata, u8* m, u8* eom )
{
   inputstream = 0;
   crc = 0xffffffff;
   wp = bk = bb = 0;
   treemem = curmemptr = m;
   endOfMemory = eom;
   inptr = inputdata.getData();
}

/* ==========================================================
 * Write the output window window[0..outcnt-1] and update crc
 * (Used for the decompressed data only.)
 */
void
ZipContext::flushWindow( u32 w )
{
    register u32 c = crc;
    u8* in = winbuffer.getData();
    for (u32 n = 0; n < w; n++) {
	    u8 ch = *outptr++ = *in++;
	    c = crc_32_tab[((s32)c ^ ch) & 0xff] ^ (c >> 8);
    }
    crc = c;
    wp = 0;
}

inline void
ZipContext::decodedByte( u32& w, u8 b )
{
    winbuffer.getData()[ w++ ] = b;
    if (w==winbuffer.getSize()) {
        flushWindow(w);
        w=0;
    }
}

E_RESULT
ZipContext::inflate( buffer& result )
{
   E_RESULT r;
   outptr = result.getData();

  /* decompress until the last block */
  do {
    // gzip_mark(&ptr);
    r = inflateBlock();
    // gzip_release(&ptr);
  } while (r==E_OK);

  if (r==E_LAST_BLOCK) {   // last block encountered
    /* Undo too much lookahead. The next read will be byte aligned so we
     * can discard unused bits in the last meaningful byte.
     */
     while (bk >= 8) {
        bk -= 8;
        inptr--;
     }

     // flush out slide
     flushWindow(wp);

     struct ENDDATA {
        u32 originalCRC;
        s32 originalLength;
     } *check = (ENDDATA*) inptr;

     if (check->originalCRC != (crc ^ 0xffffffff)) {
         //PRINTF( "\nCRC error %x<->%x orig length=%u",
         //   check->originalCRC, (crc ^ 0xffffffff), check->originalLength );
         return E_CRC_ERROR;
     }
     if (check->originalLength != (outptr - result.getData())) {
         return E_LENGTH_MISMATCH;
     }

     return (E_RESULT) check->originalLength;
  } else return r;
}


void
ZipContext::initialize()
{
    initcrctable();
}

// private, static
void
ZipContext::initcrctable()
{
   // terms of polynomial defining this crc (except x^32):
   static _CONST_ u8 p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26} ;

   u32 i;    // counter for all possible eight bit values

   /* Make exclusive-or pattern from polynomial */
   u32 e = 0;
   for (i = 0; i < sizeof(p) ; i++) {
      e |= 1L << (31 - p[i]);
   }

   crc_32_tab[0] = 0;

   for (i = 1; i < 256; i++)
   {
      u32 c = 0;
      for (s32 k = i | 256; k != 1; k >>= 1)
      {
         c = c & 1 ? (c >> 1) ^ e : c >> 1;
         if (k & 1)
            c ^= e;
      }
      crc_32_tab[i] = c;
   }
}

E_RESULT
ZipContext::inflateBlock()
/* decompress an inflated block */
{
  register u32 b;       /* bit buffer */

  register u32 k;       /* number of bits in bit buffer */

  /* make local bit buffer */
  b = bb;
  k = bk;

  /* read in last block bit */
  NEEDBITS(1)
  bool lastBlock = (s32)b & 1;
  DUMPBITS(1)


  /* read in block type */
  NEEDBITS(2)
  u32 blocktype = (u32)b & 3;
  DUMPBITS(2)


  /* restore the global bit buffer */
  bb = b;
  bk = k;

  /* inflate that block type */
  // PRINTF( "\ninflateBlock type=%u isLast=%d", blocktype, lastBlock );
  E_RESULT r;

  // Jumptable ends up in right segment ??
   switch (blocktype)
   {
   case 0:
      r = inflateStored(); break;
   case 1:
      r = inflateFixed(); break;
   case 2:
      r = inflateDynamic(); break;
   default:
      return E_INVALID_COMPRESSION_OTHER;
  }


  if (r!=E_OK) return r;
  else return (lastBlock ? E_LAST_BLOCK : E_OK);
}

E_RESULT
ZipContext::inflateStored()
/* "decompress" an inflated type 0 (stored) block. */
{

  /* make local copies of globals */
  register u32 b = bb;             // initialize bit buffer
  register u32 k = bk;
  u32 w = wp;                       // initialize window position

  /* bytes in block, go to byte boundary */
  u32 n = k & 7;
  DUMPBITS(n);

  /* get the length and its complement */
  NEEDBITS(16)
  n = ((u32)b & 0xffff);
  DUMPBITS(16)
  NEEDBITS(16)
  if (n != (u32)((~b) & 0xffff)) {
      INTERCEPT;
      return E_DATA_ERROR;    // error in compressed data
  }


  DUMPBITS(16)

  // PRINTF( "\ninflateStored length=%u", n );

  // read and output the compressed data
  while (n--)
  {
    NEEDBITS(8)
    decodedByte(w, (u8) b);
    DUMPBITS(8)
  }


  /* restore the globals from the locals */
  wp = w;                       /* restore global window pointer */
  bb = b;                       /* restore global bit buffer */
  bk = k;

  return E_OK;
}

/* copy lengths for literal codes 257..285 */
static _CONST_ u16 cplens[] __attribute__((aligned(8))) = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
    /* note: see note #13 above about the 258 in this list. */

static _CONST_ u16 mask_bits[] __attribute__((aligned(8))) = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

/* extra bits for literal codes 257..285 */
static _CONST_ u8 cplext[] __attribute__((aligned(8))) = {         
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */

/* copy offsets for distance codes 0..29 */
static _CONST_ u16 cpdist[] __attribute__((aligned(8))) = {         
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577
};

 /* extra bits for distance codes */
static _CONST_ u8 cpdext[] __attribute__((aligned(8))) = { 
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13};

E_RESULT
ZipContext::inflateDynamic()
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
   static _CONST_ u8 border[] = {    /* Order of the bit length code lengths */
      16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

  int i;                /* temporary variables */

  u32 l;                /* last length */
  HuffmanTree* tl;      /* literal/length code table */
  HuffmanTree* td;      /* distance code table */
  int bd;               /* lookup bits for td */
#ifdef PKZIP_BUG_WORKAROUND
  u8 ll[288+32];  /* literal/length and distance code lengths */
#else
  u8 ll[286+30];  /* literal/length and distance code lengths */
#endif

  /* make local bit buffer */
  register u32 b = bb;
  register u32 k = bk;

  /* read in table lengths */
  NEEDBITS(5)
  u32 nl = 257 + ((u32)b & 0x1f);      /* number of literal/length codes */
  DUMPBITS(5)
  NEEDBITS(5)
  u32 nd = 1 + ((u32)b & 0x1f);        /* number of distance codes */
  DUMPBITS(5)
  NEEDBITS(4)
  u32 nb = 4 + ((u32)b & 0xf);         /* number of bit length codes */
  DUMPBITS(4)
#ifdef PKZIP_BUG_WORKAROUND
  if (nl > 288 || nd > 32) {
#else
  if (nl > 286 || nd > 30) {
#endif
      INTERCEPT;
      return E_DATA_ERROR;   /* bad lengths */
  }

  // PRINTF( "\ninflateDynamic nl=%u nd=%u nb=%u", nl, nd, nb );

  /* read in bit-length-code lengths */
  u32 j=0;
  for (; j < nb; j++)
  {
    NEEDBITS(3)
    ll[border[j]] = (u32)b & 7;
    DUMPBITS(3)
  }
  for (; j < 19; j++)
    ll[border[j]] = 0;

  // build decoding table for trees--single level, 7 bit lookup
  s32 bl = 7;
  TreeParams params( ll, 19, 19, 0, 0 );

  if ((i=HuffmanTree::build(*this,params,&tl,bl))!=0) {
      // if (i==1) tl->free(*this);
      INTERCEPT;
      return E_DATA_ERROR;
  }

  // read in literal and distance code lengths
  u32 n = nl + nd;
  u32 m = mask_bits[bl];
  i = l = 0;
  while ((u32)i < n)
  {
    NEEDBITS((u32)bl)
    /* u32 */ j = (td = tl + ((u32)b & m))->getB();
    DUMPBITS(j)
    j = td->getN();
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      NEEDBITS(2)
      j = 3 + ((u32)b & 3);
      DUMPBITS(2)
      if ((u32)i + j > n) {
         INTERCEPT;
         return E_DATA_ERROR;
      }
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      NEEDBITS(3)
      j = 3 + ((u32)b & 7);
      DUMPBITS(3)
      if ((u32)i + j > n) {
         INTERCEPT;
         return E_DATA_ERROR;
      }
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      NEEDBITS(7)
      j = 11 + ((u32)b & 0x7f);
      DUMPBITS(7)
      if ((u32)i + j > n) {
         INTERCEPT;
         return E_DATA_ERROR;
      }
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }

  // free decoding table for trees
  // tl->free( *this );

  /* restore the global bit buffer */
  bb = b;
  bk = k;

  /* build the decoding tables for literal/length and distance codes */
  bl = LBITS;

  // params.b already ok
  params.n = nl;
  params.s = 257;
  params.d = cplens;
  params.e = cplext;

  if ((i=HuffmanTree::build(*this,params,&tl,bl))!=0) {
      // if (i==1) tl->free(*this);    // incomplete literal tree
      INTERCEPT;
      return E_DATA_ERROR;
  }

  bd = DBITS;

  params.b = ll+nl;
  params.n = nd;
  params.s = 0;
  params.d = cpdist;
  params.e = cpdext;

  if ((i=HuffmanTree::build(*this,params,&td,bd))!=0) {
    if (i == 1) {
      // error(" incomplete distance tree\n");
#ifdef PKZIP_BUG_WORKAROUND
      i = 0;
    }
#else
      // td->free(*this);

    }
    // tl->free(*this);
    INTERCEPT;
    return E_DATA_ERROR;      // incomplete code set
#endif
  }

  // decompress until an end-of-block code
  E_RESULT r = inflateCodes(tl, td, bl, bd);

  // free the decoding tables, return
  // tl->free(*this);
  // td->free(*this);

  return r;
}

E_RESULT
ZipContext::inflateFixed()
{
   // PRINTF( "\ninflateFixed" );

/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
  int i;                /* temporary variable */

  u8 l[288];      		/* length list for huft_build */

  /* set up literal table */
  for (i = 0; i < 144; i++)
    l[i] = 8;
  for (; i < 256; i++)
    l[i] = 9;
  for (; i < 280; i++)
    l[i] = 7;
  for (; i < 288; i++)          /* make a complete, but wrong code set */
    l[i] = 8;

  s32 bl = 7;
  HuffmanTree* tl=0;
  TreeParams params( l, 288, 257, cplens, cplext );

  if ((i=HuffmanTree::build( *this, params, &tl, bl))!=0) {
      // i=3 -> out of memory
      INTERCEPT;
      return (i==3) ? E_NOMEM : E_DATA_ERROR;
  }

  // set up distance table
  for (i = 0; i < 30; i++)      // make an incomplete code set
    l[i] = 5;

  s32 bd = 5;
  params.n = 30;
  params.s = 0;
  params.d = cpdist;
  params.e = cpdext;

  HuffmanTree* td=0;

  if ((i=HuffmanTree::build( *this, params, &td, bd )) > 1) {
    // tl->free(*this);
    INTERCEPT;
    return (i==3) ? E_NOMEM : E_DATA_ERROR;
  }

  // decompress until an end-of-block code
  E_RESULT r=inflateCodes(tl, td, bl, bd);

  // free the decoding tables, return
  //tl->free( *this );
  //td->free( *this );
  
  // JvB: Free all at once
  curmemptr = treemem;
  return r;
}


E_RESULT
ZipContext::inflateCodes( HuffmanTree* tl, HuffmanTree* td, s32 bl, s32 bd)
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
   // PRINTF( "\ninflateCodes" );

  /* make local copies of globals */
  register u32 b = bb;              /* initialize bit buffer */
  register u32 k = bk;

  u32 w = wp;                       /* initialize window position */

  /* inflate the coded data */
  u32 ml = mask_bits[bl];           /* precompute masks for speed */
  u32 md = mask_bits[bd];

  for (;;)                      /* do until end of block */

  {
    HuffmanTree* t;             // pointer to table entry
    register u32 e;

    NEEDBITS((u32)bl)
    if ((e = (t = tl + ((u32)b & ml))->getE()) > 16)
      do {
        if (e == 99) {
          INTERCEPT;
          return E_DATA_ERROR;
        }
        DUMPBITS(t->b)
        e -= 16;
        NEEDBITS(e)
      } while ((e = (t = t->getT() + ((u32)b & mask_bits[e]))->getE()) > 16);
    DUMPBITS(t->b)
    if (e == 16)                /* then it's a literal */

    {
      decodedByte(w, (u8)t->getN());
    }
    else                        /* it's an EOB or a length */
    {
      /* exit if end of block */
      if (e == 15)
        break;

      /* get length of block to copy */
      NEEDBITS(e)

      u32 n = t->getN() + ((u32)b & mask_bits[e]);
      DUMPBITS(e);

      /* decode distance of block to copy */
      NEEDBITS((u32)bd)
      if ((e = (t = td + ((u32)b & md))->getE()) > 16)
        do {
          if (e == 99) {
            INTERCEPT;
            return E_DATA_ERROR;
          }
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)

        } while ((e = (t = t->getT() + ((u32)b & mask_bits[e]))->getE()) > 16);
      DUMPBITS(t->b)
      NEEDBITS(e)
      u32 d = w - t->getN() - ((u32)b & mask_bits[e]);
      DUMPBITS(e)
      // Tracevv((stderr,"\\[%d,%d]", w-d, n));

      // PRINTF( "\ninflateCodes length=%u w=%u t->n=%u b=%u distance=%u", n, w, t->getN(), b, d );

      /* do the copy */
      do {
        n -= (e = (e = winbuffer.getSize() - ((d &= winbuffer.getSize()-1) > w ? d : w)) > n ? n : e);
#if !defined(NOMEMCPY) && !defined(DEBUG)
        if (w - d >= e)         /* (this test assumes unsigned comparison) */
        {
          memcpy(winbuffer.getData() + w, winbuffer.getData() + d, e);

          w += e;
          d += e;
        }
        else                      /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
          do {
            winbuffer.getData()[w++] = winbuffer.getData()[d++];
          } while (--e);
        if (w == winbuffer.getSize())
        {
          flushWindow(w);
          w = 0;
        }
      } while (n);

    }
  }

  // restore the globals from the locals
  wp = w;
  bb = b;
  bk = k;

  return E_OK;
}

}}	// namespace
