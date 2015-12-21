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

//  ----------------------------------------------------------------------------
//  Map of Int by Int
//  ----------------------------------------------------------------------------
typedef std::pair<const int, int>                   ArbIntIntPair;
typedef std::map<int, int>                          ArbIntByIntMap;
typedef std::map<int, int>::iterator                ArbIntByIntMapIter;
typedef std::map<int, int>::const_iterator          ArbIntByIntMapConstIter;
typedef std::map<int, int>::reverse_iterator        ArbIntByIntMapRIter;
typedef std::map<int, int>::const_reverse_iterator  ArbIntByIntMapConstRIter;


//  ----------------------------------------------------------------------------
//  List of std::string objects
//  ----------------------------------------------------------------------------
typedef std::list<std::string>                          ArbStringList;
typedef std::list<std::string>::iterator                ArbStringListIter;
typedef std::list<std::string>::const_iterator          ArbStringListConstIter;
typedef std::list<std::string>::reverse_iterator        ArbStringListRIter;
typedef std::list<std::string>::const_reverse_iterator  ArbStringListConstRIter;


// ----------------------------------------------------------------------------
// struct ArbCachedString
// ----------------------------------------------------------------------------
// This structure is a descriptor for a string cache on file by ArbStringCache
// ----------------------------------------------------------------------------
struct ArbCachedString {
    fpos_t  Pos;
    int     Len;

    fpos_t  pos() const;
    int     len() const;
};

// ----------------------------------------------------------------------------

inline fpos_t ArbCachedString::pos() const {
    return (Pos);
}

// ----------------------------------------------------------------------------

inline int ArbCachedString::len() const {
    return (Len);
}


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
    ArbRefCount();
    ArbRefCount(const ArbRefCount& rCopy);
    virtual ~ArbRefCount();

    void        lock() const;
    bool        unlock() const;
    void        free() const;
};

// ----------------------------------------------------------------------------

inline ArbRefCount::ArbRefCount() {
    RefCount = 1;
}

// ----------------------------------------------------------------------------

inline ArbRefCount::ArbRefCount(const ArbRefCount&) {
    RefCount = 1;
}

// ----------------------------------------------------------------------------

inline ArbRefCount::~ArbRefCount() {
    // Do nothing
}

// ----------------------------------------------------------------------------

inline void ArbRefCount::lock() const {
    RefCount++;
}

// ----------------------------------------------------------------------------

inline bool ArbRefCount::unlock() const {
    RefCount--;

    return (RefCount == 0);
}

// ----------------------------------------------------------------------------

inline void ArbRefCount::free() const {
    if (unlock()) {
        delete this;
    }
}


// ----------------------------------------------------------------------------
// class ArbProbeMatchWeighting
// ----------------------------------------------------------------------------
// This class defines the match weighting algorithm and parameterisation for
// the weighting applied to a match.
// ----------------------------------------------------------------------------
class ArbProbeMatchWeighting : public ArbRefCount {
private:
    double                  PenaltyMatrix[4][4];
    double                  Width;
    double                  Bias;

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

    void setParameters(const double aValues[16], double dWidth, double dBias);
    void getParameters(double aValues[16], double& dWidth, double& dBias) const;

    void writeXML(FILE *hFile, const char *pPrefix) const;

    double matchWeight(const char *pSequenceA, const char *pSequenceB) const;
    double matchWeightResult(const char *pProbeSequence, const char *pMatchResult) const;
};

// ----------------------------------------------------------------------------

inline void ArbProbeMatchWeighting::setParameters(const double aValues[16], double dWidth, double dBias) {
    initialise(aValues, dWidth, dBias);
}


// ----------------------------------------------------------------------------
// class ArbProbe
// ----------------------------------------------------------------------------
// This class is an abstraction of a probe used in the probe matching with
// specificity feature in Arb.
// ----------------------------------------------------------------------------
class ArbProbe : public ArbRefCount {
private:
    std::string             Name;
    std::string             Sequence;
    std::string             DisplayName;

public:
    ArbProbe();
    ArbProbe(const char *pName, const char *pSequence);
    ArbProbe(const ArbProbe& rCopy);
    virtual ~ArbProbe();

    void writeXML(FILE *hFile, const char *pPrefix) const;

    void nameAndSequence(const char* pName, const char* pSequence);

    const std::string& name() const;
    const std::string& sequence() const;
    const std::string& displayName() const;

    int allowedMismatches() const;
};

// ----------------------------------------------------------------------------

inline const std::string& ArbProbe::name() const {
    return (Name);
}

// ----------------------------------------------------------------------------

inline const std::string& ArbProbe::sequence() const {
    return (Sequence);
}

// ----------------------------------------------------------------------------

inline const std::string& ArbProbe::displayName() const {
    return (DisplayName);
}


//  ----------------------------------------------------------------------------
//  List of ArbProbe* objects
//  ----------------------------------------------------------------------------
typedef std::list<ArbProbe*>                          ArbProbePtrList;
typedef std::list<ArbProbe*>::iterator                ArbProbePtrListIter;
typedef std::list<ArbProbe*>::const_iterator          ArbProbePtrListConstIter;
typedef std::list<ArbProbe*>::reverse_iterator        ArbProbePtrListRIter;
typedef std::list<ArbProbe*>::const_reverse_iterator  ArbProbePtrListConstRIter;


//  ----------------------------------------------------------------------------
//  Map of ArbProbe* objects by string
//  ----------------------------------------------------------------------------
typedef std::pair<const std::string, ArbProbe*>                   ArbProbePtrStringPair;
typedef std::map<std::string, ArbProbe*>                          ArbProbePtrByStringMap;
typedef std::map<std::string, ArbProbe*>::iterator                ArbProbePtrByStringMapIter;
typedef std::map<std::string, ArbProbe*>::const_iterator          ArbProbePtrByStringMapConstIter;
typedef std::map<std::string, ArbProbe*>::reverse_iterator        ArbProbePtrByStringMapRIter;
typedef std::map<std::string, ArbProbe*>::const_reverse_iterator  ArbProbePtrByStringMapConstRIter;


// ----------------------------------------------------------------------------
// class ArbProbeCollection
// ----------------------------------------------------------------------------
// This class is a collection of a probes to match against in the probe
// matching with specificity feature in Arb.
// ----------------------------------------------------------------------------
class ArbProbeCollection : public ArbRefCount {
private:
    std::string                     Name;
    ArbProbePtrList                 ProbeList;
    ArbProbeMatchWeighting          MatchWeighting;
    mutable bool                    HasChanged;

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

    const ArbProbePtrList&        probeList() const;
    const ArbProbeMatchWeighting& matchWeighting() const;

    const ArbProbe *find(const char *pSequence) const;

    bool add(const char *pName, const char *pSequence, const ArbProbe **ppProbe = 0);
    bool replace(const char *oldSequence, const char *pName, const char *pSequence, const ArbProbe **ppProbe = 0);
    bool remove(const char *pSequence);
    bool clear();

    void               name(const char *pName);
    const std::string& name() const;

    bool hasChanged() const;
    bool hasProbes() const;
};

// ----------------------------------------------------------------------------

inline const ArbProbePtrList& ArbProbeCollection::probeList() const {
    return (ProbeList);
}

// ----------------------------------------------------------------------------

inline const ArbProbeMatchWeighting& ArbProbeCollection::matchWeighting() const {
    return (MatchWeighting);
}

// ----------------------------------------------------------------------------

inline const std::string& ArbProbeCollection::name() const {
    return (Name);
}

// ----------------------------------------------------------------------------

inline bool ArbProbeCollection::hasChanged() const {
    return (HasChanged);
}

// ----------------------------------------------------------------------------

inline bool ArbProbeCollection::hasProbes() const {
    return (ProbeList.size() > 0);
}


// ----------------------------------------------------------------------------
// class ArbMatchResult
// ----------------------------------------------------------------------------
class ArbMatchResult : public ArbRefCount {
private:
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
    double      weight() const;

    void padding(int nPadding) const;
    void index(int nIndex) const;
    int  index() const;
};

// ----------------------------------------------------------------------------

inline double ArbMatchResult::weight() const {
    return (Weight);
}

// ----------------------------------------------------------------------------

inline void ArbMatchResult::padding(int nPadding) const {
    Padding = nPadding;
}

// ----------------------------------------------------------------------------

inline void ArbMatchResult::index(int nIndex) const {
    Index = nIndex;
}

// ----------------------------------------------------------------------------

inline int ArbMatchResult::index() const {
    return (Index);
}


//  ----------------------------------------------------------------------------
//  List of ArbMatchResult objects
//  ----------------------------------------------------------------------------
typedef std::list<ArbMatchResult>                           ArbMatchResultList;
typedef std::list<ArbMatchResult>::iterator                 ArbMatchResultListIter;
typedef std::list<ArbMatchResult>::const_iterator           ArbMatchResultListConstIter;
typedef std::list<ArbMatchResult>::reverse_iterator         ArbMatchResultListRIter;
typedef std::list<ArbMatchResult>::const_reverse_iterator   ArbMatchResultListConstRIter;


//  ----------------------------------------------------------------------------
//  Map of ArbMatchResult* by string
//  ----------------------------------------------------------------------------
typedef std::pair<const std::string, ArbMatchResult*>                   ArbMatchResultPtrStringPair;
typedef std::map<std::string, ArbMatchResult*>                          ArbMatchResultPtrByStringMap;
typedef std::map<std::string, ArbMatchResult*>::iterator                ArbMatchResultPtrByStringMapIter;
typedef std::map<std::string, ArbMatchResult*>::const_iterator          ArbMatchResultPtrByStringMapConstIter;
typedef std::map<std::string, ArbMatchResult*>::reverse_iterator        ArbMatchResultPtrByStringMapRIter;
typedef std::map<std::string, ArbMatchResult*>::const_reverse_iterator  ArbMatchResultPtrByStringMapConstRIter;


//  ----------------------------------------------------------------------------
//  multimap of ArbMatchResult* by string
//  ----------------------------------------------------------------------------
typedef std::multimap<std::string, ArbMatchResult*>                         ArbMatchResultPtrByStringMultiMap;
typedef std::multimap<std::string, ArbMatchResult*>::iterator               ArbMatchResultPtrByStringMultiMapIter;
typedef std::multimap<std::string, ArbMatchResult*>::const_iterator         ArbMatchResultPtrByStringMultiMapConstIter;
typedef std::multimap<std::string, ArbMatchResult*>::reverse_iterator       ArbMatchResultPtrByStringMultiMapRIter;
typedef std::multimap<std::string, ArbMatchResult*>::const_reverse_iterator ArbMatchResultPtrByStringMultiMapConstRIter;


//  ----------------------------------------------------------------------------
//  multimap of ArbMatchResult* by double
//  ----------------------------------------------------------------------------
//  We use this as a means of results sorting by match weight
//  ----------------------------------------------------------------------------
typedef std::pair<const double, ArbMatchResult*>                       ArbMatchResultPtrDoublePair;
typedef std::multimap<double, ArbMatchResult*>                         ArbMatchResultPtrByDoubleMultiMap;
typedef std::multimap<double, ArbMatchResult*>::iterator               ArbMatchResultPtrByDoubleMultiMapIter;
typedef std::multimap<double, ArbMatchResult*>::const_iterator         ArbMatchResultPtrByDoubleMultiMapConstIter;
typedef std::multimap<double, ArbMatchResult*>::reverse_iterator       ArbMatchResultPtrByDoubleMultiMapRIter;
typedef std::multimap<double, ArbMatchResult*>::const_reverse_iterator ArbMatchResultPtrByDoubleMultiMapConstRIter;


//  ----------------------------------------------------------------------------
//  List of strings
//  ----------------------------------------------------------------------------
typedef std::list<std::string>                          ArbStringList;
typedef std::list<std::string>::iterator                ArbStringListIter;
typedef std::list<std::string>::const_iterator          ArbStringListConstIter;
typedef std::list<std::string>::reverse_iterator        ArbStringListRIter;
typedef std::list<std::string>::const_reverse_iterator  ArbStringListConstRIter;


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

    const ArbMatchResultPtrByStringMultiMap&  resultMap() const;

    bool isMatched(const ArbStringList& rCladeList,
                   bool&  bPartialMatch,
                   double dThreshold,
                   double dCladeMarkedThreshold,
                   double dCladePartiallyMarkedThreshold) const;

    bool isMatched(const std::string& rName, double dThreshold) const;
    const ArbStringList& commentList() const;

    bool hasResults() const;

    const ArbProbe *probe() const;

    void headline(const char *pHeadline, int nEndFullName);
    const std::string& headline() const;

    int endFullName() const;

    void enumerateResults(ArbMatchResultPtrByDoubleMultiMap& rMap, int nMaxFullName);

    int index() const;
};

// ----------------------------------------------------------------------------

inline void ArbMatchResultSet::headline(const char *pHeadline, int nEndFullName) {
    if (pHeadline != 0) {
        Headline    = pHeadline;
        EndFullName = nEndFullName;
    }
}

// ----------------------------------------------------------------------------

inline const std::string& ArbMatchResultSet::headline() const {
    return (Headline);
}

// ----------------------------------------------------------------------------

inline int ArbMatchResultSet::endFullName() const {
    return (EndFullName);
}

// ----------------------------------------------------------------------------

inline const ArbMatchResultPtrByStringMultiMap& ArbMatchResultSet::resultMap() const {
    return (ResultMap);
}

// ----------------------------------------------------------------------------

inline const ArbStringList& ArbMatchResultSet::commentList() const {
    return (CommentList);
}

// ----------------------------------------------------------------------------

inline bool ArbMatchResultSet::hasResults() const {
    bool bHasResults = ((Probe != 0) && (ResultMap.size() > 0));

    return (bHasResults);
}

// ----------------------------------------------------------------------------

inline const ArbProbe *ArbMatchResultSet::probe() const {
    return (Probe);
}

// ----------------------------------------------------------------------------

inline int ArbMatchResultSet::index() const {
    return (Index);
}


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
private:
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

    const ArbMatchResultPtrByStringMultiMap& resultsMap() const;
    const ArbMatchResultSetByStringMap&      resultSetMap() const;

    double maximumWeight() const;

    int enumerate_results(ArbMatchResultsEnumCallback pCallback, void *pContext);

    const char *resultsFileName() const;
    void openResultsFile() const;

    bool hasResults() const;
};

// ----------------------------------------------------------------------------

inline const ArbMatchResultPtrByStringMultiMap& ArbMatchResultsManager::resultsMap() const {
    return (ResultsMap);
}

// ----------------------------------------------------------------------------

inline const ArbMatchResultSetByStringMap& ArbMatchResultsManager::resultSetMap() const {
    return (ResultSetMap);
}

// ----------------------------------------------------------------------------

inline double ArbMatchResultsManager::maximumWeight() const {
    return (MaximumWeight);
}

// ----------------------------------------------------------------------------

inline bool ArbMatchResultsManager::hasResults() const {
    return (ResultSetMap.size() > 0);
}

// ----------------------------------------------------------------------------

ArbProbeCollection&     get_probe_collection();
ArbMatchResultsManager& get_results_manager();

#else
#error probe_collection.hxx included twice
#endif // PROBE_COLLECTION_HXX
