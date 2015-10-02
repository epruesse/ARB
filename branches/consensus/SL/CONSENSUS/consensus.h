// ================================================================= //
//                                                                   //
//   File      : consensus.h                                         //
//   Purpose   : interface for consensus calculation                 //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2015   //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef CONSENSUS_H
#define CONSENSUS_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

#define CAS_INTERNAL -1
#define CAS_NTREE    1
#define CAS_EDIT4    2

#ifndef CONSENSUS_AWAR_SOURCE
# error you need to define CONSENSUS_AWAR_SOURCE before including consensus.h (allowed values: CAS_NTREE, CAS_EDIT4)
#endif

struct ConsensusBuildParams {
    bool countgaps;   // count gaps? (otherwise they are completely ignored)
    int  gapbound;    // limit in % for gaps. If more gaps occur -> '-'
    bool group;       // whether to group characters (NUC: using ambiguous IUPAC codes; AMINO: using amino groups)
    int  considbound; // limit in %. Bases occurring that often are used to create ambiguity codes (other bases are ignored). gaps are ignored when checking this limit.
    int  upper;       // limit in %. If explicit base (or ambiguity code) occurs that often -> use upper case character
    int  lower;       // limit in %. If explicit base (or ambiguity code) occurs that often -> use lower case character. Otherwise use '.'

    static void force_in_range(int low, int& val, int high) {
        val = std::min(std::max(low, val), high);
    }

    void make_valid() {
        force_in_range(0, gapbound,    100);
        force_in_range(0, considbound, 100);
        force_in_range(0, upper,       100);
        force_in_range(0, lower,       100);
    }

#if (CONSENSUS_AWAR_SOURCE == CAS_NTREE)
    ConsensusBuildParams(AW_root *awr)
        : countgaps  (awr->awar(AWAR_CONSENSUS_COUNTGAPS)->read_int()),
          gapbound   (awr->awar(AWAR_CONSENSUS_GAPBOUND)->read_int()),
          group      (awr->awar(AWAR_CONSENSUS_GROUP)->read_int()),
          considbound(awr->awar(AWAR_CONSENSUS_CONSIDBOUND)->read_int()),
          upper      (awr->awar(AWAR_CONSENSUS_UPPER)->read_int()),
          lower      (awr->awar(AWAR_CONSENSUS_LOWER)->read_int())
    {
        make_valid();
    }
#else
# if (CONSENSUS_AWAR_SOURCE == CAS_EDIT4)
    ConsensusBuildParams(AW_root *awr)
        : countgaps  (awr->awar(ED4_AWAR_CONSENSUS_COUNTGAPS)->read_int()),
          gapbound   (awr->awar(ED4_AWAR_CONSENSUS_GAPBOUND)->read_int()),
          group      (awr->awar(ED4_AWAR_CONSENSUS_GROUP)->read_int()),
          considbound(awr->awar(ED4_AWAR_CONSENSUS_CONSIDBOUND)->read_int()),
          upper      (awr->awar(ED4_AWAR_CONSENSUS_UPPER)->read_int()),
          lower      (awr->awar(ED4_AWAR_CONSENSUS_LOWER)->read_int())
    {
        make_valid();
    }
# else
#  if (CONSENSUS_AWAR_SOURCE == CAS_INTERNAL)
    ConsensusBuildParams(AW_root *awr); // produce link error if used
#  else
#   error CONSENSUS_AWAR_SOURCE has invalid value
#  endif
# endif
#endif

#if defined(UNIT_TESTS) // UT_DIFF
    ConsensusBuildParams() // uses defaults of EDIT4 awars (@@@ check NTREE awar defaults)
        : countgaps(true),
          gapbound(60),
          group(1),
          considbound(30),
          upper(95),
          lower(70)
    {
        make_valid();
    }
#endif
};

#else
#error consensus.h included twice
#endif // CONSENSUS_H
