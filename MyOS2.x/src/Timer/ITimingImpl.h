#ifndef ITimingImpl_H
#define ITimingImpl_H

#include "Timer/ITiming.h"

// for TimerComponent
#include "MM/IVirtualMemory.h"
#include "InterruptHandling/IInterruptHandling.h"

// implementation
#include "TimerImpl.h"

namespace MyOS { namespace Timer {

   using Context::IContext;
   using MM::IVirtualMemory;
   using InterruptHandling::IInterruptHandling;
   
   class ITimingImpl : public ITiming {
public:
   ITimingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

	virtual u32 now( ETimeUnit units );

	// Timer functions
	virtual  myos_result_t initTimer( timer_t& timer, ITimerTarget& callback );
	virtual  myos_result_t startTimer( timer_t& timer, u32 afterUs);
	virtual  myos_result_t cancelTimer( timer_t& timer );
	virtual  myos_result_t triggerTimer( timer_t& timer );	
	virtual  myos_result_t freeTimer( timer_t& timer );

private:
   TimerImpl timer;   
};
   
      
}} // namespaces
#endif
