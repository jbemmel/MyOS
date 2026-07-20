#include "IUSBDeviceImpl.h"
#include "IUSBRootHubImpl.h"
#include "Drivers/USB/Standard/UHCIDriver.h"

#include "MultiThreading/IThread.h"

namespace MyOS { namespace Devices { namespace USB {

using Drivers::USB::Standard::UHCIDriver;

//    void* operator new( size_t );
//    void operator delete( void* );

IUSBDeviceImpl::IUSBDeviceImpl( IUSBRootHubImpl& rh, int adr, bool ls )
: IUSBDevice( UHCIDriver::getInstance(), VERSION(1,0) ), roothub(rh)
, address(0), control(*this), lowSpeed(ls)
{
    // perform a getDescriptor at adr 0 first, some devices (e.g. ZyDAS 1201) require this
    // And also, first obtain the max. packet size from the descriptor

    memset_aligned( &m_desc, 0, sizeof(USB_DEV_DESCRIPTOR) );
    m_desc.maxControlPacketSize = 64;    // maximum
    if (control.getDescriptor( roothub, m_desc )) {
       
        // if max control size is less than sizeof(desc), do it again
        // now m_desc.maxControlPacketSize is properly set
        if ( m_desc.maxControlPacketSize < sizeof(USB_DEV_DESCRIPTOR) ) {
            control.getDescriptor( roothub, m_desc );
        }
        
        // do this again, since not all device seem happy to provide all data
        // when the packet size used is less than what they are capable of ??
        // control.getDescriptor( roothub, m_desc );
        
        if (control.setAddress( roothub, adr) ) {
            this->address = adr;
            
            DPRINTK( "\nGetConfig: len=%x type=%x", m_desc.length, m_desc.type);
            DPRINTK( "\nGetConfig: usb=%x max_ctrl=%d", m_desc.usbVersion, m_desc.maxControlPacketSize );
            DPRINTK( "\nGetConfig: vendor=%x device=%x", m_desc.vendorId, m_desc.productId );
            DPRINTK( " configs=%d class=%x", m_desc.configCount, m_desc.classId );          
        } else {
            DPRINTK( "\nSetAddress failed" );
        }
    } else DPRINTK( "\nGetDescriptor failed" ); 
}
  
}}} // namespace
