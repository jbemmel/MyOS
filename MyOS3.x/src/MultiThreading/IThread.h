/**
 * This file is intended to be included by standalone MyOS applications or
 * components, that are compiled and linked separately from the core
 *
 * Therefore, access to thread methods must be through virtual methods, else
 * they get linked in..
 */
#ifndef ITHREAD_H
#define ITHREAD_H

#include "defs.h"    // NORETURN
#include "Exceptions/MyOSExceptions.h"
#include "NVPAIR.h"
#include "MM/memtypes.h"    // EDPL
#include "interfaces.h"

namespace MyOS { namespace MultiThreading {
  
using MM::OutOfMemoryException;
// using Exceptions::ThreadExitException;

class IRunnable;

/**
 * Defines user accesible methods on threads, and some helper methods
 * 
 * @todo derive from IInterface, register in context
 * @todo add SyncPoint to each thread, threads can sync on each other (signal() explicitly called or
 *       when thread exits)
 */
class IThread 
{
protected:
   // virtual ~IThread() throw() = 0;

public:

    /**
     * Starts this thread, used in case 'start' parameter in IMultiThreading#createThread was 'false' 
     */
    virtual void start() = 0;
    
    /**
     * Returns the process to which this Thread belongs
     * 
     * @return Process associated with this Thread
     * 
     * @note Could include pointer in IThread interface instead
     */
    virtual MultiProcessing::Process& getProcess() const = 0;
    
   /**
    * Performs a trick with ESP (round it up to next 4K) to get
    * the thread's context data
    * 
    * Changing the current thread now equals changing ESP!
    * For this trick to work, we allocate 2 linear frames per thread:
    * frame[0] is not mapped to detect stack overflow, frame[1] is used
    * to put the Thread structure (bottommost part) and the stack pointer
    * (everything above that)
    */
   // inline static IThread& current();

   /**
    * Memory management per thread, used with global new operator
    * 
    * @note Max allocatable block depends on allocOrder parameter in 
    * IMultiThreading#createThread
    */
   virtual void* allocate( size_t bytes ) throw (MM::OutOfMemoryException) = 0;
   
   /**
    * Frees memory previously allocated through allocate()
    */
   virtual void free( void* mem ) = 0;

   /**
    * Thread operations
    */
   virtual bool isRunning() const = 0;
   virtual bool isSleeping() const = 0;

protected:
   // These operations must only be called on currentThread
   // so use the static functions provided below
   virtual bool usSleep( u32 us, u32 accuracy ) = 0;

   /**
    * Give up the CPU for the remainder of the quantum
    * @param to : Optional thread that the caller would prefer to run
    *             should be in READY state
    */
   virtual void yield( IThread* to ) = 0;

   /**
    * Suspends this thread ( must be current ) until wakeup()
    */
   virtual void suspend() = 0;
   
   /**
    * Wakeup this thread
    * 
    * Cancels the sleep timer if active, and unblocks this thread. The
    * scheduler then decides when it should run
    * 
    * @return true iff thread was woken, false if already ready/failed
    */
   virtual bool wakeup() = 0;
   
public:
	
	/**
	 * Puts the current thread to sleep for <us> microseconds
	 * @param us 		  Number of microseconds to sleep after this call
	 * @param accuracyUs  Number of microseconds plus or minus the wakeup may be
	 * 					(default=0)
	 * 
	 * @return true iff sleep succeeded (i.e. time slept >= us)
	 */
	static inline bool do_usSleep( u32 us, u32 accuracyUs = 0 );	
	static inline void do_yield( IThread* to );   
	static inline void do_suspend();
	
	// static inline void do_exit( int exitcode ) throw (ThreadExitException) NORETURN;   
};

// Must provide impl
// inline IThread::~IThread() throw() {}

}} // namespace

/**
 * Inline functions, only when not included by Thread.h
 */
#ifndef THREAD_H 
 
#include "Thread.h"     // need sizeof(Thread) !
#include "Devices/Processor/Processor.h"

namespace MyOS { namespace MultiThreading {

// Does not seem to work (and better to hide this from interface)
//inline /* static */ IThread& IThread::current() {
//   return * (IThread*) (((u32) Processor::ESP() | (_4KB-1))+1-sizeof(Thread));
//}

inline /* static */ bool IThread::do_usSleep( u32 us, u32 usAccuracy ) {
   return Thread::currentThread().usSleep(us,usAccuracy);
}

inline /* static */ void IThread::do_yield( IThread* to ) {
   Thread::currentThread().yield(to);
}

inline /* static */ void IThread::do_suspend() {
   Thread::currentThread().suspend();
}

/*
inline void IThread::do_exit( int exitcode ) 
   throw (ThreadExitException) // NORETURN
{
   Thread::currentThread().exit(exitcode);
}
*/


}} // namespace

#endif   // ifndef THREAD_H

#endif
