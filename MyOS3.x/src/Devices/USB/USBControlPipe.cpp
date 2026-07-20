#include "USBControlPipe.h"
#include "IUSBRootHubImpl.h"
#include "IUSBDeviceImpl.h"
#include "USBIO.h"

namespace MyOS { namespace Devices { namespace USB {

bool
USBControlPipe::setAddress( IUSBRootHubImpl& rh, u8 adr )
{
    ASSERTION( dev.getAddress() == 0, E_ERROR );    // roothub will put adr in pkt
 
    USB_CONTROL_PACKET ctrl = {
        E_HOST_TO_DEV, 
        E_SET_ADDRESS,
        adr,
        0,
        0     
    };
    DPRINTK( "\nSending USB setAddress %X", adr );
    return rh.schedule( ctrl, dev );
}

bool 
USBControlPipe::getDescriptor( IUSBRootHubImpl& rh, USB_DEV_DESCRIPTOR& d ) const
{
    USB_CONTROL_PACKET ctrl = {
        E_DEV_TO_HOST, 
        E_GET_DESC,
        1<<8,   // type == device descriptor
        0,      // language
        sizeof(USB_DEV_DESCRIPTOR)     
    };   
    return rh.schedule ( ctrl, dev, &d, sizeof(USB_DEV_DESCRIPTOR) );
}
      
}}} // namespace
