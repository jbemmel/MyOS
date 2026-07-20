#include "IService.h"

#include "IO/OStream.h"

namespace MyOS { namespace Core {

IService::IService( IContainer& container, VERSION v, const char* const u, IService*& instance ) 
throw ()
: IComponent( container, v, u, (IComponent*&) instance ), nextInChain(0)
{

}
 
}} // namespaces
