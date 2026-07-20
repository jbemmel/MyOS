#ifndef IBIOSImpl_H
#define IBIOSImpl_H

#include "BIOS/IBIOS.h"
#include "interfaces.h"

namespace MyOS { namespace BIOS {

class IBIOSImpl : public IBIOS {
   
   static void makeCall( u8 intNo, VM86REGS& inout, u32* vm86stack ) ASMNAME( "VM86MakeCall" );
   
public:
   IBIOSImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   // IBIOS API
   virtual linadr_t vm86map( linadr_t mem, size_t bytes );
   virtual void vm86unmap( linadr_t vm86mem, size_t bytes );
   virtual myos_result_t vm86int( u8 i, VM86REGS& regs );   
};
   

}} // namespaces
#endif
