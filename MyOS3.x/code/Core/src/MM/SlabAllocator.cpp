/*
 * SlabAllocator.cpp
 *
 *  Created on: 7-okt-2008
 *      Author: Jeroen
 */
#include "SlabAllocator.h"
#include "IVirtualMemory.h"
#include "new.h"
#include "Atomic/atomic32.h"
#include "debug.h"

namespace MyOS {
namespace MM {

/**
 * Each BlockAllocator can be initialized to issue aligned blocks of 4*N bytes.
 * 4K of memory (1 page) is used for this
 */
struct SlabAllocator::BlockAllocator {

    volatile atomic32 head; //< 16-bit head index with upper 16-bit to avoid ABA problem
    BlockAllocator *next;   //< Pointer to next allocator, in case this one is full
    BlockAllocator **prev_next;
    const u32 blocksize;    //< in dwords

    atomic32 allocated;
    atomic32* volatile counter;
    u32 unused[2];

    u32 data[ 1016 ];   //< 32-byte aligned

    /**
     * Constructor - initializes this allocator to issue blocks of N dwords
     * @param dwords - blocksize in dwords (4 bytes)
     */
    BlockAllocator( u32 dwords, BlockAllocator *n, BlockAllocator **p, atomic32* c )
        : next(n), prev_next(p), blocksize(dwords), counter(c) {

        ASSERTION( dwords>0 && dwords<1016, E_ERROR );

        u32* index = &data[0];
        u32 align = 8;  // in dwords
        while (dwords > align) {     // Make sure e.g. 32-byte blocks are aligned
            index += align;
            align *= 2;
        }

        // for non powers of 2, align at base anyway
        if ( dwords != align ) index = &data[0];

        this->head.set( (s32) index & 0xfff ); // lower 12 bits are an index into data

        // Fill in linked list of next pointers
        do {
            u32* next = index + dwords;     // skip 1 block
            *index = (u32) next;
            index = next;
        } while ( index < &data[sizeof(data)/4] );

        *(index-dwords) = 0;     // last node: end of list(0)
    }

    /**
     * Allocates an aligned block of memory from this allocator
     */
    void* allocate( SlabAllocator &s ) {

        // DPRINTK( "\nAllocate: this=%x head=%x", this, (u32) head );

        u32 h;
        u32* block;
        do {
            h = head;

            if (RARELY(h==0)) {
                if (BlockAllocator* n = next) {
                    return n->allocate( s );
                } else {
                    return s.newAllocator( *this )->allocate( s );
                }
            }

            block = (u32*) ((u32)this + (h & 0xfff));
        } while ( RARELY( head.CmpXchg_retold(h,*block) != h ) );

        *block = 0; // optional: zero next pointer
        allocated.inc();
        return block;
    }

    /**
     * Frees an aligned block of memory previously allocated via #allocate
     *
     * @param mem - allocated memory to free (size is implicit)
     */
    void free( void* mem ) {
        counter->inc();
        u32* block = (u32*) mem;
        u32 h;
        do {
            h = head;
            *block = ((u32)this+(h&0xfff));  // point block->next to the current head

        // Insert 'mem' before the current head, adding some bits to avoid ABA
        } while ( RARELY( head.CmpXchg_retold(h,(h<<16)|((u32)block&0xfff)) != h ));

        // Free this allocator when all entries are empty. All this complexity is to
        // provide safe deallocation, avoiding RACE conditions with parallel free on
        // this or next/prev allocators
        if ( RARELY(allocated.decAndTest()) ) {

            // Avoid RACE with parallel alloc+free on same allocator
            BlockAllocator **prev_n = (BlockAllocator**) ((atomic32*) &prev_next)->Replace( 0 );
            if (prev_n) {
                if ( (*prev_n = next) ) {     // next allocator? fix its prev_next pointer
                    // RACE with parallel free on _next_ allocator
                    if (RARELY( ((atomic32*) &next->prev_next)->CmpXchg_retold( (u32)&next,(u32)prev_n)==0 )) {
                        // failed, restore previous state
                        *prev_n = this;
                        prev_next = prev_n;
                        goto done;
                    }
                }

                // If only thread processing, and allocated still 0 (parallel alloc possible)
                if ( counter->decAndTest() && allocated==0 ) {
                    // @todo PageAllocator_for( this )->free( this );
                    DPRINTK( "\nTODO: Free page for BlockAllocator @%x", this );
                    return;
                } else {
                    // restore state
                    if (next) next->prev_next = &next;
                    *prev_n = this;
                    prev_next = prev_n;
                }
            }
        }
done:
        counter->dec();
    }
};

SlabAllocator::SlabAllocator( IVirtualMemory &m ) : mem(m)
{
    memset_aligned( allocators, 0, sizeof(allocators) );
    memset_aligned( counters, 0, sizeof(counters) );
}

SlabAllocator::BlockAllocator*
SlabAllocator::newAllocator( BlockAllocator &next )
{
    physadr_t pa;   // not needed
    u32 dwords = next.blocksize;
    BlockAllocator *ba = new ( mem.allocPage(pa) ) BlockAllocator( next.blocksize, &next,
            &allocators[ dwords ], (atomic32*) &counters[ dwords ] );
    return (BlockAllocator*) ((atomic32*) &allocators[ dwords ])->Replace( (u32) ba );
}

void*
SlabAllocator::malloc( size_t bytes )
{
    // allocate(0..3) -> 4
    if ( bytes < sizeof(allocators) ) {
        u32 blocksize = (bytes+3)>>2;
        ((atomic32*) &counters[ blocksize ])->inc();    // count parallel threads, per allocator
        BlockAllocator **a = &allocators[ blocksize ];
        BlockAllocator *b = *a;
        if (!b) {
            physadr_t pa;   // not needed
            b = new ( mem.allocPage(pa) ) BlockAllocator( blocksize, 0, a,
                    (atomic32*) &counters[ blocksize ] );
            BlockAllocator *other = (BlockAllocator*) ((atomic32*) a)->CmpXchg_retold( 0, (u32) b );
            if ( RARELY( other ) ) {
                mem.freePage( b );
                b = other;
            }
        }
        void* m = b->allocate( *this );
        ((atomic32*) &counters[ blocksize ])->dec();
        return m;
    } else BUG( "TODO: larger sizes" );
}

// static
void
SlabAllocator::free( void * mem )
{
    BlockAllocator* ba = (BlockAllocator*) ( (u32)mem & 0xFFFFF000 );   // convert to page

    // inc/dec counters is done in free
    ba->free( mem );
}


}}
