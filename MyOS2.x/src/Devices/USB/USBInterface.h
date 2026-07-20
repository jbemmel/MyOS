#ifndef USBINTERFACE_H
#define USBINTERFACE_H

#include "types.h"

namespace MyOS { namespace Devices { namespace USB {

class USBEndpoint;

/**
 * A USB configuration consists of 1 or more interfaces
 * 
 */  
class USBInterface
{
public:
    USBInterface( u8* configDescriptor, size_t length );
     
private:
    #define MAX_ENDPOINTS 4

    // An interface consists of 1 or more endpoints
    USBEndpoint* endpoints[ MAX_ENDPOINTS ]; 
};  
  
}}} // namespace

#endif
