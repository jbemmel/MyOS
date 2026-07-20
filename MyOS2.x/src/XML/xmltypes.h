#ifndef XMLTYPES_H
#define XMLTYPES_H

#ifdef MYOS
#include "defs.h"	// size_t
#include "string.h"	// strlen()
#endif

// Not yet...
// #include "IInterface.h"

#ifdef TEST_PARSER
#include <cstdio>
#include <cstring>
#endif

//#include "defs.h"

namespace MyOS {
namespace Util {
namespace XML  {

/**
 * A simple minimal XML parser for MyOS
 * - stack-based memory allocation with callbacks
 * - attributes are enumerated in reverse order
 * - supports both " and ' delimiters for attribute values
 * - no support for UTF-16, comment
 * - no support for validation, assumes XML is valid
 * - limited support for CDATA 
 * 
 * e.g.  <root>
 *		    <element/> some data
 *			<element/> more data
 *		 </root>
 *
 * - Support for namespaces according to http://www.w3.org/TR/REC-xml-names/
 *
 * - UTF-8 encoding only -> should check <?xml ..> encoding and report error on chars > 0x7F
 */

class XMLElement;
class string;

/// Callback interface for XML parser
/**
 * TODO: Make it SAX C++ compliant? see http://xml.apache.org/xerces-c/apiDocs/classDocumentHandler.html
 *		 (not very standardized)
 *
 */
class IXMLHandler 
{
public:

  /**
   * This function is called at the start of a tag, after all attributes have been parsed
   * 
   * @return true if ok, false to abort parsing
   */
   virtual bool startTag( const XMLElement& e ) = 0;

  /**
   * This function is called at the end of a tag. Note that attributes can no longer be accessed here
   *
   * @return true if ok, false to abort parsing
   */
   virtual bool endTag( const XMLElement& e ) = 0;

   /**
    * This function is called when a processing instruction is encountered
	* @param piTarget - Processing instruction
	* @param char	  - Any chars found after the PI (excl whitespace at the start)
	*/
   virtual void processingInstruction( const string piTarget, const string chars ) = 0;
};

/// A utility interface for in-memory callback parsing of XML, using the stack
// INTERFACE( IXmlParser )
//   virtual bool parseXML( const buffer& xml, IXMLHandler& handler ) const = 0;
//};

// Utility class for representing XML tags & values
class string 
{
public:
	const char* s;
	size_t		length;

	string() : s(0), length(0) {}
	string( const char* _s ) : s(_s), length( strlen(_s) ) {}
	string( const char* _s, const char* _e ) : s(_s), length( _e - _s ) {}
	string( const char* _s, size_t _l ) : s(_s), length(_l) {}  

	const char* end() const { return s+length; }

   bool equals( const string& s2 ) const {
	   return (length == s2.length) && strncmp( s, s2.s, (length <= s2.length ? length : s2.length) ) == 0;
   }
   
#ifdef TEST_PARSER
   void print() const {
	  char buf[ 128 ] = { 0, };
	  if (length < 128) {
		strncpy( buf, s, length );
		printf( buf );
	  } else printf( "<many chars...>" );
   }
#endif   
   
};

class XMLAttribute 
{
   // Namespaces are XMLAttributes for which 'ns' points to XMLNS
   static XMLAttribute XMLNS;
   
   string         localname; // without quotes
   string         value;
   XMLAttribute*  prev;

   XMLAttribute const * ns; // optional, defaults to NULL (the default namespace)
                            // Every attribute must reserve space for a NS
                            
   // used for XMLNS
   XMLAttribute( ) : localname("xmlns"), value(""), prev(0), ns(this) {}
   
public:
	XMLAttribute( const char* xml, XMLAttribute* p );

   // parses start/end itself
   // 'element' is needed for namespace resolution   
   bool parse( XMLElement& e );	
  
	~XMLAttribute() {
		prev = 0;
	}
  
   const string& getLocalName() const	{ return localname; }
   const string& getValue() const		{ return value; }
   XMLAttribute* getPrev() const		{ return prev; }
   
   const XMLAttribute* resolveNamespace( const string& prefix, const XMLElement& e ) const;
   const XMLAttribute* getNS() const { return ns; }

   inline bool isNSDeclaration() const { return ns == &XMLNS; }
};

class XMLElement 
{
   string         localname;    // without '<' and excluding namespace prefix
   XMLAttribute*  atts;			// needed for namespace lookup
   XMLElement*    parent;		// needed ? yes, at least for namespace lookup
   XMLAttribute const * ns;     // optional
   
   // The default namespace applicable in the context of this element, 0 for none
   // This namespace only applies to child *elements*, not to attributes
   XMLAttribute*  defaultNs;
   
public:
	XMLElement( const char* _s, const char* _e, XMLElement* _p )
		: localname(_s,_e), atts(0), parent(_p), ns(0), defaultNs( _p ? _p->defaultNs : 0) {      
   }

	~XMLElement() {
		parent = 0;
		atts   = 0;
	}

   const string& getLocalName() const { return localname; }  
   XMLAttribute* getAttributes() const { return atts; }  
   
   void setAttributes( XMLAttribute* a ) {
      atts = a;
   }
   
   /** @return parent element, NULL if root */
   XMLElement* getParent() const { return parent; }
   
   /** @return any character data in this element, string(x,0) if none */
   const string getCData() const {
      const char* end = strchr( localname.s, '>' );
      return end[-1] != '/' 
         ? string( end+1, strchr( end, '<' ) )    // Note: simple CDATA! 
         : string(0,(size_t)0);
   }
   
   const XMLAttribute* resolveNamespace( const string& prefix ) const;
   void setDefaultNs( XMLAttribute* ns ) { this->defaultNs = ns; }
   XMLAttribute* getDefaultNS() const { return defaultNs; }
   
   const XMLAttribute* getNS() const { return ns; }
   bool determineNS( const char* prefix, size_t len );
};

}}}   // namespace

#endif
