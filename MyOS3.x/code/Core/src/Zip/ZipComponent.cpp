#include "IContext.h"
#include "ZipComponent.h"

namespace MyOS
{
namespace Zip
{

ZipComponent* ZipComponent::instance INITINSTANCE;

ZipComponent::ZipComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )
    , impl1(*this)
{
}

myos_result_t ZipComponent::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory = (IVirtualMemory*) &context.lookup(myos_name_t(IVirtualMemory::ID() ) );
    allocator.init( *iVirtualMemory, 0);

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t ZipComponent::deinitialize(Context::IContext& context)
{
    context.remove(impl1);
    impl1.deinit(context);

    //   if (iVirtualMemory) iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

myos_result_t ZipComponent::queryInterface(MyOS::Core::IComponent& requestor,
        const uuid_t& uuid, MyOS::Core::IInterface*& result)
{
    return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
