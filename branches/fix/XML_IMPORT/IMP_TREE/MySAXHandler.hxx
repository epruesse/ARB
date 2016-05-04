#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

XERCES_GLIBCXX_NAMESPACE_USE

class MySAXHandler : public DefaultHandler, private XMLFormatTarget
{
private:
    XMLFormatter fFormatter;
    const char *outFileName;
    ofstream &out;

public:
    char *buffer;
    MySAXHandler(const char* const encodingName, const XMLFormatter::UnRepFlags unRepFlags, ofstream &outFile);
    ~MySAXHandler() OVERRIDE;

    //  Implementations of the format target interface
    // -----------------------------------------------------------------------
    void writeChars(const XMLByte* const  toWrite );

    void writeChars(const XMLByte* const toWrite, const unsigned int count,
                    XMLFormatter* const  formatter);


    //  Implementations of the SAX DocumentHandler interface
    //--------------------------------------------------------------------------------
    void startDocument();
    void endDocument();

    void startElement(const XMLCh* const uri,   const XMLCh* const localname,
                      const XMLCh* const qname, const Attributes&  attributes);
    void endElement (const XMLCh* const uri,    const XMLCh* const localname,
                     const XMLCh* const qname);

    void characters            (const XMLCh* const chars, const unsigned int length);
    void ignorableWhitespace   (const XMLCh* const chars, const unsigned int length );
    void processingInstruction (const XMLCh* const target, const XMLCh* const data );

    //  Implementations of the SAX ErrorHandler interface
    //--------------------------------------------------------------------------------
    void warning    (const SAXParseException& exception);
    void error      (const SAXParseException& exception);
    void fatalError (const SAXParseException& exception);
};
