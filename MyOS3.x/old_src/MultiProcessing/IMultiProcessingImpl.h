
#ifndef IMultiProcessingImpl_H
#define IMultiProcessingImpl_H

#include "MultiProcessing/IMultiProcessing.h"

// Dependencies
#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"
#include "MultiThreading/IMultiThreading.h"

namespace MyOS { 

namespace Context { class IContext; }

namespace MultiProcessing {

   // specify dependencies for MPComponent   
   using MM::IVirtualMemory;
   using MM::IPhysicalMemory;
   using MultiThreading::IMultiThreading;
   
   using Context::IContext;

class IMultiProcessingImpl : public IMultiProcessing 
{
public:
   IMultiProcessingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   virtual Process& createProcess( IRunnable& runnable, 
           NVPAIR params[], size_t paramCount, MM::EDPL rpl, u32 allocOrder )
      throw (MM::OutOfMemoryException);
};
   
      
}} // namespaces
#endif
