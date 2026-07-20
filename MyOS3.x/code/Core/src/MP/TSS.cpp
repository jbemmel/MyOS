// #include "Init/MyOSCoreContainerDefs.h"
// #ifdef MP_IN_CORE

#include "TSS.h"
#include "CPU/Processor.h"
#include "MM/GDT.h"
#include "IThread.h"    // sizeof(Thread)
#include "MPComponent.h"

extern "C" void* tss_eip;   /// ASM label defined in Process.cpp

namespace MyOS { namespace MultiProcessing {

using namespace MM;

FarPointer TSS::tssPointer;

TSS::TSS()           // all others should be '0' !!
:  TSS_backlink(0),
   EIP( &tss_eip ), EFLAGS( Processor::FLAG_INT ),  // keep ints on after switch
   ES( Processor::ES()), CS( Processor::CS() ), SS( Processor::SS() ), DS( Processor::DS() ),
   TRAP(0),
   IObase( (u32) iomap - (u32) this )     // do create an io map, for V86 mode ( bit is 0 -> allowed)
{
   // State: o.a. LDT==0
   // processes start with interrupts *enabled*

   memset_aligned( iomap, 0, sizeof(iomap) );
}

/**
 * Initialization for the first TSS
 */
void
TSS::initOnce()   // INITSECTION
{
   // No need to set any register values, as these will be set during the first taskswitch

   // do need to initialize these, for Vm86 -> PM calls in process 0
   stacks[0].SS = SS;
   // some small portion on top of default stack, may overflow...
   stacks[0].ESP = (void*) (((u32)&(Thread::currentThread())) - 1024);

   // Create a single TSS descriptor for all TSS in the GDT (and store it)
   // -> Scheduler must update during taskswitch
   SegmentDescriptor tss_desc( SEG_TSS, this, sizeof(TSS)-1, E_DPL0 );
   Selector tss_sel = GDT::addEntry( tss_desc, E_DPL0 );
   tssPointer = FarPointer( tss_sel, 0 );   // offset is ignored for jmp

   ASMVOLATILE( "ltr %%ax" : : "a" ( tss_sel ) ); // Selector: DPL0, in GDT
   // PRINTF( "\nTSS using GDT index %u", tss_sel.index() );

   // Store current CR3 value in TSS, this value is not stored by processor!
   CR3 = Processor::CR3();

   // Enable Pentium VME (also exists for PM tasks!)
   // Processor::setCR4( Processor::CR4() | 0x1 );    not supported by bochs :(
}

void
TSS::initialize( physadr_t pbtPhysical, Thread &mainthread )
{
   CR3 = pbtPhysical;
   // ESP = (void*) (&mainthread); // ->esp not yet set! changes when started, problem?

   // setup stack 0 for Vm86->PM calls
   stacks[0].SS  = SS;
   stacks[0].ESP = (void*) ((u32)(&mainthread) - 1024);   // small...

   // These are values as Thread::select expects them (after tss_eip)
   //EBX = (u32) &mainthread;                  // 'this'
   //ECX = 10000;                              // initial quantum
   //EDX = (u32) (mainthread.getESP());        // Stack being switched in
}

}} // namespace

// #endif   // MP_IN_CORE
