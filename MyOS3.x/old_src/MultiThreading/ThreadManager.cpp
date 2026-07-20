#line 1 "ThreadManager.cpp"
#include "ThreadManager.h"

#include "Thread.h"
#include "DefaultPolicy.h"

#include "TimerFacility/TimeUtil.h"
#include "debug.h"

namespace MyOS { namespace MultiThreading {

ThreadManager* ThreadManager::instance INITINSTANCE;

ThreadManager::ThreadManager()   // INITSECTION
{
   instance = this;
   policy = &defaultPolicy;
}

bool
ThreadManager::init( Context::IContext& context )   // INITSECTION
{
   // Initialize Thread block at bottom of stack here
   // (space saved in DoDecompress)
   Thread* initial = new (&Thread::currentThread()) Thread();   
   return policy->initialize(context,*initial);
}


/** 
 * static 
 *
 * This is called from TimerImpl::onInterrupt (at the end)
 * IRQ0 handling has just finished, CS is still 0x18
 */
void 
ThreadManager::reschedule()
{
	u32 quantumUs = 10000; // 10000us == 10 ms
	Thread& next = instance->policy->selectNext( Thread::currentThread(), quantumUs );
	ASSERTIONv( next.state == Thread::E_READY, E_ERROR, next.state );

	// TODO: Optimize in case cur == next ? timer needs to be started...
		
	// DPRINTK( "\nSelecting next thread:%x", &next ); (too many)
	// u32 now = Timer::TimeUtil::now( Timer::us );
	// DPRINTP( "\n[%d] reschedule next=%x", now, &next );
	
	next.select( quantumUs );
}

}} //namespace
