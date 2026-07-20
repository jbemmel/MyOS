#ifndef IISO9660FS_H
#define IISO9660FS_H

#include "Core/IInterface.h"
#include "FS/IMountedFS.h"

using MyOS::Core::IInterface;

namespace MyOS { namespace FS { namespace ISO9660 {
   INTERFACE( IISO9660FS, "357cf5b1-0537-4164-ad4b-39896a5b0306" )


   // @return a handle to a mounted FS instance, to be unmounted when done
   virtual IMountedFS& mount( Devices::IBlockDevice& toDevice ) 
       throw (FSException) = 0;

   };
   
}   
}} // namespaces
#endif
