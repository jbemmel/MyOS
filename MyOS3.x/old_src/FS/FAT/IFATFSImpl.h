#ifndef IFATFSImpl_H
#define IFATFSImpl_H

#include "FS/FAT/IFATFS.h"
#include "debug.h"
#include "MM/IVirtualMemory.h"
#include "MM/IDemandPaging.h"

namespace MyOS { namespace FS { namespace FAT {

   using Context::IContext;
   using MM::IVirtualMemory;
   using MM::Paging::IDemandPaging;

class FATReader;   

class IFATFSImpl : public IFATFS {
public:
   IFATFSImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   // .inc
   virtual IMountedFS& mount( Devices::IBlockDevice& toDevice ) throw (FSException);
};


}
}} // namespaces
#endif
