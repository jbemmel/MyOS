<xsl:stylesheet
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='2.0'
   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   xmlns:util="util.Capitalizer"
   xmlns:string="java:java.lang.String"
   exclude-result-prefixes="xsd util string"
>
<xsl:strip-space elements="*" />
<!--
   Will generate component C++ glue code for MyOS2
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
<xsl:template name="reverse" match="requires/interfaceref" mode="reverse">
    <xsl:apply-templates select="following-sibling::*[1]" mode="reverse"/>
    if (<xsl:value-of select="util:uncapitalize(@name)"/>) <xsl:value-of select="util:uncapitalize(@name)"/>->release();
</xsl:template>

<xsl:template match="context">
<xsl:variable name="cname" select="@name"/>
<xsl:for-each select="driver">
<xsl:variable name="name"><xsl:value-of select="@name"/>Driver</xsl:variable>

<!-- Component.h -->
<xsl:result-document href="gen_src/{$cname}/{$name}.h">#ifndef <xsl:value-of select="$name"/>_H
#define <xsl:value-of select="$name"/>_H

#include "Drivers/CDriverBase.h"
<xsl:if test="@bus='PCI'">
#include "Drivers/PCI/CPCIDriver.h"
</xsl:if>

<xsl:for-each select="provides/deviceref">
<!-- Should lookup device by uuid, and use its context name instead of this hack... -->
#include "<xsl:value-of select='concat(replace(substring($cname,0,string:lastIndexOf($cname,"/")+1),"Driver","Device"),"/",@name)'/>.h"
</xsl:for-each>

<xsl:if test="requires/new">
#include "MM/Virtual/ByteAllocator.h"
</xsl:if>

// TBD: Forward declarations
<!--xsl:for-each select="requires/interfaceref">
class <xsl:value-of select="@name"/>;
</xsl:for-each-->

using MyOS::Drivers::IDriverManager;

namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {

<xsl:variable name="superclass">
<xsl:choose>
<xsl:when test="@bus='PCI'">MyOS::Drivers::PCI::CPCIDriver</xsl:when>
<xsl:otherwise>MyOS::Drivers::CDriverBase</xsl:otherwise>
</xsl:choose>
</xsl:variable>

class <xsl:value-of select="$name"/> : public <xsl:value-of select="$superclass"/> {

// public is easier than declaring friends...
public:
<xsl:for-each select="requires/interfaceref">
<xsl:value-of select="@name"/>* <xsl:value-of select="util:uncapitalize(@name)"/>;
</xsl:for-each>

    <xsl:value-of select="$name"/>( MyOS::Core::IContainer&amp; container ) throw() INITSECTION;
    virtual myos_result_t initialize( IContext&amp; context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( IContext&amp; context );

    virtual const buffer getCDL() const { return buffer(cXML); }
	virtual const UUID&amp; getCUID() const;

    inline myos_result_t destroy() { return getContainer().destroyComponent(*this); }

	// CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

<xsl:if test="@bus='PCI'">
	// CPCIDriver
	virtual bool handleDevice( Drivers::PCI::IPCIDevice &amp;dev );
</xsl:if>

private:
    static <xsl:value-of select="$name"/>* instance;
    static const char* const cXML;
public: inline static <xsl:value-of select="$name"/>&amp; getInstance() { return *instance; }

private:    
<xsl:for-each select="provides/deviceref">
   <xsl:value-of select="@name"/>* impl<xsl:value-of select="position()"/>[1];
</xsl:for-each>

<xsl:if test="requires/new">
	public: MM::ByteAllocator allocator;
</xsl:if>

<!-- To allow customization -->
#include "<xsl:value-of select="$cname"/>/<xsl:value-of select="$name"/>.inc"

};

<xsl:if test="requires/new">
// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return <xsl:value-of select="$name"/>::getInstance().allocator.allocate( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return <xsl:value-of select="$name"/>::getInstance().allocator.allocateNoThrow( s );
}

inline void deallocate( void* p, size_t s ) {
   <xsl:value-of select="$name"/>::getInstance().allocator.deallocate( p, s );
}
</xsl:if>

<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}} // namespaces

<xsl:if test="requires/new">

#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new( size_t size ) throw (MyOS::Exceptions::OutOfMemoryException) {
	return MyOS::<xsl:value-of select='string:replaceAll($cname,"/","::")'/>::allocate(size);  
}

inline void* operator new[]( size_t size ) throw (MyOS::Exceptions::OutOfMemoryException) {
	return MyOS::<xsl:value-of select='string:replaceAll($cname,"/","::")'/>::allocate(size);  
}

inline void operator delete( void* m, size_t s ) {
	MyOS::<xsl:value-of select='string:replaceAll($cname,"/","::")'/>::deallocate(m,s);
}

#endif	// NEW_DECLARED

</xsl:if>

#endif
</xsl:result-document>

<!-- Driver.cpp -->
<xsl:variable name="superclass">
<xsl:choose>
<xsl:when test="@bus='PCI'">MyOS::Drivers::PCI::CPCIDriver</xsl:when>
<xsl:otherwise>MyOS::Drivers::CDriverBase</xsl:otherwise>
</xsl:choose>
</xsl:variable>

<xsl:result-document href="gen_src/{$cname}/{$name}.cpp">
#include "Context/IContext.h"
#include "<xsl:value-of select="@name"/>Driver.h"
namespace MyOS { namespace <xsl:value-of select='string:replaceAll($cname,"/"," { namespace ")'/> {
<xsl:variable name="xml"><xsl:call-template name="copyxml"/></xsl:variable>
const char* const <xsl:value-of select="$name"/>::cXML = "<xsl:value-of select="$xml"/>";
<xsl:value-of select="$name"/>* <xsl:value-of select="$name"/>::instance INITINSTANCE;

/* virtual */ const UUID&amp; <xsl:value-of select="$name"/>::getCUID() const {
	return * (UUID*) &amp;cXML[<xsl:value-of select='util:indexOf($xml,@uuid)'/>];
}		

<xsl:value-of select="$name"/>::<xsl:value-of select="$name"/>( MyOS::Core::IContainer&amp; container ) throw()
: <xsl:value-of select="$superclass"/>( container, VERSION(<xsl:value-of select="version/@major"/>,<xsl:value-of select="version/@minor"/>), (IComponent*&amp;) instance )
{
}

myos_result_t <xsl:value-of select="$name"/>::initialize( IContext&amp; context, NVPAIR params[] ) {
<xsl:if test="requires/interfaceref">myos_result_t r;</xsl:if>
<xsl:for-each select="requires/interfaceref">
r = context.lookup( myos_name_t( &amp;cXML[<xsl:value-of select='util:indexOf($xml,@uuid)'/>], sizeof(UUID)), (IInterface*&amp;) <xsl:value-of select="util:uncapitalize(@name)"/> );
<xsl:if test="not(@optional!='') or @optional='false'">if (r!=E_MYOS_SUCCESS) return E_MYOS_E_MISREQINTF;
</xsl:if>
</xsl:for-each>

<xsl:if test="requires/new">
allocator.init( *iVirtualMemory, <xsl:value-of select="requires/new/@blockorder"/> );
</xsl:if>

return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t <xsl:value-of select="$name"/>::deinitialize( IContext&amp; context ) {
<xsl:for-each select="provides/interfaceref">
context.remove( impl<xsl:value-of select="position()"/> );
</xsl:for-each>
	CDriverBase::remove();
<xsl:for-each select="provides/interfaceref">
impl<xsl:value-of select="position()"/>.deinit(context);
</xsl:for-each>
<xsl:for-each select="requires/interfaceref[1]">
<xsl:call-template name="reverse"/>
</xsl:for-each>
	return E_MYOS_SUCCESS;
}

<xsl:value-of select='string:substring("}}}",0,util:countChars($cname,"/") )'/>
}} // namespaces
</xsl:result-document>
</xsl:for-each>
</xsl:template>

</xsl:stylesheet>