#include "Init/MyOSCoreContainerDefs.h"
#ifdef MP_IN_CORE

#include "IMultiProcessingImpl.h"
#include "Process.h"
#include "debug.h"

namespace MyOS { namespace MultiProcessing {

IMultiProcessingImpl::IMultiProcessingImpl( MyOS::Core::IComponent& c )
: IMultiProcessing( c, VERSION(1,0) )
{

}

bool 
IMultiProcessingImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // DPRINTK( "\nIMultiProcessingImpl::init called" );
   Process::initOnce();
   
   /* Small test
   static class Tester : public IRunnable {
      virtual int run( NVPAIR ps[] ) {
         DPRINTK( "\nHello from another process :)" );
         return 0;  
      }
   } tester;
   NVPAIR ps[] = { NVPAIR("hi","1"), NVPAIR() };
   this->createProcess( 0 ).start( tester, ps );
   */
	return true;
}

void 
IMultiProcessingImpl::deinit( IContext& context )
{

}

// virtual 
Process& 
IMultiProcessingImpl::createProcess( u32 allocBlockOrder )
throw (OutOfMemoryException)
{
   return * new Process( Process::getCurrent(), allocBlockOrder );
}   

// virtual 
myos_result_t
IMultiProcessingImpl::listProcesses( IO::OStream& out ) const {
   return E_MYOS_ERROR;
}

}}  // namespaces

#endif // MP_IN_CORE
