#include "Context/IContext.h"
#include "TestComponent.h"

namespace MyOS
{
namespace Tests
{
TestComponent* TestComponent::instance INITINSTANCE;

TestComponent::TestComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t TestComponent::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );

    iMultiProccessing = (IMultiProcessing*) &context.lookup(myos_name_t(IMultiProcessing::ID() ) );
    
    iDisplay = (IDisplay*) &context.lookup(myos_name_t(IDisplay::ID() ) );

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t TestComponent::deinitialize(Context::IContext& context)
{

    context.remove(impl1 );

    impl1.deinit(context);

//    if (iDisplay)
//        iDisplay->release();
//
//    if (iMultiThreading)
//        iMultiThreading->release();
//
//    if (iVirtualMemory)
//        iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

myos_result_t TestComponent::queryInterface(MyOS::Core::IComponent& requestor,
        const uuid_t& uuid, MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
