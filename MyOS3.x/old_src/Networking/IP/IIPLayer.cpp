
#include "string.h"
#include "Networking/IP/IIPLayer.h"

namespace MyOS { namespace Networking { namespace IP {



myos_result_t IIPLayer::registerService( Core::IService& svc ) {
   // TODO: pointer Memory should be cleared?
   svc.setNextInChain( serviceChain );
   serviceChain = & svc;
   return E_MYOS_SUCCESS;
}  

myos_result_t IIPLayer::removeService( Core::IService& svc ) {
   for( Core::IService* s = serviceChain, *prev = 0; s; prev=s, s = s->getNextInChain() ) {
      if (s == & svc) {
         if (prev) 
            prev->setNextInChain( s->getNextInChain() );
         else 
            serviceChain = 0;
         return E_MYOS_SUCCESS; 
      } 
   }
   return E_MYOS_E_INVALIDARG;
}
}
}}  // namespaces
