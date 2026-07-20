#line 1 "Thread.cpp"

#include "Thread.h"
#include "ThreadManager.h"
#include "MultiThreading/MTComponent.h"
#include "MM/memtypes.h"   // for E_KERNEL_MEMORY flag

#include "TimerFacility/ITiming.h"
#include "TimerFacility/ITimer.h"

// To finish init
#include "MM/MMComponent.h"
#include "MM/Virtual/GDT.h"
#include "MultiProcessing/Process.h"      // removeThread

/// Label defined in InterruptHandlingImpl
extern "C" void* after_handler;

namespace MyOS { namespace MultiThreading {

using MM::linadr_t;

#define ITIMING MTComponent::getInstance().iTiming
#define IVIRTUAL MTComponent::getInstance().iVirtualMemory

Thread *Thread::toDelete = 0;

void*
Thread::operator new( size_t )
{
   // NOTE: Currently *cannot* allocate anything but 1-page stack!
   // currentThread() depends on the fact that ESP is in same page!
   
   // Allocate 2^N linear frames, 1 not mapped and 2^N-1 mapped to kernel
   linadr_t stack = IVIRTUAL->allocateStack(0);

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
: state(E_RUNNING), next(0), esp(0) // set when switch to next thread is made
, homeContext(0), visitedContext(0)
, allocator( *MTComponent::getInstance().iVirtualMemory, 0 ), locks(1)
, timer( ITIMING->createTimer( *this ) )
{
   // XX should really clear what needs clearing here...

   // DPRINTK( "\ncurThread=%x -sizeof(Thread)=%x", &currentThread(), -sizeof(Thread) );
   
   // start quantum timer
   // XX *SHOULD NOT FIRE BEFORE PROCESS INIT FINISHES*
   // this->schedule( 10000 );   // ::TimerEvent, XX hardcoded

}


Thread::Thread( IRunnable& runnable, NVPAIR params[], size_t paramCount, 
        MM::EDPL rpl, u32 allocOrder, bool _start ) 
: state(E_CREATED), next(0), esp((u32*)this)
, allocator( *MTComponent::getInstance().iVirtualMemory, allocOrder )
, locks(1)
, timer( ITIMING->createTimer( *this ) )
{	
	// Initialize these
	this->homeContext = this->visitedContext = &MultiProcessing::Process::getCurrent();
	
	// Stackframe must match 'select' epilogue code !!
    esp = (u32*) this; // reinit, for reusing threads

    // GCC likes stack aligned to 16-byte (must be aligned @ startThread ); trial and error...
    // esp -= 1;
        
    u32 paramParams = 0;
    if (params) {
        // Don't reverse the order of the parameters!
        for (int p=paramCount-1; p>=0; --p)
        {        
            (*(--esp)) = (u32) params[p].getValue();
            (*(--esp)) = (u32) params[p].getName();
        }
        
        // Make sure stack stays 16-byte aligned!
        if (paramCount&1) {
            (*(--esp)) = 0;
            (*(--esp)) = 0;
        }
        paramParams = (u32) (esp+1+(paramCount&1)); // point to copy params on new stack        
    }

    // If the thread starts at a lower privilege level, SS:ESP must also be provided on the stack
    // SS must have same rpl
    MM::Selector cs( Processor::CS() );
    MM::Selector ds( Processor::DS() );
    if ( cs.rpl() < rpl ) {
        (*(--this->esp)) = (u32) MM::Selector( MM::GDT::E_DATA_DPL3, rpl ); // DPL must match rpl! Can't use SS()
        
        /// @todo allocate stack in virtual memory readable at CPL3!
        (*(--this->esp)) = (u32) IVIRTUAL->allocateStack(0,true);    // Allocate separate stack! @todo free it
        
        // also use other CS/DS
        cs = MM::Selector( MM::GDT::E_CODE_DPL3, rpl );
        ds = MM::Selector( MM::GDT::E_DATA_DPL3, rpl );
    }
    
    // Construct iret return frame: return address, segment selector, flags
    (*(--this->esp)) = Processor::FLAG_INT; // EFLAGS: enable interrupts    
    (*(--this->esp)) = (u32) cs;     // can select other level here!
    (*(--this->esp)) = (u32) startThread;
            
    /***
     * At this point, construct a stackframe that the code in IInterruptHandlingImpl can
     * unwind:
     *
     * pop es
     * pop ds  
     * popad   -> pop edi, esi, ebp, esp (ignored), ebx, edx, ecx, eax
     * pop eax
     * 
     * Parameters for startThread: _this = EAX, runnable = EDX, params = ECX
     */
    (*(--esp)) = (u32) this;                // pop eax   
    (*(--esp)) = (u32) this;        // pop eax (part of popad)
    (*(--esp)) = paramParams;       // ecx
    (*(--esp)) = (u32) &runnable;   // edx
    (*(--esp)) = 0; // ebx
    (*(--esp)) = 0; // esp (ignored)
    (*(--esp)) = 0; // ebp
    (*(--esp)) = 0; // esi
    (*(--esp)) = 0; // edi
    
    (*(--esp)) = (u32) ds | ((u32)ds)<<16;  // pop ds/es
            
    // state = E_READY; leave it E_CREATED
    if (_start) this->start();
}

// virtual, called in context of *another* thread!
Thread::~Thread() throw()
{
   ASSERTION( this != &currentThread(), E_ERROR );    // don't kill yourself!
   ASSERTIONv( this->state == E_ENDED, E_ERROR, this->state );
   ASSERTIONv( locks<=0, E_CRITICAL, locks );
   
   // DPRINTK( "\n~Thread this=%x", this );

   timer.free();   

   homeContext->removeThread( *this );
      
   this->state = E_DESTRUCTED;
   // calls ~ByteAllocator which frees all memory      
}

// virtual callback, in IRQ context
void
Thread::onTimer( ITimer& )
{
	// DPRINTK( "\nThread(%x)::onTimer state=%d stack=%x", this, state, Processor::ESP() );
  
	// in any case, request that the scheduler be called after this
    // (or *after* processing?)
	// ThreadManager::requestReschedule();

    Thread &cur = currentThread();
    
	switch ( this->state )
	{
	case E_RUNNING:	// i.e. quantum ended
		ASSERTION( &cur == this, E_CRITICAL );
		this->state = E_READY;	   // policy should choose upon IRQschedule() 
		// DPRINTP( "\nFull quantum done" );  // or yield() called
		CHECKPOINT('Q');
		//
		// DONT call into policy, policy gets its chance in selectNext
		//
		// ThreadManager::threadReady( *this );		
		break;
		
	case E_CREATED:  // i.e. start() called
	{
		// copy stack of current thread until return address @ InterruptHandlingImpl line 307
		u32 *start = Processor::ESP() + 7;    // skip local stackframe (7 dwords, 0x1c)
		u32 *bottom = start;
		while ( *bottom != (u32) &after_handler ) ++bottom;
		u32 dwords = ++bottom - start;    // Include after_handler address
		ASSERTION( dwords < 40, E_ERROR );
		this->esp -= dwords;
		memcpy( this->esp, start, dwords*4 );
        
		// Compensate for the 'select' epilogue code, which pops some dwords (see asm list..) 		
        esp -= 3; // other compiler flags give other results :( DANGER ):
                  // -O  : add $4, pop ebx, pop esi, ret

        // no break
	}

	case E_SUSPENDED:  // i.e. wakeup() called
	    // CANCEL(!) quantum timer of current thread
	    cur.timer.cancel();  // in IRQ
	    cur.state = E_READY;   // from E_RUNNING, until selected
	    this->state = E_READY;
	    ThreadManager::threadReady( *this );	    
	    break;

	case E_SUSPENDING: // i.e. suspend() called, this==current
	    this->state = E_SUSPENDED;
	    // Dont tell policy
	    // Dont start any timer
	    break;	    
	    
	case E_SLEEPY:  // i.e. sleep() called
		ASSERTION( &cur == this, E_CRITICAL );
		this->state = E_SLEEPING;
		// ThreadManager::startWaiting( *this );     // XX needed? current policy does not use it
		// DPRINTK( "E_SLEEPY Starting sleep timer for %x: %d us", this, sleeptime );
		this->timer.start( this->sleeptime );	// in IRQ
		break;

	case E_SLEEPING:	// i.e. timeout or wakeup() after sleep()

	    this->state = E_READY;
	    
		// Check for wake-before-sleep situation
		//
		// This occurs when the timer fires immediately after startTimer above,
	    // when sleeptime is very small (smaller than minimum programmable period)
		if ( &cur == this ) {
		    // DPRINTK( "\nWake-before-sleep" );
		    return;   // stack unwinding continues after E_SLEEPY, will call scheduler
		}
		
		// CANCEL(!) quantum timer of current thread
		cur.timer.cancel();   // in IRQ
		cur.state = E_READY;  // from E_RUNNING, until selected
		ThreadManager::threadReady( *this );
		break;

	case E_ENDED:
    {        
        Thread& idle = ThreadManager::threadExiting( *this );
        // delete this; cannot do this: stack is deleted too!
        if ( Thread *t = toDelete ) {
            t->asyncDone(true);
            delete t;          // other thread, can do this now. Happens?
        }
        toDelete = this;
        idle.select( 10000 );  // should be long enough, idle thread yields() ASAP
        return;                // never executed: this thread is gone	        
    }		
		
	default:
		ASSERTIONv( "Unexpected thread state"==0, E_CRITICAL, this->state );
	}
	
	// Current thread must be READY or SLEEPING or SUSPENDED
	ThreadManager::reschedule();
}

// virtual
void
Thread::start()
{
   timer.trigger();
}

// static, REGPARM(3) to ensure all params are passed in registers
// USERSECTION to allow execution from userspace
void
Thread::startThread( Thread& _this, IRunnable& runnable, NVPAIR params[] )
{
    // Upon entry, stack should be aligned by 16 bytes (on screen shows ESP after change)
    DPRINTK( "\nstartThread reached, thread=%x CS=%x stack=%x", 
            &_this, Processor::CS(), Processor::ESP() );

    // re-enable IRQ0, shortcut (alternative is to copy entire stack of first thread, and
    // put startThread address where former thread was interrupted)
    // Timer::ITimerFacilityImpl::timerHandlerDone();
    // InterruptHandling::IInterruptHandlingImpl::fakeIretIRQ0();
       
    // Switch to regular CS (@todo easier way? far return?)
    // ASMVOLATILE( "ljmp $8, $regularCS; regularCS:" );
    try
    {
        _this.exitcode = runnable.run( params );
        DPRINTK( "\nExitcode: %d", _this.exitcode );
    }
    /*catch (ThreadExitException &tee)
    {
        // this is normal, thrown in exit() below
        DPRINTK( "#2f#\nCaught ThreadExitException in thread(%x).run", &_this );
        _this.exitcode = tee.getExitCode();
    }*/
    catch (...)
    {
        DPRINTK( "#4f#\nCaught exception in thread(%x).run", &_this );
    }
    _this.state = E_ENDED;
    DPRINTK( "\nThread %x ended", &_this );
    _this.timer.trigger();

    BUG( "@#ERROR#@ should not return here!" );
    while (1);
}

/***
 * NOTE: Don't touch this routine for debugging, the size of the stackframe
 * is added in init()!
 */
void
Thread::select( u32 quantumUs )  // REGPARM(2)
{
	this->state = E_SELECTED;  // not really needed

	Thread &cur = currentThread();
	
   // State of current thread must be set to E_WAITING or E_READY before this
   // DPRINTK( "\nselect->schedule this=%x quantum=%d esp=%x", this, quantumUs, esp );

   // update(!) current ESP for current thread (visible to others)
   cur.esp = Processor::ESP();

   // Check for process change too
   if ( &MultiProcessing::Process::getCurrent() != this->visitedContext ) {
       // Code belongs in Process, but needs to be inline
       using namespace MM;
       using MultiProcessing::TSS;
       
       SegmentDescriptor &tssD = GDT::getEntry( TSS::getTSSSelector().index() );
       tssD.setBase( (u32) this->visitedContext );
       tssD.setAttributes( SEG_TSS );
       
       this->visitedContext->prepareSwitch( this->esp );
       CHECKPOINT('$');
       TSS::taskSwitch();   // make task switch, this stores in *next* TSS
       
      // could also do a far call, then backlink would get filled in and task
      // is considered nested (both marked busy) and NT flag bit is set
      // when NT is set, iret causes a taskswitch
      // This can also be used as task scheduling mechanism (see
      // http://www.embedded.com/columns/technicalinsights/55301875?_requestid=27221
      // if process initially has interrupts disabled (no need every time)
      
     ASMVOLATILE(
       ".globl _tss_eip;"  // used in TSS constructor to initialize EIP, needed?            
       "_tss_eip:      ;"
     );      
   } else {
       Processor::setESP( this->esp );  // perform switch by swapping stacks
   }
   
   /**
    * This is the only point where state gets set to 'RUNNING'
    * Do it after starting the timer? it may fire immediately (should be prevented!)
    */  
   this->state = Thread::E_RUNNING;
      
   /**
    * At this point, the stack has changed, but 'this' should still point to the same thread
    */  
   timer.start( quantumUs );	// after switch of esp! Critical
								// for preventing wake-before-sleep   
}

// virtual 
void* 
Thread::allocate( size_t bytes ) throw (MM::OutOfMemoryException) {
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

	// XX Tweak for time keeping overhead: not the right place (unit should be ticks!)
/*	
#ifdef DEBUG	
	if (us > 70) us -= 70;
#else	// RELEASE
	if (us > 62) us -= 62;
#endif*/	
   
	// TODO: Watch out for wake-before-sleep 
	this->sleeptime = us;
	this->state = E_SLEEPY;
	timer.trigger();
   
	// DPRINTK( "\nSleep done %b (sleeptime=%d)", this->sleeptime == us, sleeptime );
	return this->sleeptime == us;    // set to 0 in wakeup() below
}

// virtual
void
Thread::suspend()
{
    ASSERTION( this == &currentThread(), E_ERROR );
    
    DPRINTK( "\nsuspend this=%x cur=%x ESP=%x CS=%x", 
            this, &currentThread(), Processor::ESP(), Processor::CS() );
    
    this->state = E_SUSPENDING;
    timer.trigger();
}

// virtual
bool
Thread::wakeup()
{
	ASSERTIONv( state==E_SLEEPING || state==E_SUSPENDED, E_ERROR, state );
   
	DPRINTK( "\nwakeup this=%x cur=%x ESP=%x CS=%x", 
	    this, &currentThread(), Processor::ESP(), Processor::CS() );
   
	switch( this->state )
	{
	case E_SLEEPING:    // i.e. waiting for timer
	    this->sleeptime = 0;
	    // no break
	    
	case E_SUSPENDED:   // i.e. suspended waiting for wakeup		
		// state remains same, onTimer changes it
		timer.trigger();   // may be running, or not, does not matter
		return true;
		
	default:
	    DPRINTK( "\n#4f#Wakeup failed! state=%X", state );
	    return false;	    
   }
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
      timer.trigger();
   }   
}

// static, called at end of MyOSMain()
void
Thread::becomeIdle( unsigned idleQuantumMs ) // NORETURN

{
    // Finish kernel init   
    MM::MMComponent::getInstance().freeInitPages();

    Thread* cur = &currentThread();

    // Start quantum timer for init thread here instead
    DPRINTK( "\nbecomeIdle(%x) starting %dms timer...", cur, idleQuantumMs );
    if (idleQuantumMs>0)
    {
        cur->timer.start( 1000*idleQuantumMs );
    }
    try
    {
        PRINTK( "\nIDLE(%x) running...", cur );
        int count=0;
        while (1)
        {
            // Do cleanup of exited threads. Idle is forced to run after a thread exits
            if ( Thread *t = toDelete ) {
                toDelete = 0;
                t->asyncDone(true);   // deletes iff locks becomes 0
                cur->yield(0);        // Yield processor immediately
            }
            
            
            //  
            if ((++count & 0xfffff)==0)
            {
                CHECKPOINT('i');    // show life (not in release mode)
            }
        }
    }
    catch (...)
    {
        DPRINTK( "Caught exception in IDLE" );
    }
    ASMVOLATILE( "hlt" );
    while (1);
}

// @todo
/*

// semantics: running thread synchronizes on this thread
Thread::synchronize()
{
    // set internal parameter 'syncOn', trigger timer of running thread, then in onTimer:
    // add 'this' to syncList of 'syncOn' thread 
}
 
 */

}} // namespace
