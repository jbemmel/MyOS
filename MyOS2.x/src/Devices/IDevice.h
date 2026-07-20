#ifndef IDEVICE_H
#define IDEVICE_H

#include "types.h"
#include "myosresult.h"

#include "Core/IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS { 
  
class buffer;

namespace Devices {

class IOCompletionHandler;
class IORequest;

/// General interface for devices
class IDevice : public IInterface
{
	char id[4];     // important that storage remains valid...
protected:
	inline IDevice( Core::IComponent& c, VERSION v ) : IInterface(c,v) {}
   
public:
	char* setId( int i ) { *(int*)id=i; return id; }

   /// Opens the device, here ?
   // virtual myos_result_t open( ) = 0;
   // virtual void close( ) = 0;
};
 
}} // namespace

#endif
