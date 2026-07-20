#ifndef IPCISUPPORT_H
#define IPCISUPPORT_H

#include "Core/IInterface.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "pci_ids.h"	// for clients

namespace MyOS {
namespace Drivers {
namespace PCI {

using InterruptHandling::IInterruptContext;

// forward declarations
struct PCIDeviceInfo_t;
class CPCIDriver;
class IPCIDevice;
class IUnhandledPCIDevice;

/// Flags used for enable()
enum ENABLEFLAGS {
	PCI_USE_MMIO   = 0x1,
	PCI_USE_PORTIO = 0x2,
	PCI_USE_BOTH   = PCI_USE_MMIO | PCI_USE_PORTIO,
	PCI_MASTER     = 0x4,

	PCI_POWER_D0   = 0x8,	/// If set, PCI support must fire up device
	PCI_ENABLE_MWI = 0x10	/// If set, PCI support will try to enable Memory Write & Invalidate
};

/**
 *  PCI support functions that can be used by PCI device drivers
 */
class IPCISupport /* : public IInterface */
{
// protected:  
//   IPCISupport( MyOS::Core::IComponent& c, VERSION v ) : IInterface(c,v) {}
   
public:
   typedef u32 PCIClass_t;

   /// Query for unhandled devices. The driver will be given devices to handle
   virtual bool queryDevices( CPCIDriver &driver, PCIClass_t pciclass ) = 0;
      
   /// Enable the PCI device, allocates resources, updates info
   virtual bool enableDevice( IPCIDevice &device, u32 flags ) = 0;

   /// Enables IRQ handling for the device. Separate since some devices require
   /// to be enabled for configuration!
   virtual bool enableIRQ( IPCIDevice &device, IInterruptContext& h ) const = 0;
   
   /// Return a device handled by the driver
   virtual void releaseDevice( IPCIDevice &device ) = 0;
   
   /// Query for unhandled devices
   // virtual IUnhandledPCIDevice* getUnhandledDevice( u32 i ) const = 0;
};

}}}	// namespace

#endif
