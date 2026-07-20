#ifndef USBDEVICECONFIG_H
#define USBDEVICECONFIG_H

#include "types.h"

namespace MyOS { namespace Devices { namespace USB {

class USBInterface;

/**
 * All USB devices report 1 or more configurations
 * 
 * This class is responsible for parsing and storing them
 */  
class USBDeviceConfig
{
public:
    USBDeviceConfig( u8* configDescriptor, size_t length ) {}
     
private:
    #define MAX_INTERFACES 4

    // A configuration consists of 1 or more interfaces
    USBInterface* interfaces[ MAX_INTERFACES ]; 
};  
  
}}} // namespace

#endif
