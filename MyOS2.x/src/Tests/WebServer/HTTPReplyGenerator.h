/***************************************************************************
                          HTTPReplyGenerator.h  -  description
                             -------------------
    begin                : Tue Mar 26 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef HTTPREPLYGENERATOR_H
#define HTTPREPLYGENERATOR_H

#include "Context/IDirectory.h"

namespace MyOS {
namespace Tests {

using namespace Context;

class HTTPReplyGenerator : public IDirectoryCallback
{
public:

	// IDirectoryCallback
   virtual bool onIterate( const myos_name_t& name, IInterface& i, void* context );

	void generateReply( IDirectory* base, const char* const url, IO::OStream& output );
};

}}	// namespace

#endif
