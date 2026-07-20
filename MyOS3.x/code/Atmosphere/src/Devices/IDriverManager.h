#ifndef IDriverManager_H
#define IDriverManager_H
#include "IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS { 
namespace Devices { class IDevice; }
namespace Drivers {

class CDriverBase;
namespace PCI { class IPCISupport; }

enum myos_device_class {
    E_DEV_UNKNOWN,
    E_DEV_FDD,
    E_DEV_HDD,
    E_DEV_ETH,
    E_DEV_KEYBOARD,
    E_DEV_UHCI,
    
    E_DEV_PCI,  // unhandled generic PCI device
    // etc
};

INTERFACE( IDriverManager, "adf79a36-5871-11d6-858d-0010a708e02f" )

virtual myos_result_t listInstalledDrivers( IO::OStream& out ) const = 0;

virtual bool addDriver( CDriverBase& driver ) = 0;
virtual bool removeDriver( CDriverBase& driver ) = 0;

// methods used by drivers, this creates an entry under /dev/
virtual int registerDevice( myos_device_class ofClass, Devices::IDevice& dev ) = 0;

// Used for PCI devices (called from PCIDriver::detectDevices)
virtual PCI::IPCISupport& getPCISupport() = 0;

// Helper function      
virtual void usDelay( u32 us_count ) const = 0;

};
   
   
}} // namespaces
#endif
