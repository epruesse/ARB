<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [
          <!ENTITY acute "&#39;">
          ]>


<!-- used to create inter-dependencies between ARB helpfiles -->


<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               version="1.0"
               >

  <xsl:output method="text"/>

  <xsl:param name="xml"/>
  <xsl:param name="xml_location"/>

  <xsl:template match="PAGE">
    <xsl:choose>
      <xsl:when test="$xml='Xml/help_index.xml'"></xsl:when><!--skip index-->
      <xsl:otherwise>
        <xsl:value-of select="$xml"/>
        <xsl:text>: </xsl:text>
        <xsl:apply-templates/>
        <xsl:text>
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="PAGE" mode="getsource">
    <xsl:value-of select="@source"/>
  </xsl:template>
  <xsl:template match="text()|*" mode="getsource">
  </xsl:template>

  <xsl:template match="SUB|UP|LINK">
    <xsl:if test="@type='hlp'">
      <xsl:variable name="name"><xsl:value-of select="substring-before(@dest,'.hlp')"/></xsl:variable>
      <xsl:variable name="destxml"><xsl:value-of select="concat($xml_location,'/',$name,'.xml')"/></xsl:variable>
      <xsl:variable name="doc" select="document( $destxml )"/>

      <xsl:text> </xsl:text>
      <xsl:apply-templates select="$doc/PAGE" mode="getsource"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="text()">
  </xsl:template>
  <xsl:template match="*">
    <xsl:apply-templates />
  </xsl:template>

</xsl:transform>

