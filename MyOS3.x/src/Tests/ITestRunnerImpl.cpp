#include "ITestRunnerImpl.h"

#include "debug.h"

namespace MyOS { namespace Tests {

ITestRunnerImpl::ITestRunnerImpl( MyOS::Core::IComponent& c )
: ITestRunner( c, VERSION(1,0) )
{

}

// Implemented in doTest.cpp in the various subdirs
extern bool doTest( IContext& context, NVPAIR params[] );

bool 
ITestRunnerImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
	return doTest(context,params);
}

void 
ITestRunnerImpl::deinit( IContext& context )
{

}


// virtual 
myos_result_t
ITestRunnerImpl::showResults(
IO::OStream& out   
   ) const {
      return E_MYOS_E_NOTIMPL;
   }

}}  // namespaces
