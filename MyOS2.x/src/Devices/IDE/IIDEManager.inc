#ifdef INCLUDE_PHASE
namespace MyOS { namespace Devices { namespace IDE {
	class IIDEManager;
	class IIDEDevice;
}}}	

namespace MyOS { namespace Drivers { namespace IDE { namespace Default {
	using Devices::IDE::IIDEManager;
}}}}

#else
	virtual bool getDevice( u32 id, IIDEDevice*& dev ) const = 0;	
#endif
