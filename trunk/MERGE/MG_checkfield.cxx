#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include "merge.hxx"



#define AWAR_SOURCE_FIELD "/tmp/merge1/chk/source"
#define AWAR_DEST_FIELD "/tmp/merge1/chk/dest"
#define AWAR_TOUPPER "/tmp/merge1/chk/ToUpper"
#define AWAR_EXCLUDE "/tmp/merge1/chk/exclude"
#define AWAR_CORRECT "/tmp/merge1/chk/correct"
#define AWAR_ETAG "/tmp/merge1/chk/tag"


int gbs_cmp_strings(char *str1,char *str2, int *tab){   /* returns 0 if strings are equal */
    register char *s1,*s2;
    register int c1,c2;
    s1 = str1;
    s2 = str2;
    int count = 10;
    do {
        do { c1= *(s1++); } while (tab[c1] < 0);
        do { c2= *(s2++); } while (tab[c2] < 0);
        if (tab[c1] != tab[c2]) {   /* difference found */
            return 1;
        }
        count --;
    } while (count && c1 && c2);
    return 0;
}


char *GBS_diff_strings(char *str1,char * &str2, char *exclude , long ToUpper, long correct,
                       char **res1, char **res2, long *corrrected){

    char buffer1[256];
    char buffer2[256];
    char *dest1 = buffer1;
    char *dest2 = buffer2;
    register char *s1,*s2;
    register int c1,c2;
    int count = 3;
    int tab[256];
    int i;

    s1 = str1;
    s2 = str2;
    *dest1 = 0;
    *dest2 = 0;
    tab[0] = 0;
    char gapchar = '#';
    if (strlen(exclude)) gapchar = exclude[0];
    else    exclude = 0;

    for (i=1;i<256;i++) {
        tab[i] = i;
        if (exclude && strchr(exclude,i)) {
            tab[i] = -1;
            continue;
        }
        if (ToUpper && i>= 'a' && i<= 'z' ){
            tab[i] = i-'a'+'A';
        }
    }

    do {
        do { c1= *(s1++); } while (tab[c1] < 0);
        do { c2= *(s2++); } while (tab[c2] < 0);
        if (tab[c1] != tab[c2]) {   /* difference found */
            if (correct) {
                /* check subsitution */
                {
                    int c = s2[-1];
                    s2[-1] = s1[-1];
                    if (toupper(c1) == toupper(c2) ||
                        !gbs_cmp_strings(s1,s2,&tab[0]) ){
                        *corrrected = 1;
                        continue;
                    }
                    s2[-1] = c;
                }

                /* check insertion in s2 */
                if (!gbs_cmp_strings(s1-1,s2,&tab[0]) ){
                    s2[-1] = gapchar;
                    do { c2= *(s2++); } while (tab[c2] < 0); /* eat s2 */
                    *corrrected = 1;
                    continue;
                }
                /* check deletion in s2 */
                if ( !gbs_cmp_strings(s1,s2-1,&tab[0]) ){
                    int toins = c1;
                    char *toinspos = s2-1;
                    if (toinspos > str2) toinspos--;
                    if (tab[toinspos[0]]> 0) { /* real insertion */
                        void *str = GBS_stropen(strlen(str2+10));
                        int pos = s2-str2-1;
                        GBS_strncat(str,str2,pos);
                        GBS_chrcat(str,toins);
                        GBS_strcat(str,str2+pos);
                        delete str2;
                        str2 = GBS_strclose(str);
                        s2 = str2+pos+1;
                        *corrrected = 1;
                        continue;
                    }
                    int side=1; /* 0 = left   1= right */
                    if ( tab[s1[0]]<0 ) side = 0;
                    if ( ! side ) {
                        while ( toinspos > str2 &&
                                tab[toinspos[-1]] < 0 ) toinspos--;
                    }
                    toinspos[0] = toins;
                    *corrrected = 1;
                    do { c1= *(s1++); } while (tab[c1] < 0); /* eat s1 */
                    continue;
                }
            }
            if (count >=0){
                sprintf(dest1,"%i ",s1-str1-1);
                sprintf(dest2,"%i ",s2-str2-1);
                dest1 += strlen(dest1);
                dest2 += strlen(dest2);
            }
            count --;
        }
    } while (c1 && c2);

    if (c1 || c2) {
        sprintf(dest1,"... %i ",s1-str1-1);
        sprintf(dest2,"... %i ",s2-str2-1);
        dest1 += strlen(dest1);
        dest2 += strlen(dest2);
    }
    if (count<0){
        sprintf(dest1,"and %i more",1-count);
        sprintf(dest2,"and %i more",1-count);
        dest1 += strlen(dest1);
        dest2 += strlen(dest2);
    }
    if (strlen(buffer1) ) {
        *res1 = strdup(buffer1);
        *res2 = strdup(buffer2);
    }else{
        *res1 = 0;
        *res2 = 0;
    }
    return 0;
}

#undef IS_QUERIED
#define IS_QUERIED(gb_species) (1 & GB_read_usr_private(gb_species))

void mg_check_field_cb(AW_window *aww){
    AW_root *root = aww->get_root();
    GB_ERROR error = 0;
    char *source = root->awar(AWAR_SOURCE_FIELD)->read_string();
    char *dest = root->awar(AWAR_DEST_FIELD)->read_string();
    char *exclude = root->awar(AWAR_EXCLUDE)->read_string();
    long ToUpper = root->awar(AWAR_TOUPPER)->read_int();
    long correct = root->awar(AWAR_CORRECT)->read_int();
    char *tag = root->awar(AWAR_ETAG)->read_string();

    if (!strlen(source)) {
        delete source;
        delete dest;
        delete exclude;
        delete tag;
        aw_message("Please select a source field");
        return;
    }
    if (!strlen(dest)) {
        delete source;
        delete tag;
        delete dest;
        delete exclude;
        aw_message("Please select a dest field");
        return;
    }
    aw_openstatus("Checking fields");
    GB_begin_transaction(gb_merge);
    GB_begin_transaction(gb_dest);

    GBDATA *gb_species_data1 = GB_search(gb_merge,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species_data2 = GB_search(gb_dest,"species_data",GB_CREATE_CONTAINER);

    GBDATA *gb_species1;
    GBDATA *gb_species2;
    GBDATA *gb_name1;
    GBDATA *gb_field1;
    GBDATA *gb_field2;
    GBDATA *gbd;
    int     sum_species = 0;
    int species_count = 0;
    // First step: count selected species
    for (gb_species1 = GBT_first_species_rel_species_data(gb_species_data1);
         gb_species1;
         gb_species1 = GBT_next_species(gb_species1)){
        gbd = GB_search(gb_species1,dest,GB_FIND);
        if (gbd) GB_delete(gbd);
        if (!IS_QUERIED(gb_species1)) continue;
        sum_species ++;
    }
    // Delete all 'dest' fields in gb_database 2
    for (       gb_species2 = GBT_first_species_rel_species_data(gb_species_data2);
                gb_species2;
                gb_species2 = GBT_next_species(gb_species2)){
        gbd = GB_search(gb_species2,dest,GB_FIND);
        if (gbd) GB_delete(gbd);
    }

    for (       gb_species1 = GBT_first_species_rel_species_data(gb_species_data1);
                gb_species1;
                gb_species1 = GBT_next_species(gb_species1)){
        gbd = GB_search(gb_species1,dest,GB_FIND);
        if (gbd) GB_delete(gbd);

        gb_name1 = GB_find(gb_species1,"name",0,down_level);
        if (!gb_name1) continue;    // no name what happend ???

        if (!IS_QUERIED(gb_species1)) continue;
        species_count++;
        if ( (species_count & 0xf) == 0 ){
            aw_status(species_count/ (double)sum_species);
        }
        gb_species2 = GB_find(gb_species_data2,"name",
                              GB_read_char_pntr(gb_name1),down_2_level);

        if (!gb_species2) {
            sprintf(AW_ERROR_BUFFER,"WARNING: Species %s not found in DB II",
                    GB_read_char_pntr(gb_name1));
            aw_message();
            continue;
        }else{
            gb_species2 = GB_get_father(gb_species2);
        }
        char *s1 = 0;
        char *s2 = 0;
        gb_field1 = GB_search(gb_species1,source,GB_FIND);
        gb_field2 = GB_search(gb_species2,source,GB_FIND);

        if ( (!gb_field1) &&  (!gb_field2) ) continue;
        if (gb_field1 ) s1 = GB_read_as_tagged_string(gb_field1,tag);
        if (gb_field2 ) s2 = GB_read_as_tagged_string(gb_field2,tag);
        if (!s1 && !s2) continue;

        {
            char *positions1;
            char *positions2;
            if (s1 && s2){
                long corrected= 0;
                GBS_diff_strings(s1,s2,exclude,ToUpper,correct,&positions1,&positions2,&corrected);
                if (corrected) {
                    error = GB_write_as_string(gb_field2,s2);
                    GB_write_flag(gb_species2,1);
                }
            }else{
                if (s1) positions1 = strdup("missing field in other db");
                else    positions1 = strdup("missing field in this db");
                if (s2) positions2 = strdup("missing field in other db");
                else    positions2 = strdup("missing field in this db");
            }
            if (positions1){
                gbd = GB_search(gb_species2,dest,GB_STRING);
                GB_write_string(gbd,positions2);
                delete positions2;
                gbd = GB_search(gb_species1,dest,GB_STRING);
                GB_write_string(gbd,positions1);
                delete positions1;
            }
        }

        delete s1;
        delete s2;
    }
    aw_closestatus();
    if (error){
        GB_abort_transaction(gb_merge);
        GB_abort_transaction(gb_dest);
        aw_message(error);
        error = 0;
    }else{
        GB_commit_transaction(gb_merge);
        GB_commit_transaction(gb_dest);
    }
    delete tag;
    delete source;
    delete dest;
    delete exclude;
}


AW_window *create_mg_check_fields(AW_root *aw_root){
    AW_window_simple *aws = 0;

    aw_root->awar_string(AWAR_SOURCE_FIELD);
    aw_root->awar_string(AWAR_DEST_FIELD,"tmp",AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_EXCLUDE,".-",AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_ETAG,"");
    aw_root->awar_int(AWAR_TOUPPER);
    aw_root->awar_int(AWAR_CORRECT);

    aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_COMPARE_FIELDS","COMPARE DATABASE FIELDS");
    aws->load_xfig("merge/seqcheck.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"checkfield.hlp");
    aws->create_button("HELP","HELP","H");


    aws->at("exclude");
    aws->create_input_field(AWAR_EXCLUDE);

    aws->at("toupper");
    aws->create_toggle(AWAR_TOUPPER);

    aws->at("correct");
    aws->create_toggle(AWAR_CORRECT);

    aws->at("tag");
    aws->create_input_field(AWAR_ETAG,6);

    awt_create_selection_list_on_scandb(gb_dest,aws,AWAR_SOURCE_FIELD,
                                        AWT_STRING_FILTER, "source",0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_scandb(gb_dest,aws,AWAR_DEST_FIELD,
                                        (1<<GB_STRING)|(1<<GB_INT), "dest",0, &AWT_species_selector, 20, 10);


    aws->at("go");
    aws->highlight();
    aws->callback(mg_check_field_cb);
    aws->create_button("GO","GO");

    return (AW_window *)aws;
}
