#ifndef IDEVICE_H
#define IDEVICE_H

#include "types.h"
#include "myosresult.h"
#include "string.h"

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
	char id[8];     // important that storage remains valid...
protected:
	inline IDevice( Core::IComponent& c, VERSION v, const char* const u, const char* const n ) 
	    : IInterface(c,v,u,n) {}
   
public:
	inline char* setId( const char* i ) { strncpy(id,i,sizeof(id)); id[7]=0; return id; }

   /// Opens the device, here ?
   // virtual myos_result_t open( ) = 0;
   // virtual void close( ) = 0;
};
 
}} // namespace

#endif
