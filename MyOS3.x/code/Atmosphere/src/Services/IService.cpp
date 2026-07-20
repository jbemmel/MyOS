#include "IService.h"

#include "OStream.h"

namespace MyOS { namespace Core {

IService::IService( IContainer& container, VERSION v, const char* const u, IService*& instance ) 
throw ()
: IComponent( container, v, u, (IComponent*&) instance ), nextInChain(0)
{

}

// @see IComponent
// virtual 
myos_result_t 
IService::queryInterface(
        IComponent& requestor,
        const uuid_t& uuid,
        IInterface*& result
    ) 
{ return E_MYOS_E_NOTIMPL; }

}} // namespaces
