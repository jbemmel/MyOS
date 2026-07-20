#ifndef USBCONTROLPIPE_H
#define USBCONTROLPIPE_H

#include "USBEndpoint.h"
#include "types.h"

namespace MyOS { namespace Devices { namespace USB {

class IUSBRootHubImpl;
class IUSBDeviceImpl;
struct USB_DEV_DESCRIPTOR;

/**
 * All USB devices must support endpoint(0) for CONTROL messages
 */  
class USBControlPipe : public USBEndpoint
{
public:
    USBControlPipe( IUSBDeviceImpl& d ) : USBEndpoint(0), dev(d) {} 
    bool setAddress( IUSBRootHubImpl& rh, u8 adr );
    bool getDescriptor( IUSBRootHubImpl& rh, USB_DEV_DESCRIPTOR& d ) const;
    
private:
    IUSBDeviceImpl& dev;
};  
  
}}} // namespace

#endif
