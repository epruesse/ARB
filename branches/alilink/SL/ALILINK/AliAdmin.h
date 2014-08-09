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

class AliAdmin {
    int           db_nr;
    GBDATA *const gb_main;
public:
    AliAdmin(int db_nr_, GBDATA *gb_main_)
        : db_nr(db_nr_),
          gb_main(gb_main_)
    {}

};

#else
#error AliAdmin.h included twice
#endif // ALIADMIN_H
