#include "Context/IContext.h"
#include "ISO9660Component.h"

namespace MyOS
{
namespace FS
{
namespace ISO9660
{
ISO9660Component* ISO9660Component::instance INITINSTANCE;

ISO9660Component::ISO9660Component(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t ISO9660Component::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iDemandPaging = (IDemandPaging*) &context.lookup(myos_name_t(IDemandPaging::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    if (!impl1.init(context, params))
        return E_MYOS_E_FAIL;

    if (context.add(impl1)!=E_MYOS_SUCCESS)
        return E_MYOS_E_REGFAILED;

    return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t ISO9660Component::deinitialize(Context::IContext& context)
{

    context.remove(impl1 );

    impl1.deinit(context);

//    if (iDemandPaging)
//        iDemandPaging->release();
//
//    if (iVirtualMemory)
//        iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

myos_result_t ISO9660Component::queryInterface(
        MyOS::Core::IComponent& requestor, const uuid_t& uuid,
        MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
}
} // namespaces
