<?xml version="1.0"?>

<!--          <!ENTITY acute "&#180;">-->

<!DOCTYPE xsl:stylesheet [
          <!ENTITY nbsp "&#160;">
          <!ENTITY acute "&#39;">
          <!ENTITY dotwidth "20">
          <!ENTITY dotheight "16">
          <!ENTITY tab "&#x9;">
          <!ENTITY br "&#xa;">
          ]>


<!-- used to create ARB help in internal format -->


<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               version="1.0"
               >

<!--  <xsl:output method="text" encoding='iso-8859-15'/>-->
  <xsl:output method="text"/>

  <xsl:param name="myname"/>
  <xsl:param name="xml_location"/>

  <!-- includes -->

  <xsl:include href="date.xsl"/>

  <xsl:variable name="rootpath">
    <xsl:choose>
      <xsl:when test="string-length(substring-before($myname,'/'))&gt;0">../</xsl:when>
      <xsl:otherwise></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="author">ARB development</xsl:variable>
  <xsl:variable name="maildomain">arb-home.de</xsl:variable>

  <xsl:variable name="width" select="75"/> <!--columns in text output (applies only to reflown text)-->

  <xsl:variable name="lb"><xsl:text>&br;</xsl:text></xsl:variable> <!--used to detect line-breaks in XML-->

  <!-- =============== -->
  <!--     warning     -->
  <!-- =============== -->

  <xsl:template name="error">
    <xsl:param name="text" select="'Unknown error'"/>
    <xsl:message terminate="yes"><xsl:value-of select="$text"/></xsl:message>
  </xsl:template>

  <!-- ============ -->
  <!--    LINKs     -->
  <!-- ============ -->

  <!--insert-link-->
  <xsl:template name="insert-link">
    <xsl:param name="address"/>
    <xsl:param name="linktext"/>

    <xsl:copy-of select="$address"/>
<!--    [see: <xsl:value-of select="$address"/><xsl:text>]</xsl:text>-->
  </xsl:template>

  <!--insert-email-link-->
  <xsl:template name="insert-email-link">
    <xsl:param  name="linktext"/>
    <xsl:param  name="address" select="arb"/>
    <xsl:param  name="subject"/>


    <xsl:variable name="add">
      <xsl:choose>
        <xsl:when test="string-length(substring-before($address,'@'))>0"><xsl:value-of select="$address"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$address"/>@<xsl:value-of select="$maildomain"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="$linktext"/> [mailto: <xsl:value-of select="$add"/>
    <xsl:text>  subject: &acute;</xsl:text>
    <xsl:value-of select="$subject"/>
    <xsl:text>&acute;]</xsl:text>

  </xsl:template>

  <!-- ======================== -->
  <!--     link-to-document     -->
  <!-- ======================== -->
  <xsl:template name="link-to-document">
    <xsl:param name="doc"/>
    <xsl:param name="missing"/>

    <xsl:choose>
      <xsl:when test="string-length(substring-before($doc,'.ps'))&gt;0"> <!--it's a postscript link-->
        <value-of select="{concat($postscriptpath,$doc,'.gz')}"/>
      </xsl:when>
      <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="$missing='1'">
              <xsl:value-of select="concat('Missing Link to ',$doc,'.hlp')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:for-each select="document(concat($xml_location,'/',$doc,'.xml'))">
                <xsl:for-each select="PAGE/TITLE">TOPIC &acute;<xsl:copy-of select="normalize-space(text())"/>&acute;</xsl:for-each>
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- =============== -->
  <!--     uplinks     -->
  <!-- =============== -->

  <xsl:template name="help-link">
    <xsl:param name="dest"/>

    <xsl:value-of select="$dest"/>
    <xsl:if test="substring-before($dest,'.ps')=''"> <!-- not postscript: add .hlp -->
      <xsl:text>.hlp</xsl:text>
    </xsl:if>
    <xsl:text>&br;</xsl:text>
  </xsl:template>

  <xsl:template match="UP" mode="uplinks">
    <xsl:text>UP                  </xsl:text>
    <xsl:call-template name="help-link">
      <xsl:with-param name="dest" select="@dest"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="*|text()" mode="uplinks"></xsl:template>

  <!-- ================ -->
  <!--     sublinks     -->
  <!-- ================ -->

  <xsl:template match="SUB" mode="sublinks">
    <xsl:text>SUB                 </xsl:text>
    <xsl:call-template name="help-link">
      <xsl:with-param name="dest" select="@dest"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="*|text()" mode="sublinks"></xsl:template>

  <xsl:template match="ENTRY|P" mode="calc-indent">
    <xsl:text>    </xsl:text><!--this defines the indentation per level-->
    <xsl:apply-templates mode="calc-indent" select=".."/>
  </xsl:template>
  <xsl:template match="T|ENUM|LIST" mode="calc-indent">
    <xsl:apply-templates mode="calc-indent" select=".."/>
  </xsl:template>
  <xsl:template match="SECTION" mode="calc-indent"></xsl:template>

  <xsl:template name="find-last-space">
    <xsl:param name="text"/>
    <xsl:param name="len" select="string-length($text)"/>

    <xsl:variable name="beforelen" select="string-length(substring-before($text,' '))"/>
    <xsl:variable name="after" select="substring-after($text,' ')"/>
    <xsl:variable name="afterlen" select="string-length($after)"/>

    <xsl:choose>
      <xsl:when test="number($afterlen)&gt;0">
        <!--sth behind space-->
        <xsl:variable name="next-space">
          <xsl:call-template name="find-last-space">
            <xsl:with-param name="text" select="$after"/>
            <xsl:with-param name="len" select="$afterlen"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="number($beforelen)+1+number($next-space)"/>
      </xsl:when>
      <xsl:otherwise> <!--$afterlen=0-->
        <xsl:choose>
          <xsl:when test="number($beforelen)=0">
            <!--no spaces found-->
            <xsl:value-of select="'0'"/>
          </xsl:when>
          <xsl:otherwise>
            <!--space at last position-->
            <xsl:value-of select="number($beforelen)+1"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="indent-preformatted">
    <xsl:param name="indent" select="''"/>
    <xsl:param name="text"/>
    <xsl:param name="first" select="''"/>

    <xsl:variable name="line1" select="substring-before($text,$lb)"/>
    <xsl:variable name="line2" select="substring-after($text,$lb)"/>

    <xsl:choose>
      <xsl:when test="concat($line1,$line2)=''">
        <xsl:value-of select="concat($indent,$text)"/>
        <xsl:text>&br;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="string-length($line1)&gt;0">
<!--          <xsl:text>{</xsl:text>-->
          <xsl:value-of select="concat($indent,$line1)"/>
          <xsl:text>&br;</xsl:text>
<!--          <xsl:text>}</xsl:text>-->
        </xsl:if>

        <xsl:if test="string-length($line2)&gt;0">
          <xsl:call-template name="indent-preformatted">
            <xsl:with-param name="indent" select="$indent"/>
            <xsl:with-param name="text" select="$line2"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>

  <xsl:template name="reflow-paragraph">
    <xsl:param name="indent" select="''"/>
    <xsl:param name="text"/>
    <xsl:param name="prefix" select="''"/>

<!--    <xsl:text>{</xsl:text><xsl:copy-of select="$text"/><xsl:text>}</xsl:text>-->

    <xsl:variable name="printlen"><xsl:value-of select="$width - string-length($indent)"/></xsl:variable>
    <xsl:variable name="textlen"><xsl:value-of select="string-length($text)"/></xsl:variable>

    <xsl:variable name="this-indent">
      <xsl:choose>
        <xsl:when test="$prefix=''">
          <xsl:value-of select="$indent"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="substring($indent,1,string-length($indent) - string-length($prefix))"/>
          <xsl:value-of select="$prefix"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="number($printlen) &gt;= number($textlen)">
        <xsl:value-of select="concat($this-indent,$text)"/>
        <xsl:text>&br;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="last-space">
          <xsl:call-template name="find-last-space">
            <xsl:with-param name="text" select="substring($text,1,$printlen+1)"/>
            <xsl:with-param name="len" select="$printlen+1"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:variable name="print">
          <xsl:choose>
            <xsl:when test="$last-space='0'"><xsl:value-of select="substring($text,1,$printlen)"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="substring($text,1,$last-space)"/></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="rest">
          <xsl:choose>
            <xsl:when test="$last-space='0'"><xsl:value-of select="substring($text,$printlen+1)"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="substring($text,$last-space+1)"/></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:variable name="restlen"><xsl:value-of select="string-length($rest)"/></xsl:variable>

        <xsl:value-of select="concat($this-indent,$print)"/>
        <xsl:text>&br;</xsl:text>

        <xsl:if test="number($restlen)">
          <xsl:call-template name="reflow-paragraph">
            <xsl:with-param name="indent" select="$indent"/>
            <xsl:with-param name="text" select="$rest"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="ENTRY" mode="calc-prefix">
    <xsl:choose>
      <xsl:when test="name(..)='ENUM'">
        <xsl:value-of select="position()"/>
        <xsl:text>. </xsl:text>
      </xsl:when>
      <xsl:when test="name(..)='LIST'"><xsl:text>- </xsl:text></xsl:when>
      <xsl:otherwise></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*|text()" mode="calc-prefix">
  </xsl:template>

  <xsl:template match="text()" mode="reflow">
    <xsl:call-template name="error"><xsl:with-param name="text">Illegal text in reflow-mode</xsl:with-param></xsl:call-template>
  </xsl:template>

  <xsl:template match="text()" mode="preformatted">
    <xsl:call-template name="error"><xsl:with-param name="text">Illegal text in preformatted-mode</xsl:with-param></xsl:call-template>
  </xsl:template>

  <xsl:template match="LINK" mode="expand-links">
    <xsl:choose>
      <xsl:when test="@type='help'">
        <xsl:call-template name="link-to-document">
          <xsl:with-param name="doc" select="@dest"/>
          <xsl:with-param name="missing"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@type='www'">
        <xsl:call-template name="insert-link">
          <xsl:with-param name="linktext"><xsl:value-of select="@dest"/></xsl:with-param>
          <xsl:with-param name="address"><xsl:value-of select="@dest"/></xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@type='email'">
        <xsl:call-template name="insert-email-link">
          <xsl:with-param name="linktext"><xsl:value-of select="@dest"/></xsl:with-param>
          <xsl:with-param name="address"><xsl:value-of select="@dest"/></xsl:with-param>
          <xsl:with-param name="subject" select="concat('Concerning helppage ',$myname)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="error"><xsl:with-param name="text">Unknown type '<xsl:value-of select="@type"/>'</xsl:with-param></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="text()" mode="expand-links">
    <xsl:value-of select="."/>
  </xsl:template>

  <xsl:template match="*" mode="expand-links">
    <xsl:call-template name="error"><xsl:with-param name="text">Illegal content for expand-links</xsl:with-param></xsl:call-template>
  </xsl:template>

  <xsl:template match="T">
    <xsl:variable name="indent"><xsl:apply-templates mode="calc-indent" select=".."/></xsl:variable>
    <xsl:variable name="prefix"><xsl:apply-templates mode="calc-prefix" select="../.."/></xsl:variable>
    <xsl:variable name="text"><xsl:apply-templates mode="expand-links"/></xsl:variable>

    <xsl:choose>
      <xsl:when test="@reflow='1'">
        <xsl:call-template name="reflow-paragraph">
          <xsl:with-param name="indent" select="$indent"/>
          <xsl:with-param name="text" select="normalize-space($text)"/>
          <xsl:with-param name="prefix" select="$prefix"/>
        </xsl:call-template>
        <xsl:text>&br;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="indent-preformatted">
          <xsl:with-param name="indent" select="$indent"/>
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="prefix" select="$prefix"/>
        </xsl:call-template>
        <xsl:text>&br;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="T" mode="disabled">
    <xsl:choose>
      <xsl:when test="@reflow='1'"><xsl:apply-templates mode="reflow"/></xsl:when>
      <xsl:otherwise><xsl:apply-templates mode="preformatted"/></xsl:otherwise>
    </xsl:choose>
    <xsl:text>&br;</xsl:text>
  </xsl:template>

  <xsl:template match="ENTRY"><xsl:apply-templates/></xsl:template>

  <xsl:template match="P"><xsl:apply-templates/></xsl:template>

  <xsl:template match="ENUM"><xsl:apply-templates/></xsl:template>
  <xsl:template match="LIST"><xsl:apply-templates/></xsl:template>

  <xsl:template match="SECTION" mode="main">
    <xsl:text>&br;&br;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>&br;&br;</xsl:text>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- ================================ -->
  <!--     PAGE document wide layout    -->
  <!-- ================================ -->

  <!-- ============================== -->
  <!--     insert document header     -->
  <!-- ============================== -->
  <xsl:template name="header">
    <xsl:param name="title" select="'Untitled'"/>
    <xsl:param name="for"/>
    <xsl:text xml:space="preserve"># Generated from XML with Sablotron -- Stylesheet by Ralf Westram (ralf@arb-home.de)</xsl:text>
    <xsl:choose>
      <xsl:when test="$for='release'">
        <xsl:text xml:space="preserve">
#
#  ****  You may edit this file, but the next ARB update will overwrite your changes  ****
</xsl:text>
      </xsl:when>
      <xsl:when test="$for='devel'">
        <xsl:text xml:space="preserve">
#
#  ****  DO NOT EDIT (edit in $(ARBHOME)/HELP_SOURCE/oldhelp instead)  ****
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="error"><xsl:with-param name="text">Illegal content in edit_warning</xsl:with-param></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="PAGE">
      <xsl:variable name="title">
        <xsl:for-each select="TITLE">
          <xsl:value-of select="text()"/>
        </xsl:for-each>
      </xsl:variable>
      <xsl:call-template name="header">
        <xsl:with-param name="title" select="$title"/>
        <xsl:with-param name="for" select="@edit_warning"/>
      </xsl:call-template>#
# This page was converted by arb_help2xml and may look strange.
# If you think it's really bad, please send a
# <xsl:call-template name="insert-email-link">
        <xsl:with-param name="linktext">mail</xsl:with-param>
        <xsl:with-param name="address">helpfeedback</xsl:with-param>
        <xsl:with-param name="subject" select="concat('Helppage ',$myname,' looks weird')"/>
      </xsl:call-template>
# to our help keeper.

# UP references:
<xsl:apply-templates mode="uplinks"/>
# SUB references:
<xsl:apply-templates mode="sublinks"/>
# ------------------------------------------------------------
# Start of real helpfile:
TITLE&tab;<xsl:value-of select="normalize-space($title)"/>

<xsl:apply-templates select="SECTION" mode="main"/>
  </xsl:template>

  <!-- ============ -->
  <!--     text()   (should be last template)  -->
  <!-- ============ -->
  <xsl:template match="text()|*">
   <xsl:choose>
     <xsl:when test="string-length(name())>0">
       <xsl:call-template name="error"><xsl:with-param name="text">Unknown TAG <xsl:value-of select="name()"/></xsl:with-param></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
<!--        <xsl:value-of select="."/>-->
<!--        <xsl:call-template name="error"><xsl:with-param name="text">Unexpected text</xsl:with-param></xsl:call-template>-->
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


</xsl:transform>

