// ----------------------------------------------------------------------------
// probe_collection.hxx
// ----------------------------------------------------------------------------
// Declarations of classes used to create and manage probe collections in Arb.
// ----------------------------------------------------------------------------

#ifndef PROBE_COLLECTION_HXX
#define PROBE_COLLECTION_HXX

#ifndef ARB_ASSERT_H
#include "arb_assert.h"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef _GLIBCXX_ITERATOR
#include <iterator>
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_CERRNO
#include <cerrno>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif
#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif

// ----------------------------------------------------------------------------
// struct ArbCachedString
// ----------------------------------------------------------------------------
// This structure is a descriptor for a string cache on file by ArbStringCache
// ----------------------------------------------------------------------------
struct ArbCachedString {
    fpos_t Pos;
    int    Len;
};

// ----------------------------------------------------------------------------
// class ArbStringCache
// ----------------------------------------------------------------------------
// This class is used as a disk based storage of strings (typically result
// output) to minimise memory usage. The result string is only used for
// display purposes (the Arb results list) so by not storing it in physical
// memory in the ArProbeResult class we can reduce our memory consumption
// considerably.
// ----------------------------------------------------------------------------
class ArbStringCache : virtual Noncopyable {
    std::string   FileName;
    FILE         *WriteCacheFile;
    mutable FILE *ReadCacheFile;
    mutable char *ReadBuffer;
    mutable int   ReadBufferLength;
    bool          IsOpen;

protected:
    void                    open();
    void                    close();
    bool                    allocReadBuffer(int nLength) const;

public:
    ArbStringCache();
    virtual ~ArbStringCache();

    bool saveString(const char *pString, ArbCachedString& rCachedString);
    bool saveString(const char *pString, int nLength, ArbCachedString& rCachedString);
    bool loadString(std::string& rString, const ArbCachedString& rCachedString) const;
    void flush();
};


// ----------------------------------------------------------------------------
// class ArbRefCount
// ----------------------------------------------------------------------------
class ArbRefCount {
    mutable int RefCount;

public:
    ArbRefCount() : RefCount(1) {}
    ArbRefCount(const ArbRefCount&) :  RefCount(1) {}
    virtual ~ArbRefCount() {}

    void lock() const {
        RefCount++;
    }
    bool unlock() const {
        RefCount--;
        return (RefCount == 0);
    }
    void free() const {
        if (unlock()) {
            delete this;
        }
    }
};

// ----------------------------------------------------------------------------
// class ArbProbeMatchWeighting
// ----------------------------------------------------------------------------
// This class defines the match weighting algorithm and parameterisation for
// the weighting applied to a match.
// ----------------------------------------------------------------------------
class ArbProbeMatchWeighting : public ArbRefCount {
    double PenaltyMatrix[4][4];
    double Width;
    double Bias;

protected:
    int                     toIndex(char nC) const;
    double                  positionalWeight(int nPos, int nLength) const;

    void                    copy(const ArbProbeMatchWeighting& rCopy);

public:
    ArbProbeMatchWeighting();
    ArbProbeMatchWeighting(const double aValues[16],
                           double dWidth,
                           double dBias);

    ArbProbeMatchWeighting(const ArbProbeMatchWeighting& rCopy);
    virtual ~ArbProbeMatchWeighting();

    ArbProbeMatchWeighting& operator = (const ArbProbeMatchWeighting& rCopy);

    void initialise(const double aValues[16], double dWidth, double dBias);
    bool initialise(const char *pCSValues, const char *pCSWidth, const char *pCSBias);

    void setParameters(const double aValues[16], double dWidth, double dBias) {
        initialise(aValues, dWidth, dBias);
    }
    void getParameters(double aValues[16], double& dWidth, double& dBias) const;

    void writeXML(FILE *hFile, const char *pPrefix) const;

    double matchWeight(const char *pSequenceA, const char *pSequenceB) const;
    double matchWeightResult(const char *pProbeSequence, const char *pMatchResult) const;
};

// ----------------------------------------------------------------------------
// class ArbProbe
// ----------------------------------------------------------------------------
// This class is an abstraction of a probe used in the probe matching with
// specificity feature in Arb.
// ----------------------------------------------------------------------------
class ArbProbe : public ArbRefCount {
    std::string Name;
    std::string Sequence;
    std::string DisplayName;

public:
    ArbProbe();
    ArbProbe(const char *pName, const char *pSequence);
    ArbProbe(const ArbProbe& rCopy);
    virtual ~ArbProbe();

    void writeXML(FILE *hFile, const char *pPrefix) const;

    void nameAndSequence(const char* pName, const char* pSequence);

    const std::string& name()        const { return Name; }
    const std::string& sequence()    const { return Sequence; }
    const std::string& displayName() const { return DisplayName; }

    int allowedMismatches() const;
};

//  ----------------------------------------------------------------------------
//  List of ArbProbe* objects
//  ----------------------------------------------------------------------------
typedef std::list<ArbProbe*>            ArbProbePtrList;
typedef ArbProbePtrList::iterator       ArbProbePtrListIter;
typedef ArbProbePtrList::const_iterator ArbProbePtrListConstIter;

// ----------------------------------------------------------------------------
// class ArbProbeCollection
// ----------------------------------------------------------------------------
// This class is a collection of a probes to match against in the probe
// matching with specificity feature in Arb.
// ----------------------------------------------------------------------------
class ArbProbeCollection : public ArbRefCount {
    std::string            Name;
    ArbProbePtrList        ProbeList;
    ArbProbeMatchWeighting MatchWeighting;
    mutable bool           HasChanged;

protected:
    void flush();
    void copy(const ArbProbePtrList& rList);

public:
    ArbProbeCollection();
    ArbProbeCollection(const char *pName);
    ArbProbeCollection(const ArbProbeCollection& rCopy);
    virtual ~ArbProbeCollection();

    ArbProbeCollection& operator = (const ArbProbeCollection& rCopy);

    bool openXML(const char *pFileAndPath, std::string& rErrorMessage);
    bool saveXML(const char *pFileAndPath) const;

    void setParameters(const double aValues[16], double dWidth, double dBias);
    void getParameters(double aValues[16], double& dWidth, double& dBias) const;

    const ArbProbePtrList&        probeList()      const { return ProbeList; }
    const ArbProbeMatchWeighting& matchWeighting() const { return MatchWeighting; }

    const ArbProbe *find(const char *pSequence) const;

    bool add(const char *pName, const char *pSequence, const ArbProbe **ppProbe = 0);
    bool replace(const char *oldSequence, const char *pName, const char *pSequence, const ArbProbe **ppProbe = 0);
    bool remove(const char *pSequence);
    bool clear();

    void               name(const char *pName);
    const std::string& name() const { return Name; }

    bool hasChanged() const { return HasChanged; }
    bool hasProbes()  const { return ProbeList.size() > 0; }
};

// ----------------------------------------------------------------------------
// class ArbMatchResult
// ----------------------------------------------------------------------------
class ArbMatchResult : public ArbRefCount {
    const ArbProbe  *Probe;
    ArbCachedString  CachedResultA;
    ArbCachedString  CachedResultB;
    double           Weight;
    mutable int      Index;
    mutable int      Padding;

public:
    ArbMatchResult();
    ArbMatchResult(const ArbProbe *pProbe, const char *pResult, int nSplitPoint, double dWeight);
    ArbMatchResult(const ArbMatchResult& rCopy);
    virtual ~ArbMatchResult();

    ArbMatchResult& operator = (const ArbMatchResult& rCopy);

    static void addedHeadline(std::string& rHeadline);
    void        weightAndResult(std::string& rDest) const;
    void        result(std::string& sResult) const;
    double      weight() const { return Weight; }

    void padding(int nPadding) const { Padding = nPadding; }
    void index(int nIndex) const { Index = nIndex; }
    int  index() const { return Index; }
};

//  ----------------------------------------------------------------------------
//  multimap of ArbMatchResult* by string
//  ----------------------------------------------------------------------------

typedef std::multimap<std::string, ArbMatchResult*>       ArbMatchResultPtrByStringMultiMap;
typedef ArbMatchResultPtrByStringMultiMap::iterator       ArbMatchResultPtrByStringMultiMapIter;
typedef ArbMatchResultPtrByStringMultiMap::const_iterator ArbMatchResultPtrByStringMultiMapConstIter;

//  ----------------------------------------------------------------------------
//  multimap of ArbMatchResult* by double
//  ----------------------------------------------------------------------------
//  We use this as a means of results sorting by match weight
//  ----------------------------------------------------------------------------

typedef std::multimap<double, ArbMatchResult*>      ArbMatchResultPtrByDoubleMultiMap;
typedef ArbMatchResultPtrByDoubleMultiMap::iterator ArbMatchResultPtrByDoubleMultiMapIter;


//  ----------------------------------------------------------------------------
//  List of strings
//  ----------------------------------------------------------------------------

typedef std::list<std::string>                 ArbStringList;
typedef std::list<std::string>::const_iterator ArbStringListConstIter;

// ----------------------------------------------------------------------------
// class ArbMatchResultSet
// ----------------------------------------------------------------------------
class ArbMatchResultSet : public ArbRefCount {
    const ArbProbe                    *Probe;
    std::string                        Headline;
    ArbMatchResultPtrByStringMultiMap  ResultMap;
    ArbStringList                      CommentList;
    int                                Index;
    int                                EndFullName;

protected:
    void                                      flush();
    void                                      copy(const ArbMatchResultPtrByStringMultiMap& rMap);

public:
    ArbMatchResultSet();
    ArbMatchResultSet(const ArbProbe *pProbe);

    ArbMatchResultSet(const ArbMatchResultSet& rCopy);
    DECLARE_ASSIGNMENT_OPERATOR(ArbMatchResultSet);
    virtual ~ArbMatchResultSet();

    void initialise(const ArbProbe *pProbe, int nIndex);

    bool add(const char *pName,
             const char *pFullName,
             const char *pMatchPart,
             const char *pResult,
             const ArbProbeMatchWeighting& rMatchWeighting);

    bool addComment(const char *pComment);

    void findMaximumWeight(double& dMaximumWeight) const;

    const ArbMatchResultPtrByStringMultiMap&  resultMap() const { return ResultMap; }

    bool isMatched(const ArbStringList& rCladeList,
                   bool&  bPartialMatch,
                   double dThreshold,
                   double dCladeMarkedThreshold,
                   double dCladePartiallyMarkedThreshold) const;

    bool isMatched(const std::string& rName, double dThreshold) const;
    const ArbStringList& commentList() const { return CommentList; }

    bool hasResults() const {
        return (Probe != 0) && (ResultMap.size() > 0);
    }

    const ArbProbe *probe() const { return Probe; }

    void headline(const char *pHeadline, int nEndFullName) {
        if (pHeadline != 0) {
            Headline    = pHeadline;
            EndFullName = nEndFullName;
        }
    }
    const std::string& headline() const { return Headline; }
    int endFullName() const { return EndFullName; }

    void enumerateResults(ArbMatchResultPtrByDoubleMultiMap& rMap, int nMaxFullName);

    int index() const { return Index; }
};

//  ----------------------------------------------------------------------------
//  Map of ArbMatchResultSet objects by string
//  ----------------------------------------------------------------------------
typedef std::pair<const std::string, ArbMatchResultSet>                   ArbMatchResultSetStringPair;
typedef std::map<std::string, ArbMatchResultSet>                          ArbMatchResultSetByStringMap;
typedef std::map<std::string, ArbMatchResultSet>::iterator                ArbMatchResultSetByStringMapIter;
typedef std::map<std::string, ArbMatchResultSet>::const_iterator          ArbMatchResultSetByStringMapConstIter;
typedef std::map<std::string, ArbMatchResultSet>::reverse_iterator        ArbMatchResultSetByStringMapRIter;
typedef std::map<std::string, ArbMatchResultSet>::const_reverse_iterator  ArbMatchResultSetByStringMapConstRIter;


//  ----------------------------------------------------------------------------
//  Map of int by string
//  ----------------------------------------------------------------------------
typedef std::pair<int, const std::string>                   ArbStringIntPair;
typedef std::map<int, std::string>                          ArbIntByStringMap;
typedef std::map<int, std::string>::iterator                ArbIntByStringMapIter;
typedef std::map<int, std::string>::const_iterator          ArbIntByStringMapConstIter;
typedef std::map<int, std::string>::reverse_iterator        ArbIntByStringMapRIter;
typedef std::map<int, std::string>::const_reverse_iterator  ArbIntByStringMapConstRIter;


typedef bool (*ArbMatchResultsEnumCallback)(void *pContext, const char *pResult, bool bIsComment, int nItem, int nItems);


// ----------------------------------------------------------------------------
// class ArbMatchResultsManager
// ----------------------------------------------------------------------------
class ArbMatchResultsManager {
    ArbMatchResultPtrByStringMultiMap ResultsMap;
    ArbMatchResultSetByStringMap      ResultSetMap;
    double                            MaximumWeight;
    std::string                       ResultsFileName;

protected:
    void flush();
    void initFileName();

public:
    ArbMatchResultsManager();
    ArbMatchResultsManager(const ArbMatchResultsManager& rCopy);
    virtual ~ArbMatchResultsManager();

    void reset();

    ArbMatchResultSet       *addResultSet(const ArbProbe *pProbe);
    const ArbMatchResultSet *findResultSet(const char *pProbeSequence) const;
    void                     updateResults();

    const ArbMatchResultPtrByStringMultiMap& resultsMap()   const { return ResultsMap; }
    const ArbMatchResultSetByStringMap&      resultSetMap() const { return ResultSetMap; }

    double maximumWeight() const { return MaximumWeight; }

    int enumerate_results(ArbMatchResultsEnumCallback pCallback, void *pContext);

    const char *resultsFileName() const;
    void openResultsFile() const;

    bool hasResults() const { return ResultSetMap.size() > 0; }
};

// ----------------------------------------------------------------------------

ArbProbeCollection&     get_probe_collection();
ArbMatchResultsManager& get_results_manager();

#else
#error probe_collection.hxx included twice
#endif // PROBE_COLLECTION_HXX
