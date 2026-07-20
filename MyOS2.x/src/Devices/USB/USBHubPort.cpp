#include "USBHubPort.h"
#include "USBIO.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace USB {

bool 
USBHubPort::reset( IDriverManager& dm )
{
    // see Linux uhci-hub.c
    write( (read() & 0xFFF5) | 0x0200 );
    dm.usDelay( 50000 ); // USB v1.1 7.1.7.3
    write( (read() & 0xFFF5) & ~0x0200 );
    dm.usDelay( 10 ); 
    return true;    // XX check status ?
}

/**
 * Checks if a device is attached, if yes enables this port
 */    
u16 
USBHubPort::detectAndEnable( IDriverManager& dm )
{
    u16 status = read();
    if ( status & E_DEV_PRESENT ) {
        write((status & 0xFFF5) | E_ENABLED );
        dm.usDelay( 10000 );
        write( (read() & 0xFFF5) | E_CONN_CHANGED | E_ENA_CHANGED );  // clear 
        dm.usDelay( 100 );   // add this? test if this is better
        return status;
    } 
    return 0;
}

/**
 * Called by the hub when a resume bit is detected as status
 */
void 
USBHubPort::onResumeReceived()
{
 
}
    
bool 
USBHubPort::suspend()
{
	return false;
}

bool 
USBHubPort::resume()
{
	return false;	
}

  
}}} // namespace
