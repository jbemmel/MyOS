#ifndef USBENDPOINT_H
#define USBENDPOINT_H

namespace MyOS { namespace Devices { namespace USB {

struct USB_TD;

/**
 * Abstract base class for endpoints
 */  
class USBEndpoint
{
    int id; // endpoint id : [0..15], 0 is for CONTROL pipe
     
protected:
    USBEndpoint( int i ) : id(i) {}

private:
    
};  
  
}}} // namespace

#endif
