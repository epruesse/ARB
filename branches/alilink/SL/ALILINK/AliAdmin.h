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

#ifndef AW_AWAR_HXX
#include <aw_awar.hxx>
#endif
#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

#define ali_assert(cond) arb_assert(cond)

enum CopyRenameMode { // @@@ move inside AliAdmin.cxx asap
    CRM_RENAME,
    CRM_COPY,
};

class AliAdmin {
    int            db_nr; // @@@ elim
    GBDATA *const  gb_main;
    AW_window     *aws;
    const char    *select_awarname;

public:
    AliAdmin(int db_nr_, GBDATA *gb_main_, const char *select_awarname_)
        : db_nr(db_nr_),
          gb_main(gb_main_),
          aws(NULL),
          select_awarname(select_awarname_)
    {}

    GBDATA *get_gb_main() const { return gb_main; }
    __ATTR__DEPRECATED("dont use dbnr") int get_db_nr() const {
        ali_assert(db_nr>=1 && db_nr<=2); // call forbidden for ntree-admin (@@@ will be eliminated)
        return db_nr;
    }

    void store_window(AW_window *aw) { ali_assert(!aws); aws = aw; }
    AW_window *get_window() const { return aws; }

    const char *get_select_awarname() const { return select_awarname; }
    AW_awar *get_select_awar() const { return AW_root::SINGLETON->awar(get_select_awarname()); }
    const char *get_selected_ali() const { return get_select_awar()->read_char_pntr(); }
};

#else
#error AliAdmin.h included twice
#endif // ALIADMIN_H
