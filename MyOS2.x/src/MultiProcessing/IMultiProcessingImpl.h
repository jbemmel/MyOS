
#ifndef IMultiProcessingImpl_H
#define IMultiProcessingImpl_H

#include "MultiProcessing/IMultiProcessing.h"

// Dependencies
#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"
#include "MultiThreading/IMultiThreading.h"

namespace MyOS { namespace MultiProcessing {

   // specify dependencies for MPComponent   
   using MM::IVirtualMemory;
   using MM::IPhysicalMemory;
   using MultiThreading::IMultiThreading;
   
   using Context::IContext;

   class IMultiProcessingImpl : public IMultiProcessing {
public:
   IMultiProcessingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   // XML
   virtual  myos_result_t listProcesses( IO::OStream& out ) const;

   // .inc
   virtual Process& createProcess( u32 allocBlockOrder )
      throw (OutOfMemoryException);
};
   
      
}} // namespaces
#endif
