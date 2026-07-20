#line 1 "Thread.cpp"

#include "Thread.h"
#include "ThreadManager.h"
#include "MT/MTComponent.h"
#include "memtypes.h"           // for E_KERNEL_MEMORY flag
#include "Boot/memlayout.h"     // for linker symbols for signals

#include "ITiming.h"
#include "ITimer.h"
#include "new.h"    // placement new

// To finish init
#include "MM/MMComponent.h"
#include "MM/GDT.h"
#include "MP/Process.h"      // removeThread

#include "Exceptions/unwind.h"
#include "MP/PerCPU.h"       // @todo refactor, move elsewhere

namespace MyOS { namespace MultiThreading {

using MM::linadr_t;

#define ITIMING MTComponent::getInstance().iTiming
#define IVIRTUAL MTComponent::getInstance().iVirtualMemory

Queue Thread::deleteQueue;  // static, note issue with constructor -> called by first thread

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

// Helper object as IRunnable instance for IDLE, to handle signals in idle
static class IdleRunner : public IRunnable {

    virtual int run( NVPAIR params[] ) { return 0; }   // not used

    virtual void handleSignal( u32 code, void* data ) throw() {
        CHECKPOINTc( 's', E_ERROR );
        DPRINTK( "\nIDLE::handleSignal code=%u data=%x", code, data );
    }
} idleRunner;

// used for initial thread
Thread::Thread()   // INITSECTION
: state(E_RUNNING), next(0), esp(0) // set when switch to next thread is made
, homeContext(0), visitedContext(0)
, allocator( *MTComponent::getInstance().iVirtualMemory, 0 ), locks(1)
, timer( ITIMING->createTimer( *this ) ), tlsMemory(0), syncOn(0), signalReturnAddress(0)
, runnable( idleRunner ), exception_fc(0), priority(0)
{
   // XX should really clear what needs clearing here...

   // DPRINTK( "\ncurThread=%x -sizeof(Thread)=%x", &currentThread(), -sizeof(Thread) );

   // start quantum timer
   // XX *SHOULD NOT FIRE BEFORE PROCESS INIT FINISHES*
   // this->schedule( 10000 );   // ::TimerEvent, XX hardcoded

    // make sure initializer gets called
    deleteQueue.init();

}

extern "C" void* new_thread_start;  // defined in IInterruptHandlingImpl.cpp

Thread::Thread( IRunnable& runner, NVPAIR params[], size_t paramCount,
        MM::EDPL rpl, u32 allocOrder, bool _start )
: state(E_CREATED), next(0), esp((u32*)this)
, allocator( *MTComponent::getInstance().iVirtualMemory, allocOrder )
, locks(1)
, timer( ITIMING->createTimer( *this ) ), tlsMemory(0), syncOn(0), signalReturnAddress(0)
, runnable( runner ), exception_fc(0), priority(0)
{
	// Initialize these
	this->homeContext = this->visitedContext = &MultiProcessing::Process::getCurrent();

	// Stackframe must match 'select' epilogue code !!
    u32* _esp = (u32*) this;

    // GCC likes stack aligned to 16-byte (must be aligned @ startThread ); trial and error...
    // esp -= 1;

    u32 paramParams = 0;
    if (params) {
        // Don't reverse the order of the parameters!
        for (int p=paramCount-1; p>=0; --p)
        {
            (*(--_esp)) = (u32) params[p].getValue();
            (*(--_esp)) = (u32) params[p].getName();
        }

        // Make sure stack stays 16-byte aligned!
        if (paramCount&1) {
            (*(--_esp)) = 0;
            (*(--_esp)) = 0;
        }
        paramParams = (u32) (_esp+2*(paramCount&1)); // point to copy params on new stack
    }

    // If the thread starts at a lower privilege level, SS:ESP must also be provided on the stack
    // SS must have same rpl
    MM::Selector cs( Processor::CS() );
    MM::Selector ds( Processor::DS() );
    if ( cs.rpl() < rpl ) {
        (*(--_esp)) = (u32) MM::Selector( MM::GDT::E_DATA_DPL3, rpl ); // DPL must match rpl! Can't use SS()

        /// @todo allocate stack in virtual memory readable at CPL3!
        (*(--_esp)) = (u32) IVIRTUAL->allocateStack(0,true);    // Allocate separate stack! @todo free it

        // also use other CS/DS
        cs = MM::Selector( MM::GDT::E_CODE_DPL3, rpl );
        ds = MM::Selector( MM::GDT::E_DATA_DPL3, rpl );
    }

    // Construct iret return frame: return address, segment selector, flags
    (*(--_esp)) = Processor::FLAG_INT; // EFLAGS: enable interrupts
    (*(--_esp)) = (u32) cs;     // can select other level here! TODO LDT CS entry per thread to serve as ID!
    (*(--_esp)) = (u32) startThread;

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
    (*(--_esp)) = (u32) this;                // pop eax
    (*(--_esp)) = (u32) this;        // pop eax (part of popad)
    (*(--_esp)) = paramParams;       // ecx
    (*(--_esp)) = (u32) &runner;     // edx
    (*(--_esp)) = 0; // ebx
    (*(--_esp)) = 0; // esp (ignored)
    (*(--_esp)) = 0; // ebp
    (*(--_esp)) = 0; // esi
    (*(--_esp)) = 0; // edi

    (*(--_esp)) = (u32) ds | ((u32)ds)<<16;  // pop ds/es

    (*(--_esp)) = (u32) &new_thread_start;    // return address for select() routine

    // Compensate for select() prologue code, which adds 3 dwords to ESP (pushes EDI/ESI/EBX)
    _esp -= 3;

    this->esp = _esp;

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

   if (tlsMemory) IVIRTUAL->freePage( tlsMemory );

   // calls ~ByteAllocator which frees all memory
}

// virtual callback, typically (but not guaranteed) in IRQ context
void
Thread::onTimer( ITimer& ) throw()
{
	// DPRINTK( "\nThread(%x)::onTimer state=%d stack=%x", this, state, Processor::ESP() );

	// in any case, request that the scheduler be called after this
    // (or *after* processing?)
	// ThreadManager::requestReschedule();

    Thread &cur = currentThread();

	switch ( this->state )
	{
	case E_RUNNING:	// i.e. quantum ended
	    // XXX with SMP this is not necessarily true, unless a timer heap per CPU is used (TODO)
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
		/* copy stack of current thread until return address @ InterruptHandlingImpl line 307
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
    */
	    CHECKPOINT('S');

    // no break
	}

	case E_SUSPENDED:  // i.e. wakeup() called
	    // CANCEL(!) quantum timer of current thread
	    cur.timer.cancel();  // in IRQ
	    cur.state = E_READY;   // from E_RUNNING, until selected
	    this->state = E_READY;
	    ThreadManager::threadReady( *this );
	    break;

	/*
	case E_SUSPENDING: // i.e. suspend() called, this==current
	    this->state = E_SUSPENDED;
	    // Dont tell policy
	    // Dont start any timer
	    break;
	*/

	/*
	case E_SLEEPY:  // i.e. sleep() called
		ASSERTION( &cur == this, E_CRITICAL );
		this->state = E_SLEEPING;
		// ThreadManager::startWaiting( *this );     // XX needed? current policy does not use it
		// DPRINTK( "E_SLEEPY Starting sleep timer for %x: %d us", this, sleeptime );
		this->timer.start( this->sleeptime );	// in IRQ
		break;
    */

	case E_SLEEPING:	// i.e. timeout or wakeup() after sleep()
	    CHECKPOINT('W');
	    this->state = E_READY;

		// Check for wake-before-sleep situation
		//
		// This occurs when the timer fires immediately after startTimer above,
	    // when sleeptime is very small (smaller than minimum programmable period)
		if ( &cur == this ) {
		    DPRINTK( "\nWake-before-sleep" );
		    return;   // stack unwinding continues after E_SLEEPY, will call scheduler
		}

		// CANCEL(!) quantum timer of current thread
		cur.timer.cancel();   // in IRQ
		cur.state = E_READY;  // from E_RUNNING, until selected
		ThreadManager::threadReady( *this );
		break;

	/*
	case E_ENDED:
    {
        Thread& idle = ThreadManager::threadExiting( *this );
        // delete this; cannot do this: stack is deleted too!
        if ( Thread *t = toDelete ) {
            t->asyncDone(true);
            delete t;          // other thread, can do this now. Happens?
        }
        toDelete = this;
        // idle.select( 10000 );  // should be long enough, idle thread yields() ASAP
        // return;                // never executed: this thread is gone
        break;

        // TODO: give idle high priority, to ensure it gets selected? No harm done, next ended thread
        // will also free this one
    }*/

	default:
		ASSERTIONv( "Unexpected thread state"==0, E_CRITICAL, this->state );
	}

	// Current thread must be READY or SLEEPING or SUSPENDED
	ThreadManager::requestReschedule( 0 /* todo this->cpu */ );
}

// virtual
void
Thread::start()
{
    /*
    this->state = E_READY;
    ThreadManager::threadReady( *this );
    Thread &cur = currentThread();
    cur.state = E_READY;
    cur.timer.cancel();
    ThreadManager::reschedule();
    */

    //
    // Need to start thread from interrupt context, iretd loads proper CS value
    //
    // timer.trigger();    // not guaranteed to fire immediately, does not result in IRQ
    timer.start( 1 );      // some very small value
}

// static, REGPARM(3) to ensure all params are passed in registers
// USERSECTION to allow execution from userspace
void
Thread::startThread( Thread& _this, IRunnable& runnable, NVPAIR params[] )
{
    // Upon entry, stack should be aligned by 16 bytes (on screen shows ESP after change)
    DPRINTK( "\nstartThread reached, thread=%x CS=%x stack=%x runnable=%x",
            &_this, Processor::CS(), Processor::ESP(), &runnable );

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
    /*catch (SecurityException &se) {     // test.. works!
        PRINTK( "#4f#\nCaught SecurityException exception in thread(%x).run", &_this );
    }*/
    catch (...)
    {
        PRINTK( "#4f#\nCaught exception in thread(%x).run", &_this );
    }
    PRINTK( "#4f#\nCalling exitThread(%x) from startThread", &_this );
    _this.exitThread();
}

void
Thread::exitThread() // NORETURN
{
    this->timer.cancel();
    this->state = E_ENDED;
    DPRINTK( "\nThread %x ended", this );

    Thread& idle = ThreadManager::threadExiting( *this );

    // delete this; cannot do this: stack is deleted too!
    ZombieThread tombstone( *this );
    deleteQueue.enqueue( tombstone );
    idle.select( 10000 );

    // should not come here
    ThreadManager::call_reschedule();

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
	// this->state = E_SELECTED;  // not really needed
	Thread &cur = currentThread();

	// Optimization: if scheduler selects running thread, don't change anything
	if ( &cur != this ) {

	    // State of current thread must be set to E_WAITING or E_READY before this
       // DPRINTK( "\nselect->schedule this=%x quantum=%d esp=%x", this, quantumUs, esp );

       // update(!) current ESP for current thread (visible to others)
       cur.esp = Processor::ESP();

       // Check for process change too
       if ( &MultiProcessing::Process::getCurrent() == this->visitedContext ) {
           Processor::setESP( this->esp );  // perform switch by swapping stacks
       } else {
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
       }
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
	// NOT_IN_IRQ; // Not needed (?)

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
	// this->state = E_SLEEPY;
	// timer.trigger(); XXX Not guaranteed to kick in immediately!

	this->state = E_SLEEPING;
	this->timer.start( us );

	ThreadManager::call_reschedule();

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

    // this->state = E_SUSPENDING;
    // timer.trigger();
    timer.cancel();
    this->state = E_SUSPENDED;
    ThreadManager::call_reschedule();
}

static _Unwind_Reason_Code Stop_Fn(int, _Unwind_Action actions, _Unwind_Exception_Class,
      struct _Unwind_Exception *, struct _Unwind_Context *, void *)
{
    if (actions & _UA_END_OF_STACK) {
        Thread::currentThread().exitThread();
    }

    return _URC_NO_REASON;
}

static void force_unwind_cleanup(_Unwind_Reason_Code, struct _Unwind_Exception *ex)
{
    Thread &cur = Thread::currentThread();
    cur.free( ex );
}

static void clean_exit() NORETURN;
static void clean_exit()
{
    // dont use stack-based allocation!
    Thread &cur = Thread::currentThread();

    void *mem = cur.allocate( sizeof(_Unwind_Exception) );
    _Unwind_Exception *ex = new ( mem ) _Unwind_Exception();
    ex->exception_class = 0;
    ex->exception_cleanup = &force_unwind_cleanup;
    _Unwind_SjLj_ForcedUnwind( ex, &Stop_Fn, 0 );
}

// virtual
void
Thread::exit( int _exitcode ) // NORETURN
{
    this->exitcode = _exitcode;
    clean_exit();
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
		// timer.trigger();   // may be running, or not, does not matter
	    timer.cancel();
	    this->state = E_READY;
	    ThreadManager::threadReady( *this );

	    if ( InterruptHandling::inIRQ() ) {
	        ThreadManager::requestReschedule();
	    } else {
	        ThreadManager::call_reschedule();
	    }
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
      timer.cancel();
      this->state = E_READY;
      ThreadManager::threadReady( *this );
      ThreadManager::call_reschedule();
   }
}

// static, called at end of MyOSMain()
void
Thread::becomeIdle( unsigned idleQuantumMs ) // NORETURN
{
    if ( MultiProcessing::getCurrentCPU() == 0 ) {
        MM::MMComponent::getInstance().freeInitPages();
    }

    Thread* cur = &currentThread();

    // Start quantum timer for init thread here instead
    DPRINTK( "\nbecomeIdle(%x) esp=%x starting %dms timer...",
            cur, Processor::ESP(), idleQuantumMs );
    if (idleQuantumMs>0)
    {
        // TODO Need to synchronize TSC / split timer heaps before doing this!
        //if ( MultiProcessing::getCurrentCPU() == 0 ) {
            cur->timer.start( 1000*idleQuantumMs );
        //}
    }
    try
    {
        PRINTK( "\nIDLE(%x) running on CPU%d...", cur, MultiProcessing::getCurrentCPU() );
        int count=0;
        while (1)
        {
            // Do cleanup of exited threads. Idle is forced to run after a thread exits
            // @todo Only on CPU#0 ??
            if ( ZombieThread *grave = (ZombieThread*) deleteQueue.dequeue() ) {
                grave->ended.asyncDone();   // deletes iff locks becomes 0
                cur->yield(0);              // Yield processor immediately
            }

            // do 'hlt' here, gives much more efficient Bochs simulation (ips++)
#if 1
            ASMVOLATILE( "hlt" );   // Pentium 4 has 'pause'
#endif

            if ((++count & 0xff) == 0xff) {
                CHECKPOINT('i');    // show life (not in release mode)
            }
        }
    }
    catch (...)
    {
        DPRINTK( "Caught exception in IDLE" );
    }
    while (1) ASMVOLATILE( "hlt" );
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

void*
Thread::initTLSVariable( u32 index, u32 size, void* value )
{
    ASSERTION( (index+size) < _4KB, E_ERROR );

    // Always first initialized before 'get' is called, TODO use mmap() for each thread!
    if RARELY(this->tlsMemory==0) {
        MM::physadr_t mapping(0);
        this->tlsMemory = (u8*) IVIRTUAL->allocPage( mapping );   // XXX max size 1 page for now
    }

    // DPRINTK( "\nTLS index:%u", index );

    void* var = &tlsMemory[index];
    if (value) {
        memcpy( var, value, size );
    }   // else already 0

    // DPRINTK( "\nTLS value at:%x", var );
    return var;
}

// static
void
Thread::processSignals() // NORETURN, SIGNAL_SAFE
{
    // GCC reserves room on the stack anyway to align ESP to n-bytes (!)
    // depending on the preferred-stack-boundary setting
    // Since we don't return here, may need to correct for that

    // Do a second invocation, such that we have a return location we can modify
    currentThread().processSignals2( Processor::ESP()-1 );
    while(1);
}

void
Thread::processSignals2( u32 *returnAddress )    // NOINLINE SIGNAL_SAFE
{
    DPRINTK( "\n#2f#processSignals intermezzo..." );

    ASSERTION( signalReturnAddress, E_CRITICAL );

    PendingSignal *ps;
    while ( ps = (PendingSignal*) signalQueue.dequeue() ) {
        // Make a copy on stack
        PendingSignal copy( *ps );
        this->free( ps );
        try {
            this->runnable.handleSignal( copy.code, copy.data );
        } catch (...) {
            DPRINTK( "\nCaught an exception in signal handler" );
        }

        // Special case: kill (magic code)
        if ( copy.code == 0xdeaddead ) {
            this->signalReturnAddress = (void*) clean_exit;
            this->exitcode = 0xdeaddead;
        }
    }

    // Replace the return address in processSignal with the original one
    *returnAddress = (u32) signalReturnAddress;
    signalReturnAddress = 0;
}

void
Thread::signal( u32 code, void* data )
{
    ASSERTION( this != &currentThread(), E_CRITICAL );

    void *mem = this->allocate( sizeof(PendingSignal) );
    PendingSignal *ps = new ( mem ) PendingSignal( code, data ); // could add currentThread for info
    signalQueue.enqueue( *ps );

    // If there is already a return address, some signal is already pending
    if ( this->signalReturnAddress ) {
        DPRINTK( "Signal already pending" );
        return;
    }

    // Block the thread before poisoning its stack (only needed on multi-CPU)
    E_THREADSTATE oldstate = (E_THREADSTATE) ((atomic32*) &state)->Replace( E_SUSPENDED_ON_SIGNAL );
    switch (oldstate)
    {
    case E_RUNNING:
        // TODO, only happens for multi CPU
        BUG( "Not implemented: signalling a running thread" );
        break;

    case E_SUSPENDED_ON_SIGNAL:
        // RACE with some other thread sending a signal
        DPRINTK( "\nAssuming RACE with other thread, signal set" );
        return;
    }

    // Poison the thread's stack with the address of ::processSignals
    for ( u32* _sp = this->esp; _sp < (u32*) this; ++_sp ) {
        u32 dword = *_sp;
        if ( (dword >= (u32) &SIGNAL_SAFE_END) && (dword < (u32) &_START_OF_DATA_) ) {

            // Found a potential return address on the stack which is in kernel code space, verify
            // that there is a 'call' instruction just before it

            /*
            Code    Mnemonic    Description
            E8 cw   CALL rel16  Call near, relative, displacement relative to next instruction
            E8 cd   CALL rel32  Call near, relative, displacement relative to next instruction
            FF /2   CALL r/m16  Call near, absolute indirect, address given in r/m16
            FF /2   CALL r/m32  Call near, absolute indirect, address given in r/m32
            9A cd   CALL ptr16:16   Call far, absolute, address given in operand
            9A cp   CALL ptr16:32   Call far, absolute, address given in operand
            FF /3   CALL m16:16     Call far, absolute indirect, address given in m16:16
            FF /3   CALL m16:32     Call far, absolute indirect, address given in m16:32
            */

            u8* ip = (u8*) dword;
            // handle 3 cases (in order of frequency of occurrence in kernel code): near calls
            if ( ip[-5] == 0xe8 || ip[-3] == 0xff || ip[-2] == 0xff ) {
                this->signalReturnAddress = (void*) dword;  // No RACE with processSignals2, thread blocked
                *_sp = (u32) processSignals;
                DPRINTK( "\nStack poisoned at %x, return address was %x", _sp, dword );
                state = oldstate;   // restore!
                return;
            } else DPRINTK( "\n#4f#Not a call site? %x", dword );
        }
    }
    DPRINTK( "\n#3f#Error: could not poision stack" );
    state = oldstate;   // restore!
}

}} // namespace
