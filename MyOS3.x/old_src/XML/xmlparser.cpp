#ifdef TEST_PARSER
 #include <fstream>
#endif

#include "xmlparser.h"


namespace MyOS {
namespace Util {
namespace XML  {

// Some literal strings, static initialization happens ok?
static /* const */ string STR_XMLNS( "xmlns", 5 );

/* static */ XMLAttribute XMLAttribute::XMLNS;

// XMLParser::XMLParser()  // INITSECTION
//  : INTERFACE_INIT(IXmlParser, XmlParserComponent::getInstance(), VERSION(1,1) )
//{
//   
//}  

const XMLAttribute* 
XMLAttribute::resolveNamespace( const string& prefix, const XMLElement& e ) const
{
   if ( this->isNSDeclaration() ) {
      // .. then this attribute itself denotes a namespace, compare it
      if ( prefix.equals( this->localname ) ) {
         return this;       
      }
   } 
   
   // continue search
	if (prev) 
		return prev->resolveNamespace( prefix, e );
	else if (e.getParent())
		return e.getParent()->resolveNamespace( prefix );
	else
		return 0;
}

const XMLAttribute*
XMLElement::resolveNamespace( const string& prefix ) const
{
   // could be inlined below
   for ( const XMLElement* p = this; p; p = p->parent ) {
      if ( p->atts ) return p->atts->resolveNamespace( prefix, *p ); 
   }
   return 0;   // not found, error!
}

bool
XMLElement::determineNS( const char* prefix, size_t len )
{
   if (prefix) {
      const string nsprefix( prefix, len );

      this->ns = this->resolveNamespace( nsprefix );

#ifdef TEST_PARSER
	  if (!this->ns) {
		  char pre[ 12 ]; strncpy( pre, prefix, len ); pre[len]='\0';
		  printf( "\n>>> ERROR: determineNS -> namespace prefix not found: %s\n", pre );
	  }
#endif

      return this->ns != 0;
   } else {
		// take latest default namespace
		this->ns = this->defaultNs;
		return true;
   }
}

// constructor 
XMLAttribute::XMLAttribute( const char* xml, XMLAttribute* p ) 
   : localname(xml,(size_t)0), prev(p), ns(0)
{

}

// subtract 0x28 (' character + 1) to detect spaces, !, ", #, $, %, & and '
#define mayHaveQuote(x) ((x - 0x28282828) & 0x80808080)
#define SW (sizeof (int) / sizeof (char))

static const char* findEndQuote(const char *s, char quote) {
	const char *p = s-1;
	register int d;
    do {
        p++;
        if ((((int) p) & (SW - 1)) == 0) {
            do {
                d  = *((int *) p);
                p += SW;
            } while (!mayHaveQuote(d));
            p -= SW;
        }
    } while (*p != quote);		// TODO: does not stop at '\0'
    return p;
}


bool
XMLAttribute::parse( XMLElement& e )
{
   register const char* n = localname.s;   

next:
	switch (*++n)
	{
	case 0:		// end of XML document, not well formed
		return false;

	case 0x20 :
	case 0x09 :
	case 0x0d :
	case 0x0a :
	case '='  :
		localname.length = n - localname.s;
		break;
       
      case ':':   // namespace support, recursive (not time efficient)
      {
         const string nsprefix( localname.s, (size_t) (n - localname.s) );
         localname.s = ++n;
         if (nsprefix.equals( STR_XMLNS ) ) {
            this->ns = &XMLNS;
            // no break, goto next below            
            
         } else if ((this->ns = (prev!=0) 
            ? prev->resolveNamespace( nsprefix, e )	// TODO: Do this lazily
            : e.resolveNamespace( nsprefix ) )) {
            // no break, goto next below
            
         } else {
            return false;  // abort, invalid 
         }            
      }         
       
		default:
			goto next;	// XX this allows too many characters in tag names, TODO: restrict
	}

	// only support quoted values? Or parse until non-whitespace
next2:
    switch (*++n)
	{
	case 0:		// end of XML document
		return false;

	case '"':
	case '\'':
		break;

	default:
		goto next2;
	}

	value.s = ++n;				// XML does not allow whitespace around '=', not checked here

#if 0
	register const char delim = n[-1];
	register char c;
	while ( (c=*n) != delim) 		// supports empty "" value
	{
		switch (c) {
			case 0: case '<': case '&':		// value cannot contain < or &
				return false;
		}
		++n;
	}
#else
	n = findEndQuote( n, n[-1] );
#endif

	value.length = n - value.s;

	// attributes called 'xmlns' with no prefix denote the default namespace
	if ( this->ns == 0 && localname.equals( STR_XMLNS ) ) {
		this->ns = &XMLNS;
		// standard: empty string means no namespace -> override parent
		e.setDefaultNs( value.length ? this : 0 );
	}
	return true;
}


// private static 
const char* XMLParser::eatwhitespace( const char* s ) 
{
	do switch (*s) {	// according to XML specs, whitespace ::=    (#x20 | #x9 | #xD | #xA)+ 
		case 0x20 :
		case 0x09 :
		case 0x0d :
		case 0x0a :
			++s;
			break;

		default:
			return s;	// also 0 ends up here
	} while (1);
/*
	while (*s <= ' ') {		// skips a bit more, also past '\0'
		++s;
	}
	return s;
*/
}

/**
 * Stack usage: 24 bytes excl parameters
 */
bool XMLParser::childElements( XMLElement* parent, const char* xml, IXMLHandler& handler )
{
	// assumes no whitespace at start, whitespace after last element
	register const char* end = xml;
	const char* nsStart = 0;

	// skip until whitespace
	next: switch (*++end)
   {
		case 0x20 :
		case 0x09 :
		case 0x0d :
		case 0x0a :
		case '>'  :	// no attributes then
			break;

		case '?'  :
			if (end[-1]=='<') {
				xml = end = parseProcDirective( end, handler );
			}
			goto next;

		case '/'  :	// this means we've encountered an </end> tag (with no atts); not checked for name match (could do that)
			if (parent) handler.endTag( *parent );

			// skip the tagname
			next2:switch (*++end)
			{
				case '>': break;
				case 0	: return false;	// end of document
				default	: goto next2;
			}

			// look for next tag start (or ending tag)
			next3:switch (*++end)
			{
				case '<': break;
				case 0	: return false;	// end of document
				default	: goto next3;
			}

			// check for siblings that are not end tags
			if (end[1] != '/') {
				xml = end;
				goto next;
			} else return true;

		// XML does not allow whitespace before or after the colon						
		case ':'  : // support for namespaces, cannot resolve until atts are parsed
			nsStart = xml + 1;
			xml = end;
      
         // no break; goto next

		default:
			goto next;
	}

	// Instantiate a new tag on the stack
	XMLElement tag( xml+1, end, parent );

	// Skip any whitespace before '>' character
	end = eatwhitespace( end );

	switch (*end) 
	{
		case '>':
			// No attributes, recurse

			// first skip any content
			next4:switch (*++end)
			{
				case '<': break;
				case 0	: return false;	// end of document
				default	: goto next4;
			}

			return tag.determineNS( nsStart, xml - nsStart )
				&& handler.startTag( tag ) 
				&& childElements( &tag, end, handler );

		case '/':	// empty tag -> with no attributes, check; continue with parent's child elements
			return end[1]=='>'
				&& tag.determineNS( nsStart, xml - nsStart )
				&& handler.startTag( tag ) 
				&& handler.endTag( tag )
				&& childElements( parent->getParent(), eatwhitespace(end+2), handler );
			
		default:
			// resolve namespace after attributes are parsed, but before 'startTag'
			return parseAttribute( tag, end, handler, nsStart );
	}
}


/**
 * Assumptions:
 * xml pointing to first char of name
 *
 * Stack usage: 24+n bytes
 */
bool XMLParser::parseAttribute( XMLElement& tag, const char* xml, 
							   IXMLHandler& handler, const char* nsprefix )
{
	XMLAttribute att( xml, tag.getAttributes() );
	if (!att.parse(tag)) {
#ifdef TEST_PARSER
		  printf( "\n>>> ERROR: parseAttribute -> error parsing attribute\n" );
#endif
		return false;
	}
	if ( tag.getDefaultNS() != &att ) tag.setAttributes( &att );  // filter default ns

	// if not '/>' or '>' encountered, recursively continue with parseAttribute
	xml = eatwhitespace( att.getValue().end() + 1 );	// eat whitespace after "
	switch ( *xml )
	{
	case '/':	// check that '>' follows, self-ending tag
      if (xml[1]=='>') {
   		return 
			tag.determineNS( nsprefix, tag.getLocalName().s - nsprefix - 1 )
			&& handler.startTag( tag ) && handler.endTag( tag )
   			&& tag.getParent()
               ? childElements( tag.getParent(), eatwhitespace(xml+2), handler )
   				: true;
      } else {
#ifdef TEST_PARSER
		  printf( "\n>>> ERROR: parseAttribute -> ill shaped self-closing tag\n" );
#endif
		  return false; // SYNTAX ERROR              
	  }

	case '>':
		return tag.determineNS( nsprefix, tag.getLocalName().s - nsprefix - 1 )
			&& handler.startTag( tag ) 
			&& childElements( &tag, eatwhitespace(xml+1), handler );

	default:
		return parseAttribute( tag, xml, handler, nsprefix );   // recurse
	}
}

// static 
const char*
XMLParser::parseProcDirective( const char* xml, IXMLHandler& handler )
{
	// xml[-1] == '<' && xml[0] == '?'
	const char* pi = ++xml;

next: switch ( *++xml )
	{
		case 0x20 :
		case 0x09 :
		case 0x0d :
		case 0x0a :
		case '?'  :	// end of directive, no attributes then			
			break;

		default:
			goto next;
	}

	char* end = (char*) xml;

	// Skip any whitespace before '?' character or attributes
	xml = eatwhitespace( xml );
	const char* atts = xml;

next2: switch (*xml++) 
	{
		case 0:		// end-of-document
			return xml-1;

		case '?':
			if (*xml=='>') {
				handler.processingInstruction( string(pi,end), string(atts,xml-1) );
				return eatwhitespace(++xml);
			}
			
		default:
			goto next2;
	}

	return 0;	// never reached
}

bool 
XMLParser::parseXML( const buffer& xml, IXMLHandler& handler ) const
{
	// does not check for buffer overflow...         
	return childElements( 0, (const char*) xml.getData(), handler );
}


#ifdef TEST_PARSER

extern "C" int main( int argc, char* argv[] );

int main( int argc, char* argv[] )
{
	// const char* xml = "<ns1:test xmlns='urn:1.0' xmlns:ns1='NS1' att1=\"12345\" att2=\"\"><child ns1:name='x b'/><child2 name2=\"c2\"><l3 x=\"a\"/></child2><self_ending /></ns1:test>";
	// ok : const char* xml = "<test att1=\"12345\"><child name=\"x b\"/><child2 name2=\"c2\"><l3 x=\"a\"/></child2></test>";

/*
	std::ifstream is;
	is.open( "E:\\Projects\\LSMSGUI\\patches\\mssecure.xml" );

	// get length of file:
	is.seekg (0, std::ios::end);
	size_t length = is.tellg();
	is.seekg (0, std::ios::beg);

	// allocate memory:
	char* xml = new char [length];

	// read data as a block:
	is.read (xml,length);
	is.close();
*/

const char* xml = 
"<?xml version='1.0' encoding='UTF-8'?>"	// additional '?'s are actually valid according to the spec !
"<soap11:Envelope"
   " xmlns=\"urn:GoogleSearch\""
   " xmlns:soap11=\"http://schemas.xmlsoap.org/soap/envelope/\">"
 "<soap11:Body>"
    "<doGoogleSearch>"
      "<key>00000000000000000000000000000000</key>"
      "<q>shrdlu winograd maclisp teletype</q>"
      "<start>0</start>"
      "<maxResults>10</maxResults>"
      "<filter>true</filter>"
      "<restrict></restrict>"
      "<safeSearch>false</safeSearch>"
      "<lr></lr>"
      "<ie>latin1</ie>"
      "<oe>latin1</oe>"
    "</doGoogleSearch>"
  "</soap11:Body>"
"</soap11:Envelope>";

	class Tester: public IXMLHandler {
		virtual bool startTag( const XMLElement& e ) {
			//printf( "\nstartTag tag=" ); e.getLocalName().print();
			//printf( " CData=" ); e.getCData().print();
			if (e.getNS()!=0) {
				const XMLAttribute* ns = e.getNS();
				//printf( " NS=" ); ns->getValue().print();
			}

			XMLAttribute* att = e.getAttributes();
			while (att) {
				//printf( " att.name=" ); att->getLocalName().print();
				//printf( " att.val=" ); att->getValue().print();
				//if (att->getNS()!=0) { printf( " NS=" ); att->getNS()->getValue().print(); }
				att = att->getPrev();
			}

			return true;
		}
		virtual bool endTag( const XMLElement& e ) {
			//printf( "\nendTag tag=" ); e.getLocalName().print();

			XMLAttribute* att = e.getAttributes();
			while (att) {
				//printf( " att.name=" ); att->getLocalName().print();
				//printf( " att.val=" ); att->getValue().print();
				//if (att->getNS()!=0) { printf( " NS=" ); att->getNS()->getValue().print(); }
				att = att->getPrev();
			}

			return true;			
		}

		virtual void processingInstruction( const string &piTarget, const string &atts ) {
			// printf( "\nPI: " ); piTarget.print(); printf( " atts=" ); atts.print();
		}
	} tester;

	XMLParser p;	
	bool b = p.parseXML( buffer(xml), tester );
	return b;
}

#endif

}}}   // namespace
