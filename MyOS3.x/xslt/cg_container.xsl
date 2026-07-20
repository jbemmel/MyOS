<xsl:stylesheet
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='2.0'
   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   xmlns:util="util.Capitalizer"
   xmlns:string="java:java.lang.String"
   exclude-result-prefixes="xsd util string"
>
<xsl:strip-space elements="*" />
<!--
   Will generate container C++ glue code for MyOS2
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
</xsl:template>

<xsl:template match="@*">
<xsl:text> </xsl:text>
<xsl:value-of select="name()"/>
<xsl:text>=\"</xsl:text>
<xsl:value-of select="."/>
<!--xsl:call-template name="output-attribute-value"/-->
<xsl:text>\"</xsl:text>
</xsl:template>

<!-- Template to iterate in reverse document order -->
<xsl:template name="reverse" match="contains/*" mode="reverse">
    <xsl:apply-templates select="following-sibling::*[1]" mode="reverse"/>
    <xsl:value-of select="util:uncapitalize(@name)"/>.deinitialize(context);
</xsl:template>

<xsl:template match="context">
<xsl:variable name="cname" select="@name"/>
<xsl:for-each select="container">
<xsl:variable name="name"><xsl:value-of select="@name"/>Container</xsl:variable>

<!-- Core defines -->
<xsl:result-document href="gen_src/{$cname}/{$name}Defs.h">#ifndef <xsl:value-of select="$name"/>Defs_H
#define <xsl:value-of select="$name"/>Defs_H
<xsl:for-each select="contains/(componentref|driverref)">#define <xsl:value-of select='@name'/>_IN_CORE
</xsl:for-each>
#endif

</xsl:result-document>

<!-- Container.h -->
<xsl:result-document href="gen_src/{$cname}/{$name}.h">#ifndef <xsl:value-of select="$name"/>_H
#define <xsl:value-of select="$name"/>_H

#include "Core/IComponent.h"
<xsl:for-each select="contains/componentref">
#include "<xsl:value-of select='concat(@context,"/",@name)'/>Component.h"
</xsl:for-each>
<xsl:for-each select="contains/driverref">
#include "<xsl:value-of select='concat(@context,"/",@name)'/>Driver.h"
</xsl:for-each>
<xsl:for-each select="contains/serviceref">
#include "<xsl:value-of select='concat(@context,"/",@name)'/>Service.h"
</xsl:for-each>

namespace MyOS { namespace <xsl:value-of select="$cname"/> {

class <xsl:value-of select="$name"/> : public MyOS::Core::IContainer {
public:
    <xsl:value-of select="$name"/>( ) INITSECTION;
    virtual myos_result_t initialize( IContext&amp; context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( IContext&amp; context );

	// IComponent
	virtual myos_result_t queryInterface(
      IComponent&amp; requestor,
      const UUID&amp; uuid,
      IInterface*&amp; result
    );

	// IComponent
	virtual myos_result_t queryInterface(
      IComponent&amp; requestor,
      const char* const name,
      VERSION version,
      IInterface*&amp; result
    );

	// IContainer
	virtual myos_result_t queryComponent(
      IComponent&amp; requestor,
      const UUID&amp; uuid,
      IComponent*&amp; result
    );

	virtual myos_result_t queryComponent(
      IComponent&amp; requestor,
      const char* const name,
      VERSION version,
      IComponent*&amp; result
    );

	virtual myos_result_t destroyComponent( IComponent&amp; c );
    virtual myos_result_t destroy();

	virtual const buffer getContent() const { return buffer(""); }

private:
    static <xsl:value-of select="$name"/>* instance;
    static const char* const XML;
public: inline static <xsl:value-of select="$name"/>&amp; getInstance() { return *instance; }

private:    
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
<xsl:value-of select='concat(string:replaceAll(@context,"/","::"),"::",@name)'/><xsl:if test="name()='componentref'">Component </xsl:if><xsl:if test="name()='driverref'">Driver </xsl:if><xsl:if test="name()='serviceref'">Service </xsl:if><xsl:value-of select="util:uncapitalize(@name)"/>;
</xsl:for-each>

};
}} // namespaces
#endif
</xsl:result-document>

<!-- Container.cpp -->
<xsl:result-document href="gen_src/{$cname}/{$name}.cpp">
#include "Context/IContext.h"
#include "<xsl:value-of select="@name"/>Container.h"
namespace MyOS { namespace <xsl:value-of select="$cname"/> {
<xsl:variable name="xml"><xsl:call-template name="copyxml"/></xsl:variable>
const char* const <xsl:value-of select="$name"/>::XML = "<xsl:value-of select="$xml"/>";
<xsl:value-of select="$name"/>* <xsl:value-of select="$name"/>::instance INITINSTANCE;

<xsl:value-of select="$name"/>::<xsl:value-of select="$name"/>( )
: IContainer( VERSION(<xsl:value-of select="version/@major"/>,<xsl:value-of select="version/@minor"/>), (IContainer*&amp;) instance )
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
,<xsl:value-of select="util:uncapitalize(@name)"/>(*this)
</xsl:for-each>
{
}

myos_result_t <xsl:value-of select="$name"/>::initialize( IContext&amp; context, NVPAIR params[] ) {
	myos_result_t r;
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
if ((r=<xsl:value-of select="util:uncapitalize(@name)"/>.initialize(<xsl:choose><xsl:when test="@boot">context</xsl:when><xsl:otherwise>repository.getContext()</xsl:otherwise></xsl:choose>,params))!=E_MYOS_SUCCESS) return r;
</xsl:for-each>
return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t <xsl:value-of select="$name"/>::deinitialize( IContext&amp; context ) {
<xsl:for-each select="contains/(componentref|driverref|serviceref)[1]">
<xsl:call-template name="reverse"/>
</xsl:for-each>
return E_MYOS_SUCCESS;
}

myos_result_t <xsl:value-of select="$name"/>::queryComponent( 
	MyOS::Core::IComponent&amp; requestor, const UUID&amp; uuid, MyOS::Core::IComponent*&amp; result ) {
<xsl:if test="$config/IContainer.queryComponent">
<xsl:for-each select="contains/componentref">
<xsl:if test="position()!=1">else </xsl:if>if (strncmp( uuid.asString(), &amp;XML[<xsl:value-of select='string:indexOf($xml,@uuid)'/>], sizeof(UUID))==0) {
	result = &amp;<xsl:value-of select="util:uncapitalize(@name)"/>; return E_MYOS_SUCCESS;	
}
</xsl:for-each></xsl:if>
	return E_MYOS_E_NOINTERFACE;
}

myos_result_t <xsl:value-of select="$name"/>::queryComponent( 
	MyOS::Core::IComponent&amp; requestor, const char* const name, VERSION v, MyOS::Core::IComponent*&amp; result ) {
<xsl:if test="$config/IContainer.queryComponent">
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
<xsl:if test="position()!=1">else </xsl:if>if (strncmp( name, &amp;XML[<xsl:value-of select='string:indexOf($xml,@name)'/>], <xsl:value-of select='string:length(@name)'/> )==0) {
	// TODO: if (v==VERSION::ANY() || specific version
	result = &amp;<xsl:value-of select="util:uncapitalize(@name)"/>; return E_MYOS_SUCCESS;	
}
</xsl:for-each></xsl:if>
	return E_MYOS_E_NOINTERFACE;
}

myos_result_t <xsl:value-of select="$name"/>::queryInterface( 
	MyOS::Core::IComponent&amp; requestor, const UUID&amp; uuid, MyOS::Core::IInterface*&amp; result ) {
<xsl:if test="$config/IComponent.queryInterface">
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
<xsl:if test="position()!=1">else </xsl:if>if ( <xsl:value-of select="util:uncapitalize(@name)"/>.queryInterface( requestor, uuid, result ) == E_MYOS_SUCCESS ) return E_MYOS_SUCCESS;
</xsl:for-each></xsl:if>
	return E_MYOS_E_NOINTERFACE;
}

myos_result_t <xsl:value-of select="$name"/>::queryInterface( 
	MyOS::Core::IComponent&amp; requestor, const char* const name, VERSION v, MyOS::Core::IInterface*&amp; result ) {
<xsl:if test="$config/IComponent.queryInterface">
<xsl:for-each select="contains/(componentref|driverref|serviceref)">
<xsl:if test="position()!=1">else </xsl:if>if ( <xsl:value-of select="util:uncapitalize(@name)"/>.queryInterface( requestor, name, v, result ) == E_MYOS_SUCCESS ) return E_MYOS_SUCCESS;
</xsl:for-each></xsl:if>
	return E_MYOS_E_NOINTERFACE;
}


myos_result_t <xsl:value-of select="$name"/>::destroyComponent( IComponent&amp; c ) {
	return E_MYOS_E_NOTIMPL;
}

myos_result_t <xsl:value-of select="$name"/>::destroy() {
	return E_MYOS_E_NOTIMPL;
}


}} // namespaces
</xsl:result-document>
</xsl:for-each>
</xsl:template>

</xsl:stylesheet>