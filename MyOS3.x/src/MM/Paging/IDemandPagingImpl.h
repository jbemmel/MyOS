#ifndef IDemandPagingImpl_H
#define IDemandPagingImpl_H

#include "MM/IDemandPaging.h"
#include "InterruptHandling/IInterruptHandling.h"	// for component
#include "Exceptions/MyOSExceptions.h"

namespace MyOS { 
namespace MM { namespace Paging {

/**
 * NOTE: This could be in a separate component if needed
 * 
 */
class IDemandPagingImpl : public IDemandPaging 
{  
   static void pagefaultAsm() throw (Exceptions::PagingException) ASMNAME("PFASM");
   static void pagefault( linadr_t adr, u32 ec ) 
      throw (Exceptions::PagingException) ASMNAME("pagefault") REGPARM(2) ;
   
   static void doThrow( void* adr, int cause ) throw (Exceptions::PagingException) NORETURN;
  
public:
   IDemandPagingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   // .inc
   virtual linadr_t map( Paging::DemandPagedEntity *first, size_t itemsize, size_t count, bool readonly );   
   virtual physadr_t unmap( linadr_t bufpage );
      
};
   

}}} // namespaces
#endif
