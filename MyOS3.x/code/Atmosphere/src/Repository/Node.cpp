/***************************************************************************
                          Nodeimpl.cpp  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "Node.h"
#include "IDirectory.h"
#include "OStream.h"
#include "IInterface.h"

#include "debug.h"

namespace MyOS {
namespace Context {

s32
NodeIMPL::toXML( void *params )
{
   // DPRINTK( "\ntoXML name=%n myInterface=%x", params, &name, myInterface );
  
   toXMLparameters *p = (toXMLparameters*) params;
   // myos_name_t ifname = myInterface->getName();
   p->out.printf( "\n<NODE name=\"%n\" version=\"%d.%d\"/>",
      &name, version.major, version.minor );

   s32 count = 1;
   if ((p->levels > 0) && !(name.equals(SELF) || name.equals(PARENT)) ) {
      IDirectory *dir = myInterface->castTo<IDirectory>();
      if (dir) {
         // DPRINTK( "\nRecursing name=%n", &name );       
         count += dir->list( myos_name_t(""), p->levels - 1, p->out );
      }
   }
   return count;
}

s32
NodeIMPL::callback( void *params )
{
	callbackParameters *p = (callbackParameters*) params;
	u32 count = 0;
	if (p->callback.onIterate( name, *myInterface, p->context ))
		++count;
	else if (p->flags & IDirectory::A_STOP_AT_FAILURE)
		return 0;

	if (p->flags & IDirectory::A_RECURSIVE) {
		IDirectory *dir = myInterface->castTo <IDirectory> ();

		// This excludes all directories starting with '.', also eg .config
      if (dir && (dir->getName().startsWith('.')==false)) {
         count += dir->onAllNodes( p->callback, p->context, p->flags );
      }
	}
	return count;
}

}} // namespace
