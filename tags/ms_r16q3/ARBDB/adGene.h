// =============================================================== //
//                                                                 //
//   File      : adGene.h                                          //
//   Purpose   : Specific defines for ARB genome                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2002      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ADGENE_H
#define ADGENE_H

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#define GENOM_ALIGNMENT "ali_genom"
#define GENOM_DB_TYPE   "genom_db"  // main flag (true = genom db, false/missing=normal db)

#define ARB_HIDDEN "ARB_display_hidden"

/* GEN_position is interpreted as
 *
 * join( complement[0](start_pos[0]..stop_pos[0]),
 *       complement[1](start_pos[1]..stop_pos[1]),
 *       complement[2](start_pos[2], stop_pos[2]),
 *       ... )
 *
 * start_pos is always lower than stop_pos
 * joined genes on complementary strand are normally ordered backwards
 * (i.e. part with highest positions comes first)
 */

struct GEN_position {
    int            parts;
    bool           joinable;                        // true = join(...), false = order(...) aka not joinable or unknown
    size_t        *start_pos;
    size_t        *stop_pos;
    unsigned char *complement;                      // ASCII 0 = normal or ASCII 1 = complementary

    // [optional elements]

    // start_uncertain/stop_uncertain contain one char per part (or NULL which means "all positions are certain")
    // meaning of characters:
    // '<' = position MIGHT be lower
    // '=' = position is certain
    // '>' = position MIGHT be higher
    // '+' = position is directly BEHIND (but before next base position, i.e. specifies a location between to base positions)
    // '-' = position is BEFORE

    unsigned char *start_uncertain;
    unsigned char *stop_uncertain;
};

#else
#error adGene.h included twice
#endif // ADGENE_H
