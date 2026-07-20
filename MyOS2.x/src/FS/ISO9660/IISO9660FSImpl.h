#ifndef IISO9660FSImpl_H
#define IISO9660FSImpl_H

#include "FS/ISO9660/IISO9660FS.h"
#include "MM/IVirtualMemory.h"
#include "MM/IDemandPaging.h"

namespace MyOS { namespace FS { namespace ISO9660 {

   using Context::IContext;
   using MM::IVirtualMemory;
   using MM::IDemandPaging;
   
class IISO9660FSImpl : public IISO9660FS 
{
public:
   IISO9660FSImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   // .inc
   virtual IMountedFS& mount( Devices::IBlockDevice& toDevice ) throw (FSException) ;
      
};
   
}
}} // namespaces
#endif
