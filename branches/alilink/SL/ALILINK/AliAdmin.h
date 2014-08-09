// ============================================================== //
//                                                                //
//   File      : AliAdmin.h                                       //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef ALIADMIN_H
#define ALIADMIN_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ali_assert(cond) arb_assert(cond)

class AliAdmin {
    int           db_nr;
    GBDATA *const gb_main;

public:
    AliAdmin(int db_nr_, GBDATA *gb_main_)
        : db_nr(db_nr_),
          gb_main(gb_main_)
    {}

    GBDATA *get_gb_main() const { return gb_main; }
    __ATTR__DEPRECATED_LATER("dont use dbnr") int get_db_nr() const {
        ali_assert(db_nr>=1 && db_nr<=2); // call forbidden for ntree-admin (@@@ will be eliminated)
        return db_nr;
    }
};

#else
#error AliAdmin.h included twice
#endif // ALIADMIN_H
