#include "Context/IContext.h"
#include "Base64Component.h"

namespace MyOS
{
namespace Decoding
{
namespace Base64
{
Base64Component* Base64Component::instance INITINSTANCE;

Base64Component::Base64Component(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t Base64Component::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iDisplay = (IDisplay*) &context.lookup(myos_name_t(IDisplay::ID() ) );
    iVirtualMemory = (IVirtualMemory*) &context.lookup(myos_name_t(IVirtualMemory::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t Base64Component::deinitialize(Context::IContext& context)
{

    context.remove(impl1 );

    impl1.deinit(context);

//    if (iVirtualMemory)
//        iVirtualMemory->release();
//
//    if (iDisplay)
//        iDisplay->release();

    return E_MYOS_SUCCESS;
}

myos_result_t Base64Component::queryInterface(
        MyOS::Core::IComponent& requestor, const uuid_t& uuid,
        MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
}
} // namespaces
