#include "IContext.h"
#include "DHCPClientService.h"
#include "DHCPClientSvcInstance.h"

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
namespace DHCP
{
DHCPClientService* DHCPClientService::instance INITINSTANCE;

DHCPClientService::DHCPClientService(MyOS::Core::IContainer& container) throw() :
    IService(container, VERSION(1, 1), ID(), (IService*&) instance )
{
}

myos_result_t DHCPClientService::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iTiming = (ITimerFacility*) &context.lookup(myos_name_t(ITimerFacility::ID() ) );

    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );

    iIPLayer = (IIPLayer*) &context.lookup(myos_name_t(IIPLayer::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    return iIPLayer->registerService( *this);
}

/** Also called after partial init() that fails */
myos_result_t DHCPClientService::deinitialize(Context::IContext& context)
{

    if (iIPLayer)
        iIPLayer->removeService( *this);

    // if (iIPLayer) ((IInterface*)iIPLayer)->release();    

    // if (iTiming) ((IInterface*)iTiming)->release();    

    // if (iVirtualMemory) ((IInterface*)iVirtualMemory)->release();    

    return E_MYOS_SUCCESS;
}

// virtual 
bool 
DHCPClientService::onServicePointCreated( void* servicePoint )
{
    using Networking::IP::IIPEndpoint;
    
    // attach new session to this (assumed) IIPEndPoint 
    IIPEndpoint* endpoint = (IIPEndpoint*) servicePoint;
    DPRINTK( "\nDHCPClient onServicePointCreated: %x", endpoint );
    DHCPClientSvcInstance* svc = new DHCPClientSvcInstance( *endpoint );
    return svc->start( 0 /* no client id yet */ );
}

}}}} // namespaces
