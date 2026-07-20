// TODO: Move TimeUtil code to this class, to avoid needless calling overhead

#include "ITimingImpl.h"
#include "TimeUtil.h"
#include "TimerEvent.h"

#include "Timer/TimerComponent.h"

namespace MyOS { namespace Timer {

void*
TimerEvent::operator new( size_t ) 
{
	return allocate( sizeof(TimerEvent) );	
}

void
TimerEvent::operator delete( void* p ) 
{
	deallocate( p, sizeof(TimerEvent) );	
}


ITimingImpl::ITimingImpl( MyOS::Core::IComponent& c )
: ITiming( c, VERSION(2,0) )
{

}

bool 
ITimingImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	timer.init();
	return true;
}

void 
ITimingImpl::deinit( IContext& context )
{
   timer.deinit();
}

// virtual 
u32 
ITimingImpl::now( ETimeUnit units )
{
	return TimeUtil::now(units);
}

// virtual  
myos_result_t 
ITimingImpl::initTimer( timer_t& timer, ITimerTarget& callback )
{
	// DPRINTK( "\ninitTimer cb=%x", callback );
	
	TimerEvent* event = new TimerEvent();
	event->setCallback( callback );
	timer = event;
	return E_MYOS_SUCCESS;
}

// virtual  
myos_result_t 
ITimingImpl::startTimer( timer_t& timer, u32 afterUs )
{
	// DPRINTK( "\nstartTimer(%d) CS=%x", afterUs, Processor::CS() );
	
	TimerEvent* event = (TimerEvent*) timer.p();
	ASSERTION( event, E_ERROR );
	InterruptHandling::inIRQ() 
		? event->onTimer_schedule( afterUs ) 
		: event->schedule( afterUs );
	return E_MYOS_SUCCESS;	// TODO: check result		
}

// virtual  
myos_result_t 
ITimingImpl::cancelTimer( timer_t& timer )
{
	TimerEvent* event = (TimerEvent*) timer.p();
	ASSERTION( event, E_ERROR );
	event->cancel();
	return E_MYOS_SUCCESS;	// TODO: check result
}

// virtual  
myos_result_t 
ITimingImpl::triggerTimer( timer_t& timer )
{
	TimerEvent* event = (TimerEvent*) timer.p();
	ASSERTION( event, E_ERROR );
	event->trigger();
	return E_MYOS_SUCCESS;	// TODO: check result
}


// virtual  
myos_result_t 
ITimingImpl::freeTimer( timer_t& timer )
{
	if (timer.p()) {
		delete (TimerEvent*) timer.p();	
		return E_MYOS_SUCCESS;
	}
	return E_MYOS_E_FAIL;
}

}}  // namespaces
