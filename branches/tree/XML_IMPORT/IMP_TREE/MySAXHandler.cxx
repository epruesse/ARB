#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/SAXException.hpp>
#include "testParser.hxx"

#include <strstream.h>

MySAXHandler::MySAXHandler(const char* const encodingName, const XMLFormatter::UnRepFlags unRepFlags, ofstream &outFile) :
    fFormatter(encodingName, this, XMLFormatter::NoEscapes, unRepFlags),
    out(outFile)
{
    //    fFormatter << fFormatter.getEncodingName(); //getting the encoding name
}

MySAXHandler::~MySAXHandler()
{
}

// Output FormatterTarget interface
//--------------------------------------------------------------------------------
void MySAXHandler::writeChars(const XMLByte* const toWrite)
{
}

void MySAXHandler::writeChars(const XMLByte* const toWrite, const unsigned int count,
                              XMLFormatter* const formatter)
{
    // writing to the destination file "out"
    out.write ((char *) toWrite, (int) count) ;
}


//DocumentHandler
//--------------------------------------------------------------------------------
void MySAXHandler::startDocument()
{
    //    cout<<endl<<"startDocument \n"<<endl;
}

void MySAXHandler::endDocument()
{
    //    cout<<endl<<"\nendDocument "<<endl;
}

void MySAXHandler::startElement(const XMLCh* const uri,   const XMLCh* const localname,
                                const XMLCh* const qName, const Attributes&  attributes)
{
    //    fFormatter << XMLFormatter::NoEscapes << chOpenAngle;  // opens the tag "<"

    fFormatter << localname << chSpace << chColon << chLF ;

    unsigned int len = attributes.getLength();
    for (unsigned int index = 0; index < len; index++)
    {
        fFormatter  << XMLFormatter::NoEscapes << chSpace ;
        //        fFormatter  << attributes.getLocalName(index) ;

        fFormatter // << chEqual << chDoubleQuote
                    << XMLFormatter::AttrEscapes
                    << attributes.getValue(index)
                    << XMLFormatter::NoEscapes << chSpace
            /*                    << chDoubleQuote*/;
    }
    //    fFormatter << chCloseAngle;  // closes the tag ">"
}

void MySAXHandler::endElement(const XMLCh* const uri,    const XMLCh* const localname,
                               const XMLCh* const qname)
{
    //   fFormatter << XMLFormatter::NoEscapes << chOpenAngle <<chForwardSlash;
    //    fFormatter << localname ;//<< chCloseAngle;
}

void MySAXHandler::characters(const XMLCh* const chars, const unsigned int length)
{
    //   fFormatter.formatBuf(chars, length, XMLFormatter::CharEscapes);
}

void MySAXHandler::ignorableWhitespace(const XMLCh* const chars, const unsigned int length)
{
    fFormatter.formatBuf(chars, length, XMLFormatter::NoEscapes);
}


void MySAXHandler::processingInstruction (const XMLCh* const target, const XMLCh* const data ) 
{
    //   cout<<"Processing Instruction : "<<target<<" - "<<data;
}

//  MySAXHandler: Overrides of the SAX ErrorHandler interface
//--------------------------------------------------------------------------------
void MySAXHandler::error(const SAXParseException& e)
{
    cerr << "\nError at file " << StrX(e.getSystemId())
         << ", line "          << e.getLineNumber()
         << ", char "          << e.getColumnNumber()
         << "\n  Message: "    << StrX(e.getMessage()) << endl;
}

void MySAXHandler::fatalError(const SAXParseException& e)
{
    cerr << "\nFatal Error at file " << StrX(e.getSystemId())
         << ", line "                << e.getLineNumber()
         << ", char "                << e.getColumnNumber()
         << "\n  Message: "          << StrX(e.getMessage()) << endl;
}

void MySAXHandler::warning(const SAXParseException& e)
{
    cerr << "\nWarning at file " << StrX(e.getSystemId())
         << ", line "            << e.getLineNumber()
         << ", char "            << e.getColumnNumber()
         << "\n  Message: "      << StrX(e.getMessage()) << endl;
}


