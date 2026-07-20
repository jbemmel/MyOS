/*
 * PhysAllocator.h
 *
 *  Created on: 25-sep-2008
 *      Author: Jeroen
 */

#ifndef PHYSALLOCATOR_H_
#define PHYSALLOCATOR_H_

#include "types.h"

namespace MyOS { namespace MM {

/**
 * A physical memory allocator
 *
 * @note Not SMP safe yet. Would like to be able to allocate from an interrupt context
 *       --> need spinlock
 */
class PhysAllocator
{

private:
    struct Chunk1G {
        // 1 bit for each 4K page -> 1G(2^30) / 4K(2^12) = 2^18 bits = 2^13 dwords (8 x 4K pages)
        u32 bits4k[ 1<<13 ];

        // 1 bit for each 32K region (1 x 4K page)
        u32 bits32k[ 1<<10 ];

        // 1 bit for each 4M region -> 2^30 / 2^22 = 2^8 bits = 8 dwords
        u32 bits4m[ 1 << 3 ];
    };

    /// Pointers to 1G maps, 0 when physical memory is not present
    Chunk1G* gb[4];

    u32 cache4K;    //< MRU 4K page, cached
};

}}

#endif /* PHYSALLOCATOR_H_ */
