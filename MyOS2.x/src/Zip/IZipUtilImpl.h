#ifndef IZipUtilImpl_H
#define IZipUtilImpl_H

#include "Zip/IZipUtil.h"
#include "ZipHeader.h"     // E_RESULT

// for ZipComponent.h
#include "MM/IVirtualMemory.h"
using MyOS::MM::IVirtualMemory;

namespace MyOS { namespace Zip {

   class IZipUtilImpl : public IZipUtil {
public:
   IZipUtilImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   // IZipUtil
   virtual  myos_result_t unzip( const buffer& zipped, buffer& result ) const;

private:
   // separate mainly since component compilation fails to allocate registers
   E_RESULT doInflate( const buffer& data, buffer& result ) const;
   inline u8* allocate( size_t s ) const;
   inline void free( u8* mem, size_t s ) const;   
};

         
}} // namespaces
#endif
