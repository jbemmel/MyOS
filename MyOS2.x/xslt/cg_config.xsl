<xsl:stylesheet
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='2.0'
   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   xmlns:string="java:java.lang.String"
   exclude-result-prefixes="xsd string"
>
<xsl:strip-space elements="*" />
<!--
   Will generate C++ glue code for MyOS2 (for config)
   @author Jeroen van Bemmel
-->

<xsl:output method='text' omit-xml-declaration='yes' />

<xsl:template match="config">
<xsl:result-document href="gen_src/myosconfig.h">
<xsl:for-each select=".//*">
#define CONFIG_<xsl:value-of select="translate(name(.),'.','_')"/>
</xsl:for-each>
#define MYOS_BUILD "@build@"
#define MYOS_BUILDDATE "@builddate@"

</xsl:result-document>
</xsl:template>

</xsl:stylesheet>