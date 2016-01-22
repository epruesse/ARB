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

#include <item_sel_list.h>

#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>

#include <arbdbt.h>
#include <arb_strbuf.h>

#include <cctype>

#define AWAR_CHECK AWAR_MERGE_TMP "chk/"

#define AWAR_SOURCE_FIELD AWAR_CHECK "source"
#define AWAR_DEST_FIELD   AWAR_CHECK "dest"
#define AWAR_TOUPPER      AWAR_CHECK "ToUpper"
#define AWAR_EXCLUDE      AWAR_CHECK "exclude"
#define AWAR_CORRECT      AWAR_CHECK "correct"
#define AWAR_ETAG         AWAR_CHECK "tag"


static int gbs_cmp_strings(char *str1, char *str2, int *tab) { // returns 0 if strings are equal
    char *s1, *s2;
    int c1, c2;
    s1 = str1;
    s2 = str2;
    int count = 10;
    do {
        do { c1 = *(s1++); } while (tab[c1] < 0);
        do { c2 = *(s2++); } while (tab[c2] < 0);
        if (tab[c1] != tab[c2]) {   // difference found
            return 1;
        }
        count --;
    } while (count && c1 && c2);
    return 0;
}


static char *GBS_diff_strings(char *str1, char * &str2, char *exclude, long ToUpper, long correct,
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
        if (tab[c1] != tab[c2]) {   // difference found
            if (correct) {
                // check substitution
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

                // check insertion in s2
                if (!gbs_cmp_strings(s1-1, s2, &tab[0])) {
                    s2[-1] = gapchar;
                    do { c2 = *(s2++); } while (tab[c2] < 0); // eat s2
                    *corrrected = 1;
                    continue;
                }
                // check deletion in s2
                if (!gbs_cmp_strings(s1, s2-1, &tab[0])) {
                    int toins = c1;
                    char *toinspos = s2-1;
                    if (toinspos > str2) toinspos--;
                    if (tab[(unsigned char)toinspos[0]]> 0) { // real insertion
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
                    int side=1; // 0 = left   1= right
                    if (tab[(unsigned char)s1[0]]<0) side = 0;
                    if (! side) {
                        while (toinspos > str2 &&
                                tab[(unsigned char)toinspos[-1]] < 0) toinspos--;
                    }
                    toinspos[0] = toins;
                    *corrrected = 1;
                    do { c1 = *(s1++); } while (tab[c1] < 0); // eat s1
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

static void mg_check_field_cb(AW_window *aww) {
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
        error = GB_begin_transaction(GLOBAL_gb_src);

        if (!error) {
            error = GB_begin_transaction(GLOBAL_gb_dst);

            GBDATA *gb_src_species_data = GBT_get_species_data(GLOBAL_gb_src);
            GBDATA *gb_dst_species_data = GBT_get_species_data(GLOBAL_gb_dst);

            GBDATA *gb_src_species;
            GBDATA *gb_dst_species;

            // First step: count selected species
            arb_progress progress("Checking fields", mg_count_queried(GLOBAL_gb_src));

            // Delete all 'dest' fields in target database
            for (gb_dst_species = GBT_first_species_rel_species_data(gb_dst_species_data);
                 gb_dst_species && !error;
                 gb_dst_species = GBT_next_species(gb_dst_species))
            {
                GBDATA *gbd    = GB_search(gb_dst_species, dest, GB_FIND);
                if (gbd) error = GB_delete(gbd);
            }

            for (gb_src_species = GBT_first_species_rel_species_data(gb_src_species_data);
                 gb_src_species && !error;
                 gb_src_species = GBT_next_species(gb_src_species))
            {
                {
                    GBDATA *gbd    = GB_search(gb_src_species, dest, GB_FIND);
                    if (gbd) error = GB_delete(gbd);
                }

                if (!error) {
                    if (IS_QUERIED_SPECIES(gb_src_species)) {
                        const char *src_name = GBT_read_name(gb_src_species);
                        gb_dst_species    = GB_find_string(gb_dst_species_data, "name", src_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
                        if (!gb_dst_species) {
                            aw_message(GBS_global_string("WARNING: Species %s not found in target DB", src_name));
                        }
                        else {
                            gb_dst_species = GB_get_father(gb_dst_species);

                            GBDATA *gb_src_field = GB_search(gb_src_species, source, GB_FIND);
                            GBDATA *gb_dst_field = GB_search(gb_dst_species, source, GB_FIND);

                            char *src_val = gb_src_field ? GB_read_as_tagged_string(gb_src_field, tag) : 0;
                            char *dst_val = gb_dst_field ? GB_read_as_tagged_string(gb_dst_field, tag) : 0;

                            if (src_val || dst_val) {
                                char *src_positions = 0;
                                char *dst_positions = 0;

                                if (src_val && dst_val) {
                                    long corrected = 0;
                                    GBS_diff_strings(src_val, dst_val, exclude, ToUpper, correct, &src_positions, &dst_positions, &corrected);
                                    if (corrected) {
                                        error = GB_write_as_string(gb_dst_field, dst_val);
                                        if (!error) GB_write_flag(gb_dst_species, 1);
                                    }
                                }
                                else {
                                    src_positions = GBS_global_string_copy("field missing in %s DB", src_val ? "other" : "this");
                                    dst_positions = GBS_global_string_copy("field missing in %s DB", dst_val ? "other" : "this");
                                }

                                if (src_positions && !error) {
                                    error             = GBT_write_string(gb_dst_species, dest, dst_positions);
                                    if (!error) error = GBT_write_string(gb_src_species, dest, src_positions);
                                }

                                free(dst_positions);
                                free(src_positions);
                            }

                            free(dst_val);
                            free(src_val);
                        }
                        progress.inc_and_check_user_abort(error);
                    }
                }
            }

            error = GB_end_transaction(GLOBAL_gb_src, error);
            error = GB_end_transaction(GLOBAL_gb_dst, error);
        }
    }
    if (error) aw_message(error);

    free(tag);
    free(exclude);
    free(dest);
    free(source);
}


AW_window *create_mg_check_fields_window(AW_root *aw_root) {
    aw_root->awar_string(AWAR_SOURCE_FIELD);
    aw_root->awar_string(AWAR_DEST_FIELD, "tmp", AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_EXCLUDE, ".-", AW_ROOT_DEFAULT);
    aw_root->awar_string(AWAR_ETAG, "");
    aw_root->awar_int(AWAR_TOUPPER);
    aw_root->awar_int(AWAR_CORRECT);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_COMPARE_FIELDS", "COMPARE DATABASE FIELDS");
    aws->load_xfig("merge/seqcheck.fig");

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("checkfield.hlp"));
    aws->create_button("HELP", "HELP", "H");


    aws->at("exclude");
    aws->create_input_field(AWAR_EXCLUDE);

    aws->at("toupper");
    aws->create_toggle(AWAR_TOUPPER);

    aws->at("correct");
    aws->create_toggle(AWAR_CORRECT);

    aws->at("tag");
    aws->create_input_field(AWAR_ETAG, 6);

    create_selection_list_on_itemfields(GLOBAL_gb_dst, aws, AWAR_SOURCE_FIELD, true, SPECIES_get_selector(), FIELD_FILTER_STRING,        SF_STANDARD, "source", 0, 20, 10, NULL);
    create_selection_list_on_itemfields(GLOBAL_gb_dst, aws, AWAR_DEST_FIELD,   true, SPECIES_get_selector(), (1<<GB_STRING)|(1<<GB_INT), SF_STANDARD, "dest",   0, 20, 10, NULL); // @@@ create/use field filter alias?

#if defined(WARN_TODO)
#warning check code above. Maybe one call has to get GLOBAL_gb_src ?
#endif


    aws->at("go");
    aws->highlight();
    aws->callback(mg_check_field_cb);
    aws->create_button("GO", "GO");

    return aws;
}
