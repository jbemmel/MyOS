/***************************************************************************
                          SyncPoint.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "CSyncPoint.h"
#include "../IThread.h"

#include "debug.h"

// #include "SimpleLock.h"

namespace MyOS {
namespace MultiThreading {
namespace Synchronization {   

bool
CSyncPoint::waitUs( u32 microsecs )
{
    // ASSERTION( waiter==0, Debug::AS_FATAL );	// usually a sign of multiple wait() calls

    CHECKPOINT( 'w' );

	bool result = true;
	// SimpleLock l; SimpleLock::PROTECTED region(l);
	{
		(Thread*&) waiter = &Thread::currentThread();
		// usSleep==true iff timeout occurs
        if (((Thread*)waiter)->usSleep( microsecs )) {
            (u32&) waiter = 0;
			result = false;
		} // else waiter is cleared in signal()
	}
	return result;
}

bool
CSyncPoint::signal()
{
    if (Thread* w = (Thread*) waiter.Replace(0)) {
		// DPRINTK( "\nAbout to signal() thread %x", w );

        // take care of signal()-before-switch-completed
//        if (w == &Thread::getCurrent()) {
//            CHECKPOINT( 'U' );
//            w->blockCanceled();
//        } else {
//            CHECKPOINT( 's' );
//            w->unblock( /* switchImmediately ?? */ );
//        }
		return w->wakeup();
	} else {
      CHECKPOINT( 'X' );
      return false;
   }
}

}}}	// namespace
