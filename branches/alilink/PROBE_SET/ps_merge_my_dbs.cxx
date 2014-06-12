// =============================================================== //
//                                                                 //
//   File      : ps_merge_my_dbs.cxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_tools.hxx"
#include "ps_database.hxx"

#include <sys/times.h>

//  ====================================================

int main(int argc,   char *argv[]) {

    if (argc < 3) {
        printf("Missing arguments\n Usage %s <output database name> <input database name> <input database name2> [[input3]...]\n", argv[0]);
        exit(1);
    }

    //
    // init database object
    //
    const char *input_DB_name = argv[2];
    struct tms before;
    times(&before);
    printf("Opening 1st input-probe-set-database '%s'..\n", input_DB_name);
    PS_Database *db = new PS_Database(input_DB_name, PS_Database::READONLY);
    db->load();
    PS_print_time_diff(&before, "(enter to continue)  ");

    //
    // merge in other databasefiles
    //
    for (int i = 3; i < argc; ++i) {
        input_DB_name = argv[i];
        printf("Appending input-probe-set-database '%s'..\n", input_DB_name);
        times(&before);
        db->merge(input_DB_name);
        PS_print_time_diff(&before);
    }
    printf("Merged databases (enter to continue)\n");

    //
    // write one big whole tree to file
    //
    const char *output_DB_name = argv[1];
    times(&before);
    printf("Writing output-probe-set-database '%s'..\n", output_DB_name);
    db->saveTo(output_DB_name);
    PS_print_time_diff(&before, "(enter to continue)  ");

    printf("cleaning up...\n");
    if (db) delete db;

    return 0;
}
