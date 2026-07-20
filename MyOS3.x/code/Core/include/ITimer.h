#ifndef ITimer_H
#define ITimer_H

#include "IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS {
namespace Timer {

/**
 * Interface for timer objects 
 */
INTERFACE( ITimer, "e89b1a98-9992-4e63-9c49-fcff56b37e9d" )

	/**
	 * (re)starts this timer
	 * @param afterUs - period(us) to wait before firing
	 */
	virtual myos_result_t start( u32 afterUs ) = 0;

	/**
	 * Cancels this timer (no-op when not running)
	 */
	virtual myos_result_t cancel() = 0;

	/**
	 * Triggers this timer, causing a call to ITimerTarget#onTimer (as a softIRQ(0)).
	 * If the timer was set, it is now cancelled
	 */
	// virtual myos_result_t trigger() = 0;

	/**
	 * Releases the resources associated with this timer
	 * 
	 * @note The timer is cancelled if still running
	 */
	virtual myos_result_t free() = 0;

	/**
	 * Operator == for equality testing
	 */
	inline bool operator ==( const ITimer& t ) const {
	    return this==&t;
	}
};

}
} // namespaces
#endif
