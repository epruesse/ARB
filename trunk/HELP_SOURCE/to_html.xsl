
<!DOCTYPE xsl:stylesheet [
          <!ENTITY nbsp "&#160;">
          <!ENTITY acute "&#180;">
          <!ENTITY dotwidth "20">
          <!ENTITY dotheight "16">
          ]>


<!-- used to create ARB help in HTML format -->


<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               version="1.0"
               >

  <xsl:output method="html" encoding="iso-8859-1"/>

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

  <xsl:variable name="tableBorder">0</xsl:variable>

  <xsl:variable name="author">ARB development</xsl:variable>
  <xsl:variable name="maildomain">arb-home.de</xsl:variable>

  <xsl:variable name="fontColor">black</xsl:variable>
  <xsl:variable name="backgroundColor">white</xsl:variable>
  <xsl:variable name="linkColor">blue</xsl:variable>
  <xsl:variable name="visitedLinkColor">green</xsl:variable>
  <xsl:variable name="activeLinkColor">red</xsl:variable>

  <xsl:variable name="linkSectionsColor">#ccccff</xsl:variable>

  <!-- =============== -->
  <!--     warning     -->
  <!-- =============== -->

  <xsl:template name="error">
    <xsl:param name="text" select="Unknown error"/>
    <xsl:message terminate="yes"><xsl:value-of select="$text"/></xsl:message>
  </xsl:template>

  <!-- ============ -->
  <!--    LINKs     -->
  <!-- ============ -->

  <!--insert-link-->
  <xsl:template name="insert-link">

    <xsl:param name="address"/>
    <xsl:param name="linktext"/>

    <xsl:variable name="external">
      <xsl:choose>
        <xsl:when test="starts-with($address,'http://download.arb-home.de/')">0</xsl:when>
        <xsl:when test="starts-with($address,'http://')">1</xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$external = 1">
        <!--if we have an external link -> create popup window-->
        <A href="{$address}" target="_blank"><FONT color="{$externalLinkColor}"><xsl:value-of select="$linktext"/></FONT></A>
      </xsl:when>
      <xsl:otherwise>
        <A href="{$address}"><xsl:value-of select="$linktext"/></A>
      </xsl:otherwise>
    </xsl:choose>
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
    <xsl:variable name="addsubj">
      <xsl:choose>
        <xsl:when test="string-length($subject)>0"><xsl:value-of select="$add"/>?subject=<xsl:value-of select="$subject"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$add"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <A href="mailto:{$addsubj}">
      <xsl:choose>
        <xsl:when test="string-length($linktext)>0"><xsl:value-of select="$linktext"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$add"/></xsl:otherwise>
      </xsl:choose>
    </A>
  </xsl:template>

  <!--MAIL-->
  <xsl:template match="MAIL">
    <xsl:call-template name="insert-email-link">
      <xsl:with-param name="linktext" select="@text"/>
      <xsl:with-param name="address" select="@to"/>
      <xsl:with-param name="subject" select="@subj"/>
    </xsl:call-template>
  </xsl:template>

  <!--LINK-->
  <xsl:template match="LINK">
    <xsl:call-template name="insert-link">
      <xsl:with-param name="address" select="@href"/>
      <xsl:with-param name="linktext">
	<xsl:apply-templates/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>


  <!-- ============================== -->
  <!--     insert document header     -->
  <!-- ============================== -->
  <xsl:template name="header" xml:space="preserve">
    <xsl:param name="title" select="'Untitled'"/>
    <xsl:comment>Generated from XML with Sablotron -- Stylesheet by Ralf Westram (coder@reallysoft.de) </xsl:comment>
    <HEAD>
      <META NAME="Author" CONTENT="{$author}"/>
      <meta http-equiv="expires" content="86400"/>
      <TITLE>ARB help: <xsl:value-of select="$title"/></TITLE>
    </HEAD>
  </xsl:template>

  <!-- ======================== -->
  <!--     link-to-document     -->
  <!-- ======================== -->
  <xsl:template name="link-to-document">
    <xsl:param name="doc"/>

    <xsl:choose>
      <xsl:when test="string-length(substring-before($doc,'.ps'))&gt;0"> <!--it's a postscript link-->
        <A href="{concat($rootpath,$doc)}">
          <xsl:value-of select="$doc"/> (Postscript)
        </A>
      </xsl:when>
      <xsl:otherwise>
        <A href="{concat($rootpath,$doc)}.html">
          <xsl:for-each select="document(concat($xml_location,'/',$doc,'.xml'))">
            <xsl:for-each select="PAGE/TITLE">
              <xsl:value-of select="text()"/>
            </xsl:for-each>
          </xsl:for-each>
        </A>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- =============== -->
  <!--     uplinks     -->
  <!-- =============== -->

  <xsl:template match="UP" mode="uplinks">
    <LI>
      <xsl:call-template name="link-to-document">
        <xsl:with-param name="doc" select="@dest"/>
      </xsl:call-template>
    </LI>
  </xsl:template>

  <xsl:template match="*" mode="uplinks">
  </xsl:template>

  <!-- ================ -->
  <!--     sublinks     -->
  <!-- ================ -->

  <xsl:template match="SUB" mode="sublinks">
    <LI>
      <xsl:call-template name="link-to-document">
        <xsl:with-param name="doc" select="@dest"/>
      </xsl:call-template>
    </LI>
  </xsl:template>

  <xsl:template match="*" mode="sublinks">
  </xsl:template>


  <xsl:template match="text()" mode="reflow">
    <xsl:value-of select="."/>
  </xsl:template>
  <xsl:template match="text()" mode="preformatted">
    <xsl:value-of select="."/>
  </xsl:template>
  <xsl:template match="text()">
    <xsl:call-template name="error"><xsl:with-param name="text">Unexpected text</xsl:with-param></xsl:call-template>
  </xsl:template>

  <xsl:template match="T" mode="condensed">
    <xsl:choose>
      <xsl:when test="@reflow='1'">
        <xsl:apply-templates mode="reflow"/>
      </xsl:when>
      <xsl:otherwise>
        <PRE><FONT color="navy"><xsl:apply-templates mode="preformatted"/></FONT></PRE>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="condensed">
    <xsl:apply-templates select="."/>
  </xsl:template>

  <xsl:template match="T">
    <xsl:choose>
      <xsl:when test="@reflow='1'">
        <P/>
        <BR/>
        <B><xsl:apply-templates mode="reflow"/></B>
      </xsl:when>
      <xsl:otherwise>
        <PRE><FONT color="navy"><xsl:apply-templates mode="preformatted"/></FONT></PRE>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <xsl:template match="ENTRY">
    <LI><xsl:apply-templates mode="condensed"/></LI>
  </xsl:template>

  <xsl:template match="P">
    <TABLE border="0" width="95%" align="center"><TR><TD>
          <xsl:apply-templates mode="condensed"/>
        </TD></TR></TABLE>
  </xsl:template>

  <xsl:template match="ENUM">
    <OL><xsl:apply-templates/></OL>
  </xsl:template>
  <xsl:template match="LIST">
    <UL><xsl:apply-templates/></UL>
  </xsl:template>

  <xsl:template match="P|T" mode="top-level">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="SECTION" mode="main">
    <A name="{@name}"><H2><xsl:value-of select="@name"/></H2></A>
    <xsl:apply-templates mode="top-level"/>
    <P/><BR/>
  </xsl:template>

  <xsl:template match="SECTION" mode="content">
    <LI><A href="#{@name}"><xsl:value-of select="@name"/></A></LI>
  </xsl:template>

  <!-- ================================ -->
  <!--     PAGE document wide layout    -->
  <!-- ================================ -->

  <xsl:template match="PAGE">
    <HTML>
      <xsl:variable name="title">
        <xsl:for-each select="TITLE">
          <xsl:value-of select="text()"/>
        </xsl:for-each>
      </xsl:variable>
      <xsl:call-template name="header">
        <xsl:with-param name="title" select="$title"/>
      </xsl:call-template>
      <BODY LEFTMARGIN="100" TEXT="{$fontColor}" BGCOLOR="{$backgroundColor}" LINK="{$linkColor}" VLINK="{$visitedLinkColor}" ALINK="{$activeLinkColor}">
        <P align="right"><FONT size="-1">
          This page was converted by <I>arb_help2xml</I> and may look strange.<BR/>
          If you think it's really bad, please send a
      <xsl:call-template name="insert-email-link">
        <xsl:with-param name="linktext">mail</xsl:with-param>
        <xsl:with-param name="address">helpfeedback</xsl:with-param>
        <xsl:with-param name="subject" select="concat('Helppage ',$myname,' looks weird')"/>
      </xsl:call-template>
          to our help keeper.
        </FONT></P>
        <TABLE width="95%" align="center">
          <TR bgcolor="{$linkSectionsColor}">
            <TD valign="top" width="50%">Main topics:<BR/>
              <UL><xsl:apply-templates mode="uplinks"/></UL>
<!--              rootpath=<xsl:value-of select="$rootpath"/>-->
<!--              myname=<xsl:value-of select="$myname"/>-->
            </TD>
            <TD valign="top">Related topics:<BR/>
              <UL><xsl:apply-templates mode="sublinks"/></UL>
            </TD>
          </TR>
          <TR>
            <TD colspan="2">
              <P/><BR/>
              <H1><xsl:value-of select="$title"/></H1>
              <UL><xsl:apply-templates select="SECTION" mode="content"/></UL>
              <P/><BR/>
              <xsl:apply-templates select="SECTION" mode="main"/>
            </TD>
          </TR>
        </TABLE>
      </BODY>
    </HTML>
  </xsl:template>

  <!-- ============ -->
  <!--     text()   (should be last template)  -->
  <!-- ============ -->
  <xsl:template match="text()|*">
   <xsl:choose>
     <xsl:when test="string-length(name())>0">
       <xsl:call-template name="error"><xsl:with-param name="text">Unbekannter TAG <xsl:value-of select="name()"/></xsl:with-param></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
<!--        <xsl:value-of select="."/>-->
<!--        <xsl:call-template name="error"><xsl:with-param name="text">Unexpected text</xsl:with-param></xsl:call-template>-->
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


</xsl:transform>

