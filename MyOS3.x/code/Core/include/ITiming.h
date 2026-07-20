#ifndef ITiming_H
#define ITiming_H

#include "IInterface.h"

using MyOS::Core::IInterface;

#include "timetypes.h"

namespace MyOS {
namespace Timer {

/**
 * Provides a timer facility.
 * 
 * Storage for timers is provided by clients of this interface. The timer facility maintains 
 * an internal heap of pending timers, which has a fixed maximum capacity
 * 
 * @todo better name: ITimerFactory
 */
INTERFACE( ITimerFacility, "cd5a76a0-bf79-4750-8356-9f8a0627465a" )

    /**
     * Returns the current time in the given units
     * 
     * @note Could be placed in a different interface, its unrelated
     */
	virtual u32 now(ETimeUnit units) = 0;

    /**
     * Initializes a new timer
     * @param timer  - timer to initialize
     * @param target - callback handler
     */
	virtual ITimer& createTimer( ITimerTarget& target ) = 0;
};

}
} // namespaces
#endif
