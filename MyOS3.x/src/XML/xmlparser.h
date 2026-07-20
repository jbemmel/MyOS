#ifndef XMLPARSER_H
#define XMLPARSER_H

// #include "IXmlParser.h"
#include "xmltypes.h"
#include "mybuffer.h"

namespace MyOS {
namespace Util {
namespace XML  {

class XMLParser // : public IXmlParser
{
public:
   virtual ~XMLParser() {}

   /// IXmlParser
   /*virtual*/ bool parseXML( const buffer& xml, IXMLHandler& handler ) const;

private:
	static const char* eatwhitespace( const char* s );
	static const char* parseProcDirective( const char* xml, IXMLHandler& handler );
	static bool parseAttribute( XMLElement& tag, const char* xml, IXMLHandler& handler, const char* nsStart );
	static bool childElements( XMLElement* parent, const char* xml, IXMLHandler& handler );
};

}}}   // namespace

#endif
