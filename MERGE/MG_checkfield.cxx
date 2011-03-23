// =============================================================== //
//                                                                 //
//   File      : MG_checkfield.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"

#include <awt.hxx>
#include <awt_item_sel_list.hxx>
#include <awt_sel_boxes.hxx>

#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>

#include <arbdbt.h>
#include <arb_strbuf.h>

#include <cctype>

#define AWAR_SOURCE_FIELD "/tmp/merge1/chk/source"
#define AWAR_DEST_FIELD   "/tmp/merge1/chk/dest"
#define AWAR_TOUPPER      "/tmp/merge1/chk/ToUpper"
#define AWAR_EXCLUDE      "/tmp/merge1/chk/exclude"
#define AWAR_CORRECT      "/tmp/merge1/chk/correct"
#define AWAR_ETAG         "/tmp/merge1/chk/tag"


int gbs_cmp_strings(char *str1, char *str2, int *tab) { /* returns 0 if strings are equal */
    char *s1, *s2;
    int c1, c2;
    s1 = str1;
    s2 = str2;
    int count = 10;
    do {
        do { c1 = *(s1++); } while (tab[c1] < 0);
        do { c2 = *(s2++); } while (tab[c2] < 0);
        if (tab[c1] != tab[c2]) {   /* difference found */
            return 1;
        }
        count --;
    } while (count && c1 && c2);
    return 0;
}


char *GBS_diff_strings(char *str1, char * &str2, char *exclude, long ToUpper, long correct,
                       char **res1, char **res2, long *corrrected) {

    char  buffer1[256];
    char  buffer2[256];
    char *dest1 = buffer1;
    char *dest2 = buffer2;
    char *s1, *s2;
    int   c1, c2;
    int   count = 3;
    int   tab[256];
    int   i;

    s1 = str1;
    s2 = str2;
    *dest1 = 0;
    *dest2 = 0;
    tab[0] = 0;
    char gapchar = '#';
    if (strlen(exclude)) gapchar = exclude[0];
    else    exclude = 0;

    for (i=1; i<256; i++) {
        tab[i] = i;
        if (exclude && strchr(exclude, i)) {
            tab[i] = -1;
            continue;
        }
        if (ToUpper && i >= 'a' && i <= 'z') {
            tab[i] = i-'a'+'A';
        }
    }

    do {
        do { c1 = *(s1++); } while (tab[c1] < 0);
        do { c2 = *(s2++); } while (tab[c2] < 0);
        if (tab[c1] != tab[c2]) {   /* difference found */
            if (correct) {
                /* check substitution */
                {
                    int c = s2[-1];
                    s2[-1] = s1[-1];
                    if (toupper(c1) == toupper(c2) ||
                        !gbs_cmp_strings(s1, s2, &tab[0])) {
                        *corrrected = 1;
                        continue;
                    }
                    s2[-1] = c;
                }

                /* check insertion in s2 */
                if (!gbs_cmp_strings(s1-1, s2, &tab[0])) {
                    s2[-1] = gapchar;
                    do { c2 = *(s2++); } while (tab[c2] < 0); /* eat s2 */
                    *corrrected = 1;
                    continue;
                }
                /* check deletion in s2 */
                if (!gbs_cmp_strings(s1, s2-1, &tab[0])) {
                    int toins = c1;
                    char *toinspos = s2-1;
                    if (toinspos > str2) toinspos--;
                    if (tab[(unsigned char)toinspos[0]]> 0) { /* real insertion */
                        GBS_strstruct *str = GBS_stropen(strlen(str2+10));
                        int pos = s2-str2-1;
                        GBS_strncat(str, str2, pos);
                        GBS_chrcat(str, toins);
                        GBS_strcat(str, str2+pos);
                        delete str2;
                        str2 = GBS_strclose(str);
                        s2 = str2+pos+1;
                        *corrrected = 1;
                        continue;
                    }
                    int side=1; /* 0 = left   1= right */
                    if (tab[(unsigned char)s1[0]]<0) side = 0;
                    if (! side) {
                        while (toinspos > str2 &&
                                tab[(unsigned char)toinspos[-1]] < 0) toinspos--;
                    }
                    toinspos[0] = toins;
                    *corrrected = 1;
                    do { c1 = *(s1++); } while (tab[c1] < 0); /* eat s1 */
                    continue;
                }
            }
            if (count >= 0) {
                sprintf(dest1, "%ti ", s1-str1-1);
                sprintf(dest2, "%ti ", s2-str2-1);
                dest1 += strlen(dest1);
                dest2 += strlen(dest2);
            }
            count --;
        }
    } while (c1 && c2);

    if (c1 || c2) {
        sprintf(dest1, "... %ti ", s1-str1-1);
        sprintf(dest2, "... %ti ", s2-str2-1);
        dest1 += strlen(dest1);
        dest2 += strlen(dest2);
    }
    if (count<0) {
        sprintf(dest1, "and %i more", 1-count);
        sprintf(dest2, "and %i more", 1-count);
        dest1 += strlen(dest1);
        dest2 += strlen(dest2);
    }
    if (strlen(buffer1)) {
        *res1 = strdup(buffer1);
        *res2 = strdup(buffer2);
    }
    else {
        *res1 = 0;
        *res2 = 0;
    }
    return 0;
}

int mg_count_queried(GBDATA *gb_main) {
    int queried  = 0;
    for (GBDATA *gb_spec = GBT_first_species(gb_main);
         gb_spec;
         gb_spec = GBT_next_species(gb_spec))
    {
        if (IS_QUERIED_SPECIES(gb_spec)) queried++;
    }
    return queried;
}

void mg_check_field_cb(AW_window *aww) {
    AW_root  *root    = aww->get_root();
    GB_ERROR  error   = 0;
    char     *source  = root->awar(AWAR_SOURCE_FIELD)->read_string();
    char     *dest    = root->awar(AWAR_DEST_FIELD)->read_string();
    char     *exclude = root->awar(AWAR_EXCLUDE)->read_string();
    long      ToUpper = root->awar(AWAR_TOUPPER)->read_int();
    long      correct = root->awar(AWAR_CORRECT)->read_int();
    char     *tag     = root->awar(AWAR_ETAG)->read_string();

    if (source[0] == 0) {
        error = "Please select a source field";
    }
    else if (dest[0] == 0) {
        error = "Please select a dest field";
    }
    else {
        error = GB_begin_transaction(GLOBAL_gb_merge);

        if (!error) {
            error = GB_begin_transaction(GLOBAL_gb_dest);

            GBDATA *gb_species_data1 = GB_search(GLOBAL_gb_merge, "species_data", GB_CREATE_CONTAINER);
            GBDATA *gb_species_data2 = GB_search(GLOBAL_gb_dest,  "species_data", GB_CREATE_CONTAINER);

            GBDATA *gb_species1;
            GBDATA *gb_species2;

            // First step: count selected species
            arb_progress progress("Checking fields", mg_count_queried(GLOBAL_gb_merge));

            // Delete all 'dest' fields in gb_database 2
            for (gb_species2 = GBT_first_species_rel_species_data(gb_species_data2);
                 gb_species2 && !error;
                 gb_species2 = GBT_next_species(gb_species2))
            {
                GBDATA *gbd    = GB_search(gb_species2, dest, GB_FIND);
                if (gbd) error = GB_delete(gbd);
            }

            for (gb_species1 = GBT_first_species_rel_species_data(gb_species_data1);
                 gb_species1 && !error;
                 gb_species1 = GBT_next_species(gb_species1))
            {
                {
                    GBDATA *gbd    = GB_search(gb_species1, dest, GB_FIND);
                    if (gbd) error = GB_delete(gbd);
                }

                if (!error) {
                    if (IS_QUERIED_SPECIES(gb_species1)) {
                        const char *name1 = GBT_read_name(gb_species1);
                        gb_species2       = GB_find_string(gb_species_data2, "name", name1, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
                        if (!gb_species2) {
                            aw_message(GBS_global_string("WARNING: Species %s not found in DB II", name1));
                        }
                        else {
                            gb_species2 = GB_get_father(gb_species2);

                            GBDATA *gb_field1 = GB_search(gb_species1, source, GB_FIND);
                            GBDATA *gb_field2 = GB_search(gb_species2, source, GB_FIND);
                            char   *s1        = gb_field1 ? GB_read_as_tagged_string(gb_field1, tag) : 0;
                            char   *s2        = gb_field2 ? GB_read_as_tagged_string(gb_field2, tag) : 0;

                            if (s1 || s2) {
                                char *positions1 = 0;
                                char *positions2 = 0;

                                if (s1 && s2) {
                                    long corrected = 0;
                                    GBS_diff_strings(s1, s2, exclude, ToUpper, correct, &positions1, &positions2, &corrected);
                                    if (corrected) {
                                        error = GB_write_as_string(gb_field2, s2);
                                        if (!error) GB_write_flag(gb_species2, 1);
                                    }
                                }
                                else {
                                    positions1 = GBS_global_string_copy("field missing in %s DB", s1 ? "other" : "this");
                                    positions2 = GBS_global_string_copy("field missing in %s DB", s2 ? "other" : "this");
                                }

                                if (positions1 && !error) {
                                    error             = GBT_write_string(gb_species2, dest, positions2);
                                    if (!error) error = GBT_write_string(gb_species1, dest, positions1);
                                }

                                free(positions2);
                                free(positions1);
                            }

                            free(s2);
                            free(s1);
                        }
                        progress.inc_and_check_user_abort(error);
                    }
                }
            }

            error = GB_end_transaction(GLOBAL_gb_merge, error);
            error = GB_end_transaction(GLOBAL_gb_dest, error);
        }
    }
    if (error) aw_message(error);

    free(tag);
    free(exclude);
    free(dest);
    free(source);
}


AW_window *create_mg_check_fields(AW_root *aw_root) {
    AW_window_simple *aws = 0;

    aw_root->awar_string(AWAR_SOURCE_FIELD);
    aw_root->awar_string(AWAR_DEST_FIELD, "tmp", AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_EXCLUDE, ".-", AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_ETAG, "");
    aw_root->awar_int(AWAR_TOUPPER);
    aw_root->awar_int(AWAR_CORRECT);

    aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_COMPARE_FIELDS", "COMPARE DATABASE FIELDS");
    aws->load_xfig("merge/seqcheck.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"checkfield.hlp");
    aws->create_button("HELP", "HELP", "H");


    aws->at("exclude");
    aws->create_input_field(AWAR_EXCLUDE);

    aws->at("toupper");
    aws->create_toggle(AWAR_TOUPPER);

    aws->at("correct");
    aws->create_toggle(AWAR_CORRECT);

    aws->at("tag");
    aws->create_input_field(AWAR_ETAG, 6);

    awt_create_selection_list_on_itemfields(GLOBAL_gb_dest, aws, AWAR_SOURCE_FIELD,
                                            AWT_STRING_FILTER, "source", 0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_itemfields(GLOBAL_gb_dest, aws, AWAR_DEST_FIELD,
                                            (1<<GB_STRING)|(1<<GB_INT), "dest", 0, &AWT_species_selector, 20, 10);

#if defined(DEVEL_RALF)
#warning check code above. Maybe one call has to get GLOBAL_gb_merge ?
#endif // DEVEL_RALF


    aws->at("go");
    aws->highlight();
    aws->callback(mg_check_field_cb);
    aws->create_button("GO", "GO");

    return (AW_window *)aws;
}
