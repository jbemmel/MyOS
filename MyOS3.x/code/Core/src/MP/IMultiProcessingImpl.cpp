//#include "Init/MyOSCoreContainerDefs.h"
//#ifdef MP_IN_CORE

#include "IMultiProcessingImpl.h"
#include "Process.h"
#include "debug.h"

#include "IH/LocalAPIC.h"

namespace MyOS { namespace MultiProcessing {

// const uuid_t IMultiProcessing::ID = "2c902461-c339-4008-ad73-84732a7c9f70";

IMultiProcessingImpl::IMultiProcessingImpl( MyOS::Core::IComponent& c )
: IMultiProcessing( c, VERSION(1,0) )
{

}

bool
IMultiProcessingImpl::init( Context::IContext& context, NVPAIR params[] ) /* INITSECTION */
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

   // Init SMP here, @todo parse ACPI tables
#ifdef SMP
   InterruptHandling::LocalAPIC::startOtherCPU( 1 );
#endif

   return true;
}

void
IMultiProcessingImpl::deinit( Context::IContext& context )
{

}

// virtual
Process&
IMultiProcessingImpl::createProcess( IRunnable& runnable, NVPAIR params[],
        size_t paramCount, MM::EDPL rpl, u32 allocOrder )
throw (MM::OutOfMemoryException)
{
   return * new Process( runnable, params, paramCount, rpl, allocOrder );
}

}}  // namespaces

// #endif // MP_IN_CORE
