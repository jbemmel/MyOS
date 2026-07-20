#ifdef INCLUDE_PHASE
#include "MM/IVirtualMemory.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "Timer/ITiming.h"

namespace MyOS { namespace Devices { namespace Floppy {
	class IFloppy;
}}}

namespace MyOS { namespace Drivers { namespace Floppy { namespace Default {
	using MM::IVirtualMemory;
	using InterruptHandling::IInterruptHandling;
	
	using Devices::Floppy::IFloppy;
	using Timer::ITiming;
}}}}

#endif
