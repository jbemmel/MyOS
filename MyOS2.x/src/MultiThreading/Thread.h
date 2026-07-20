/**
 * Some details about the threading implementation
 * 
 * - Threads could be reused by calling 'init' again after they have 
 *   succesfully exitted (TODO: Fix that)
 */

#ifndef THREAD_H
#define THREAD_H

#include "Atomic/atomic32.h"
#include "IThread.h"

// pretty tight coupling with MM classes...
#include "MM/Virtual/ByteAllocator.h"
#include "Timer/ITiming.h"
#include "Processor/Processor.h"

#include "Init/MyOSCoreContainerDefs.h"
#ifdef MP_IN_CORE
namespace MyOS { namespace MultiProcessing { class Process; } }
#endif

namespace MyOS { namespace MultiThreading {

using Timer::timer_t;

class IRunnable;

class Thread : public IThread, public Timer::ITimerTarget
{
	// TimerEvent + ITimerTarget
	timer_t timer;
	virtual void onTimer( );

	friend class ThreadManager;

   // used for initial Thread
   inline void* operator new( size_t, void* m ) { return m; }
   Thread() INITSECTION;

   // Makes this thread the current one, called by ThreadManager
   void select( u32 quantumUs ) REGPARM(2);

   // datamembers
   friend class SchedulingPolicy;
   enum { 
      E_CREATED=1, E_READY=2, E_SELECTED=3, E_RUNNING=4, 
      E_WAITING=5, E_SLEEPY=6, E_ENDED=7, E_DESTRUCTED=8 
   } state;

   friend class DefaultPolicy;   // XX TODO redesign
   void* policydata;
   Thread* next;                 // for making linked lists

   u32* esp;      // Ring0 ESP register, rest of state is saved on stack
#ifdef MP_IN_CORE
   friend class MultiProcessing::Process;
   MultiProcessing::Process *homeContext, *visitedContext;
#endif   
   u32 sleeptime; // in us
   int exitcode;

   Thread* toDelete; // if !=0, this thread should delete it

   // each thread gets its own allocator, for super non-blocked operation!
   MM::Allocator allocator;
   u32 stackorder;   // size of stack of this thread, in 2^n

   atomic32 locks;  // Lock counter, to prevent deletion with pending asyncs

   static void startThread( 
      Thread& _this, IRunnable& runnable, NVPAIR ps[] ) REGPARM(0);
      
public:

	// for debugging
	int getState() const { return state; }

   /**
    * @see IThread, but this one gives access to all of 'Thread' members
    */
   inline static Thread& currentThread() {
      return * (Thread*) (((u32) Processor::ESP() | (_4KB-1))+1-sizeof(Thread));
   }

   // Needs special care when allocating memory
   void* operator new( size_t, u32 stackorder );
   void operator delete( void* );
   explicit Thread( u32 allocOrder );
   virtual ~Thread();

   // Used to (re)initialize stack
   void init( IRunnable& runnable, NVPAIR params[], size_t paramCount );
   
	// Used by Process
	void start();

   // IThread methods
	virtual void start( IRunnable& r, NVPAIR params[], size_t paramCount );	
	virtual bool isRunning() const  { return state == E_RUNNING; }
	virtual bool isSleeping() const { return state == E_WAITING; }
	virtual bool usSleep( u32 us, u32 usAccuracy = 0 );
	virtual bool wakeup();
	virtual void yield( IThread* to );
	virtual void exit( int exitcode ) throw (ThreadExitException) NORETURN;
	virtual bool registerIdleWork( void (*f)() );

   // perhaps virtual too
   bool isReady() const  { return state == E_READY; }
   
   void addPending() { ++locks; }
   void asyncDone( bool mayDeleteNow ) { 
      if ((--locks)<=0) {  // it may fall below 0
         if (mayDeleteNow)
            delete this; 
         else
            currentThread().toDelete = this; // XX hope its 0
      }         
   }
   
   // Use the allocator, used by global 'new' operator in IThread.h
   virtual void* allocate( size_t bytes ) throw (OutOfMemoryException);
   virtual void free( void* mem );

   // Used by TSS
   inline u32* getESP() const { return esp; }

   // Used for exception handling
   inline void* allocateNoThrow( size_t bytes ) throw() {
      return allocator.allocateAutoSizeNoThrow( bytes );     
   }

   inline bool canThrow() const { return state==E_RUNNING || state==E_ENDED; }
   
   // used for exception handling, public linked list of frames
   void* exception_fc;
   u8 emergencyMem[ 52 ];     // == sizeof(eh) + OutOfMemoryEx
   
   // Called by MyOSMain when its done
   static void becomeIdle() NORETURN;

   // debugging
   inline void showThreadMem() const { allocator.print(); }
};
      
}} // namespace

#endif
