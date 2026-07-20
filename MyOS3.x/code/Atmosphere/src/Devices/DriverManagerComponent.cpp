#include "IContext.h"
#include "DriverManagerComponent.h"

namespace MyOS
{
namespace Drivers
{
DriverManagerComponent* DriverManagerComponent::instance INITINSTANCE;

DriverManagerComponent::DriverManagerComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t DriverManagerComponent::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iInterruptHandling = (IInterruptHandling*) &context.lookup( myos_name_t( IInterruptHandling::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t DriverManagerComponent::deinitialize(Context::IContext& context)
{

    context.remove(impl1 );

    impl1.deinit(context);

//    if (iInterruptHandling)
//        iInterruptHandling->release();
//    if (iVirtualMemory)
//        iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

myos_result_t DriverManagerComponent::queryInterface(
        MyOS::Core::IComponent& requestor, const uuid_t& uuid,
        MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
