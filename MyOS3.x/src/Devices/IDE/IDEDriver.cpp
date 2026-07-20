#include "Context/IContext.h"
#include "IDEDriver.h"
namespace MyOS
{
namespace Drivers
{
namespace IDE
{
namespace Default
{
IDEDriver* IDEDriver::instance INITINSTANCE;

IDEDriver::IDEDriver(MyOS::Core::IContainer& container) throw() :
    MyOS::Drivers::CDriverBase(container, VERSION(2, 1), ID(),
            (IComponent*&) instance )
{
}

myos_result_t IDEDriver::initialize(Context::IContext& context, NVPAIR params[])
{
    iVirtualMemory = (IVirtualMemory*) &context.lookup(myos_name_t(IVirtualMemory::ID() ) );
    iInterruptHandling = (IInterruptHandling*) &context.lookup(myos_name_t(IInterruptHandling::ID() ));
    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );
    allocator.init( *iVirtualMemory, 0);

    return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t IDEDriver::deinitialize(Context::IContext& context)
{

    CDriverBase::remove();

    // if (iVirtualMemory) iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

}
}
}
} // namespaces
