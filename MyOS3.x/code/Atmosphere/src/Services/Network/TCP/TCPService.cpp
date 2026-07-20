#include "IContext.h"
#include "TCPService.h"

#include "IIPLayer.h"
#include "IVirtualMemory.h"
#include "ITiming.h"
#include "IMultiThreading.h"

namespace MyOS
{
namespace Services
{
namespace Network
{
namespace TCP
{
TCPService* TCPService::instance INITINSTANCE;

TCPService::TCPService(MyOS::Core::IContainer& container) throw() :
    IService(container, VERSION(1, 1), ID(), (IService*&) instance )
{
    // todo: in constructor of IService
    this->setNextInChain(0);
}

myos_result_t TCPService::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory = (IVirtualMemory*) &context.lookup(myos_name_t(IVirtualMemory::ID() ) );
    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );
    iTiming = (ITimerFacility*) &context.lookup(myos_name_t(ITimerFacility::ID() ) );
    iIPLayer = (IIPLayer*) &context.lookup(myos_name_t(IIPLayer::ID() ) );
    allocator.init( *iVirtualMemory, 0);

    return iIPLayer->registerService( *this);
}

/** Also called after partial init() that fails */
myos_result_t TCPService::deinitialize(Context::IContext& context)
{

    if (iIPLayer)
        iIPLayer->removeService( *this);

    // if (iIPLayer) ((IInterface*)iIPLayer)->release();    

    // if (iVirtualMemory) ((IInterface*)iVirtualMemory)->release();    

    return E_MYOS_SUCCESS;
}

}
}
}
} // namespaces
