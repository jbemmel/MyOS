#include "Thread.h"
#include "ThreadManager.h"
#include "MultiThreading/MTComponent.h"
#include "MM/memtypes.h"   // for E_KERNEL_MEMORY flag

#include "Timer/ITiming.h"

// To finish init
#include "MM/MMComponent.h"

#ifdef MP_IN_CORE
#include "MultiProcessing/Process.h"      // removeThread
#endif

namespace MyOS { namespace MultiThreading {

using MM::linadr_t;

#define ITIMING MTComponent::getInstance().iTiming

void*
Thread::operator new( size_t, u32 /* stackorder */ )
{
   // NOTE: Currently *cannot* allocate anything but 1-page stack!
   // currentThread() depends on the fact that ESP is in same page!
   
   // Allocate 2^N linear frames, 1 not mapped and 2^N-1 mapped to kernel
   linadr_t stack = MTComponent::getInstance().iVirtualMemory->allocateStack(0);

   // Allocate Thread structure at the bottom, stack growing upwards from there
   Thread* t = (Thread*) ((u32) stack + (1UL<<(0+12)) - sizeof(Thread));
   t->stackorder = 0; // stackorder;
   return t;
}

void
Thread::operator delete( void* _this )
{
   // DPRINTK( "\nThread::delete this=%x esp=%x", _this, Processor::ESP() );
   
   ASSERTION( _this != &currentThread(), E_ERROR );    // don't kill yourself!
   Thread* t = (Thread*) _this;
   MTComponent::getInstance().iVirtualMemory->freeStack( 
      (void*) ((u32)_this & 0xFFFFF000), t->stackorder );
}

// used for initial thread
Thread::Thread()   // INITSECTION
: state(E_RUNNING), esp(0) // set when switch to next thread is made
, toDelete(0)
, allocator( *MTComponent::getInstance().iVirtualMemory, 0 ), locks(1)
{
   // XX should really clear what needs clearing here...

   // DPRINTK( "\ncurThread=%x -sizeof(Thread)=%x", &currentThread(), -sizeof(Thread) );
   
   // start quantum timer
   // XX *SHOULD NOT FIRE BEFORE PROCESS INIT FINISHES*
   // this->schedule( 10000 );   // ::TimerEvent, XX hardcoded
   
   ITIMING->initTimer( timer, *this );
}


Thread::Thread( u32 allocOrder ) 
: state(E_CREATED), next(0), esp((u32*)this)
, toDelete(0)
, allocator( *MTComponent::getInstance().iVirtualMemory, allocOrder )
, locks(1)
{
	ITIMING->initTimer( timer, *this );
}

// virtual, called in context of *another* thread!
Thread::~Thread()
{
   ASSERTION( this != &currentThread(), E_ERROR );    // don't kill yourself!
   ASSERTIONv( this->state == E_ENDED, E_ERROR, this->state );
   ASSERTIONv( locks<=0, E_CRITICAL, locks );
   
   // DPRINTK( "\n~Thread this=%x", this );

	ITIMING->freeTimer( timer );
   
#ifdef MP_IN_CORE
   homeContext->removeThread( *this );
#endif      
   this->state = E_DESTRUCTED;
   // calls ~ByteAllocator which frees all memory      
}

// virtual callback
void
Thread::onTimer( )
{
	// in any case, request that the scheduler be called after this
	InterruptHandling::signalScheduler();		// There can be several threads ready
	
	// DPRINTK( "\nThread::onTimer this=%x state=%d", this, state );
	switch ( this->state )
	{
	case E_RUNNING:	// i.e. quantum ended
		this->state = E_READY;	// policy should choose upon IRQschedule() 
		ThreadManager::threadReady( *this );		
		return;
		
	case E_READY:  // i.e. start() called
		currentThread().state = E_READY;	// from E_RUNNING, until selected
		ThreadManager::threadReady( *this );
		return;
          
	case E_SLEEPY:  // i.e. sleep() called
		this->state = E_WAITING;
      	// Order OK?
		ThreadManager::startWaiting( *this );
		ITIMING->startTimer( this->timer, this->sleeptime );	// in IRQ
		return;

	case E_ENDED:
	{
		// TODO: Think about semantics of onEndThread...		
		Thread& _next = ThreadManager::endThread( *this );
		_next.toDelete = this;
		return;
	}

	case E_WAITING:	// i.e. timeout after sleep() ends

		// Check for wake-before-sleep situation
		//
		// This occurs when the timer fires after onTimer_schedule above,
		// before ThreadManager::finishINT gets a chance to run select()
		// Seems solved now, by putting the schedule() *after* esp swap
		ASSERTIONv( &currentThread() != this, E_CRITICAL, this );
		currentThread().state = E_READY;	// from E_RUNNING, until selected
		this->state = E_READY;
		ThreadManager::endWaiting( *this );
		return;
		
	default:
		ASSERTIONv( "Unexpected thread state"==0, E_ERROR, this->state );
	}
}

void
Thread::init( IRunnable& runnable, NVPAIR params[], size_t paramCount )
{
   ASSERTION( state == E_CREATED, E_ERROR ); // for now, no reuse yet
   // DPRINTK( "\nThread::init good params = %x %x %x", this, &runnable, params );   
   
    // Stackframe must match 'select' epilogue code !!
    esp = (u32*) this; // reinit, for reusing threads

    // GCC likes stack aligned to 16-byte; trial and error...
    esp -= 1;    
   
    // Don't reverse the order of the parameters!
    if (params) for (int p=paramCount-1; p>=0; --p) {
        (*(--esp)) = (u32) params[p].getValue();
        (*(--esp)) = (u32) params[p].getName();
    }   
    (*(--esp)) = (u32) (esp+1);    // point to copy params on new stack
    (*(--esp)) = (u32) &runnable;
    (*(--esp)) = (u32) this;
    (*(--esp)) = 0;                  // bogus caller address ?? (could be &exit)
    (*(--esp)) = (u32) startThread;  // "return" address
   
   // conform to 'select' epilogue code, which pops some dwords (see asm list..)
#ifdef DEBUG
   esp -= 3;
#else
#if __GNUC__==4
	esp -= 3;
#else
	esp -= 2;   // other compiler flags give other results :( DANGER ):
#endif   
#endif

#ifdef MP_IN_CORE
   // .. and/or perhaps curProcess.attachThread( *this )
   homeContext = visitedContext = &MultiProcessing::Process::getCurrent();
#endif
   state = E_READY;
}

// simple, used by process
void
Thread::start()
{
   ASSERTION( &currentThread() != this, E_ERROR );
   ITIMING->startTimer( timer, 1000 );	// 1000 us? More short ?
}

// virtual
void
Thread::start( IRunnable& r, NVPAIR params[], size_t paramCount )
{
	this->init( r, params, paramCount );
	this->start();
}

// static, REGPARM(0) to ensure all params are passed on stack
void
Thread::startThread( Thread& _this, IRunnable& runnable, NVPAIR params[] )
{
   try {
      _this.exitcode = runnable.run( params );
      DPRINTK( "\nExitcode: %d", _this.exitcode );
   } catch (ThreadExitException &tee) {
      // this is normal, thrown in exit() below
      DPRINTK( "#2f#\nCaught ThreadExitException in thread(%x).run", &_this );
      _this.exitcode = tee.getExitCode();
   } catch (...) {
      DPRINTK( "#4f#\nCaught exception in thread(%x).run", &_this );
   }
   _this.state = E_ENDED;
   DPRINTK( "\nThread %x ended", &_this );
   ITIMING->triggerTimer( _this.timer );
   
   BUG( "@#ERROR#@ should not return here!" );
}

/***
 * NOTE: Don't touch this routine for debugging, the size of the stackframe
 * is added in init()!
 */
void
Thread::select( u32 quantumUs )  // REGPARM(2)
{
	this->state = E_SELECTED;

   // State of current thread must be set to E_WAITING or E_READY before this
   // DPRINTK( "\nselect->schedule this=%x quantum=%d esp=%x", this, quantumUs, esp );

   // save(!) current ESP for current thread, also save for first thread
   // since space for Thread instance is reserved in DoDecompress
   currentThread().esp = (u32*) Processor::ESP();

   // Check for process change too
   
/***
 * This is **REALLY TRICKY** CODE!!!
 * 
 * Think about it. TR points at both the old and the new TSS at the moment
 * of the task switch. That means the register values get saved in the
 * TSS of the *next* process (being switched to). However, CR3 does not get 
 * stored...
 * 
 * Then CR3 is updated using the value in the new TSS, so a context switch
 * is effective, but all registers are reloaded with their previous values...
 * And, I need to store the values of ECX, EDX, EBX in the new TSS, to keep
 * them around when the reverse switch happens. Does this go OK with N processes
 * intermixing??
 */      
   ASMVOLATILE( 
#ifdef MP_IN_CORE
".globl tss_eip               ;" // used in TSS constructor
      "cli                    ;" // just in case, not sure it is needed...
      "cmpl %1, (curProcess)  ;" // could do CMPXCHG if needed
      "je tss_eip             ;" // Skip if process==current
      "movl %1, (curProcess)  ;"
                                 // could update curProcess->TSS here...
      "movl %%ecx, 44(%1)     ;" // save ECX (quantum( in TSS image
      "movl %%edx, 48(%1)     ;" // save EDX (ESP) in TSS image
      "movl %%ebx, 52(%1)     ;" // save EBX (this) in TSS image
      "andl $0x00FFFFFF, %1   ;" // clear upper 8 bits, assuming 0xFF.. area                                 
      "orl  $0x89000000, %1   ;" // Present, available i386 TSS, DPL0
      "movl %1, (GDT+34)      ;" // Modify TSS descriptor, 
                                 // **assumes** TSS-index==4, +2 offset !!
      "ljmp $32,$0            ;" // make task switch, this stores in *next* TSS
#endif
"tss_eip:                     ;" // Label used to init EIP in TSS
      "movl %0, %%esp         ;" // When called in IRQ context, no need to
                                 // save any registers or CS ..
      "sti                    ;" // to match 'cli' above                                 
   :  
   : "r"(this->esp) 
#ifdef MP_IN_CORE
     , "r"(this->visitedContext)
#endif   
   : "memory" );

   this->state = Thread::E_RUNNING;
   ITIMING->startTimer( timer, quantumUs );	// after switch of esp! Critical
											// for preventing wake-before-sleep   

   // check for thread to delete
   Thread* t;
   if (RARELY((t=toDelete))) {
      toDelete = 0;
      t->asyncDone(true);   // deletes iff locks becomes 0
   }
}

// virtual 
void* 
Thread::allocate( size_t bytes ) throw (OutOfMemoryException) {
   // DPRINTK( "\nThread::allocate size=%d", bytes );
   return allocator.allocateAutoSize( bytes );  
}   

// virtual 
void 
Thread::free( void* mem ) {
   // DPRINTK( "\nThread::free this=%x mem=%x", this, mem );
   return allocator.freeAutoSize( mem );  
}

// virtual 
bool 
Thread::usSleep( u32 us, u32 usAccuracy /* not used yet... */ )
{
	NOT_IN_IRQ;
   
	// DPRINTK( "\nusSleep us=%d this=%x", us, this );

	// XX Tweak for time keeping overhead
#ifdef DEBUG	
	if (us > 38) us -= 38;
#else	// RELEASE
	if (us > 30) us -= 30;
#endif	
   
	// TODO: Watch out for wake-before-sleep 
	this->sleeptime = us;
	this->state = E_SLEEPY;
	ITIMING->triggerTimer( timer );
   
	// DPRINTK( "\nSleep done %b (sleeptime=%d)", this->sleeptime == us, sleeptime );
	return this->sleeptime == us;    // set to 0 in wakeup() below
}

// virtual
bool
Thread::wakeup()
{
	ASSERTIONv( state==E_WAITING, E_ERROR, state );
   
	// DPRINTK( "\nwakeup this=%x cur=%x ESP=%x CS=%x", 
	//	this, &currentThread(), Processor::ESP(), Processor::CS() );
   
   // XX: Also E_SLEEPY? If so, add test for 0 in onTimer...
	if (state == E_WAITING /*|| state == E_SLEEPY*/) {
		this->sleeptime = 0;
		if (!InterruptHandling::inIRQ()) {		
			ITIMING->triggerTimer( timer );
		} else {	// already IRQ context
			ITIMING->cancelTimer( timer );
			this->state = E_READY;
			currentThread().state = E_READY;	// from E_RUNNING, until selected
			ThreadManager::threadReady( *this );
			InterruptHandling::signalScheduler();
		}
		return true;
   }
   DPRINTK( "\nWakeup failed! state=%X", state );
   return false;
}

// virtual
void
Thread::yield( IThread* to ) 
{
   NOT_IN_IRQ;      
   
   if (to) {
      ASSERTION( ((Thread*)to)->state == E_READY, E_ERROR );
      // Notify the policy, then switch (here is ok?) ?
      // Perhaps better to set the timer on the 'to' thread
      BUG( "TODO: Yield to thread, policy.onHandover()" );    
   } else {
      // simply force the timer to expire now
      CHECKPOINT('y');
      // this->trigger();     TODO
   }   
}

// virtual
void
Thread::exit( int ec ) throw (ThreadExitException) // NORETURN
{
   ASSERTION( state==E_RUNNING, E_ERROR );   
   ASSERTIONv( toDelete==0, E_ERROR, toDelete );
   DPRINTK( "\nThread::exit this=%x code=%d", this, ec );
   
   this->state = E_ENDED;
   throw ThreadExitException(ec);
}

// virtual 
bool 
Thread::registerIdleWork( void (*f)() )
{
   // TODO
   return false;  
}

// static
void
Thread::becomeIdle() // NORETURN
{
	// Finish kernel init   
	MM::MMComponent::getInstance().freeInitPages();      

	Thread* cur = &currentThread();
   
   // Start quantum timer for init thread here instead !!
   ITIMING->startTimer( cur->timer, 10000 );   // XX hardcoded   
   try {
      PRINTK( "\nIDLE(%x) running...", &currentThread() );
      while (1)
      {
         for (int i=0; i<10000000; ++i);   // wait a bit
#ifdef DEBUG
         CHECKPOINT('i');
#else      
         PRINTK("i"); // show life
#endif      
      }     
   } catch (...) {
      DPRINTK( "Caught exception in IDLE" );
   }
   ASMVOLATILE( "hlt" );
   while (1);
}
   
}} // namespace
