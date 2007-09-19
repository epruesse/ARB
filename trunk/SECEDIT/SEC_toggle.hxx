// ================================================================= //
//                                                                   //
//   File      : SEC_toggle.hxx                                      //
//   Purpose   : Support for multiple structure                      //
//   Time-stamp: <Fri Sep/14/2007 16:45 MET Coder@ReallySoft.de>     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef SEC_TOGGLE_HXX
#define SEC_TOGGLE_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif

class SEC_graphic;

class SEC_structure_toggler {
    GBDATA      *gb_structures; // contains all structures
    GBDATA      *gb_current;    // contains current structure
    SEC_graphic *gfx;           // needed to trigger refresh
    GB_ERROR     error;
    int          Count;

    int  current();
    void set_current(int idx);
    
    GBDATA *find(int num);
    GBDATA *create(const char *name); // create new structure (storing current)

    GB_ERROR store(GBDATA *gb_struct);
    GB_ERROR restore(GBDATA *gb_struct);

    GB_ERROR setName(GBDATA *gb_struct, const char *new_name);
    
public:
    SEC_structure_toggler(GBDATA *gb_main, const char *ali_name, SEC_graphic *Gfx); // might set error

    GB_ERROR get_error() const { return error; }
    
    GB_ERROR next();
    GB_ERROR copyTo(const char *name);
    GB_ERROR remove();

    const char *name();
    GB_ERROR setName(const char *new_name);

    int getCount() const { return Count; }
};


#else
#error SEC_toggle.hxx included twice
#endif // SEC_TOGGLE_HXX
