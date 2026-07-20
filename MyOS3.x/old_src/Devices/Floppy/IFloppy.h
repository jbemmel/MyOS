#ifndef IFloppy_H
#define IFloppy_H

#include "Devices/IBlockDevice.h"

namespace MyOS { namespace Devices { namespace Floppy {
   SUB_INTERFACE( IFloppy, IBlockDevice, 
   	  "413af5fe-9d0e-4b3c-a6cb-ecf23e0b5cfd" )


virtual  myos_result_t getLabel(
   IO::OStream& out   
   ) const = 0;
   
   };
   
}   
}} // namespaces
#endif
