/***************************************************************************
                          IInterface.cpp  -  description
                             -------------------
    begin                : Thu Jul 25 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

    To avoid inclusiong in every container, each container gets a dummy implementation
    which is corrected by IRepository upon registration (class ptr is changed)

 ***************************************************************************/
#include "IInterface.h"

namespace MyOS { namespace Core {

// virtual
myos_result_t
IInterface::get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const {

#if 1 // ndef COMPONENTCOMPILATION
   if (*call=='\0' || strncmp(call,"IDL",3)==0) {   // IDL
      output.putBuffer( getIDL() );
      return E_MYOS_SUCCESS;
   } else return E_MYOS_E_NOINTERFACE;
#else
   return E_MYOS_E_NOINTERFACE;  // dummy, replaced upon registration
#endif
}

}}  // namespaces
