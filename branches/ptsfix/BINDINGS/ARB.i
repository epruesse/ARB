%{
#include <arbdb.h>
#include <arbdbt.h>

GB_shell thegbshell;
%}


/* map GB_error to char pointer -- errors are message strings */
%typemap(out) GB_ERROR = const char*;

%inline %{

/* ARB:: */

/* Accessing database */

GBDATA*
open(const char *path, const char *opent) {
    return GB_open(path, opent);
}

GB_ERROR
save(GBDATA *gb, const char *path, const char *savetype) {
    return GB_save(gb, path, savetype);
}

GB_ERROR
save_as(GBDATA *gb, const char *path, const char *savetype) {
    return GB_save_as(gb, path, savetype);
}

void 
close(GBDATA *gbd) {
    GB_close(gbd);
}

const char*
get_db_path(GBDATA *gbd) {
    return GB_get_db_path(gbd);
}

/* transactions */

GB_ERROR 
abort_transaction(GBDATA *gbd) {
    return GB_abort_transaction(gbd);
}

GB_ERROR 
begin_transaction(GBDATA *gbd) {
    return GB_begin_transaction(gbd);
}

GB_ERROR
commit_transaction(GBDATA *gbd) {
    return GB_commit_transaction(gbd);
}

/* read/write operations */

char*
read_as_string(GBDATA *gbd) {
    return GB_read_as_string(gbd);
}

char*
read_string(GBDATA *gbd) {
    return GB_read_string(gbd);
}

GB_ERROR
write_string(GBDATA *gbd, const char *s) {
    return GB_write_string(gbd, s);
}

void
write_flag(GBDATA *gbd, long flag) {
    return GB_write_flag(gbd, flag);
}

int
read_flag(GBDATA *gbd) {
    return GB_read_flag(gbd);
}

GB_ERROR
check_key(const char *key) {
    return GB_check_key(key);
}

GBDATA*
search(GBDATA *gbd, const char *fieldpath, GB_TYPES create) {
    return GB_search(gbd, fieldpath, create);
}

/* others */

GB_ERROR
await_error(void) {
    return GB_await_error();
}

uint32_t
checksum(const char *seq, long length, int ignore_case, const char *exclude) {
    return GB_checksum(seq, length, ignore_case, exclude);
}


/* BIO */

/* read and write database (GBDATA) items */

char*
read_string(GBDATA *gb_container, const char *fieldpath) {
    return GBT_read_string(gb_container, fieldpath);
}

char*
read_as_string(GBDATA *gb_container, const char *fieldpath) {
    return GBT_read_as_string(gb_container, fieldpath);
}

GB_CSTR
read_name(GBDATA *gb_item) {
    return GBT_read_name(gb_item);
}

GB_ERROR
write_int(GBDATA *gb_container, const char *fieldpath, long content) {
    return GBT_write_int(gb_container, fieldpath, content);
}

GB_ERROR
write_string(GBDATA *gb_container, const char *fieldpath, const char *content) {
    return GBT_write_string(gb_container, fieldpath, content);
}

/* access AWARS (values from GUI) */

GB_ERROR
remote_action(GBDATA *gb_main, const char *application, const char *action_name) {
    return GBT_remote_action(gb_main, application, action_name);
}

GB_ERROR
remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    return GBT_remote_awar(gb_main, application, awar_name, value);
}

GB_ERROR
remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    return GBT_remote_read_awar(gb_main, application, awar_name);
}

/* iterate through species */

GBDATA*
first_species(GBDATA *gb_main) {
    return GBT_first_species(gb_main);
}

GBDATA*
next_species(GBDATA *gb_species) {
    return GBT_next_species(gb_species);
}

GBDATA*
first_marked_species(GBDATA *gb_main) {
    return GBT_first_marked_species(gb_main);
}

GBDATA*
next_marked_species(GBDATA *gb_species) {
    return GBT_next_marked_species(gb_species);
}

/* other */

/*
char *get_name_of_next_tree(GBDATA *gb_main, const char *tree_name) {
    return GBT_get_name_of_next_tree(gb_main, tree_name);
}
*/

char*
get_default_alignment(GBDATA *gb_main) {
    GBT_get_default_alignment(gb_main);
}

void
message(GBDATA *gb_main, const char *msg) {
    GBT_message(gb_main, msg);
}

%}
