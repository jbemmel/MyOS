#include "TimerEvent.h"
#include "TimerComponent.h"

namespace MyOS { namespace Timer {
  
TimerEvent::TimerEvent( ITimerTarget &t ) throw() 
: HeapItem()
, ITimer( TimerComponent::getInstance(), VERSION(1,0) )
, target( (u32) &t )  // not initializing firesAt 
{
}     

/*
inline void
TimerEvent::operator delete( void* p )
{
    deallocate( p, sizeof(TimerEvent) );
}*/

// virtual 
myos_result_t 
TimerEvent::start( u32 afterUs ) 
{ 
    TimerComponent::getTFImpl().startTimer( *this, afterUs );
    return E_MYOS_SUCCESS;
};
    
// virtual 
myos_result_t 
TimerEvent::cancel() 
{ 
    TimerComponent::getTFImpl().cancelEvent( *this ); 
    return E_MYOS_SUCCESS;
};
    
/* virtual 
myos_result_t 
TimerEvent::trigger() 
{ 
    TimerComponent::getTFImpl().triggerEvent( *this );
    return E_MYOS_SUCCESS;
};
*/
    
// virtual 
myos_result_t 
TimerEvent::free() 
{ 
    TimerComponent::getTFImpl().cancelEvent( *this, true );     // completely remove it!
    deallocate( this, sizeof(TimerEvent) ); // match allocate() in ITimingImpl
    return E_MYOS_SUCCESS;
};
    
  
}} // namespace
