#ifndef IPCIDeviceImpl_H
#define IPCIDeviceImpl_H

#include "Drivers/PCI/IPCIDevice.h"
#include "Drivers/PCI/PCIRegister.h"    // PCIDeviceHandle
#include "Drivers/PCI/PCIConfig.h"      // 
#include "Drivers/PCI/PCI_RESOURCE_INFO.h"
#include "DRINGLIST.h"

#include "MM/IVirtualMemory.h"

namespace MyOS { namespace Drivers { namespace PCI {

   using Context::IContext;

   class IPCIDeviceImpl : public IPCIDevice {
public:
   IPCIDeviceImpl( E_PCI_ID vendor, PCIDeviceHandle h ) INITSECTION;

   // bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   // void deinit( IContext& context );

    // IPCIDevice
    virtual  myos_result_t getID( IO::OStream& out ) const;
    virtual  myos_result_t getCapabilities( IO::OStream& out ) const;
    virtual  myos_result_t hasDriver( IO::OStream& out ) const;
    virtual const PCIDeviceInfo_t& getInfo() const { return info; }

// added after template
   
    inline E_PCI_ID getVendorID() const     { return (E_PCI_ID) info.id.vendor; }
    inline E_PCI_ID getDeviceID() const     { return (E_PCI_ID) info.id.device; }

    inline linadr_t getMMIOAddress() const     { return info.MMIOstart; }
    inline E_PCI_CLASS getClass() const        { return (E_PCI_CLASS) info.classID; }
    inline PCIDeviceHandle getHandle() const    { return info.handle; }

    bool enable( MM::IVirtualMemory& mem, u32 flags );
    void releaseResources( MM::IVirtualMemory& mem );
    bool setPowerState( PCI_PM_CONTROL::PCI_POWERSTATE ps );

    // for resource handling
    inline PCI_RESOURCE_INFO& getResources()        { return resources; }
    inline const PCIDeviceInfo_t& getDeviceInfo() const   { return info; }
    inline void setDriver( CPCIDriver &d )      { driver=&d; }

   // IInterface override for getInfo()
   // virtual myos_result_t get(
   //  myos_call_t call, NVPAIR parameters[], IO::OStream& output ) const;

protected:   
    u8 findCapability( PCI_CAPABILITY t ) const;

    PCIDeviceInfo_t info;

    /// from PCIConfig
    PCI_HEADER_TYPE header_type;
    PCI_RESOURCE_INFO resources;

    /// pointer to implementation driver, if any
    CPCIDriver *driver;

    /// For forming ring lists
    friend class IPCISupportImpl;
    DRINGLIST<IPCIDeviceImpl>::Node ringnode;    
};
   
}}} // namespaces
#endif
