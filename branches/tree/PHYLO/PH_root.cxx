// ================================================================= //
//                                                                   //
//   File      : PH_root.cxx                                         //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "phylo.hxx"
#include <arbdbt.h>

PH_root *PH_root::SINGLETON = NULL;

GB_ERROR PH_root::open(const char *db_server) {
    GB_ERROR error = 0;

    gb_main             = GB_open(db_server, "rwt");
    if (!gb_main) error = GB_await_error();

    return error;
}
