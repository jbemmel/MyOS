#include "IContext.h"
#include "MMComponent.h"

namespace MyOS
{
namespace MM
{

//const uuid_t MMComponent::ID = "b4e0fdc1-c04a-4149-baaf-037bd91ed349";
MMComponent* MMComponent::instance INITINSTANCE;

MMComponent::MMComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance), impl1(
            *this)
{
}

myos_result_t MMComponent::initialize(IContext& context, NVPAIR params[])
{

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1) != E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t MMComponent::deinitialize(Context::IContext& context)
{

    context.remove(impl1);
    impl1.deinit(context);
    return E_MYOS_SUCCESS;
}

myos_result_t MMComponent::queryInterface(MyOS::Core::IComponent& requestor,
        const uuid_t& uuid, MyOS::Core::IInterface*& result)
{
    return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
