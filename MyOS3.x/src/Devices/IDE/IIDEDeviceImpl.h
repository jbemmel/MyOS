#ifndef IIDEDeviceImpl_H
#define IIDEDeviceImpl_H

#include "Devices/IDE/IIDEDevice.h"
#include "Atomic/Queue2.h"

namespace MyOS {    
namespace Devices {
namespace IDE {

using Context::IContext;

class IDEController;

class IIDEDeviceImpl : public IIDEDevice 
{
protected: 
   IDEController& controller;
   Queue requests;   // Separate queue for MASTER and SLAVE devices

   u8 masterSlave;   // opaque value from controller, pass in methods
 
public:
   IIDEDeviceImpl( IDEController& c, u8 masterSlave ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   // XML, implemented in subclasses
   virtual myos_result_t getVolumeLabel( IO::OStream& out ) const = 0;

   // Called by controller
   virtual void onSeekDone( IORequest& rq ) = 0;
   virtual void onReadDone( IORequest& rq ) = 0;
   virtual void process( IORequest& rq ) = 0;

   inline bool hasWork() const { return !requests.isEmpty(); }
   inline IORequest* nextRq()  { return (IORequest*) requests.dequeue(); }
   
};
   
}}} // namespaces

#endif
