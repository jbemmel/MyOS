#ifndef ITestRunnerImpl_H
#define ITestRunnerImpl_H
#include "Tests/ITestRunner.h"

#include "MM/IVirtualMemory.h"
#include "MultiThreading/IMultiThreading.h"
#include "Devices/Display/IDisplay.h"

namespace MyOS { namespace Tests {

   using Context::IContext;
   using namespace MM;
   using namespace MultiThreading;
   using namespace Devices::Display;

   class ITestRunnerImpl : public ITestRunner {
public:
   ITestRunnerImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext& context );

   virtual  myos_result_t showResults( IO::OStream& out ) const;
   
};
   

}} // namespaces
#endif
