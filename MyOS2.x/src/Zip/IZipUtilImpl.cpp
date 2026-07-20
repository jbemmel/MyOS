#include "IZipUtilImpl.h"

#include "ZipContext.h"
#include "Zip/ZipComponent.h"

namespace MyOS { namespace Zip {

IZipUtilImpl::IZipUtilImpl( MyOS::Core::IComponent& c )
: IZipUtil( c, VERSION(1,0) )
{

}

bool 
IZipUtilImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
 ZipContext::initialize();    // repeat this, since BSS is cleared after boot
 return true;
}

void 
IZipUtilImpl::deinit( IContext& context )
{

}

// private helper
inline u8*
IZipUtilImpl::allocate( size_t s ) const {
   //return (u8*) ZipComponent::getInstance().iVirtualMemory->allocate( 
   //   ZipComponent::getInstance(), s 
   //);
   return 0;   // TODO
}

inline void
IZipUtilImpl::free( u8* mem, size_t s ) const {
   // if (mem) ZipComponent::getInstance().iVirtualMemory->free( mem, s );
}

// virtual 
myos_result_t
IZipUtilImpl::unzip( const buffer& zipped, buffer& result ) const {
   ZipHeader header;
   E_RESULT r = header.parse(zipped);
   if ( r == E_OK ) {
      // allocate memory for result
      size_t s = header.getUnzippedSize();
      u8* resultmem = allocate(s);

      // verify allocation
      if (resultmem) {
         result = buffer( resultmem, s );
         r = doInflate( header.getData(), result );
         if ((r&E_ERROR)==0) return E_MYOS_SUCCESS;
      } else r = E_NOMEM;

      // failed, free mem
      free( resultmem, s );   // ok to deallocate 0, checked above
      result = buffer(0,0);
    }
    return E_MYOS_E_FAIL;
}

// private helper function
E_RESULT
IZipUtilImpl::doInflate( const buffer& data, buffer& result ) const
{
   // allocate temporary memory, @todo this could be on demand in ZipContext
   u8* temp = allocate( _32KB ); // can be bigger
   u8* win  = allocate( _32KB );

   E_RESULT r = E_NOMEM;
   if (temp && win) {
      // PRINTF( "\ndoInflate win=%x temp=%x result=%x", win, temp, result.getData() );
      buffer win32k( win, _32KB );
      ZipContext ctxt( data, win32k, temp, temp+_32KB );
      r = ctxt.inflate(result);
   }

   // ok to free 0 ptr
   free( win, _32KB );
   free( temp, _32KB );
   return r;
}

}}  // namespaces
