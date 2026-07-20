#include "ThreadManager.h"

#include "Thread.h"
#include "DefaultPolicy.h"
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


// static 
void 
ThreadManager::IRQ_schedule( ) // ASMNAME( "IRQ_schedule" );
{
	// DPRINTK( "\nIRQ_schedule" );
	
	Thread& next = instance->policy->selectNext( Thread::currentThread() );
	ASSERTIONv( next.state == Thread::E_READY, E_ERROR, next.state );

	// TODO: Optimize in case cur == next ? timer needs to be started...
	
	// XX quantum should come from policy, 10000us == 10 ms
	// DPRINTK( "\nSelecting next thread:%x", &next ); (too many)
	next.select( 10000 );
}

}} //namespace
