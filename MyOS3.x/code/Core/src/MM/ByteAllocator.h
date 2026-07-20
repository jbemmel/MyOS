/**
 * TODO: Add alignment mask parameter
 */
#ifndef BYTEALLOCATOR_H
#define BYTEALLOCATOR_H

#include "defs.h"   // size_t
#include "types.h"
#include "memtypes.h"
#include "Atomic/atomic32.h"

#include "debug.h"

namespace MyOS {
namespace MM {

class IVirtualMemory;

/**
 * Allocates (GLOBAL) blocks of bytes from pages
 * - blocks dont have to be the same size
 */
class EXPORT ByteAllocator
{
   // pteflags_t attributes;   /// TODO: Attributes to use when allocating pages
	atomic32 currentAddress;    /// The current address within the current page

	/// Linked list of pages allocated by this allocator
    struct PAGELINK
    {
        // double-linked, such that deallocating some block in between can
        // automatically take it from in between the linked list
        PAGELINK *prev,*next;

        // init to sizeof(PAGELINK), max = sizeof(Page) - sizeof(PAGELINK)
        atomic32 bytesfreed;

        // dont use constructor, in case '0' is returned when 'nothrow' is set
        inline PAGELINK* init( PAGELINK *p )
        {
            next = 0;
            prev = p;
            (u32&) bytesfreed = sizeof(PAGELINK);
            if (p) p->next = this;
            return this;
        }

        // Called before de-allocating the page
        inline void unlink()
        {
            if (prev) prev->next = next;
            if (next) next->prev = prev;
        }

        // Returns address to start allocating from
        inline u32 reuse()
        {
            ASSERTION( bytesfreed <= 0x2000, E_ERROR );
            memset_aligned( this+1, 0, bytesfreed - sizeof(PAGELINK) );
            (u32&) bytesfreed = sizeof(PAGELINK);
            return (u32) (this+1);
        }

    } *currentBlock;  /// head of linked list of chunks

    /* Convert an address to the linknode of that page
    inline PAGELINK* getLink( linadr_t adr ) const {
        // PAGELINK is the first 12 bytes in a page
        return (PAGELINK*) ((u32)adr&0xFFFFF000);
    }
    */

   IVirtualMemory* memory; // interface to use, set by init()
   u32 blockOrder;         // Order of blocks to allocate/free each time

public:
   inline explicit ByteAllocator() throw() : currentBlock(0), memory(0) {}

   // It is not always possible to call this constructor
   // If not, call 'init' instead
   inline ByteAllocator( IVirtualMemory& memif, u32 blockorder ) throw()
   : currentBlock(0), memory(&memif), blockOrder(blockorder+12)
   {
   }

   inline void init( IVirtualMemory& memif, u32 blockorder ) {
      this->memory = &memif;
      this->blockOrder = blockorder+12;   // keep it in byte units!
   }

   ~ByteAllocator() throw();

   inline size_t maxBlockSize() { return (1<<blockOrder) - sizeof(PAGELINK); }

    /**
     * 	Allocates <BLOCKSIZE> bytes of memory, dword-aligned
     */
    void* allocate( size_t BLOCKSIZE ) throw (MM::OutOfMemoryException) MALLOC_FUNCTION;
    void deallocate( linadr_t block, size_t BLOCKSIZE ) throw();

   // Only used by Exception, rare case
   void* allocateNoThrow( size_t BLOCKSIZE ) throw () MALLOC_FUNCTION;

   // for debugging
   void print() const;

protected:

    // Warning: fix this if blockallocator is ever dynamically allocated
    void* operator new( size_t );
    void operator delete( void* ) {}

};

/// A specialized BlockAllocater derivative that stores the size in the
/// allocated memory (a la 'new')
class EXPORT Allocator : public ByteAllocator
{
#ifdef DEBUG
   size_t allocated;
#endif

public:
   explicit inline Allocator( ) : ByteAllocator() {}

   using ByteAllocator::print;

   inline Allocator( IVirtualMemory& memif, u32 blockorder )
       : ByteAllocator(memif,blockorder) {}

#ifdef DEBUG
   // inline ~Allocator() { ASSERTION( allocated==0, Debug::AS_FATAL ); }
#endif

   inline void* allocateAutoSize( size_t BLOCKSIZE )
      throw (MM::OutOfMemoryException) MALLOC_FUNCTION
   {
      BLOCKSIZE += sizeof(size_t);
      size_t* mem = (size_t*) ByteAllocator::allocate( BLOCKSIZE );
      *mem = BLOCKSIZE;
#ifdef DEBUG
      allocated += BLOCKSIZE;
#endif
      return mem+1;
   }

   inline void* allocateAutoSizeNoThrow( size_t BLOCKSIZE )
      throw () MALLOC_FUNCTION
   {
      BLOCKSIZE += sizeof(size_t);
      size_t* mem = (size_t*) ByteAllocator::allocateNoThrow( BLOCKSIZE );
      if (mem) {
         *mem = BLOCKSIZE;
#ifdef DEBUG
         allocated += BLOCKSIZE;
#endif
         return mem+1;
      } else return 0;  // nothrow
   }


   inline void freeAutoSize( linadr_t mem ) throw() {
      size_t* size = &((size_t*)mem)[-1];
#ifdef DEBUG
      allocated -= *size;
#endif
      ByteAllocator::deallocate( size, *size );
   }
};

}}  // namespace

#endif
