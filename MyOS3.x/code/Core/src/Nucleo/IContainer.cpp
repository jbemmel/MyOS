#include "IContainer.h"

namespace MyOS { namespace Core {

static const uuid_t dummy_id = "";

// could add constructor with buffer to IComponent
IContainer::IContainer( VERSION v, IContainer*& instance ) throw ()
: IComponent( *this, v, dummy_id, (IComponent*&) instance ) 
{

}

 
}} // namespaces
