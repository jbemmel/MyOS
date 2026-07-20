#include "Context/IContext.h"
#include "NE2000Driver.h"

namespace MyOS
{
namespace Drivers
{
namespace Network
{
namespace NE2000
{
NE2000Driver* NE2000Driver::instance INITINSTANCE;

NE2000Driver::NE2000Driver(MyOS::Core::IContainer& container) throw() :
    MyOS::Drivers::CDriverBase(container, VERSION(2, 1), ID(),
            (IComponent*&) instance )
{
}

myos_result_t NE2000Driver::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iInterruptHandling = (IInterruptHandling*) &context.lookup( myos_name_t( IInterruptHandling::ID() ) );

    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t NE2000Driver::deinitialize(Context::IContext& context)
{

    CDriverBase::remove();

    // if (iVirtualMemory) iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

}
}
}
} // namespaces
