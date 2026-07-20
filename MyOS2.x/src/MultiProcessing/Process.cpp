#include "Init/MyOSCoreContainerDefs.h"
#ifdef MP_IN_CORE

#include "Process.h"
#include "MultiProcessing/MPComponent.h"

#include "MultiThreading/IThread.h"    // currentThread
#include "MM/Physical/PBT.h"           // PBT

#include "MM/new.h"  // placement new
#include "debug.h"

namespace MyOS { namespace MultiProcessing {

using namespace MM;

// static
Process* Process::currentProcess;   // ASMNAME "curProcess"

void*
Process::operator new( size_t ) throw (OutOfMemoryException)
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
   currentProcess = new Process();   
}

Process::Process()   // INITSECTION, used for P(0)
: mainThread( Thread::currentThread() )
, parent(0)
{
   pbt = &PBT::getCurrentPBT();
   tss.initOnce();

   mainThread.homeContext = this;
   mainThread.visitedContext = this;
}

Process::Process( Process& _parent, u32 blockOrder )
: mainThread( MPComponent::getInstance().iMultiThreading->createThread(blockOrder) )
, parent( &_parent )
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

bool
Process::start( IRunnable& runner, NVPAIR params[], size_t paramCount )
{
   DPRINTK( "\nProcess::start this=%x mainThread=%x", this, &mainThread );
   
   /*
    * Dont use interface, as we still need to initialize the TSS
    * Drawback is more tight integration (could redesign interface a bit)
    * 
   MPComponent::getInstance().iMultiThreading->startThread( 
      mainThread, runner, params 
   );
   */
   mainThread.init( runner, params, paramCount );
   tss.initialize( pbt, mainThread );     // uses mainThread.ESP
   mainThread.start();
   return true;
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
  
}} // namespace

#endif   // MP_IN_CORE
