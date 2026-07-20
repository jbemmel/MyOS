#ifndef ITestRunnerImpl_H
#define ITestRunnerImpl_H
#include "Tests/ITestRunner.h"

#include "MM/IVirtualMemory.h"
#include "MultiThreading/IMultiThreading.h"
#include "Devices/Display/IDisplay.h"
#include "MultiProcessing/IMultiProcessing.h"

namespace MyOS { 

namespace Context { class IContext; }

namespace Tests {

   using Context::IContext;
   using namespace MM;
   using namespace MultiThreading;
   using namespace Devices::Display;
   using namespace MultiProcessing;

   class ITestRunnerImpl : public ITestRunner {
public:
   ITestRunnerImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   virtual  myos_result_t showResults( IO::OStream& out ) const;
   
};
   

}} // namespaces
#endif
