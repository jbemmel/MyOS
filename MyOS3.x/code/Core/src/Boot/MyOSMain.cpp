#include "defs.h"
 
#include "Nucleo/MyOSCoreContainer.h"
#include "BootContext.h"

// #include "Devices/CMOS/CMOS.h"

#include "debug.h"

// linker constants, .bss is page-aligned
extern u8 _BSS_START_;
extern u32 _BSS_SIZE_; // beware: must take the *address*

/**
 * Extra pages allocated by DoDecompress, stored *after* changing CR3
 */
int extraPagesAllocated = 0;

namespace MyOS { namespace Init {

// To avoid putting it on the stack?
// static u8 coreMemory[ sizeof(MyOSCoreContainer) ];

void MyOSMain() ASMNAME( "_MyOSmain" ) INITSECTION;

/**
 * This routine gets called after decompression (DoDecompress.cpp) and some
 * initial page table setup
 * 
 * Machine state:
 * CS  = 0x8 (Code seg)
 * DS = ES = SS = 0x10 (Data seg)
 * FS = GS = 0
 * ESP = 0xff82ff5c (1 page stack, at the bottom there is room for thread struct
 * CR0 = 0x80010011
 * 
 * EDX = #pages loaded for module
 */
void MyOSMain()
{
    // First thing: store module pagecount
    int modulePages = Processor::EDX();
    
    // Should not get too big for stack...
    // ASSERTION( sizeof(MyOSCoreContainer) < 0x1000, E_ERROR );

    /// first of all, make sure to clear .bss (using linker constants)
    /// Note: This also clears the exception frame registration of this func! 
    // Could do this in DoDecompress, before changing virtual mapping
    memset_aligned( &_BSS_START_, 0, (u32) &_BSS_SIZE_ );
    
    // After(!) clearing BSS, save it
    extraPagesAllocated = modulePages;
    
    static MyOSCoreContainer core;
    static BootContext bootcontext( core );
    
    // MyOSCoreContainer& core = * new (coreMemory) MyOSCoreContainer();

   // bootstrap, 2 phases:
   // Phase 1: Before MM and Repository are initialized, components register 
   //          with the bootcontext
   // Phase 2: Proper context is created (using dynamic malloc)
   //          Existing registrations are copied to the new context 
   //          (@see IContextImpl::init)
   //          All subsequent components are initialized in the new context
   {
      // BootContext bootcontext( core );      
      myos_result_t r = core.initialize( bootcontext, 0 );
      
      // for test: NMI are disabled upon startup!
      // volatile Devices::CMOS cmos;
      
      DPRINTK( "\nEndless loop, result was %x", r /* cmos.isNMIEnabled() */ );
      // DPRINTK( " size(CORE)=%d size(Thread)=%d", sizeof(MyOSCoreContainer), sizeof(MultiThreading::Thread) );
   }  // this calls ~bootcontext

/*
   {
      IInterface* mm;
      if (bootcontext.lookup(UUID("2a147d6f-cab0-43b8-8b9f-4687312c101d"),mm)==E_MYOS_SUCCESS) {
         MyOS::Devices::LPT::LPTStream printer;
         mm->castTo<MM::IVirtualMemory>()->listTables( printer );
         mm->castTo<MM::IVirtualMemory>()->listPools( printer );
      } else DPRINTK( "IVirtualMem not found!" );
   }
*/  
   // Let this thread become the idle thread, no return from here
   // Use large interval (1 sec) for debugging
   MultiThreading::Thread::becomeIdle( 1000 );
}

}} // namespaces
