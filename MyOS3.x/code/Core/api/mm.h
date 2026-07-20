#ifndef MYOS_MM_H
#define MYOS_MM_H

namespace MyOS {

/**
 * Namespace for Memory Management (MM) functionality
 *
 * All memory management takes place in the context of the currently active
 * process / address space
 */
namespace MM {

enum E_MALLOC_FLAGS {
    E_NONE          = 0,
    E_NON_PAGED     = 1,
    E_CONTIGUOUS    = 2,
    E_EXECUTABLE    = 4,    //< Uses a particular address range
    E_STACK         = 8,    //< Adds a guard page
};

MYOS_API void* malloc( size_t bytes, unsigned flags = E_NONE );
MYOS_API void free( void* mem );

enum E_KMALLOC_FLAGS {

    // All the flags from E_MALLOC_FLAGS, plus:

    E_KM_DMA        = 0x100,
    E_KM_ISA_DMA    = 0x200,    //< Like E_KM_DMA, but in first 16Mb physical
};

/**
 * Allocates shared kernel memory (in the address range 0xff800000 - 0xffc00000)
 */
MYOS_API void* kmalloc( size_t bytes, unsigned flags = E_NONE );
MYOS_API void kfree( void* mem );

/**
 * Namespace to indicate special scoped malloc, lifetime of returned memory
 * equals lifetime of current thread
 */
namespace CurrentThread {
    MYOS_API void* malloc( size_t bytes, unsigned flags = E_NONE );
    MYOS_API void free( void* mem );
}


}}

#endif
