#ifndef PS_DEFS_HXX
#define PS_DEFS_HXX

#ifndef _CPP_MAP
#include <map>
#endif
#ifndef _CPP_SET
#include <set>
#endif
#ifndef _CPP_VECTOR
#include <vector>
#endif
#ifndef _CPP_STRING
#include <string>
#endif

typedef int SpeciesID;

// ----------------------------------------------------------------
// SpeciesID <-> shortname mapping
// ----------------------------------------------------------------
typedef std::map<std::string, SpeciesID> Name2IDMap;
typedef Name2IDMap::iterator             Name2IDMapIter;
typedef Name2IDMap::const_iterator       Name2IDMapCIter;
typedef std::map<SpeciesID,std::string>  ID2NameMap;
typedef ID2NameMap::iterator             ID2NameMapIter;
typedef ID2NameMap::const_iterator       ID2NameMapCIter;

// ----------------------------------------------------------------
// SpeciesID list (as vector)
// ----------------------------------------------------------------
typedef std::vector<SpeciesID>   IDVector;
typedef IDVector::iterator       IDVectorIter;
typedef IDVector::const_iterator IDVectorCIter;

// ----------------------------------------------------------------
// SpeciesID list (as set)
// ----------------------------------------------------------------
typedef std::set<SpeciesID>   IDSet;
typedef IDSet::iterator       IDSetIter;
typedef IDSet::const_iterator IDSetCIter;

// ----------------------------------------------------------------
// SpeciesID <-> SpeciesID (as map)
// ----------------------------------------------------------------
typedef std::map<SpeciesID,SpeciesID> ID2IDMap;
typedef ID2IDMap::iterator            ID2IDMapIter;
typedef ID2IDMap::const_iterator      ID2IDMapCIter;

// ----------------------------------------------------------------
// SpeciesID <-> SpeciesID (as set)
// ----------------------------------------------------------------
typedef std::pair<SpeciesID,SpeciesID> ID2IDPair;
typedef std::set<ID2IDPair>            ID2IDSet;
typedef ID2IDSet::iterator             ID2IDSetIter;
typedef ID2IDSet::const_iterator       ID2IDSetCIter;

// ----------------------------------------------------------------
// (SpeciesID,SpeciesID) <-> set of SpeciesID (as map)
// ----------------------------------------------------------------
typedef std::map<ID2IDPair,IDSet>     IDID2IDSetMap;
typedef IDID2IDSetMap::iterator       IDID2IDSetMapIter;
typedef IDID2IDSetMap::const_iterator IDID2IDSetMapCIter;

// ----------------------------------------------------------------
// classes
// ----------------------------------------------------------------
class PS_Callback {
public:
    virtual void callback( void *_caller ) =0;
};

// ----------------------------------------------------------------
// functions
// ----------------------------------------------------------------
#ifndef NDEBUG
# define ps_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define ps_assert(bed)
#endif /* NDEBUG */

#else
#error ps_defs.hxx included twice
#endif
