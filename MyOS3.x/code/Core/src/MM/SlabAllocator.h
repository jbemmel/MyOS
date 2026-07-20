/*
 * SlabAllocator.h
 *
 *  Created on: 7-okt-2008
 *      Author: Jeroen
 */

#ifndef SLABALLOCATOR_H_
#define SLABALLOCATOR_H_

#include "types.h"
#include "string.h" // memset_aligned

namespace MyOS {
namespace MM {

class IVirtualMemory;

/**
 * A generic O(1) concurrent memory allocator which returns aligned blocks of up to 4000 bytes size
 *
 * @note 'False cache line sharing' occurs when 2 threads allocate memory blocks that belong to
 * the same line of cache. This can decrease SMP performance. One solution is to use a SlabAllocator
 * per thread (or BlockAllocators per thread), it is also possible to align on cache lines.
 * Currently not addressed
 *
 */
class EXPORT SlabAllocator
{
    struct BlockAllocator;

    BlockAllocator* allocators[ 32 ];   // Blocks of 4,8,16..128 bytes
    s32 counters[ 32 ];                 //< Counters for parallel threads, needed for safe free

    IVirtualMemory &mem;

    BlockAllocator* newAllocator( BlockAllocator &next );

public:
    /**
     * Constructor - initializes a new SlabAllocator
     */
    SlabAllocator( IVirtualMemory &m ); // temporary, todo PageAllocator global/private

    /**
     * Allocates [bytes] of memory
     *
     * @return aligned memory block ( 2^n aligned if log2(bytes) == n, else at least 4-byte aligned )
     */
    void* malloc( size_t bytes );

    /**
     * Allocates [bytes] of memory, and initializes this to 0
     *
     * @return zeroed aligned memory block
     */
    inline void* calloc( size_t bytes ) {
        void *m = this->malloc( bytes );
        memset_aligned( m, 0, bytes );
        return m;
    }

    /**
     * Frees a memory block previously allocated through an instance of SlabAllocator
     */
    static void free( void* mem );
};

}}

#endif /* SLABALLOCATOR_H_ */
