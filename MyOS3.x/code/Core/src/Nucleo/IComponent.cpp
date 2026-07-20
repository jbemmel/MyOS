#include "IComponent.h"

#include "OStream.h"

namespace MyOS { namespace Core {

IComponent::IComponent( IContainer& container, VERSION v, const char* const u, IComponent*& instance ) 
throw ()
: cVersion(v), myContainer(container), uuid(u)
{
   instance = this;
}
 
}} // namespaces
