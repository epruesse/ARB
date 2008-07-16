// ================================================================= //
//                                                                   //
//   File      : sec_bonddef.hxx                                     //
//   Purpose   :                                                     //
//   Time-stamp: <Mon Sep/10/2007 10:55 MET Coder@ReallySoft.de>     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef SEC_BONDDEF_HXX
#define SEC_BONDDEF_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif

using namespace AW;

#define SEC_BOND_BASE_CHARS 5
#define SEC_BOND_BASE_CHAR  "ACGTU"
#define SEC_BOND_PAIR_CHARS 8
#define SEC_BOND_PAIR_CHAR  ".o~#=+- @" // order is important for IUPAC bonds

class AW_device;
class AW_root;

class SEC_bond_def {
    char              bond[SEC_BOND_BASE_CHARS][SEC_BOND_BASE_CHARS];
    GB_alignment_type ali_type;

    int       get_index(char c) const;
    void      clear();
    GB_ERROR  insert(const char *pairs, char character);
    char      get_bond(char base1, char base2) const;
    char     *get_pair_string(char pair_char);

    void paint(AW_device *device, int gc, char bond, const Position& p1, const Position& p2, const Vector& toNextBase, const double& char_radius, AW_CL cd1, AW_CL cd2) const;

public:

    SEC_bond_def(GB_alignment_type aliType) : ali_type(aliType) { clear(); }

    GB_ERROR update(AW_root *awr, const char *changed_awar_name);

    void paint(AW_device *device, char base1, char base2, const Position& p1, const Position& p2, const Vector& toNextBase, const double& char_radius, AW_CL cd1, AW_CL cd2) const;
};


#else
#error sec_bonddef.hxx included twice
#endif // SEC_BONDDEF_HXX
