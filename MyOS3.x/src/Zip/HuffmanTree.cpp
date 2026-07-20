#include "HuffmanTree.h"
#include "ZipContext.h"
#include "string.h"
#include "mem.h"

namespace MyOS { namespace Zip {

/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */

#define DEBG(X)
#define DEBG1(X)

/**
 * NOTE: This routine requires a large stack! (0x5cc bytes)
 * Perhaps I can split it up ??
 */
int
HuffmanTree::build( ZipContext& context, TreeParams& params, 
                   HuffmanTree** result, s32& maxlookupbits  )
{
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory. */
{
  // JvB: This needs a huge stack, can it be less ??
  u32 c[BMAX+1];           /* bit length count table */
  register u32 j;          /* counter */
  HuffmanTree* u[BMAX];    /* table stack */
  u32 v[N_MAX];            /* values in order of bit length */
  u32 x[BMAX+1];           /* bit offsets, then code stack */

DEBG("huft1 ");

  /* Generate counts for each bit length */
  memset_aligned(c, 0, sizeof(c));    // (dont use! Is not in uncompressed section)
  // for (u32 i=0; i<BMAX+1; ++i) c[i]=0;

  register u8* p1 = params.b;
  register u32 i = params.n;
  do {
    // Tracecv(*p1, (stderr, (n-i >= ' ' && n-i <= '~' ? "%c %d\n" : "0x%x %d\n"),
	//    n-i, *p1));
    c[*p1]++;                    /* assume all entries <= BMAX */
    p1++;                      /* Can't combine with above line (Solaris bug) */
  } while (--i);
  if (c[0] == params.n)                /* null input--all zero length codes */
  {
    result=0;
    maxlookupbits = 0;
    return 0;
  }

DEBG("huft2 ");

  /* Find minimum and maximum length, bound *m by those */
  s32 l = maxlookupbits;
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  s32 k=j;      // minimum code length
  if ((u32)l < j)
    l = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;


  s32 g = i;                        /* maximum code length */
  if ((u32)l > i)
    l = i;
  maxlookupbits = l;

DEBG("huft3 ");

  /* Adjust last length count to fill out codes, if needed */
   u32 y;
   for (y = 1 << j; j < i; j++, y <<= 1)
      if ((y -= c[j]) < 0)
         return 2;                 /* bad input: more codes than bits */

   if ((y -= c[i]) < 0) return 2;
   c[i] += y;

DEBG("huft4 ");

  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  register u32* p2 = c + 1;

/*
  u32* xp = x + 2;
  while (--i) {                 // note that i == g from above
    *xp++ = (j += *p2++);
  }
*/
  for( u32 xi=2; --i; ++xi) x[xi] = (j += *p2++);
  

DEBG("huft5 ");

  /* Make a table of values in order of bit lengths */
  p1 = params.b;  
  i = 0;
  do {
    if ((j = *p1++) != 0)
      v[x[j]++] = i;
  } while (++i < params.n);

DEBG("h6 ");

  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p2 = v;                        /* grab values in bit order */
  register s32 w = -l;          /* bits decoded == (l * h) */
  u[0] = 0;                     /* just to keep compilers happy */
  register HuffmanTree* q = 0; 
  u32 z = 0;                    /* ditto */
DEBG("h6a ");

  HuffmanTree r;
  s32 h=-1;     /* no tables yet--level -1 */

  /* go through the bit lengths (k already is bits in shortest code) */  
  for (; k <= g; k++)
  {
DEBG("h6b ");
    u32 a = c[k];
    while (a--)
    {

DEBG("h6b1 ");
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l)
      {
DEBG1("1 ");
        h++;
        w += l;                 /* previous table always l bits */

        /* compute minimum size table less than or equal to l bits */
        z = (z = g - w) > (u32)l ? l : z;  /* upper limit on table size */
        u32 f;
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
DEBG1("2 ");
          f -= a + 1;           /* deduct codes from patterns left */
          u32* xp = c + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
DEBG1("3 ");
        z = 1 << j;             /* table entries for j-bit table */

        /* allocate and link in new table */
        if ((q = context.allocate( z + 1 )) == 0)
        {
          // if (h) u[0]->free( context );
          return 3; // E_NOMEM;       /* not enough memory */
        }
DEBG1("4 ");
        // hufts += z + 1;         /* track memory usage */
        *result = q + 1;             /* link to list for huft_free() */
        *(result=&q->v.t) = 0; 
        u[h] = ++q;             /* table starts after link */

DEBG1("5 ");
        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.b = (u8)l;         /* bits to dump before this table */
          r.e = (u8)(16 + j);  /* bits in this table */
          r.v.t = q;            /* pointer to this table */
          j = i >> (w - l);     /* (get around Turbo C bug) */
          u[h-1][j] = r;        /* connect to last table */
        }
DEBG1("6 ");
      }
DEBG("h6c ");

      /* set up table entry in r */


      r.b = (u8)(k - w);
      if (p2 >= v + params.n)
        r.e = 99;               /* out of values--invalid code */
      else if (*p2 < params.s)
      {
        r.e = (u8)(*p2 < 256 ? 16 : 15);  /* 256 is end-of-block code */
        r.v.n = (u16)(*p2);               /* simple code is just the value */
	     p2++;                             /* one compiler does not like *p++ */
      }
      else
      {
        r.e = (u8)params.e[*p2 - params.s];   /* non-simple--look up in lists */
        r.v.n = params.d[*p2++ - params.s];
      }
DEBG("h6d ");

      /* fill code-like entries with r */
      u32 f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      while ((i & ((1 << w) - 1)) != x[h])
      {
        h--;                    /* don't need to update q */
        w -= l;
      }
DEBG("h6e ");
    }
DEBG("h6f ");
  }

DEBG("huft7 ");

  /* Check if we were given an incomplete table */
return (y != 0 && g != 1);
}

}

/*
void
HuffmanTree::free( ZipContext& ctxt )
// Free the malloc'ed tables built by huft_build(), which makes a linked
//   list of the tables it made, with the links in a dummy first entry of
//   each table.
{
  // Go through linked list, freeing from the malloced (t[-1]) address.
  register HuffmanTree *p = this;
  while (p)
  {
    register HuffmanTree *q = (--p)->v.t;
    ctxt.free(p);
    p = q;
  } 
}
*/


}}	// namespace
