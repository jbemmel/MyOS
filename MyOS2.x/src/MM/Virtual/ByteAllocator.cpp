#include "ByteAllocator.h"
#include "MM/MMComponent.h"
#include "string.h" // memset

#include "debug.h"

namespace MyOS {
namespace MM {

ByteAllocator::~ByteAllocator()
{
   // DPRINTK( "\n~ByteAllocator this=%x curBlock=%x", this, currentBlock ); 
 
   /// Delete all remaining pages
   PAGELINK *p;
   while ((p = currentBlock)) {
      currentBlock = p->prev;
      memory->free( p, blockOrder-12 );
   }
}

/// There are still some RACE conditions here: memset(0) || next allocation ...
void*
ByteAllocator::allocate( size_t BLOCKSIZE ) throw (OutOfMemoryException)
{
   ASSERTION( BLOCKSIZE <= this->maxBlockSize(), E_ERROR );
   ASSERTION( memory, E_ERROR ); // call init() first...
   CHECKPOINT('A'); 
 
   // Don't call from IRQ context! Might throw exception which eventually
   // kills the 'current' thread, but we don't know which one that is!
   //
   // Actually, it *can* be called in an IRQ context, but only if caller
   // makes sure to use 'try'...'catch' at one point!
   // NOT_IN_IRQ;
 
   u32 nextAdr,blockbytes;      // jump crosses initialization. so declare it here
#ifdef DEBUG
	BLOCKSIZE += 4;	// make room for magic marker
#endif

	/// Make sure it is dword aligned (and free skipped (misaligned) bytes in deallocate())
	if (u32 misaligned=BLOCKSIZE&3) BLOCKSIZE += 4-misaligned;

	/// At first use, curpage == 0
	if (currentBlock==0) goto allocnew;	

	nextAdr = (currentAddress+=BLOCKSIZE);
	blockbytes = (1<<blockOrder);   
	if ( nextAdr > (((u32)currentBlock) + blockbytes)) {

		// make sure those fragmented bytes get freed
		if ( (u32)(currentBlock->bytesfreed += ((u32)currentBlock + blockbytes - (nextAdr-BLOCKSIZE))) == blockbytes ) {
			// reuse the page, keep prev&next
			currentBlock->reuse();
		} else {
allocnew:
         void* newBlock = memory->allocate( blockOrder-12 ); 
         currentBlock = ((PAGELINK*)newBlock)->init(currentBlock);
		}
		(u32&) currentAddress = ((u32) currentBlock) + BLOCKSIZE + sizeof(PAGELINK);
	}

#ifdef DEBUG
	((u32*) (currentAddress-BLOCKSIZE))[0] = 0xabbccdde;
	BLOCKSIZE -= 4;
#endif
	// Screen::cursor() << "returning " << (void*) (currentAddress-BLOCKSIZE);
	// DPRINTP( "\nAllocate %d(@%x) returns %x freed=%d", BLOCKSIZE, this, (currentAddress-BLOCKSIZE), (u32) currentBlock->bytesfreed );
	return (void*) (currentAddress-BLOCKSIZE);
}

// XX Duplicate code, only 'allocateNoThrow' differs...
void*
ByteAllocator::allocateNoThrow( size_t BLOCKSIZE ) throw ()
{
   u32 nextAdr,blockbytes;      // jump crosses initialization. so declare it here
#ifdef DEBUG
   BLOCKSIZE += 4;   // make room for magic marker
#endif

   /// Make sure it is dword aligned (and free skipped (misaligned) bytes in deallocate())
   if (u32 misaligned=BLOCKSIZE&3) BLOCKSIZE += 4-misaligned;

   /// At first use, curpage == 0
   if (currentBlock==0) goto allocnew; 

   nextAdr = (currentAddress+=BLOCKSIZE);
   blockbytes = (1<<blockOrder);   
   if ( nextAdr > (((u32)currentBlock) + blockbytes)) {

      // make sure those fragmented bytes get freed
     if ( (u32)(currentBlock->bytesfreed += ((u32)currentBlock + blockbytes - (nextAdr-BLOCKSIZE))) == blockbytes ) {
         // reuse the page, keep prev&next
         currentBlock->reuse();
      } else {
allocnew:
         ASSERTION( memory, E_CRITICAL );
         void* newBlock = memory->allocateNoThrow( blockOrder-12 );
         currentBlock = ((PAGELINK*)newBlock)->init(currentBlock);
     }
    (u32&) currentAddress = ((u32) currentBlock) + BLOCKSIZE + sizeof(PAGELINK);
 }

#ifdef DEBUG
   ((u32*) (currentAddress-BLOCKSIZE))[0] = 0xabbccdde;
   BLOCKSIZE -= 4;
#endif
//   DPRINTK( "\nAllocateNoThrow returns %x", (currentAddress-BLOCKSIZE) );
   return (void*) (currentAddress-BLOCKSIZE);
}

void
ByteAllocator::deallocate( linadr_t block, size_t BLOCKSIZE )
{
	// DPRINTP( "\nByteAllocator::deallocate block=%x size=%d", block, BLOCKSIZE );
	CHECKPOINT('D'); 
 
	// TRACE_MEM( block );
#ifdef DEBUG
	BLOCKSIZE += 4;
	(u32&) block -= 4;
	ASSERTIONv( ((u32*)block)[0] == 0xabbccdde, E_CRITICAL, block );
	((u32*)block)[0] = 0xcddccddc;
#endif

   // correct misaligned BLOCKSIZE
   if (u32 misaligned=BLOCKSIZE&3) BLOCKSIZE += 4-misaligned;

   const u32 blockbytes = (1<<blockOrder);

   // Assuming allocated blocks are 1^order aligned...
	// PAGELINK *l = getLink(block);
   PAGELINK *l = (PAGELINK*) ((u32)block & ~(blockbytes-1));

   // Use this to detect duplicate frees
	ASSERTIONv( (u32) l->bytesfreed <= (u32) (blockbytes-BLOCKSIZE), E_CRITICAL, l->bytesfreed );

   // If either
   // 1) The 'bytesfreed' count of the page equals the total #bytes
   // 2) Free is on current page (currentAddress & 0xFFFFF000 == l) and 
   //    'bytesfreed' equals bytes allocated so far on this page 
   //    ((currentAddress&0xFFF)-sizeof(PAGELINK)) then do something
   //
   // Which of the above cases is more common, depends on allocation patterns

   // '>=' just in case
	if ( (u32) (l->bytesfreed += BLOCKSIZE) >= blockbytes ) {
      
      // rare case that l==currentBlock, could reuse but...
      l->unlink();
      memory->free( l, blockOrder-12 );
	} else if (( (currentAddress& ~(blockbytes-1) ) == (u32) currentBlock ) && 
              ((u32)l->bytesfreed == (currentAddress&(blockbytes-1)))) {
      (u32&) currentAddress = l->reuse();      
   }
}

void
ByteAllocator::print() const
{
   for (PAGELINK* p = currentBlock; p!=0; p=p->prev) {
      DPRINTK( "\nPage @%x bytesfreed=%d", p, (u32) p->bytesfreed );  
   }
}

}}  // namespace
