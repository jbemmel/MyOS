#ifndef IMultiThreadingImpl_H
#define IMultiThreadingImpl_H

#include "MultiThreading/IMultiThreading.h"
#include "ThreadManager.h"
#include "Exceptions/MyOSExceptions.h" // OutOfMemory

// for MTComponent.h
#include "MM/IVirtualMemory.h"

namespace MyOS { namespace MultiThreading {

   using Context::IContext;
   using MM::IVirtualMemory;
   using MM::OutOfMemoryException;
   
class IMultiThreadingImpl : public IMultiThreading {
 
public:
   IMultiThreadingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual Thread& createThread( IRunnable& runnable, NVPAIR params[], size_t paramCount, 
           MM::EDPL rpl = MM::E_DPL0, u32 allocOrder=0, bool start = true ) 
       throw(MM::OutOfMemoryException);

private:
   ThreadManager threadManager;
   
};
   
      
}} // namespaces
#endif
