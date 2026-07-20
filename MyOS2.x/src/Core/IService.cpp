#include "IService.h"

#include "IO/OStream.h"

namespace MyOS { namespace Core {

const char* const IService::iXML = "<interface uuid=\"6b1c7191-da20-4d17-b62c-f7c27fcf494e\" name=\"IService\" url=\"http://www.myos.org/Interfaces\" version=\"1\"><method name=\"getCDL\" kind=\"get\" stream=\"true\"/></interface>";

const UUID& IService::getUUID() const {
   return * (UUID*) &iXML[17]; 
}

IService::IService( IContainer& container, VERSION v, IService*& instance ) 
throw ()
: IComponent( container, v, (IComponent*&) instance ), nextInChain(0)
{

}
 
}} // namespaces
