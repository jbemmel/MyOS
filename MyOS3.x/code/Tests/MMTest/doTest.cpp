/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause
 * one of the tests to be selected at all times
 */
#include "IContext.h"
#include "debug.h"
#include "../TestComponent.h"

#include "MM/SlabAllocator.h"

namespace MyOS { namespace Tests {

/**
 * A test for MM::SlabAllocator
 */

using MM::SlabAllocator;
using MM::IVirtualMemory;

bool doTest( IContext& context, NVPAIR params[] )
{
   PRINTK( "\nStarting MM test..." );

   IVirtualMemory *vm = context.lookup( myos_name_t(IVirtualMemory::ID())).castTo<IVirtualMemory>();
   SlabAllocator sa( *vm );

   void *block32 = sa.malloc( 32 );
   void *block18 = sa.malloc( 18 );
   PRINTK( "\nGot 32-byte block: %x 18-byte: %x", block32, block18 );
   sa.free( block32 );
   sa.free( block18 );

   return false;
}

}}
