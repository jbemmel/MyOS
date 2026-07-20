#include "IComponent.h"

#include "IO/OStream.h"

namespace MyOS { namespace Core {

const char* const IComponent::iXML = "<interface uuid=\"ba261d7c-b146-11d6-9269-0010a708e02d\" name=\"IComponent\" url=\"http://www.myos.org/Interfaces\" version=\"1\"><method name=\"getCDL\" kind=\"get\" stream=\"true\"/></interface>";

const UUID& IComponent::getUUID() const {
   return * (UUID*) &iXML[17]; 
}

IComponent::IComponent( IContainer& container, VERSION v, IComponent*& instance ) 
throw ()
: IInterface( *this, VERSION(1,0) ), cVersion(v), myContainer(container)
{
   instance = this;
}

// virtual 
myos_result_t 
IComponent::get( const char* const call, NVPAIR parameters[], IO::OStream& out ) const {

   // Should perhaps be in utility class
   if (strncmp(call,"getCDL",6)==0) {  // @see IComponent.xml
         out.putBuffer( getCDL() );
         return E_MYOS_SUCCESS;
      } else return IInterface::get(call,parameters,out);
  }
 
}} // namespaces
