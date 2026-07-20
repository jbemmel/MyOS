#ifndef IFATFS_H
#define IFATFS_H

#include "Core/IInterface.h"
#include "FS/IMountedFS.h"

using MyOS::Core::IInterface;



namespace MyOS { namespace FS { namespace FAT {
   INTERFACE( IFATFS, "0c6ad02d-8042-4a4d-ba4f-d336289ccc3b" )

   // @return a handle to a mounted FS instance, to be unmounted when done
   virtual IMountedFS& mount( Devices::IBlockDevice& toDevice ) 
       throw (FSException) = 0;


   };
   
}   
}} // namespaces
#endif
