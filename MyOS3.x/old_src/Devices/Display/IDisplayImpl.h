#ifndef IDisplayImpl_H
#define IDisplayImpl_H

#include "Devices/Display/IDisplay.h"

#include "ScreenBuffer.h"
#include "ScreenStream.h"

namespace MyOS { 

namespace Context { class IContext; }

namespace Devices { namespace Display {

   class IDisplayImpl : public IDisplay {
public:
   IDisplayImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   // IDisplay
   virtual  myos_result_t clear() ;
   virtual  myos_result_t print( IO::IStream& in ) ;

   virtual IO::OStream& cout() { return coutstream; }
   inline ScreenStream& cerr() { return cerrstream; }

private:
   ScreenBuffer theScreenBuffer;

    /// @todo These should eventually refer to the same memory...
   ScreenStream coutstream;
   ScreenStream cerrstream;  
};
   
      
}}} // namespaces

#endif
