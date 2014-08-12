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
#ifndef ARB_MSG_H
#include <arb_msg.h>
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
    const char    *tmp_awarbase;

    const char *tmp_awarname(const char *entryName) const {
        return GBS_global_string("%s%s", tmp_awarbase, entryName);
    }

public:
    AliAdmin(int db_nr_, GBDATA *gb_main_, const char *select_awarname_, const char *tmp_awarbase_)
        : db_nr(db_nr_),
          gb_main(gb_main_),
          aws(NULL),
          select_awarname(select_awarname_),
          tmp_awarbase(tmp_awarbase_)
    {
        ali_assert(strrchr(tmp_awarbase, '/')[1] == 0); // has to end with '/'
    }

    GBDATA *get_gb_main() const { return gb_main; }
    __ATTR__DEPRECATED("dont use dbnr") int get_db_nr() const {
        ali_assert(db_nr>=1 && db_nr<=2); // call forbidden for ntree-admin (@@@ will be eliminated)
        return db_nr;
    }

    void store_window(AW_window *aw) { ali_assert(!aws); aws = aw; }
    AW_window *get_window() const { return aws; }

    const char *select_name()  const { return select_awarname; }
    const char *dest_name()    const { return tmp_awarname("alignment_dest"); }
    const char *type_name()    const { return tmp_awarname("alignment_type"); }
    const char *len_name()     const { return tmp_awarname("alignment_len"); }
    const char *aligned_name() const { return tmp_awarname("aligned"); }
    const char *security_name()const { return tmp_awarname("security"); }
    const char *remark_name()  const { return tmp_awarname("alignment_rem"); }
    const char *auto_name()    const { return tmp_awarname("auto_format"); }

    AW_awar *select_awar()  const { return AW_root::SINGLETON->awar(select_name()); }
    AW_awar *dest_awar()    const { return AW_root::SINGLETON->awar(dest_name()); }
    AW_awar *type_awar()    const { return AW_root::SINGLETON->awar(type_name()); }
    AW_awar *len_awar()     const { return AW_root::SINGLETON->awar(len_name()); }
    AW_awar *aligned_awar() const { return AW_root::SINGLETON->awar(aligned_name()); }
    AW_awar *security_awar()const { return AW_root::SINGLETON->awar(security_name()); }
    AW_awar *remark_awar()  const { return AW_root::SINGLETON->awar(remark_name()); }
    AW_awar *auto_awar()    const { return AW_root::SINGLETON->awar(auto_name()); }

    const char *get_selected_ali()const { return select_awar()->read_char_pntr(); }
    const char *get_dest_ali()    const { return dest_awar()->read_char_pntr(); }
};

#else
#error AliAdmin.h included twice
#endif // ALIADMIN_H
