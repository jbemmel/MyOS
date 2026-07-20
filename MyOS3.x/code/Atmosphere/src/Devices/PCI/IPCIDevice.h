#ifndef IPCIDevice_H
#define IPCIDevice_H

#include "Devices/IDevice.h"
#include "memtypes.h"            // linadr_t
#include "pci_ids.h"             // E_PCI_ID
#include "PCIRegister.h"         // PCIDeviceHandler

namespace MyOS { 
namespace Context { class IContext; }
namespace Drivers { namespace PCI {

typedef u16 PCIVendor_t;
typedef u16 PCIDeviceID_t;
typedef u16 PCIClass_t;

using MM::linadr_t;
using Devices::IDevice;

class CPCIDriver;

struct PCIDeviceInfo_t {
    PCIDeviceInfo_t( PCIDeviceHandle h ) : handle(h) {}

    const PCIDeviceHandle handle;

    struct PCIID {
        PCIVendor_t vendor;
        PCIDeviceID_t device;
        
        operator u32() const { return ((u32) vendor << 16) | (u16) device; }
    } id PACKED;

    PCIClass_t classID;
    u8 revision;

   // the following are only filled in once initDevice() has been called...
   u8 assignedIRQ;

   linadr_t MMIOstart;     ///< linear address of MMIO for device
   size_t MMIOsize;        ///< Size of the MMIO region in bytes

   u16 IObase, IOsize;     ///< baseAddress of port IO, if detected
} PACKED;

SUB_INTERFACE( IPCIDevice, IDevice, 
   	  "1f3184c3-e99a-44c4-afed-922e6b1ff94b" )


virtual  myos_result_t getID(
   IO::OStream& out   
   ) const = 0;

virtual  myos_result_t getCapabilities(
   IO::OStream& out   
   ) const = 0;

virtual  myos_result_t hasDriver(
   IO::OStream& out   
   ) const = 0;
   
// TODO: Perhaps refactor, need more methods ?
virtual const PCIDeviceInfo_t& getInfo() const = 0;


};
   
}   
}} // namespaces
#endif
