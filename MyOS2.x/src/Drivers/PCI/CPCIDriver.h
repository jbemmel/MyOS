/***************************************************************************
                          PCIDriver.h  -  description
                             -------------------
    begin                : Mon Aug 20 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef IPCIDRIVER_H
#define IPCIDRIVER_H

#include "../CDriverBase.h"

namespace MyOS {
namespace Drivers {
namespace PCI {

class IUnhandledPCIDevice;
class IPCIDevice;

/**
 *  Interface for PCI device drivers
 *
 *  Drivers are responsible for detecting 0 or more devices they support in the 
 *  system, and managing these devices: initialize, ...
 */
class CPCIDriver : public CDriverBase
{
protected:
   inline CPCIDriver( IContainer& container, VERSION v, IComponent*& instance ) 
      throw() : CDriverBase(container,v,instance) {}

public:
   /// callback, called by PCISupport when the driver registers
   virtual bool handleDevice( IPCIDevice &dev ) = 0;

   // virtual IPCIDevice* getDevice( ID id ) const = 0; use central store

   /// Used for directory listing
   // virtual const char* getDriverName() const = 0;
};

}}}	// namespace

#endif

