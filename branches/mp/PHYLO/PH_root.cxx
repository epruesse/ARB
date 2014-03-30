#include "phylo.hxx"
#include <arbdb.h>

GB_ERROR PH_root::open(const char *db_server) {
    GB_ERROR error = 0;

    gb_main             = GB_open(db_server, "rwt");
    if (!gb_main) error = GB_await_error();
    else GLOBAL_gb_main = gb_main;

    return error;
}