/***************************************************************************
                          HTTPReplyGenerator.cpp  -  description
                             -------------------
    begin                : Tue Mar 26 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "HTTPReplyGenerator.h"
// #include "WebServerApplication.h"
#include "buffer.h"
#include "IO/OStream.h"

namespace MyOS {
namespace Tests {

void
HTTPReplyGenerator::generateReply( IDirectory* base, const char* const url, IO::OStream& htmlout )
{
	htmlout.printf( "<html>\n<body>" );

	if (url[2] != '\0') {		// not "./"
		IInterface *result;
		if (const char *restpath = base->findNode( myos_name_t(url), VERSION::ANY(), result )) {
			if (*restpath!='\0') {
      	   if (result->get( restpath, 0, htmlout ) < 0) {	// could check if call is supported before doing this
           	   htmlout.printf( "<ERROR> Call not supported: %s", restpath );
   			}
            goto done;
			} else if (!(base = result->castTo<IDirectory>())) {
				htmlout.printf( "<ERROR> Not a directory: %s", url );
				goto done;
			}
		}
	}

	base->onAllNodes( *this, &htmlout, 0 );
done:
	htmlout.printf( "\n</body>\n</html>\n" );
}

// I_DirectoryCallback
bool
HTTPReplyGenerator::onIterate( const myos_name_t& name, IInterface &i, void *context )
{
	// LOG( WebServer, "onIterate name=%s", name );
	IO::OStream* out = (IO::OStream *) context;
	if (strcmp(name,".")!=0) {
		if (i.castTo<IDirectory>()!=0) {
			out->printf(
				"<A HREF=\"./%n/\"><IMG BORDER=0 SRC=\"/dir.gif\" ALT=\"[DIR]\"></A><A HREF=\"./%n/\">%n</A><br>\n",
				&name, &name, &name );
		} else {
			myos_name_t ifname = i.getName();
			out->printf( "<A HREF=\"./%n.xml\"><IMG BORDER=0 SRC=\"/file.gif\" ALT=\"[NODE]\"></A><A HREF=\"./%n.xml\">%n</A> %n<br>\n",
				&name, &name, &name, &ifname );
		}
	}
	return true;
}

}}	// namespace
