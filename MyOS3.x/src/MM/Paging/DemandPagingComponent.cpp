#include "Context/IContext.h"
#include "../DemandPagingComponent.h"

#include "InterruptHandling/IInterruptHandling.h"

namespace MyOS
{
namespace MM
{
namespace Paging
{

using InterruptHandling::IInterruptHandling;

// const uuid_t DemandPagingComponent::ID = "26954748-c804-4133-aa78-151c80f636da";
DemandPagingComponent* DemandPagingComponent::instance INITINSTANCE;

DemandPagingComponent::DemandPagingComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t DemandPagingComponent::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iInterruptHandling
            = (IInterruptHandling*) &context.lookup(myos_name_t(IInterruptHandling::ID() ) );
    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t DemandPagingComponent::deinitialize(Context::IContext& context)
{

    context.remove(impl1 );

    impl1.deinit(context);

    //   if (iInterruptHandling) iInterruptHandling->release();    

    return E_MYOS_SUCCESS;
}

myos_result_t DemandPagingComponent::queryInterface(
        MyOS::Core::IComponent& requestor, const uuid_t& uuid,
        MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
}
} // namespaces
