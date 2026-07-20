#include "IContainer.h"

namespace MyOS { namespace Core {

// could add constructor with buffer to IComponent
IContainer::IContainer( VERSION v, IContainer*& instance ) throw ()
: IComponent( *this, v, (IComponent*&) instance ) 
{

}


// virtual 
myos_result_t 
IContainer::get( const char* const call, NVPAIR parameters[], IO::OStream& out ) const {

   // Should perhaps be in utility class
   if (strncmp(call,"getContent",10)==0) {  // @see IContainer.xml
         out.putBuffer( getContent() );
         return E_MYOS_SUCCESS;
      } else return IComponent::get(call,parameters,out);
}

 
}} // namespaces
