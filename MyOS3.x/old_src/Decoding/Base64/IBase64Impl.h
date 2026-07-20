#ifndef IBase64Impl_H
#define IBase64Impl_H
#include "Decoding/Base64/IBase64.h"

#include "Devices/Display/IDisplay.h"
#include "MM/IVirtualMemory.h"
#include "MM/Virtual/ByteAllocator.h"

namespace MyOS { namespace Decoding { namespace Base64 {

   using Context::IContext;
   using Devices::Display::IDisplay;
   using MM::IVirtualMemory;

class IBase64Impl : public IBase64 
{
public:
   IBase64Impl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual  myos_result_t decode( IO::IStream& in ) ;

private:
   u8* decode64( char* src, u8* dest ) const;

   static const u8 decode64_0[128];

   // These should not be contained but allocated, else MyOSCore crashes
   // (it is stack allocated and contains this impl...)
   u16 *decode64_1;
   u32 *decode64_2;
   u32 *decode64_3;
};
   
}
}} // namespaces
#endif
