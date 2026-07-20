#ifndef IIDEDevice_H
#define IIDEDevice_H

#include "Devices/IBlockDevice.h"

namespace MyOS { namespace Devices { namespace IDE {
   SUB_INTERFACE( IIDEDevice, IBlockDevice, 
   	  "95089540-5aad-462b-8596-d75a0c244ce8" )


virtual  myos_result_t getVolumeLabel(
   IO::OStream& out   
   ) const = 0;
   
   };
   
}   
}} // namespaces
#endif
