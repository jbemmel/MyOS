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
#include "Core/IScriptable.h"

#include "debug.h"

namespace MyOS
{
namespace Tests
{

void HTTPReplyGenerator::generateReply(IDirectory* base, const char* const url,
        IO::OStream& htmlout)
{
    if (url[2] != '\0')
    { // not "./"
        IInterface *result;
        if (const char *restpath = base->findNode( myos_name_t(url), VERSION::ANY(), result ))
        {
            DPRINTK( "\nrestpath=%s", restpath );
            if (*restpath!='\0')
            {
                Core::IScriptable* s = result->getScriptable();
                if (s && s->get( restpath, 0, htmlout ) < 0)
                { // could check if call is supported before doing this
                    htmlout.printf( "<html>\n<body>ERROR Call not supported: %s</body>\n</html>", restpath );
                }
            }
            else if ((base = result->castTo<IDirectory>())) {
                goto do_html;
            } else {
                htmlout.printf( "<html>\n<body>ERROR Not a directory: %s</body>\n</html>", url );
            }
        }
    } else {
do_html:        
        htmlout.printf("<html>\n<body>");
        base->onAllNodes( *this, &htmlout, 0 );
        htmlout.printf( "\n</body>\n</html>\n" );
    }
}

// I_DirectoryCallback
bool
HTTPReplyGenerator::onIterate( const myos_name_t& name, IInterface &i, void *context )
{
    // LOG( WebServer, "onIterate name=%s", name );
    IO::OStream* out = (IO::OStream *) context;
    if (strcmp(name,".")!=0)
    {
        if (i.castTo<IDirectory>()!=0)
        {
            // dont add '<IMG BORDER=0 SRC=\"/dir.gif\" ALT=\"[DIR]\">' yet, triggers more requests
            out->printf( "<A HREF=\"./%n/\">./%n</A><br>\n", &name, &name /*i.getName()*/ );
        }
        else
        {
            out->printf( "<A HREF=\"./%s.xml\">%s</A><br>\n", i.getUUID(), i.getName() );
        }
    }
    return true;
}

}} // namespace
