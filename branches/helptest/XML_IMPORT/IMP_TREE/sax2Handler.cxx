#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <strstream.h>
#include <iostream.h>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/SAXException.hpp>
#include "xmlParser.hxx"


// Global Variables 
//--------------------------------------------------------------------------------
const XMLCh* gxAttrItemLength = 0;
static int giBrLnCtr          = 0;
static int giGrNameCtr        = 0;
static bool gbLastBranch      = false;

XMLCh gxAttrGroupName[100];
XMLCh gxAttrBranchLength[10];
char *gcBranchLengthArray[100];
char *gcGroupNameArray[100];
const char *gcLastItem;
//--------------------------------------------------------------------------------

Sax2Handler::Sax2Handler(const char* const encodingName, const XMLFormatter::UnRepFlags unRepFlags, ofstream &outFile) :
    fFormatter(encodingName, this, XMLFormatter::NoEscapes, unRepFlags),
    out(outFile)
{
}

Sax2Handler::~Sax2Handler()
{
}

// Output FormatterTarget interface
//--------------------------------------------------------------------------------
void Sax2Handler::writeChars(const XMLByte* const toWrite)
{
}

void Sax2Handler::writeChars(const XMLByte* const toWrite, const unsigned int count,
                              XMLFormatter* const formatter)
{
    // writing to the destination file "out"
    out.write ((char *) toWrite, (int) count) ;
}


//DocumentHandler
//--------------------------------------------------------------------------------
void Sax2Handler::startDocument()
{
}

void Sax2Handler::endDocument()
{
    fFormatter << chCloseParen << chSemiColon;
}


void Sax2Handler::startElement(const XMLCh* const uri,   const XMLCh* const localname,
                                const XMLCh* const qName, const Attributes&  attributes)
{
    XMLCh attrLength[25];     XMLString::transcode("length",attrLength,24);
    XMLCh attrGroupName[25];  XMLString::transcode("groupname",attrGroupName,24);

    const char *temp = XMLString::transcode(localname);
    fFormatter << XMLFormatter::NoEscapes ;

    if (strcmp(temp,"COMMENT")==0) {
        fFormatter << chOpenSquare ;
    }

    if (strcmp(temp,"BRANCH")==0) {
        if (gbLastBranch) {
            fFormatter << chComma ;
            gbLastBranch = false;
        }
        fFormatter << chOpenParen ;
        gcBranchLengthArray[giBrLnCtr++] = XMLString::transcode(attributes.getValue(attrLength));
        gcGroupNameArray[giGrNameCtr++]  = XMLString::transcode(attributes.getValue(attrGroupName));
    }

    gxAttrItemLength = attributes.getValue(attrLength);

    if (strcmp(temp,"ITEM")==0) {
        if ((gcLastItem && strcmp(gcLastItem,temp)==0) || gbLastBranch) 
            fFormatter << chComma ; 
    }

    XMLCh attrItem[25];   XMLString::transcode("itemname",attrItem,24);    
    fFormatter  << XMLFormatter::AttrEscapes
                << attributes.getValue(attrItem)
                << XMLFormatter::NoEscapes ;
}

void Sax2Handler::endElement(const XMLCh* const uri,    const XMLCh* const localname,
                               const XMLCh* const qname)
{
    const char *temp = XMLString::transcode(localname);
    if (strcmp(temp,"BRANCH")==0) {
        gbLastBranch = true;

        fFormatter << chCloseParen ;

        XMLString::transcode(gcGroupNameArray[--giGrNameCtr],gxAttrGroupName,99);
        if (gxAttrGroupName)  fFormatter << gxAttrGroupName ;

        XMLString::transcode(gcBranchLengthArray[--giBrLnCtr],gxAttrBranchLength,9);
        fFormatter << chColon << gxAttrBranchLength ;
    } 
    else if (strcmp(temp,"ITEM")==0) {
        fFormatter << chColon << gxAttrItemLength;
    } 
    else if (strcmp(temp,"COMMENT")==0) {
        fFormatter << chCloseSquare << chOpenParen ;
    }

    gcLastItem = temp;
}

void Sax2Handler::characters(const XMLCh* const chars, const unsigned int length)
{
    fFormatter.formatBuf(chars, length, XMLFormatter::CharEscapes);
}

void Sax2Handler::ignorableWhitespace(const XMLCh* const chars, const unsigned int length)
{
    fFormatter.formatBuf(chars, length, XMLFormatter::NoEscapes);
}


void Sax2Handler::processingInstruction (const XMLCh* const target, const XMLCh* const data ) 
{
}

//  Sax2Handler: Overrides of the SAX ErrorHandler interface
//--------------------------------------------------------------------------------
void Sax2Handler::error(const SAXParseException& e)
{
    cerr << "\nError at file " << StrX(e.getSystemId())
         << ", line "          << e.getLineNumber()
         << ", char "          << e.getColumnNumber()
         << "\n  Message: "    << StrX(e.getMessage()) << endl;
}

void Sax2Handler::fatalError(const SAXParseException& e)
{
    cerr << "\nFatal Error at file " << StrX(e.getSystemId())
         << ", line "                << e.getLineNumber()
         << ", char "                << e.getColumnNumber()
         << "\n  Message: "          << StrX(e.getMessage()) << endl;
}

void Sax2Handler::warning(const SAXParseException& e)
{
    cerr << "\nWarning at file " << StrX(e.getSystemId())
         << ", line "            << e.getLineNumber()
         << ", char "            << e.getColumnNumber()
         << "\n  Message: "      << StrX(e.getMessage()) << endl;
}

