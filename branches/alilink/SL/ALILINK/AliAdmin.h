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
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ali_assert(cond) arb_assert(cond)

enum CopyRenameMode { // @@@ move inside AliAdmin.cxx asap
    CRM_RENAME,
    CRM_COPY,
};

class AliAdmin {
    int            db_nr;
    GBDATA *const  gb_main;
    AW_window     *aws;

public:
    AliAdmin(int db_nr_, GBDATA *gb_main_)
        : db_nr(db_nr_),
          gb_main(gb_main_),
          aws(NULL)
    {}

    GBDATA *get_gb_main() const { return gb_main; }
    __ATTR__DEPRECATED_LATER("dont use dbnr") int get_db_nr() const {
        ali_assert(db_nr>=1 && db_nr<=2); // call forbidden for ntree-admin (@@@ will be eliminated)
        return db_nr;
    }

    void store_window(AW_window *aw) { ali_assert(!aws); aws = aw; }
    AW_window *get_window() const { return aws; }
};

#else
#error AliAdmin.h included twice
#endif // ALIADMIN_H
