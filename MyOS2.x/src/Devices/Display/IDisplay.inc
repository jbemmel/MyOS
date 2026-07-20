#ifdef INCLUDE_PHASE

namespace MyOS { namespace Devices { namespace Display {
	class IDisplay;
}}}
	
namespace MyOS { namespace Drivers { namespace Display { namespace Default {
	using Devices::Display::IDisplay;
}}}}
	
#else
	virtual IO::OStream& cout() = 0;
#endif
