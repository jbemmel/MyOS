#include "Context/IContext.h"
#include "FATComponent.h"

namespace MyOS
{
namespace FS
{
namespace FAT
{
FATComponent* FATComponent::instance INITINSTANCE;

FATComponent::FATComponent(MyOS::Core::IContainer& container) throw() :
    IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

    , impl1(*this)

{
}

myos_result_t FATComponent::initialize(Context::IContext& context,
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
myos_result_t FATComponent::deinitialize(Context::IContext& context)
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

myos_result_t FATComponent::queryInterface(MyOS::Core::IComponent& requestor,
        const uuid_t& uuid, MyOS::Core::IInterface*& result)
{

    return E_MYOS_E_NOINTERFACE;
}

}
}
} // namespaces
