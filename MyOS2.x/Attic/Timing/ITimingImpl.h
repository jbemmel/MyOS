#ifndef ITimingImpl_H
#define ITimingImpl_H

#include "Timing/ITiming.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "MM/IVirtualMemory.h"

#include "Timer.h"

// testing..
#include "APICTimer.h"

namespace MyOS { namespace Timing {

   using Context::IContext;
   using namespace InterruptHandling;
   using MM::IVirtualMemory;

   class TimerEvent;

class ITimingImpl : public ITiming 
{
public:
   ITimingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual myos_result_t get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;
   virtual myos_result_t put( const char* const call, NVPAIR parameters[], IO::IStream&  input );

   virtual  myos_result_t scheduleCallback( TimerEvent& e, u32 afterUs ) ;

private:
   Timer theTimer;
   APICTimer apicTimer;   
};
         
}} // namespaces
#endif
