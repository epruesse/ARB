#include <stdio.h>
#include <arbdb.h>
#include <arbdbt.h>

GBDATA *gb_main;

void test_cb(GBDATA *gbd, int *clientdata, GB_CB_TYPE gbtype){
    char *s = GB_read_string(gbd);
    GB_write_string(gbd,"undo");
    printf("test_cb called: renaming %s to undo !!!\n",s);
}


void modify_database(void){
    GBDATA *gb_species_data, *gb_species, *gb_name;
    printf("    modify rename * to hello \n");
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    gb_name = GB_find(gb_species_data, "name", "*", down_2_level);
    gb_species = GB_get_father(gb_name);
    GBDATA *gbd;
    GB_write_string(gb_name,"hello");
    printf("    adding callback\n");
    GB_add_callback(gb_name, GB_CB_CHANGED, test_cb, 0);
}

void search_database(void){
    GBDATA *gb_species_data, *gb_name;
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    gb_name = GB_find(gb_species_data, "name", "hello", down_2_level);
    printf("    searched species hello: found %X\n",gb_name);

    gb_name = GB_find(gb_species_data, "name", "undo", down_2_level);
    printf("    searched species undo: found %X\n",gb_name);
}


int
main(int argc, char **argv)
{
    char *server = ":";

    if (argc==2) server = argv[1];
    gb_main = GBT_open(server,"r",0);
    printf("begin\n");
    GB_change_my_security(gb_main,6,0);
    GB_begin_transaction(gb_main);
    search_database();
    modify_database();
    search_database();
    printf("abort\n");
    GB_abort_transaction(gb_main);

    printf("begin\n");
    GB_begin_transaction(gb_main);
    search_database();
    modify_database();
    search_database();
    printf("commit\n");
    GB_commit_transaction(gb_main);

    printf("begin\n");
    GB_begin_transaction(gb_main);
    search_database();
    printf("commit\n");
    GB_commit_transaction(gb_main);
    GB_close(gb_main);
    return 0;
}
