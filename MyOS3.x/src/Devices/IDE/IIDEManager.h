#ifndef IIDEManager_H
#define IIDEManager_H

#include "Core/IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS { namespace Devices { namespace IDE {

class IIDEDevice;

INTERFACE( IIDEManager, "9abd60a9-6b81-4d69-bb19-92f5e88de4cc" )


    virtual  myos_result_t listDevices( IO::OStream& out ) const = 0;
    
    virtual bool getDevice( u32 id, IIDEDevice*& dev ) const = 0;
};
   
}   
}} // namespaces
#endif
