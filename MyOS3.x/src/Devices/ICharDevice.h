#ifndef ICHARDEVICE_H
#define ICHARDEVICE_H

#include "IDevice.h"

namespace MyOS { namespace Devices {

/// General interface for character devices
class ICharDevice : public IDevice
{
protected:
   inline ICharDevice( Core::IComponent& c, VERSION v, const char* const u, const char* const n ) 
       : IDevice(c,v,u,n) {}

public:

   /// Starts non-blocking read of data ? for single chars ?
   // virtual myos_result_t read( char& data ) = 0;

   /// Starts non-blocking write of data (implies that data is copied...)
   // virtual myos_result_t write( char data ) = 0;   
};

}} // namespace

#endif
