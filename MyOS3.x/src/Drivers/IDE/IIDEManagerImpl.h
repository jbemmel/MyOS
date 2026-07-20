#ifndef IIDEManagerImpl_H
#define IIDEManagerImpl_H

// for component
#include "MM/IVirtualMemory.h"
#include "MultiThreading/IMultiThreading.h"
#include "InterruptHandling/IInterruptHandling.h"

#include "Devices/IDE/IIDEManager.h"
#include "Devices/IDE/IDEController.h"

namespace MyOS { namespace Devices { namespace IDE {
	// fwd, needed for friend declaration
	class ATAPIDevice;
}}}

namespace MyOS { namespace Drivers { namespace IDE {

	using Context::IContext;
	using namespace MM;
	using namespace MultiThreading;
	using namespace InterruptHandling;

	using namespace MyOS::Devices::IDE;

class IIDEManagerImpl : public IIDEManager 
{   
public:
   IIDEManagerImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual bool getDevice( u32 id, IIDEDevice*& dev ) const;
   
   // XML
   virtual  myos_result_t listDevices( IO::OStream& out ) const;
   
private:
   IDEController primary, secondary;      
};
   
}
}} // namespaces
#endif
