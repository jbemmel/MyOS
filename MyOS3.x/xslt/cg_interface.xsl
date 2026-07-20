<xsl:stylesheet
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='2.0'
   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   xmlns:util="util.Capitalizer"
   xmlns:string="java:java.lang.String"
   exclude-result-prefixes="xsd util string"
>
<xsl:strip-space elements="*" />
<!--
   Will generate C++ glue code for MyOS2 (for interfaces)
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
<xsl:for-each select="interface">
<xsl:variable name="iname" select="@name"/>

<!-- 
   Interface .h file 
-->
<xsl:result-document href="gen_src/{$cname}/{$iname}.h">#ifndef <xsl:value-of select="$iname"/>_H
#define <xsl:value-of select="$iname"/>_H
#include "Core/IInterface.h"

using MyOS::Core::IInterface;

<xsl:if test="servicePoint">
#include "Core/IService.h"
</xsl:if>

namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {
   INTERFACE( <xsl:value-of select="$iname"/>, "<xsl:value-of select="@uuid"/>" )

<xsl:for-each select="method">
virtual <!--xsl:value-of select="@result"/--> myos_result_t <xsl:value-of select="@name"/>(
   <xsl:call-template name="paramdecl"/><xsl:if test="params and @stream='true'">,</xsl:if>
<xsl:if test="@stream='true'">
   <xsl:if test="@kind='get'">IO::OStream&amp; out</xsl:if>
   <xsl:if test="@kind='put'">IO::IStream&amp; in</xsl:if>
</xsl:if>   
   ) <xsl:if test="@kind='get'">const</xsl:if> = 0;
</xsl:for-each>

<xsl:if test="servicePoint">
   virtual myos_result_t registerService( Core::IService&amp; svc );
   virtual myos_result_t removeService( Core::IService&amp; svc );
      
   protected: 
      Core::IService* serviceChain;	// @todo init to 0 in constructor!
      inline myos_result_t notifyServices( void* servicePoint ) {
         bool all = true;
         for( Core::IService* s = serviceChain; s; s = s->getNextInChain() ) {   
            all &amp;= s->onServicePointCreated( servicePoint );
         }
         return all ? E_MYOS_SUCCESS : E_MYOS_E_FAIL;
      }
   
</xsl:if>

   };
   
<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>   
}} // namespaces
#endif
</xsl:result-document>

<!--
   Interface .cpp file
-->
<xsl:result-document href="gen_src/{$cname}/{$iname}.cpp">
<xsl:variable name="xml"><xsl:call-template name="copyxml"/></xsl:variable>
#include "string.h"
#include "<xsl:value-of select='concat($cname,"/",$iname)'/>.h"

namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {

<xsl:if test="servicePoint">

myos_result_t <xsl:value-of select="$iname"/>::registerService( Core::IService&amp; svc ) {
   // TODO: pointer Memory should be cleared?
   svc.setNextInChain( serviceChain );
   serviceChain = &amp; svc;
   return E_MYOS_SUCCESS;
}  

myos_result_t <xsl:value-of select="$iname"/>::removeService( Core::IService&amp; svc ) {
   for( Core::IService* s = serviceChain, *prev = 0; s; prev=s, s = s->getNextInChain() ) {
      if (s == &amp; svc) {
         if (prev) 
            prev->setNextInChain( s->getNextInChain() );
         else 
            serviceChain = 0;
         return E_MYOS_SUCCESS; 
      } 
   }
   return E_MYOS_E_INVALIDARG;
}
</xsl:if>

<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}}  // namespaces
</xsl:result-document>

<!-- Template for implementation header file -->
<xsl:variable name="c" select='concat($iname,"Impl")'/>
<xsl:result-document href="gen_src/{$cname}/{$c}.h.template">
#ifndef <xsl:value-of select="$c"/>_H
#define <xsl:value-of select="$c"/>_H
#include "<xsl:value-of select='concat($cname,"/",$iname)'/>.h"
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