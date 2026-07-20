#ifndef IBIOSImpl_H
#define IBIOSImpl_H

#include "BIOS/IBIOS.h"

#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"
#include "InterruptHandling/IInterruptHandling.h"

namespace MyOS { namespace BIOS {

   using Context::IContext;
   using namespace MM;
   using namespace InterruptHandling;

class IBIOSImpl : public IBIOS {
   
   static void makeCall( u8 intNo, VM86REGS& inout, u32* vm86stack ) ASMNAME( "VM86MakeCall" );   
public:
   IBIOSImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual  myos_result_t vm86int( u8 i, VM86REGS& regs );   
};
   

}} // namespaces
#endif
