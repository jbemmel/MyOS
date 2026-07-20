#ifndef IDriverManagerImpl_H
#define IDriverManagerImpl_H

#include "IDriverManager.h"
#include "../Repository/IDirectory.h"
#include "IInterruptHandling.h"
#include "PCI/IPCISupportImpl.h"
#include "IVirtualMemory.h"

namespace MyOS { namespace Drivers {

   using Context::IContext;
   using Context::IDirectory;
   using InterruptHandling::IInterruptHandling;
   using MM::IVirtualMemory;

   class IDriverManagerImpl : public IDriverManager {
public:
   IDriverManagerImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

	virtual  myos_result_t listInstalledDrivers( IO::OStream& out ) const;
	virtual bool addDriver( CDriverBase& driver );
    virtual bool removeDriver( CDriverBase& driver );  
    
    virtual int registerDevice( myos_device_class ofClass, Devices::IDevice& dev );

    virtual PCI::IPCISupport& getPCISupport() { return pciSupport; }
    
    // helper
    virtual void usDelay( u32 us_count ) const;
private:
    PCI::IPCISupportImpl pciSupport;
    IDirectory* devices;
};
   

}} // namespaces
#endif
