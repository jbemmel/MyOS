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
   using Exceptions::OutOfMemoryException;
   
class IMultiThreadingImpl : public IMultiThreading {
 
public:
   IMultiThreadingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual  myos_result_t listThreads( IO::OStream& out ) const;

   // .inc
   virtual Thread& createThread( u32 stackorder, u32 allocOrder ) 
      throw (OutOfMemoryException);

private:
   ThreadManager threadManager;
   
};
   
      
}} // namespaces
#endif
