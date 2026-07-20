#ifndef USBHUBPORT_H
#define USBHUBPORT_H

#include "../IOPort.h"
#include "Drivers/IDriverManager.h"

namespace MyOS { namespace Devices { namespace USB {

using Drivers::IDriverManager;

class IUSBDeviceImpl;

/**
 * Detected and created by USBHub
 */
class USBHubPort
{
    int ioPort;             // USB-IObase + 1, 2, ... 
    IUSBDeviceImpl* device; // When attached, this points to the device
 
public:
    inline USBHubPort() : ioPort(-1), device(0) {}
    void init( int io ) { ioPort = io; }
    
    bool reset( IDriverManager& dm );

    /**
     * Checks if a device is attached, if yes enables this port
     * @return statusbits
     */    
    u16 detectAndEnable( IDriverManager& dm );

    /**
     * When the hub has discovered details about the device, the port is notified
     */
    inline void setDevice( IUSBDeviceImpl& d ) { device = &d; } 
    
    /**
     * Called by the hub when a resume bit is detected as status
     */
    void onResumeReceived();
    
    bool suspend();
    bool resume();  // only after suspend
    
private:
    // Reads this USB port
    inline u16 read() {
        return inw( ioPort );
    }

    inline void write( u16 bits ) {
      outw( ioPort, bits );
    }
    
};  
  
}}} // namespace

#endif
