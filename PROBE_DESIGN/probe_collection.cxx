// ----------------------------------------------------------------------------
// probe_collection.cxx
// ----------------------------------------------------------------------------
// Implementations of classes used to create and manage probe collections in Arb.
// ----------------------------------------------------------------------------


#include "probe_collection.hxx"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <math.h>
#include <arbdbt.h>
#include <algorithm>

using namespace xercesc;

const size_t ArbMIN_PROBE_LENGTH = 6;

typedef std::pair<const std::string, ArbMatchResult*> ArbMatchResultPtrStringPair;
typedef std::pair<const double,      ArbMatchResult*> ArbMatchResultPtrDoublePair;

// ----------------------------------------------------------------------------
// Provide access to global objects
// ----------------------------------------------------------------------------

ArbProbeCollection& get_probe_collection() {
    static ArbProbeCollection g_probe_collection;
    return g_probe_collection;
}
ArbMatchResultsManager& get_results_manager() {
    static ArbMatchResultsManager g_results_manager;
    return g_results_manager;
}

static ArbStringCache& get_string_cache() {
    static ArbStringCache g_string_cache;
    return g_string_cache;
}

// ----------------------------------------------------------------------------
// ArbStringCache method implementations
// ----------------------------------------------------------------------------
void ArbStringCache::open() {
    char *uniqueName   = GB_unique_filename("ArbString", "cache");
    char *pTmpFileName = GB_create_tempfile(uniqueName);

    if (pTmpFileName != 0) {
        WriteCacheFile  = fopen(pTmpFileName, "wb");
        ReadCacheFile   = fopen(pTmpFileName, "rb");
        IsOpen          = ((WriteCacheFile != 0) && (ReadCacheFile != 0));
        FileName        = pTmpFileName;
    }

    free(pTmpFileName);
    free(uniqueName);
}

// ----------------------------------------------------------------------------

void ArbStringCache::close() {
    if (ReadCacheFile != 0) {
        fclose(ReadCacheFile);

        ReadCacheFile = 0;
    }

    if (WriteCacheFile != 0) {
        fclose(WriteCacheFile);
        unlink(FileName.c_str());

        WriteCacheFile = 0;
    }

    if (ReadBuffer != 0) {
        delete [] ReadBuffer;
        ReadBuffer = 0;
    }

    ReadBufferLength  = 0;
    IsOpen            = false;
}

// ----------------------------------------------------------------------------

bool ArbStringCache::allocReadBuffer(int nLength) const {
    if (nLength + 1 > ReadBufferLength) {
        if (ReadBuffer != 0) {
            delete [] ReadBuffer;
        }

        ReadBufferLength  = nLength + 1;
        ReadBuffer        = new char[ReadBufferLength];
    }

    return (ReadBuffer != 0);
}

// ----------------------------------------------------------------------------

ArbStringCache::ArbStringCache() {
    IsOpen            = false;
    WriteCacheFile    = 0;
    ReadCacheFile     = 0;
    ReadBuffer        = 0;
    ReadBufferLength  = 0;

    open();
}

// ----------------------------------------------------------------------------

ArbStringCache::~ArbStringCache() {
    close();
}

// ----------------------------------------------------------------------------

bool ArbStringCache::saveString(const char *pString, ArbCachedString& rCachedString) {
    bool bSaved = saveString(pString, strlen(pString), rCachedString);

    return (bSaved);
}

// ----------------------------------------------------------------------------

bool ArbStringCache::saveString(const char *pString, int nLength, ArbCachedString& rCachedString) {
    bool bSaved = false;

    if (IsOpen && (pString != 0)) {
        rCachedString.Len = nLength;

        fgetpos(WriteCacheFile, &rCachedString.Pos);
        fwrite(pString, nLength * sizeof(char), 1, WriteCacheFile);

        bSaved = true;
    }

    return (bSaved);
}

// ----------------------------------------------------------------------------

bool ArbStringCache::loadString(std::string& rString, const ArbCachedString& rCachedString) const {
    bool bLoaded = false;

    if (IsOpen                  &&
        (rCachedString.Len > 0) &&
        allocReadBuffer(rCachedString.Len))
    {
        fpos_t nPos = rCachedString.Pos;
        fsetpos(ReadCacheFile, &nPos);

        size_t read = fread(ReadBuffer, sizeof(char), rCachedString.Len, ReadCacheFile);
        if (read == size_t(rCachedString.Len)) {
            ReadBuffer[rCachedString.Len] = '\0';

            rString = ReadBuffer;
            bLoaded = true;
        }
    }

    return (bLoaded);
}

// ----------------------------------------------------------------------------

void ArbStringCache::flush() {
    if (IsOpen) {
        // Close and re-open to delete old cache file and create a new one
        close();
        open();
    }
}


// ----------------------------------------------------------------------------
// ArbProbeMatchWeighting method implementations
// ----------------------------------------------------------------------------
int ArbProbeMatchWeighting::toIndex(char nC) const {
    int nIndex = 0;

    switch (nC) {
        case 'A':
        case 'a': {
            nIndex = 0;
            break;
        }

        case 'C':
        case 'c': {
            nIndex = 1;
            break;
        }

        case 'G':
        case 'g': {
            nIndex = 2;
            break;
        }

        case 'U':
        case 'u':
        case 'T':
        case 't': {
            nIndex = 3;
            break;
        }

        default: {
            arb_assert(0);
            break;
        }
    }

    return (nIndex);
}

// ----------------------------------------------------------------------------

double ArbProbeMatchWeighting::positionalWeight(int nPos, int nLength) const {
    double dS       = -(::log(10) / Width);
    double dP       = ((2.0 * nPos - nLength) / nLength) - Bias;
    double dWeight  = ::exp(dS * dP * dP);

    return (dWeight);
}

// ----------------------------------------------------------------------------

void ArbProbeMatchWeighting::copy(const ArbProbeMatchWeighting& rCopy) {
    Width = rCopy.Width;
    Bias  = rCopy.Bias;

    for (int cx = 0 ; cx < 4 ; cx++) {
        for (int cy = 0 ; cy < 4 ; cy++) {
            PenaltyMatrix[cy][cx] = rCopy.PenaltyMatrix[cy][cx];
        }
    }
}

// ----------------------------------------------------------------------------

ArbProbeMatchWeighting::ArbProbeMatchWeighting()
    : ArbRefCount(),
      PenaltyMatrix()
{
    double aDefaultValues[16] = {
        0.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        1.0, 1.0, 1.0, 0.0
    };

    double dWidth = 1.0;
    double dBias  = 0.0;

    initialise(aDefaultValues, dWidth, dBias);
}

// ----------------------------------------------------------------------------

ArbProbeMatchWeighting::ArbProbeMatchWeighting(const double aValues[16], double dWidth, double dBias)
    : ArbRefCount(),
      PenaltyMatrix()
{
    Width = 1.0;
    Bias  = 0.0;

    initialise(aValues, dWidth, dBias);
}

// ----------------------------------------------------------------------------

ArbProbeMatchWeighting::ArbProbeMatchWeighting(const ArbProbeMatchWeighting& rCopy)
    : ArbRefCount(),
      PenaltyMatrix()
{
    copy(rCopy);
}

// ----------------------------------------------------------------------------

ArbProbeMatchWeighting::~ArbProbeMatchWeighting() {
    // Do Nothing
}

// ----------------------------------------------------------------------------

ArbProbeMatchWeighting& ArbProbeMatchWeighting::operator = (const ArbProbeMatchWeighting& rCopy) {
    copy(rCopy);
    return *this;
}

// ----------------------------------------------------------------------------

void ArbProbeMatchWeighting::initialise(const double aValues[16], double dWidth, double dBias) {
    Width = dWidth;
    Bias  = dBias;

    int cz = 0;

    for (int cx = 0 ; cx < 4 ; cx++) {
        for (int cy = 0 ; cy < 4 ; cy++) {
            PenaltyMatrix[cy][cx] = aValues[cz];

            cz++;
        }
    }
}

// ----------------------------------------------------------------------------

bool ArbProbeMatchWeighting::initialise(const char *pCSValues, const char *pCSWidth, const char *pCSBias) {
    bool bInitialised = false;

    if ((pCSValues != 0) &&
        (pCSWidth  != 0) &&
        (pCSBias   != 0))
    {
        double dWidth = 0;
        double dBias  = 0;
        double aValues[16] = {
            0.0, 1.0, 1.0, 1.0,
            1.0, 0.0, 1.0, 1.0,
            1.0, 1.0, 0.0, 1.0,
            1.0, 1.0, 1.0, 0.0
        };

        int nItems = ::sscanf(pCSValues,
                              "%lg%lg%lg%lg"
                              "%lg%lg%lg%lg"
                              "%lg%lg%lg%lg"
                              "%lg%lg%lg%lg",
                              aValues,      aValues + 1,  aValues + 2,  aValues + 3,
                              aValues + 4,  aValues + 5,  aValues + 6,  aValues + 7,
                              aValues + 8,  aValues + 9,  aValues + 10, aValues + 11,
                              aValues + 12, aValues + 13, aValues + 14, aValues + 15);

        nItems += ::sscanf(pCSWidth, "%lg", &dWidth);
        nItems += ::sscanf(pCSBias, "%lg", &dBias);

        if (nItems == (16 + 2)) {
            initialise(aValues, dWidth, dBias);

            bInitialised = true;
        }
    }

    return (bInitialised);
}

// ----------------------------------------------------------------------------

void ArbProbeMatchWeighting::getParameters(double aValues[16],
        double& dWidth,
        double& dBias) const
{
    int cz = 0;

    for (int cx = 0 ; cx < 4 ; cx++) {
        for (int cy = 0 ; cy < 4 ; cy++) {
            aValues[cz] = PenaltyMatrix[cy][cx];

            cz++;
        }
    }

    dWidth  = Width;
    dBias   = Bias;
}

// ----------------------------------------------------------------------------

void ArbProbeMatchWeighting::writeXML(FILE *hFile, const char *pPrefix) const {
    bool bFirst = true;

    ::fprintf(hFile, "%s<match_weighting width=\"%g\" bias=\"%g\">\n", pPrefix, Width, Bias);
    ::fprintf(hFile, "%s  <penalty_matrix values=\"", pPrefix);

    for (int cx = 0 ; cx < 4 ; cx++) {
        for (int cy = 0 ; cy < 4 ; cy++) {
            if (bFirst) {
                ::fprintf(hFile, "%g", PenaltyMatrix[cy][cx]);

                bFirst = false;
            }
            else {
                ::fprintf(hFile, " %g", PenaltyMatrix[cy][cx]);
            }
        }
    }

    ::fprintf(hFile, "\"/>\n");
    ::fprintf(hFile, "%s</match_weighting>\n", pPrefix);
}

// ----------------------------------------------------------------------------

double ArbProbeMatchWeighting::matchWeight(const char *pSequenceA, const char *pSequenceB) const {
    double dWeight = -1.0;

    if ((pSequenceA != 0) && (pSequenceB != 0)) {
        int nLengthA = strlen(pSequenceA);
        int nLengthB = strlen(pSequenceB);
        int nLength  = (nLengthA < nLengthB) ? nLengthA : nLengthB;

        const char *pA = pSequenceA;
        const char *pB = pSequenceB;

        dWeight = 0.0;

        for (int cn = 0 ; cn < nLength ; cn++) {
            switch (*pA) {
                case 'N':
                case 'n': {
                    // Wildcard match so don't penalise it
                    break;
                }

                default: {
                    switch (*pB) {
                        case 'N':
                        case 'n': {
                            // Wildcard match so don't penalise it
                            break;
                        }

                        default: {
                            dWeight += PenaltyMatrix[toIndex(*pA)][toIndex(*pB)] * positionalWeight(cn, nLength);
                            break;
                        }
                    }
                    break;
                }
            }

            pA++;
            pB++;
        }
    }
    else {
        arb_assert(0);
    }

    return (dWeight);
}

// ----------------------------------------------------------------------------

double ArbProbeMatchWeighting::matchWeightResult(const char *pProbeSequence, const char *pMatchResult) const {
    double dWeight = -1.0;

    if ((pProbeSequence != 0) && (pMatchResult != 0)) {
        int nMatchLength;
        int nLengthResult = strlen(pMatchResult);
        int nLength       = strlen(pProbeSequence);

        const char *pS = pProbeSequence;
        const char *pR = pMatchResult;
        int         cn;

        if (nLength <= nLengthResult) {
            nMatchLength = nLength;
        }
        else {
            nMatchLength = nLengthResult;
        }

        dWeight = 0.0;

        for (cn = 0 ; cn < nMatchLength ; cn++) {
            char cB;

            switch (*pR) {
                case '-':
                case '=': {
                    cB = *pS;
                    break;
                }

                // I have no idea about this one. There is no mention in Arb about what
                // .... means in terms of match results so I'll just assume it to be the
                // same as U
                case '.': {
                    cB = 'U';
                    break;
                }

                default: {
                    cB = *pR;
                    break;
                }
            }

            switch (*pS) {
                case 'N':
                case 'n': {
                    // Wildcard match so don't penalise it
                    break;
                }

                default: {
                    switch (cB) {
                        case 'N':
                        case 'n': {
                            // Wildcard match so don't penalise it
                            break;
                        }

                        default: {
                            dWeight += PenaltyMatrix[toIndex(*pS)][toIndex(cB)] * positionalWeight(cn, nLength);
                            break;
                        }
                    }
                    break;
                }
            }

            pS++;
            pR++;
        }

        int nNoMatch = toIndex('U');

        for (; cn < nLength ; cn++) {
            dWeight += PenaltyMatrix[toIndex(*pS)][nNoMatch] * positionalWeight(cn, nLength);

            pS++;
        }
    }
    else {
        arb_assert(0);
    }

    return (dWeight);
}


// ----------------------------------------------------------------------------
// ArbProbe method implementations
// ----------------------------------------------------------------------------
ArbProbe::ArbProbe()
    : ArbRefCount(),
      Name(),
      Sequence(),
      DisplayName()
{
    // Do nothing
}

// ----------------------------------------------------------------------------

ArbProbe::ArbProbe(const char *pName, const char *pSequence)
    : ArbRefCount(),
      Name(),
      Sequence(),
      DisplayName()
{
    nameAndSequence(pName, pSequence);
}

// ----------------------------------------------------------------------------

ArbProbe::ArbProbe(const ArbProbe& rCopy)
    : ArbRefCount(),
      Name(),
      Sequence(),
      DisplayName()
{
    // Note that we do a copy of Name and Sequence via c_str() because std:string
    // shares internal buffers between strings if using a copy constructor and
    // this can potentially result in memory corrupting if the owner string is deleted
    nameAndSequence(rCopy.Name.c_str(), rCopy.Sequence.c_str());
}

// ----------------------------------------------------------------------------

ArbProbe::~ArbProbe() {
    // Do nothing
}

// ----------------------------------------------------------------------------

void ArbProbe::writeXML(FILE *hFile, const char *pPrefix) const {
    ::fprintf(hFile, "%s<probe seq=\"%s\" name=\"%s\"/>\n", pPrefix, Sequence.c_str(), Name.c_str());
}

// ----------------------------------------------------------------------------

void ArbProbe::nameAndSequence(const char *pName, const char *pSequence) {
    if (pName != 0) {
        Name = pName;
    }
    else {
        Name = "";
    }

    if (pSequence != 0) {
        Sequence = pSequence;
    }
    else {
        Sequence = "";

        arb_assert(0);
    }

    DisplayName = Name + ":" + Sequence;
}

// ----------------------------------------------------------------------------

int ArbProbe::allowedMismatches() const {
    size_t nProbe_Length     = Sequence.length();
    size_t nAllowedMistaches = (nProbe_Length - ArbMIN_PROBE_LENGTH) / 2;

    // Arb probe server doesn't seem to like having more that 20 mis-matches.
    if (nAllowedMistaches > 20) {
        nAllowedMistaches = 20;
    }

    return (nAllowedMistaches);
}


// ----------------------------------------------------------------------------
// ArbProbeCollection method implementations
// ----------------------------------------------------------------------------
void ArbProbeCollection::flush() {
    ArbProbePtrListIter Iter;

    for (Iter = ProbeList.begin() ; Iter != ProbeList.end() ; ++Iter) {
        ArbProbe *pProbe = *Iter;

        if (pProbe != 0) {
            pProbe->free();
        }
    }

    ProbeList.clear();
}

// ----------------------------------------------------------------------------

void ArbProbeCollection::copy(const ArbProbePtrList& rList) {
    ArbProbePtrListConstIter  Iter;

    for (Iter = rList.begin() ; Iter != rList.end() ; ++Iter) {
        ArbProbe *pProbe = *Iter;

        if (pProbe != 0) {
            ProbeList.push_back(pProbe);
            pProbe->lock();
        }
    }
}

// ----------------------------------------------------------------------------

ArbProbeCollection::ArbProbeCollection()
    : ArbRefCount(),
      Name(),
      ProbeList(),
      MatchWeighting()
{
    HasChanged = false;
}

// ----------------------------------------------------------------------------

ArbProbeCollection::ArbProbeCollection(const char *pName)
    : ArbRefCount(),
      Name(),
      ProbeList(),
      MatchWeighting()
{
    // Note that we do a copy of Name via c_str() because std:string shares
    // internal buffers between strings if using a copy constructor and this can
    // potentially result in memory corrupting if the owner string is deleted
    name(pName);

    HasChanged = false;
}

// ----------------------------------------------------------------------------

ArbProbeCollection::ArbProbeCollection(const ArbProbeCollection& rCopy)
    : ArbRefCount(),
      Name(),
      ProbeList(),
      MatchWeighting(rCopy.MatchWeighting)
{
    name(rCopy.name().c_str());
    copy(rCopy.ProbeList);

    HasChanged = false;
}

// ----------------------------------------------------------------------------

ArbProbeCollection::~ArbProbeCollection() {
    flush();
}

// ----------------------------------------------------------------------------

ArbProbeCollection& ArbProbeCollection::operator = (const ArbProbeCollection& rCopy) {
    flush();
    name(rCopy.name().c_str());
    copy(rCopy.ProbeList);

    MatchWeighting  = rCopy.MatchWeighting;
    HasChanged      = false;

    return (*this);
}

// ----------------------------------------------------------------------------

static bool elementHasName(DOMElement *pElement, const char *pName) {
    bool bHasName = false;

    if (pName != 0) {
        XMLCh *pNameStr;

        pNameStr  = XMLString::transcode(pName);
        bHasName  = XMLString::equals(pElement->getTagName(), pNameStr);
        XMLString::release(&pNameStr);
    }

    return (bHasName);
}

// ----------------------------------------------------------------------------

static bool isElement(DOMNode *pNode, DOMElement*& pElement, const char *pName) {
    bool  bIsElement = false;

    if (pNode != 0) {
        short nNodeType = pNode->getNodeType();

        if (nNodeType == DOMNode::ELEMENT_NODE) {
            pElement = dynamic_cast<xercesc::DOMElement*>(pNode);

            if (pName != 0) {
                bIsElement = elementHasName(pElement, pName);
            }
            else {
                bIsElement = true;
            }
        }
    }

    return (bIsElement);
}

// ----------------------------------------------------------------------------

bool ArbProbeCollection::openXML(const char *pFileAndPath, std::string& rErrorMessage) {
    bool bOpened = false;

    rErrorMessage = "";

    if (pFileAndPath != 0 && pFileAndPath[0]) {
        struct stat   FileStatus;
        int           nResult = ::stat(pFileAndPath, &FileStatus);

        if ((nResult == 0) && !S_ISDIR(FileStatus.st_mode)) {
            flush();

            XMLPlatformUtils::Initialize();

            // We need braces to ensure Parser goes out of scope be Terminate() is called
            {
                XercesDOMParser Parser;

                Parser.setValidationScheme(XercesDOMParser::Val_Never);
                Parser.setDoNamespaces(false);
                Parser.setDoSchema(false);
                Parser.setLoadExternalDTD(false);

                try {
                    Parser.parse(pFileAndPath);

                    DOMDocument *pDoc  = Parser.getDocument();
                    DOMElement  *pRoot = pDoc->getDocumentElement();

                    if ((pRoot != 0) && elementHasName(pRoot, "probe_collection")) {
                        char        *pNameAttr          = 0;
                        XMLCh       *pNameAttrStr       = XMLString::transcode("name");
                        DOMNodeList *pPC_Children;
                        XMLSize_t    nPC_Count;
                        bool         bHasProbeList      = false;
                        bool         bHasMatchWeighting = false;

                        pNameAttr = XMLString::transcode(pRoot->getAttribute(pNameAttrStr));
                        name(pNameAttr);
                        XMLString::release(&pNameAttr);

                        pPC_Children  = pRoot->getChildNodes();
                        nPC_Count     = pPC_Children->getLength();

                        for (XMLSize_t cx = 0 ; cx < nPC_Count ; ++cx) {
                            DOMNode    *pPC_Node    = pPC_Children->item(cx);
                            DOMElement *pPC_Element = 0;

                            if (isElement(pPC_Node, pPC_Element, "probe_list")) {
                                DOMNodeList *pPL_Children;
                                XMLSize_t    nPL_Count;
                                XMLCh       *pSeqAttrStr;

                                pSeqAttrStr   = XMLString::transcode("seq");
                                pPL_Children  = pPC_Element->getChildNodes();
                                nPL_Count     = pPL_Children->getLength();

                                for (XMLSize_t cy = 0 ; cy < nPL_Count ; ++cy) {
                                    DOMNode    *pPL_Node    = pPL_Children->item(cy);
                                    DOMElement *pPL_Element = 0;
                                    char       *pSeqAttr    = 0;

                                    if (isElement(pPL_Node, pPL_Element, "probe")) {
                                        pNameAttr = XMLString::transcode(pPL_Element->getAttribute(pNameAttrStr));
                                        pSeqAttr  = XMLString::transcode(pPL_Element->getAttribute(pSeqAttrStr));

                                        add(pNameAttr, pSeqAttr);

                                        XMLString::release(&pNameAttr);
                                        XMLString::release(&pSeqAttr);

                                        bHasProbeList = true;
                                    }
                                }
                            }
                            else if (isElement(pPC_Node, pPC_Element, "match_weighting")) {
                                DOMNodeList *pMW_Children;
                                XMLSize_t    nMW_Count;
                                XMLCh       *pWidthAttrStr = XMLString::transcode("width");
                                XMLCh       *pBiasAttrStr  = XMLString::transcode("bias");
                                char        *pWidthStr     = XMLString::transcode(pPC_Element->getAttribute(pWidthAttrStr));
                                char        *pBiasStr      = XMLString::transcode(pPC_Element->getAttribute(pBiasAttrStr));

                                pMW_Children    = pPC_Element->getChildNodes();
                                nMW_Count       = pMW_Children->getLength();

                                for (XMLSize_t cy = 0 ; cy < nMW_Count ; ++cy) {
                                    DOMNode    *pPM_Node    = pMW_Children->item(cy);
                                    DOMElement *pPM_Element = 0;

                                    if (isElement(pPM_Node, pPM_Element, "penalty_matrix")) {
                                        XMLCh *pValuesAttrStr = XMLString::transcode("values");
                                        char  *pValuesAttr    = XMLString::transcode(pPM_Element->getAttribute(pValuesAttrStr));

                                        if (MatchWeighting.initialise(pValuesAttr, pWidthStr, pBiasStr)) {
                                            bHasMatchWeighting = true;
                                        }
                                        else {
                                            rErrorMessage = "Too few penalty_matrix values";
                                        }

                                        XMLString::release(&pValuesAttrStr);
                                        XMLString::release(&pValuesAttr);
                                    }
                                }

                                XMLString::release(&pWidthAttrStr);
                                XMLString::release(&pBiasAttrStr);
                                XMLString::release(&pWidthStr);
                                XMLString::release(&pBiasStr);
                            }
                        }

                        XMLString::release(&pNameAttrStr);

                        bOpened = bHasProbeList && bHasMatchWeighting;

                        if (!bHasProbeList) {
                            rErrorMessage += "\nprobe_list missing or empty";
                        }

                        if (!bHasMatchWeighting) {
                            rErrorMessage += "\nmatch_weighting missing or empty";
                        }
                    }

                    if (bOpened) {
                        HasChanged  = false;
                    }
                }
                catch (xercesc::DOMException& e1) {
                    char *message = xercesc::XMLString::transcode(e1.getMessage());

                    rErrorMessage  = "Error parsing file: ";
                    rErrorMessage += message;

                    XMLString::release(&message);
                }
                catch (xercesc::XMLException& e2) {
                    char *message = xercesc::XMLString::transcode(e2.getMessage());

                    rErrorMessage  = "Error parsing file: ";
                    rErrorMessage += message;

                    XMLString::release(&message);
                }
            }

            XMLPlatformUtils::Terminate();
        }
        else if (nResult == EACCES) {
            rErrorMessage = "Search permission is denied";
        }
        else if (nResult == EIO) {
            rErrorMessage = "Error reading from the file system";
        }
        else if (nResult == ELOOP) {
            rErrorMessage = "Loop exists in symbolic links";
        }
        else if (nResult == ENAMETOOLONG) {
            rErrorMessage = "Path name too long";
        }
        else if (nResult == ENOENT) {
            rErrorMessage = "Component of path existing file or missing";
        }
        else if (nResult == ENOTDIR) {
            rErrorMessage = "Component of path not a directory";
        }
        else if (nResult == EOVERFLOW) {
            rErrorMessage = "";
        }
    }
    else {
        rErrorMessage = "Please select a file to load";
    }

    return (bOpened);
}

// ----------------------------------------------------------------------------

bool ArbProbeCollection::saveXML(const char *pFileAndPath) const {
    bool bSaved = false;

    if (pFileAndPath != 0) {
        FILE *hFile = ::fopen(pFileAndPath, "wt");

        if (hFile != 0) {
            ArbProbePtrListConstIter  Iter;

            ::fprintf(hFile, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
            ::fprintf(hFile, "<!DOCTYPE probe_collection>\n");
            ::fprintf(hFile, "<probe_collection name=\"%s\">\n", Name.c_str());
            ::fprintf(hFile, "  <probe_list>\n");

            for (Iter = ProbeList.begin() ; Iter != ProbeList.end() ; ++Iter) {
                const ArbProbe *pProbe = *Iter;

                if (pProbe != 0) {
                    pProbe->writeXML(hFile, "    ");
                }
            }

            ::fprintf(hFile, "  </probe_list>\n");

            MatchWeighting.writeXML(hFile, "  ");

            ::fprintf(hFile, "</probe_collection>\n");

            ::fclose(hFile);

            HasChanged = false;
        }
    }

    return (bSaved);
}

// ----------------------------------------------------------------------------

void ArbProbeCollection::setParameters(const double aValues[16], double dWidth, double dBias) {
    MatchWeighting.setParameters(aValues, dWidth, dBias);

    HasChanged = true;
}

// ----------------------------------------------------------------------------

void ArbProbeCollection::getParameters(double aValues[16], double& dWidth, double& dBias) const {
    MatchWeighting.getParameters(aValues, dWidth, dBias);
}

// ----------------------------------------------------------------------------

struct hasSequence {
    std::string seq;
    hasSequence(std::string seq_) : seq(seq_) {}
    bool operator()(const ArbProbe *probe) {
        return probe && probe->sequence() == seq;
    }
};

const ArbProbe *ArbProbeCollection::find(const char *pSequence) const {
    if (pSequence) {
        ArbProbePtrListConstIter found = find_if(ProbeList.begin(), ProbeList.end(), hasSequence(pSequence));
        if (found != ProbeList.end()) {
            return *found;
        }
    }
    return NULL;
}

// ----------------------------------------------------------------------------

bool ArbProbeCollection::add(const char *pName, const char *pSequence, const ArbProbe **ppProbe) {
    bool bAdded = false;

    if (pSequence && (strlen(pSequence) >= ArbMIN_PROBE_LENGTH)) {
        ArbProbe *pNewProbe = new ArbProbe(pName, pSequence);

        if (pNewProbe != 0) {
            ProbeList.push_back(pNewProbe);

            if (ppProbe != 0) *ppProbe = pNewProbe;

            HasChanged  = true;
            bAdded      = true;
        }
    }

    return (bAdded);
}

bool ArbProbeCollection::replace(const char *oldSequence, const char *pName, const char *pSequence, const ArbProbe **ppProbe) {
    bool bReplaced = false;

    if (oldSequence && pSequence && (strlen(pSequence) >= ArbMIN_PROBE_LENGTH)) {
        ArbProbePtrListIter found = find_if(ProbeList.begin(), ProbeList.end(), hasSequence(oldSequence));
        if (found != ProbeList.end()) {
            (*found)->free();
            *found = new ArbProbe(pName, pSequence);

            if (ppProbe != 0) *ppProbe = *found;

            HasChanged = true;
            bReplaced  = true;
        }
    }

    return (bReplaced);
}


// ----------------------------------------------------------------------------

bool ArbProbeCollection::remove(const char *pSequence) {
    bool bRemoved  = false;

    if (pSequence != 0) {
        ArbProbePtrListIter Iter;
        std::string         rSequence(pSequence);

        for (Iter = ProbeList.begin() ; Iter != ProbeList.end() ; ++Iter) {
            ArbProbe *pTestProbe = *Iter;

            if ((pTestProbe != 0) && (pTestProbe->sequence() == rSequence)) {
                ProbeList.erase(Iter);

                pTestProbe->free();

                HasChanged  = true;
                bRemoved    = true;
                break;
            }
        }
    }

    return (bRemoved);
}

// ----------------------------------------------------------------------------

bool ArbProbeCollection::clear() {
    bool bClear = false;

    if (ProbeList.size() > 0) {
        flush();

        HasChanged  = true;
        bClear      = true;
    }

    return (bClear);
}

// ----------------------------------------------------------------------------

void ArbProbeCollection::name(const char *pName) {
    if (pName != 0) {
        Name = pName;
    }
    else {
        Name = "";
    }

    HasChanged = true;
}


// ----------------------------------------------------------------------------
// ArbMatchResult method implementations
// ----------------------------------------------------------------------------
ArbMatchResult::ArbMatchResult()
    : ArbRefCount(),
      CachedResultA(),
      CachedResultB()
{
    Probe   = 0;
    Weight  = 0.0;
    Padding = 0;
    Index   = 0;
}

// ----------------------------------------------------------------------------

ArbMatchResult::ArbMatchResult(const ArbProbe *pProbe, const char *pResult, int nSplitPoint, double dWeight)
    : ArbRefCount(),
      CachedResultA(),
      CachedResultB()
{
    ArbStringCache& g_string_cache = get_string_cache();
    g_string_cache.saveString(pResult, nSplitPoint, CachedResultA);
    g_string_cache.saveString(pResult + nSplitPoint, CachedResultB);

    Probe   = pProbe;
    Weight  = dWeight;
    Padding = 0;
    Index   = 0;

    if (Probe != 0) {
        Probe->lock();
    }
}

// ----------------------------------------------------------------------------

ArbMatchResult::ArbMatchResult(const ArbMatchResult& rCopy)
    : ArbRefCount(),
      CachedResultA(rCopy.CachedResultA),
      CachedResultB(rCopy.CachedResultB)
{
    Probe   = rCopy.Probe;
    Weight  = rCopy.Weight;
    Padding = rCopy.Padding;
    Index   = rCopy.Index;

    if (Probe != 0) {
        Probe->lock();
    }
}

// ----------------------------------------------------------------------------

ArbMatchResult::~ArbMatchResult() {
    if (Probe != 0) {
        Probe->free();
    }
}

// ----------------------------------------------------------------------------

ArbMatchResult& ArbMatchResult::operator = (const ArbMatchResult& rCopy) {
    Probe         = rCopy.Probe;
    CachedResultA = rCopy.CachedResultA;
    CachedResultB = rCopy.CachedResultB;
    Padding       = rCopy.Padding;
    Weight        = rCopy.Weight;

    if (Probe != 0) {
        Probe->lock();
    }

    return (*this);
}

// ----------------------------------------------------------------------------

void ArbMatchResult::addedHeadline(std::string& rHeadline) {
    rHeadline = "---error--probe-----------------";
}

// ----------------------------------------------------------------------------

void ArbMatchResult::weightAndResult(std::string& rDest) const {
    char        sBuffer[64] = {0};
    std::string sResult;

    result(sResult);
    sprintf(sBuffer, "%8g %22.22s   ", Weight, Probe != 0 ? Probe->name().c_str() : "");

    rDest  = sBuffer;
    rDest += sResult;
}

// ----------------------------------------------------------------------------

void ArbMatchResult::result(std::string& sResult) const {
    std::string sResultA;
    std::string sResultB;

    ArbStringCache& g_string_cache = get_string_cache();
    g_string_cache.loadString(sResultA, CachedResultA);
    g_string_cache.loadString(sResultB, CachedResultB);

    sResult.append(sResultA);

    if (Padding > 0) {
        sResult.append(Padding, ' ');
    }

    sResult.append(sResultB);
}


// ----------------------------------------------------------------------------
// ArbMatchResultSet method implementations
// ----------------------------------------------------------------------------
void ArbMatchResultSet::flush() {
    ArbMatchResultPtrByStringMultiMapIter Iter;

    for (Iter = ResultMap.begin() ; Iter != ResultMap.end() ; ++Iter) {
        ArbMatchResult *pResult = (*Iter).second;

        if (pResult != 0) {
            pResult->free();
        }
    }

    ResultMap.clear();
    CommentList.clear();

    Headline    = "";
    EndFullName = 0;
}

// ----------------------------------------------------------------------------

void ArbMatchResultSet::copy(const ArbMatchResultPtrByStringMultiMap& rMap) {
    ArbMatchResultPtrByStringMultiMapConstIter Iter;

    for (Iter = rMap.begin() ; Iter != rMap.end() ; ++Iter) {
        const std::string&    sKey    = (*Iter).first;
        const ArbMatchResult *pResult = (*Iter).second;

        if (pResult != 0) {
            ArbMatchResult *pCopy = new ArbMatchResult(*pResult);

            if (pCopy != 0) {
                ResultMap.insert(ArbMatchResultPtrStringPair(sKey, pCopy));
            }
        }
    }

    ResultMap.clear();
    CommentList.clear();
}

// ----------------------------------------------------------------------------

ArbMatchResultSet::ArbMatchResultSet()
    : ArbRefCount(),
      Headline(),
      ResultMap(),
      CommentList()
{
    Probe       = 0;
    Index       = 0;
    EndFullName = 0;
}

// ----------------------------------------------------------------------------

ArbMatchResultSet::ArbMatchResultSet(const ArbProbe *pProbe)
    : ArbRefCount(),
      Headline(),
      ResultMap(),
      CommentList()
{
    Probe       = 0;
    Index       = 0;
    EndFullName = 0;

    initialise(pProbe, 0);
}

// ----------------------------------------------------------------------------

ArbMatchResultSet::ArbMatchResultSet(const ArbMatchResultSet& rCopy)
    : ArbRefCount(),
      Headline(rCopy.Headline.c_str()),
      ResultMap(),
      CommentList(rCopy.CommentList)
{
    Probe       = 0;
    Index       = 0;
    EndFullName = 0;

    initialise(rCopy.Probe, rCopy.Index);
    copy(rCopy.ResultMap);

    EndFullName = rCopy.EndFullName;
}

// ----------------------------------------------------------------------------

ArbMatchResultSet::~ArbMatchResultSet() {
    flush();

    if (Probe != 0) {
        Probe->free();
    }
}

// ----------------------------------------------------------------------------

void ArbMatchResultSet::initialise(const ArbProbe *pProbe, int nIndex) {
    Probe = pProbe;
    Index = nIndex;

    if (Probe != 0) {
        Probe->lock();
    }

    flush();
}

// ----------------------------------------------------------------------------

bool ArbMatchResultSet::add(const char *pName,
                            const char *pFullName,
                            const char *pMatchPart,
                            const char *pResult,
                            const ArbProbeMatchWeighting& rMatchWeighting)
{
    bool bAdded = false;

    if ((pResult    != 0) &&
        (pName      != 0) &&
        (pFullName  != 0) &&
        (pMatchPart != 0) &&
        (Probe      != 0) &&
        (Probe->sequence().length() > 0))
    {
        double          dWeight;
        const char     *pMatchStart = pMatchPart;
        bool            bContinue   = true;
        std::string     sKey(pName);
        ArbMatchResult *pMatchResult;

        while (bContinue) {
            switch (*pMatchStart) {
                case '-': {
                    pMatchStart++;

                    bContinue = false;
                    break;
                }

                case '\0':
                case '=': {
                    bContinue = false;
                    break;
                }

                default: {
                    pMatchStart++;

                    bContinue = true;
                }
            }
        }

        dWeight       = rMatchWeighting.matchWeightResult(Probe->sequence().c_str(), pMatchStart);
        pMatchResult  = new ArbMatchResult(Probe, pResult, EndFullName, dWeight);

        if (pMatchResult != 0) {
            pMatchResult->index(Index);
            ResultMap.insert(ArbMatchResultPtrStringPair(sKey, pMatchResult));

            bAdded = true;
        }
    }

    return (bAdded);
}

// ----------------------------------------------------------------------------

bool ArbMatchResultSet::isMatched(const ArbStringList& rCladeList,
                                  bool& bPartialMatch,
                                  double dThreshold,
                                  double dCladeMarkedThreshold,
                                  double dCladePartiallyMarkedThreshold) const
{
    bool  bMatched    = false;
    int   nCladeSize  = rCladeList.size();

    if (nCladeSize > 0) {
        int nMatchedSize          = (int)(nCladeSize * dCladeMarkedThreshold + 0.5);
        int nPartiallyMatchedSize = (int)(nCladeSize * dCladePartiallyMarkedThreshold + 0.5);
        int nMatchedCount         = 0;

        for (ArbStringListConstIter Iter = rCladeList.begin() ; Iter != rCladeList.end() ; ++Iter) {
            const std::string&  rName = *Iter;

            if (isMatched(rName, dThreshold)) {
                nMatchedCount++;
            }
        }

        bMatched      = (nMatchedCount >= nMatchedSize);
        bPartialMatch = false;

        // Only check for partial match if we don't have a match. If a partial
        // match is found then isMatched() should return true.
        if (!bMatched) {
            bPartialMatch = (nMatchedCount >= nPartiallyMatchedSize);
            bMatched      = bPartialMatch;
        }
    }

    return (bMatched);
}

// ----------------------------------------------------------------------------

bool ArbMatchResultSet::isMatched(const std::string& rName, double dThreshold) const {
    bool bMatched = false;

    ArbMatchResultPtrByStringMultiMapConstIter Iter = ResultMap.find(rName);

    if (Iter != ResultMap.end()) {
        const ArbMatchResult *pResult = (*Iter).second;

        bMatched = pResult->weight() <= dThreshold;
    }

    return (bMatched);
}

// ----------------------------------------------------------------------------

bool ArbMatchResultSet::addComment(const char *pComment) {
    bool bAdded = false;

    if ((pComment != 0) &&
        (Probe    != 0) &&
        (Probe->sequence().length() > 0))
    {
        CommentList.push_back(std::string(pComment));

        bAdded = true;
    }

    return (bAdded);
}

// ----------------------------------------------------------------------------

void ArbMatchResultSet::findMaximumWeight(double& dMaximumWeight) const {
    ArbMatchResultPtrByStringMultiMapConstIter Iter;

    for (Iter = ResultMap.begin() ; Iter != ResultMap.end() ; ++Iter) {
        const ArbMatchResult *pResult = (*Iter).second;

        if ((pResult != 0) && (dMaximumWeight < pResult->weight())) {
            dMaximumWeight = pResult->weight();
        }
    }
}

// ----------------------------------------------------------------------------

void ArbMatchResultSet::enumerateResults(ArbMatchResultPtrByDoubleMultiMap& rMap, int nMaxFullName) {
    ArbMatchResultPtrByStringMultiMapIter Iter;

    for (Iter = ResultMap.begin() ; Iter != ResultMap.end() ; ++Iter) {
        ArbMatchResult *pResult = (*Iter).second;

        if (pResult != 0) {
            pResult->padding(nMaxFullName - EndFullName);
            rMap.insert(ArbMatchResultPtrDoublePair(pResult->weight(), pResult));
        }
    }
}


// ----------------------------------------------------------------------------
// ArbMatchResultsManager method implementations
// ----------------------------------------------------------------------------
void ArbMatchResultsManager::flush() {
    ArbMatchResultPtrByStringMultiMapIter Iter;

    for (Iter = ResultsMap.begin() ; Iter != ResultsMap.end() ; ++Iter) {
        ArbMatchResult *pMatchResult = (*Iter).second;

        if (pMatchResult != 0) {
            pMatchResult->free();
        }
    }

    ResultsMap.clear();
}

// ----------------------------------------------------------------------------

void ArbMatchResultsManager::initFileName() {
    char *uniqueName   = GB_unique_filename("ArbMatchResults", "txt");
    char *pTmpFileName = GB_create_tempfile(uniqueName);

    if (pTmpFileName != 0) {
        ResultsFileName = pTmpFileName;
    }

    free(pTmpFileName);
    free(uniqueName);
}

// ----------------------------------------------------------------------------

ArbMatchResultsManager::ArbMatchResultsManager()
    : ResultsMap(),
      ResultSetMap(),
      ResultsFileName()
{
    MaximumWeight = 0.0;
    initFileName();
}

// ----------------------------------------------------------------------------

ArbMatchResultsManager::ArbMatchResultsManager(const ArbMatchResultsManager& rCopy)
    : ResultsMap(),
      ResultSetMap(rCopy.ResultSetMap),
      ResultsFileName()
{
    MaximumWeight = rCopy.MaximumWeight;
    updateResults();
    initFileName();
}

// ----------------------------------------------------------------------------

ArbMatchResultsManager::~ArbMatchResultsManager() {
    ResultSetMap.clear();
    flush();

    if (ResultsFileName.length() > 0) {
        unlink(ResultsFileName.c_str());
    }

    // This assumes that there is only ever on instance of ArbMatchResultsManager
    // which is true at the moment. Slightly dodgey but it will stop the cache
    // file from ballooning out too much.
    get_string_cache().flush();
}

// ----------------------------------------------------------------------------

void ArbMatchResultsManager::reset() {
    ResultSetMap.clear();
    flush();

    if (ResultsFileName.length() > 0) {
        unlink(ResultsFileName.c_str());
    }

    // This assumes that there is only ever on instance of ArbMatchResultsManager
    // which is true at the moment. Slightly dodgey but it will stop the cache
    // file from ballooning out too much.
    get_string_cache().flush();
}

// ----------------------------------------------------------------------------

ArbMatchResultSet *ArbMatchResultsManager::addResultSet(const ArbProbe *pProbe) {
    ArbMatchResultSet *pResultSet = 0;

    if (pProbe != 0) {
        pResultSet = (ArbMatchResultSet*)findResultSet(pProbe->sequence().c_str());

        if (pResultSet == 0) {
            ResultSetMap[pProbe->sequence()] = ArbMatchResultSet();

            pResultSet = (ArbMatchResultSet*)findResultSet(pProbe->sequence().c_str());
        }
    }

    return (pResultSet);
}

// ----------------------------------------------------------------------------

const ArbMatchResultSet *ArbMatchResultsManager::findResultSet(const char *pProbeSequence) const {
    const ArbMatchResultSet *pResultSet = 0;

    if (pProbeSequence != 0) {
        ArbMatchResultSetByStringMapConstIter Iter = ResultSetMap.find(std::string(pProbeSequence));

        if (Iter != ResultSetMap.end()) {
            pResultSet = &((*Iter).second);
        }
    }

    return (pResultSet);
}

// ----------------------------------------------------------------------------

void ArbMatchResultsManager::updateResults() {
    ArbMatchResultSetByStringMapIter            Iter;
    ArbMatchResultPtrByStringMultiMapConstIter  IterR;

    flush();

    MaximumWeight = 0.0;

    for (Iter = ResultSetMap.begin() ; Iter != ResultSetMap.end() ; ++Iter) {
        ArbMatchResultSet&  rMatchResultSet = (*Iter).second;

        rMatchResultSet.findMaximumWeight(MaximumWeight);

        for (IterR = rMatchResultSet.resultMap().begin() ; IterR != rMatchResultSet.resultMap().end() ; ++IterR) {
            const std::string&    rKey         = (*IterR).first;
            const ArbMatchResult *pMatchResult = (*IterR).second;

            pMatchResult->lock();
            ResultsMap.insert(ArbMatchResultPtrStringPair(rKey, (ArbMatchResult*)pMatchResult));
        }
    }
}

// ----------------------------------------------------------------------------

int ArbMatchResultsManager::enumerate_results(ArbMatchResultsEnumCallback pCallback, void *pContext) {
    int nResults = 0;

    bool bAborted = false;

    if (pCallback != 0) {
        ArbMatchResultPtrByDoubleMultiMap rResultsMap;

        // Need to compile the results sorted in ascending match weight.
        ArbMatchResultSetByStringMapIter  Iter;
        int                               nItem         = 0;
        int                               nItems        = 1;
        int                               nMaxFullName  = 0;
        std::string                       sHeadline;

        for (Iter = ResultSetMap.begin() ; (Iter != ResultSetMap.end()) && !bAborted ; ++Iter) {
            ArbMatchResultSet&  rMatchResultSet = (*Iter).second;

            if (rMatchResultSet.endFullName() > nMaxFullName) {
                ArbMatchResult::addedHeadline(sHeadline);

                sHeadline   += rMatchResultSet.headline();
                nMaxFullName = rMatchResultSet.endFullName();
            }
        }

        if (pCallback(pContext, sHeadline.c_str(), true, nItem, nItems)) {
            bAborted = true;
        }

        for (Iter = ResultSetMap.begin() ; (Iter != ResultSetMap.end()) && !bAborted ; ++Iter) {
            ArbMatchResultSet&  rMatchResultSet = (*Iter).second;

            rMatchResultSet.enumerateResults(rResultsMap, nMaxFullName);

            ArbStringListConstIter CommentIter;

            for (CommentIter = rMatchResultSet.commentList().begin() ;
                 CommentIter != rMatchResultSet.commentList().begin() ;
                 ++CommentIter)
            {
                const std::string& rComment = *CommentIter;

                if (pCallback(pContext, rComment.c_str(), true, nItem, nItems)) {
                    bAborted = true;
                    break;
                }
            }
        }

        ArbMatchResultPtrByDoubleMultiMapIter ResIter;

        nItems = rResultsMap.size();

        for (ResIter = rResultsMap.begin() ; (ResIter != rResultsMap.end()) && !bAborted ; ++ResIter) {
            const ArbMatchResult *pResult = (*ResIter).second;

            if (pResult != 0) {
                std::string sResult;

                pResult->weightAndResult(sResult);

                if (pCallback(pContext, sResult.c_str(), false, nItem, nItems)) {
                    bAborted = true;
                    break;
                }

                nResults++;
                nItem++;
            }
        }
    }

    return (nResults);
}

// ----------------------------------------------------------------------------

const char *ArbMatchResultsManager::resultsFileName() const {
    return (ResultsFileName.c_str());
}

// ----------------------------------------------------------------------------

void ArbMatchResultsManager::openResultsFile() const {
    pid_t pid = fork();

    if (pid == 0) {
        // We are the child process. Execute system command to open the file.
        std::string sCommand("\"xdg-open ");

        sCommand += ResultsFileName;
        sCommand += "\"";

        execl("/bin/sh", sCommand.c_str(), (char *)0);
        exit(0);
    }
}
