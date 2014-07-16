// ============================================================== //
//                                                                //
//   File      : dbconn.cxx                                       //
//   Purpose   : Connector to running ARB                         //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2011   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "dbconn.h"
#include <arbdb.h>

class ARBDB_connector : virtual Noncopyable {
    GB_shell  shell;
    GBDATA   *gb_main;

public:
    ARBDB_connector() {
        gb_main = GB_open(":", "rwt");
        if (!gb_main) {
            GB_print_error();
            exit(EXIT_FAILURE);
        }
    }
    ~ARBDB_connector() {
        GB_close(gb_main);
    }

    GBDATA *main() const { return gb_main; }
};


GBDATA *runningDatabase() {
    static SmartPtr<ARBDB_connector> db;
    if (db.isNull()) db = new ARBDB_connector;
    return db->main();
}

