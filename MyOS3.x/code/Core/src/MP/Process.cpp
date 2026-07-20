//#include "Init/MyOSCoreContainerDefs.h"
// #ifdef MP_IN_CORE

#include "Process.h"
#include "MPComponent.h"

#include "IThread.h"    // currentThread
#include "MM/PBT.h"           // PBT
#include "MM/GDT.h"

#include "new.h"  // placement new
#include "debug.h"

namespace MyOS { namespace MultiProcessing {

using namespace MM;

struct PerCPU cpuState[ MAX_CPUs ];

volatile u32 runningCPUs = 1;   // BSP is always running

void*
Process::operator new( size_t ) throw (MM::OutOfMemoryException)
{
   physadr_t adr;
   void *m = MPComponent::getInstance().iVirtualMemory->allocPage( adr );
   // This should have been clear already! (but it usually isn't)
   // memset_aligned( m, 0, _4KB );
   return m;
}

void
Process::operator delete( void* p ) {
   MPComponent::getInstance().iVirtualMemory->freePage(p);
}

// static
void
Process::initOnce()     // INITSECTION
{
   cpuState[ getCurrentCPU() ].currentProcess = new Process();
}

Process::Process()   // INITSECTION, used for P(0)
: mainThread( Thread::currentThread() )
, parent(0), nextTLSindex(0)
{
   pbt = &PBT::getCurrentPBT();
   tss.initOnce();

   mainThread.homeContext = this;
   mainThread.visitedContext = this;
}

Process::Process( IRunnable& runnable, NVPAIR params[], size_t paramCount, MM::EDPL rpl, u32 allocOrder )
: mainThread( (Thread&) MPComponent::getInstance().iMultiThreading->createThread(
                runnable,params,paramCount,rpl,allocOrder,false) )
, parent( &getCurrent() ), nextTLSindex(0)
{
   // DPRINTK( "\nCreating new process at %x", this );
   // Moved to Thread::init (set to curProcess)
   // mainThread.homeContext = mainThread.visitedContext = this;

   // allocate a page for the PBT (this copies the appropriate parent process entries)
   // Since the PBT is allocated from kernel memory, this address is valid for any process
   physadr_t m;
   pbt = new ( MPComponent::getInstance().iVirtualMemory->allocPage(m) ) PBT(m);

   // NOTE: MainThread is created while still in context of *PARENT* process !!
   // CANNOT DO THIS: Need parameters on stack of new thread, in start()
   // tss.initialize( pbt, mainThread );

   // add yourself to parent process' linked list (at head)
   next = parent->subprocesses;
   parent->subprocesses = this;

   mainThread.homeContext = this;
   mainThread.visitedContext = this;

   tss.initialize( m, mainThread );     // uses mainThread.ESP
   mainThread.start();
}

/**
 * This method is called in the context of another process
 * when the main thread is (automatically) destroyed
 */
Process::~Process()
{
   if (parent && pbt) {    // free PBT, not for P0
      MPComponent::getInstance().iVirtualMemory->freePage( pbt );
      pbt = 0;
   }
}

void
Process::removeThread( Thread& t )
{
   // perhaps maintain counter, and delete iff count==0 ?
   if (&t == &mainThread) {
      DPRINTK( "\nMainThread destroyed, deleting process..." );
      delete this;
   }
}

u32
Process::getNextTLSIndex( u32 varSize, u32 align )
{
    u32 old,i;
    do {
        i = nextTLSindex;
        old = i;
        if ( i & (align-1) ) {
            i = (i | (align-1)) + 1;
        }
        i += varSize;
    } while ( cmpxchg( &nextTLSindex, old, i ) != old );

    return i - varSize;
}

}} // namespace

// #endif   // MP_IN_CORE
