<xsl:stylesheet
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='2.0'
   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   xmlns:util="util.Capitalizer"
   xmlns:string="java:java.lang.String"
   exclude-result-prefixes="xsd util string"
>
<xsl:strip-space elements="*" />
<!--
   Will generate C++ glue code for MyOS2 (for devices)
   @author Jeroen van Bemmel
-->

<xsl:output method='text' omit-xml-declaration='yes' />
<xsl:variable name="config" select='document("../xml/myosconfig.xml")/config'/>
<xsl:template name="copyxml">
  <xsl:for-each select="../../*">
  <xsl:copy>
  <xsl:copy-of select="@*"/>
  <xsl:apply-templates/>
  </xsl:copy>
  </xsl:for-each>
</xsl:template>

<!--
<xsl:template match="text()">
  <xsl:value-of select="normalize-space(.)"/>
</xsl:template>
-->

<xsl:template match="*">
<xsl:if test="name()!='native'">
<xsl:text>&lt;</xsl:text>
<xsl:value-of select="name()"/>
<xsl:apply-templates select="@*"/>
<xsl:choose><xsl:when test="./*">
<xsl:text>&gt;</xsl:text>
<xsl:apply-templates/>
<xsl:text>&lt;/</xsl:text>
<xsl:value-of select="name()"/>
<xsl:text>&gt;</xsl:text>
</xsl:when><xsl:otherwise>/&gt;</xsl:otherwise></xsl:choose>
</xsl:if>
</xsl:template>

<xsl:template match="@*">
<xsl:text> </xsl:text>
<xsl:value-of select="name()"/>
<xsl:text>=\"</xsl:text>
<xsl:value-of select="."/>
<!--xsl:call-template name="output-attribute-value"/-->
<xsl:text>\"</xsl:text>
</xsl:template>

<xsl:template name="paramdecl">
<xsl:for-each select="params/param">
<xsl:if test="position()!=1">,</xsl:if><xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/><xsl:if test="position!=last()">,</xsl:if>
</xsl:for-each>
</xsl:template>

<!-- 
It is possible to use GPERF to generate perfect hash functions for method names, instead of doing a series
of string comparisons

However, there are some disadvantages:
- increased executable size for lookup tables ed
- often each interface has only a few methods (separated in get / put)
- difficult code generation, method invocation remains specific 

Therefore, I chose to use the more simplistic approach: a series of string comparisons
(Names are tested in the order they appear in the XML file)
-->

<xsl:template match="context">
<xsl:variable name="cname" select="@name"/>
<xsl:for-each select="device">
<xsl:variable name="iname" select="@name"/>
<xsl:variable name="super">
 <xsl:choose>
  <xsl:when test="@type='block'">IBlockDevice</xsl:when>
  <xsl:when test="@type='char'" >ICharDevice</xsl:when>
  <xsl:when test="@type='packet'">IPacketDevice</xsl:when>
  <xsl:otherwise>IDevice</xsl:otherwise>
 </xsl:choose>
</xsl:variable> 
<xsl:result-document href="gen_src/{$cname}/{$iname}.cpp">
<xsl:variable name="xml"><xsl:call-template name="copyxml"/></xsl:variable>
#include "string.h"
#include "<xsl:value-of select='concat($cname,"/",$iname)'/>.h"
namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {

const char* const <xsl:value-of select="$iname"/>::iXML = 
<xsl:choose><xsl:when test="$config/IDevice.XML">
"<xsl:value-of select="$xml"/>";
</xsl:when><xsl:otherwise>"&lt;not provided&gt;";</xsl:otherwise>
</xsl:choose>

/* virtual */ const UUID&amp; <xsl:value-of select="$iname"/>::getUUID() const {
<xsl:choose><xsl:when test="$config/IDevice.XML">
	return * (UUID*) &amp;iXML[<xsl:value-of select='util:indexOf($xml,@uuid)'/>];
</xsl:when><xsl:otherwise>
	return * (UUID*) "<xsl:value-of select="@uuid"/>";
</xsl:otherwise></xsl:choose>
}		

/* virtual */ const myos_name_t <xsl:value-of select="$iname"/>::getName() const {
<xsl:choose><xsl:when test="$config/IDevice.XML">
	return myos_name_t( &amp;iXML[<xsl:value-of select='util:indexOf($xml,@name)'/>], <xsl:value-of select='string:length(@name)'/> );
</xsl:when><xsl:otherwise>
	return myos_name_t("<xsl:value-of select="@name"/>",<xsl:value-of select="string-length(@name)"/>);
</xsl:otherwise></xsl:choose>	
}		

MyOS::myos_result_t
<xsl:value-of select="$iname"/>::get( const char* const call, MyOS::NVPAIR parameters[], IO::OStream&amp; output ) const {
<xsl:if test="$config/IDevice.XML">
<xsl:for-each select="method[@kind='get' and not(@remote='false')]">
<!-- Issue: methods with same prefix name will match! Sort on length, try longest names first! -->
<xsl:sort select="string-length(@name)" order="descending"/>
<xsl:if test="position()!=1">else </xsl:if>if (strncmp(call,&amp;iXML[<xsl:value-of select='util:indexOf($xml,@name)'/>],<xsl:value-of select="string-length(@name)"/> )==0) {
   return this-><xsl:value-of select="@name"/>( /* TODO: params */ <xsl:if test="@stream='true'">output</xsl:if> ); }
</xsl:for-each>
</xsl:if>
   return IInterface::get(call,parameters,output);
}

MyOS::myos_result_t
<xsl:value-of select="$iname"/>::put( const char* const call, MyOS::NVPAIR ps[], IO::IStream&amp; input ) {
<xsl:if test="$config/IDevice.XML">
<xsl:for-each select="method[@kind='put' and not(@remote='false')]">
<xsl:sort select="string-length(@name)" order="descending"/>
<xsl:if test="position()!=1">else </xsl:if>if (strncmp(call,&amp;iXML[<xsl:value-of select='util:indexOf($xml,@name)'/>],<xsl:value-of select="string-length(@name)"/> )==0) {
   return this-><xsl:value-of select="@name"/>( 
   	<xsl:if test="@stream='true'">input</xsl:if>
   	<xsl:for-each select="params/param">
   	  <xsl:if test="../../@stream='true' or position()!=1">,</xsl:if>ps[<xsl:value-of select="position()-1"/>].asUnsigned()
	</xsl:for-each>   	  
   ); }
</xsl:for-each>
</xsl:if>
   return IInterface::put(call,ps,input);
}

<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}}  // namespaces
</xsl:result-document>

<!-- Interface header file -->
<xsl:result-document href="gen_src/{$cname}/{$iname}.h">#ifndef <xsl:value-of select="$iname"/>_H
#define <xsl:value-of select="$iname"/>_H

#include "Devices/<xsl:value-of select="$super"/>.h"

<!-- Do an include phase outside of any scope, to be able to include specific
     headers -->
#define INCLUDE_PHASE
<xsl:for-each select="native">
#include "<xsl:value-of select="@file"/>"</xsl:for-each>
#undef INCLUDE_PHASE


namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {
   class <xsl:value-of select="$iname"/> : public <xsl:value-of select="$super"/>
   {
protected:   
   	  static const char* const iXML;
public:

	<!-- for castTo -->
	static const char* const getXML() { return iXML; }

   // public for Component, could be friend if containment was fixed...
   inline <xsl:value-of select="$iname"/>( MyOS::Core::IComponent&amp; c, VERSION v)
   		: <xsl:value-of select="$super"/>(c,v<xsl:if test="@size">,<xsl:value-of select="@size"/></xsl:if>) {}

<xsl:for-each select="native">
#include "<xsl:value-of select="@file"/>"</xsl:for-each>

	virtual const buffer getIDL() const { return buffer(iXML); }
	virtual const UUID&amp; getUUID() const;
	virtual const myos_name_t getName() const;
	
	virtual myos_result_t get( const char* const call, MyOS::NVPAIR parameters[], IO::OStream&amp; output ) const;
	virtual myos_result_t put( const char* const call, MyOS::NVPAIR parameters[], IO::IStream&amp;  input );

<xsl:for-each select="method">
virtual <!--xsl:value-of select="@result"/--> myos_result_t <xsl:value-of select="@name"/>(
   <xsl:call-template name="paramdecl"/><xsl:if test="params and @stream='true'">,</xsl:if>
<xsl:if test="@stream='true'">
   <xsl:if test="@kind='get'">IO::OStream&amp; out</xsl:if>
   <xsl:if test="@kind='put'">IO::IStream&amp; in</xsl:if>
</xsl:if>   
   ) <xsl:if test="@kind='get'">const</xsl:if> = 0;
</xsl:for-each>   
   };
   
<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>   
}} // namespaces
#endif
</xsl:result-document>

<!-- Template for implementation header file -->
<xsl:variable name="c" select='concat($iname,"Impl")'/>
<xsl:result-document href="gen_src/{$cname}/{$c}.h.template">
#ifndef <xsl:value-of select="$c"/>_H
#define <xsl:value-of select="$c"/>_H
#include "<xsl:value-of select='concat($cname,"/",$iname)'/>.h"

<xsl:for-each select="requires/interfaceref">
#include "<xsl:value-of select="@name"/>.h"
</xsl:for-each>

namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {

   using Context::IContext;

   class <xsl:value-of select="$c"/> : public <xsl:value-of select="$iname"/> {
public:
   <xsl:value-of select="$c"/>( MyOS::Core::IComponent&amp; c ) INITSECTION;

   bool init( IContext&amp; context, NVPAIR params[] ) INITSECTION;
   void deinit( IContext&amp; context );

<xsl:for-each select="method">
virtual <!--xsl:value-of select="@result"/--> myos_result_t <xsl:value-of select="@name"/>(
   <xsl:call-template name="paramdecl"/><xsl:if test="params and @stream='true'">,</xsl:if>
<xsl:if test="@stream='true'">   
   <xsl:if test="@kind='get'">IO::OStream&amp; out</xsl:if>
   <xsl:if test="@kind='put'">IO::IStream&amp; in</xsl:if>
</xsl:if>   
   ) <xsl:if test="@kind='get'">const</xsl:if>;
</xsl:for-each>   
   };
   
<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}} // namespaces
#endif
</xsl:result-document>

<!-- Template for implementation cpp file -->
<xsl:result-document href="gen_src/{$cname}/{$c}.cpp.template">
#include "<xsl:value-of select="$c"/>.h"
namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {

<xsl:value-of select="$c"/>::<xsl:value-of select="$c"/>( MyOS::Core::IComponent&amp; c )
: <xsl:value-of select="$iname"/>( c, VERSION(<xsl:value-of select="@version"/>,0) )
{

}

bool 
<xsl:value-of select="$c"/>::init( IContext&amp; context, NVPAIR params[] ) /* INITSECTION */
{
	return true;
}

void 
<xsl:value-of select="$c"/>::deinit( IContext&amp; context )
{

}

<xsl:for-each select="method">
// virtual 
<!--xsl:value-of select="@result"/-->myos_result_t
<xsl:value-of select="$c"/>::<xsl:value-of select="@name"/>(
<xsl:call-template name="paramdecl"/><xsl:if test="params and @stream='true'">,</xsl:if>
<xsl:if test="@stream='true'">
   <xsl:if test="@kind='get'">IO::OStream&amp; out</xsl:if>
   <xsl:if test="@kind='put'">IO::IStream&amp; in</xsl:if>
</xsl:if>   
   ) <xsl:if test="@kind='get'">const</xsl:if> {
      return E_MYOS_E_NOTIMPL;
   }
</xsl:for-each>
<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}}  // namespaces
</xsl:result-document>

</xsl:for-each>
</xsl:template>

</xsl:stylesheet>