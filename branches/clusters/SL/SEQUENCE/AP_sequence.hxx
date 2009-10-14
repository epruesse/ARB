// =============================================================== //
//                                                                 //
//   File      : AP_sequence.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_SEQUENCE_HXX
#define AP_SEQUENCE_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define ap_assert(cond) arb_assert(cond)

typedef enum {
    AP_FALSE,
    AP_TRUE
} AP_BOOL;

typedef double AP_FLOAT;

class ARB_Tree_root;

class AP_sequence {
protected:
    AP_FLOAT cashed_real_len;

public:
    ARB_Tree_root *root;
    static char *mutation_per_site; // if != 0 then mutations are set by combine
    static char *static_mutation_per_site[3];   // if != 0 then mutations are set by combine

    AP_BOOL  is_set_flag;
    long     sequence_len;
    long     update;
    AP_FLOAT costs;

    static long global_combineCount;

    AP_sequence(ARB_Tree_root *rooti);
    virtual ~AP_sequence(void);

    virtual AP_sequence *dup(void) = 0;             // used to get the real new element
    virtual void set_gb(GBDATA *gb_sequence ); // by default calls set((char *))
    virtual void set(const char *sequence) = 0;
    /* seq = acgtututu   */
    virtual AP_FLOAT combine(const AP_sequence* lefts, const AP_sequence *rights) = 0;
    virtual void partial_match(const AP_sequence* part, long *overlap, long *penalty) const = 0;
    virtual AP_FLOAT real_len(void);
};


#else
#error AP_sequence.hxx included twice
#endif // AP_SEQUENCE_HXX
