#ifndef USBDEVICEIMPL_H
#define USBDEVICEIMPL_H

#include "Devices/USB/IUSBDevice.h"
#include "USBControlPipe.h"
#include "USBIO.h"

namespace MyOS { namespace Devices { namespace USB {

class IUSBRootHubImpl;
  
class IUSBDeviceImpl : public IUSBDevice
{
public:
    void* operator new( size_t );
    void operator delete( void* );
    IUSBDeviceImpl( IUSBRootHubImpl& roothub, int address, bool lowSpeed );

    int getAddress() const { return address; }
    bool isLowSpeed() const { return lowSpeed; }
    u8 getMaxControlPacketSize() const { return m_desc.maxControlPacketSize; }

private:
    IUSBRootHubImpl& roothub;
    int address;

    // Always at least 1 endpoint(0)
    USBControlPipe control;

    USB_DEV_DESCRIPTOR m_desc;
    
    bool lowSpeed;  // from port status bit
};  
  
}}} // namespace

#endif
