/***************************************************************************
                          SyncPoint.h  -  description
                             -------------------
    begin                : Sun Oct 7 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef SYNCPOINT_H
#define SYNCPOINT_H

#include "Atomic/atomic32.h"

namespace MyOS {
namespace MultiThreading {
   
class Thread;
   
namespace Synchronization {   

/**
 *	A synchronization class for the current threads, with timeout
 *
 *	@note Design decision: signal is lost when there is no thread waiting. There 
 * 	could be a counter for the number of times signal was called, but then there 
 * 	is a risk of getting obscure behaviour ("synchronizing" on a signal that was 
 *  sent a long time ago)
 *
 *	Besides, with timeouts the waiting thread will eventually resume itself
 * 
 *  29/1/2005: The use of syncpoints is discouraged from an design point of view
 *  		   because they work badly in IRQ contexts and hold unnescessary
 *             stack space while waiting. Schedule a timer function instead, and
 * 			   restructure the code to continue in that function instead of 
 *             after the syncpoint.waitUs(...) call
 * 
 * @todo MultiSyncPoint where list of threads can be waiting
 */
class CSyncPoint
{
public:
	inline CSyncPoint() : waiter(0) {}

	/*
	 * 	Wait for at most <millis> milliseconds, returns true if signal() was called
	 *	Will wait indefinitely iff microsecs==0
	 * 
	 * @param microsecs - Number of us to wait
	 * @return true if signal() was caught before timeout
	 */
	bool waitUs( u32 microsecs );

	/// Resume the waiting thread, is lost when there is none
	bool signal();

   /// Usable from an IRQ context: does not switch, but returns the waiting thread to be awakened
   inline Thread* signalIRQ() {
      return (Thread*) waiter.Replace(0);
   }

private:
	ATOMIC32<Thread*> waiter;		// the thread waiting for the signal
};

}}}	// namespace

#endif
