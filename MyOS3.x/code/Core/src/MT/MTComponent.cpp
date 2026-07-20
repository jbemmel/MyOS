#include "IContext.h"
#include "MTComponent.h"

#include "ITiming.h"

namespace MyOS
{
namespace MultiThreading
{

//const uuid_t MTComponent::ID = "4b6c1c62-ce07-4242-996e-c47bf87eefd3";

MTComponent* MTComponent::instance INITINSTANCE;

MTComponent::MTComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t MTComponent::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory = (IVirtualMemory*) &context.lookup(myos_name_t(IVirtualMemory::ID() ) );
    iTiming = (ITimerFacility*) &context.lookup(myos_name_t(ITimerFacility::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t MTComponent::deinitialize(Context::IContext& context)
{

    context.remove(impl1);

    impl1.deinit(context);

    //   if (iTiming) iTiming->release();
    //
    //   if (iVirtualMemory) iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

myos_result_t MTComponent::queryInterface(MyOS::Core::IComponent& requestor,
        const uuid_t& uuid, MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
