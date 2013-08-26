#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include "xmlParser.hxx"


static void usage() 
{
    cout << "\nUsage: xml2newick <xml_file> <out_put_file_name>"<<  endl;
}

int main(int argC, char* argV[])
{
    const char* encodingName = "LATIN1";
    const char* fileName = "example" ;
    const char* xmlFile = 0;

    XMLFormatter::UnRepFlags unRepFlags = XMLFormatter::UnRep_CharRef;

    // Initialize the XML4C2 system
    try  {
         XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        cerr << "Error during initialization! :\n"
             << StrX(toCatch.getMessage()) << endl;
        return 1;
    }

    // Check command line and extract arguments.
    if (argC < 3) {
        usage();
        XMLPlatformUtils::Terminate();
        return 1;
    }

    int parmInd = 1;
    if (parmInd < argC) {
        xmlFile  = argV[parmInd++]; // xml source file name
        fileName = argV[parmInd];   // destination file name
    }

   //  Create a SAX parser object and set the validations

    SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();

    //  Create the handler object and install it as the document and error
    //  handler for the parser. Then parse the file and catch any exceptions
    //  that propagate out

    ofstream outFile;
    outFile.open(fileName, ios::out); //open destination file to write

    int errorCount = 0;
    try {
        Sax2Handler handler(encodingName, unRepFlags, outFile);
        parser->setContentHandler(&handler);
        parser->setErrorHandler(&handler);
        parser->parse(xmlFile);
        errorCount = parser->getErrorCount();
        outFile.close();                        // close the file 
    }
    catch (const XMLException& toCatch) {
        cerr << "\nAn error occurred\n  Error: "
             << StrX(toCatch.getMessage())<< endl;
        XMLPlatformUtils::Terminate();
        return 4;
    }

    //  Delete the parser itself.  Must be done prior to calling Terminate, below.

    delete parser;

    // And call the termination method
    XMLPlatformUtils::Terminate();

    if (errorCount > 0)
        return 4;
    else
        return 0;
}

