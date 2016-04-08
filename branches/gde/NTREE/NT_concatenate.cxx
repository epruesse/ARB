// =======================================================================================
/*                                                                                       */
//    File       : NT_concatenate.cxx
//    Purpose    : 1.Concatenatenation of sequences or alignments
//                 2.Merging the fields of similar species and creating a new species
//    Author     : Yadhu Kumar
//    web site   : http://www.arb-home.de/
/*                                                                                       */
//        Copyright Department of Microbiology (Technical University Munich)
/*                                                                                       */
// =======================================================================================

#include "NT_local.h"

#include <items.h>
#include <item_sel_list.h>
#include <awt_sel_boxes.hxx>
#include <AW_rename.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <arb_progress.h>
#include <arb_strbuf.h>
#include <arb_strarray.h>
#include <awt_modules.hxx>
#include <arb_global_defs.h>

using namespace std;

#define AWAR_CON_SEQUENCE_TYPE       "tmp/concat/sequence_type"
#define AWAR_CON_NEW_ALIGNMENT_NAME  "tmp/concat/new_alignment_name"
#define AWAR_CON_ALIGNMENT_SEPARATOR "tmp/concat/alignment_separator"
#define AWAR_CON_SELECTED_ALI        "tmp/concat/database_alignments"
#define AWAR_CON_MERGE_FIELD         "tmp/concat/merge_field"
#define AWAR_CON_STORE_SIM_SP_NO     "tmp/concat/store_sim_sp_no"

#define AWAR_CON_ALLOW_OVERWRITE_ALI   "tmp/concat/overwrite"
#define AWAR_CON_INSGAPS_FOR_MISS_ALIS "tmp/concat/insgaps"

#define MOVE_DOWN  0
#define MOVE_UP    1

struct SpeciesConcatenateList {
    GBDATA *species;
    char   *species_name;

    SpeciesConcatenateList *next;
};

// --------------------------creating and initializing AWARS----------------------------------------
void NT_createConcatenationAwars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    char *ali_default = GBT_get_default_alignment(gb_main);
    char *ali_type    = NULL;

    if (ali_default) {
        ali_type = GBT_get_alignment_type_string(gb_main, ali_default);
        if (!ali_type) {
            // Note: this message will appear during startup (i.e. stick to general statement here!)
            aw_message(GBS_global_string("Failed to detect type of default alignment (%s)\n"
                                         "(Reason: %s)", ali_default, GB_await_error()));
        }
    }
    if (!ali_type) ali_type = strdup("rna");

    aw_root->awar_string(AWAR_CON_SEQUENCE_TYPE,       ali_type,         aw_def);
    aw_root->awar_string(AWAR_CON_NEW_ALIGNMENT_NAME,  "ali_concat",     aw_def)->set_srt("ali_*=*:*=ali_*"); // auto-prefix with "ali_"
    aw_root->awar_string(AWAR_CON_ALIGNMENT_SEPARATOR, "XXX",            aw_def);
    aw_root->awar_string(AWAR_CON_SELECTED_ALI,        "",               aw_def);
    aw_root->awar_string(AWAR_CON_MERGE_FIELD,         "full_name",      aw_def);
    aw_root->awar_string(AWAR_CON_STORE_SIM_SP_NO,     "merged_species", aw_def);

    aw_root->awar_int(AWAR_CON_ALLOW_OVERWRITE_ALI,   0, aw_def);
    aw_root->awar_int(AWAR_CON_INSGAPS_FOR_MISS_ALIS, 1, aw_def);

    free(ali_type);
    free(ali_default);
}

// ------------------------Selecting alignments from the database for concatenation----------------------

inline char *get_alitype_eval(AW_root *aw_root) {
    return GBS_global_string_copy("%s=", aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->read_char_pntr()); 
}

static void alitype_changed_cb(AW_root *aw_root, AW_DB_selection *db_sel) {
    char *ali_type = get_alitype_eval(aw_root);
    awt_reconfigure_ALI_selection_list(db_sel, ali_type);
    free(ali_type);
}

static AW_DB_selection* createSelectionList(GBDATA *gb_main, AW_window *aws, const char *awarName) {

#ifdef DEBUG
    static bool ran=false;
    nt_assert(!ran);
    ran=true;                 // prevents calling this function for the second time
#endif

    AW_root         *aw_root  = aws->get_root();
    char            *ali_type = get_alitype_eval(aw_root);
    AW_DB_selection *db_sel   = awt_create_ALI_selection_list(gb_main, aws, awarName, ali_type);

    free(ali_type);
    return db_sel;
}

// ----------  Create SAI to display alignments that were concatenated --------------

static GB_ERROR create_concatInfo_SAI(GBDATA *gb_main, const char *new_ali_name, const char *ali_separator, const StrArray& ali_names) {
    GB_ERROR  error       = NULL;
    GBDATA   *gb_extended = GBT_find_or_create_SAI(gb_main, "ConcatInfo");

    if (!gb_extended) error = GB_await_error();
    else {
        GBDATA *gb_data = GBT_add_data(gb_extended, new_ali_name, "data", GB_STRING);

        if (!gb_data) {
            error = GB_await_error();
        }
        else {
            int new_ali_length = GBT_get_alignment_len(gb_main, new_ali_name);
            int sep_len        = strlen(ali_separator);

            char *info = (char*)malloc(new_ali_length+1);
            memset(info, '=', new_ali_length);

            int offset       = 0;
            int last_ali_idx = ali_names.size()-1;

            for (int a = 0; a <= last_ali_idx; ++a) {
                const char *ali         = ali_names[a];
                int         ali_len     = GBT_get_alignment_len(gb_main, ali);
                int         ali_str_len = strlen(ali);

                char *my_info = info+offset;

                int half_ali_len = ali_len/2;
                for (int i = 0; i<5; ++i) {
                    if (i<half_ali_len) {
                        my_info[i]           = '<';
                        my_info[ali_len-i-1] = '>';
                    }
                }

                if (ali_str_len<ali_len) {
                    int namepos = half_ali_len - ali_str_len/2;
                    memcpy(my_info+namepos, ali, ali_str_len);
                }

                offset += ali_len;
                if (a != last_ali_idx) {
                    memcpy(info+offset, ali_separator, sep_len);
                    offset += sep_len;
                }
            }

            nt_assert(offset == new_ali_length); // wrong alignment length!
            info[new_ali_length] = 0;
            
            if (!error) error = GB_write_string(gb_data, info);
            free(info);
        }
    }
    return error;
}

// ---------------------------------------- Concatenation function ----------------------------------
static void concatenateAlignments(AW_window *aws, AW_selection *selected_alis) {
    nt_assert(selected_alis);

    GB_push_transaction(GLOBAL.gb_main);

    long      marked_species = GBT_count_marked_species(GLOBAL.gb_main);
    AW_root  *aw_root        = aws->get_root();
    char     *new_ali_name   = aw_root->awar(AWAR_CON_NEW_ALIGNMENT_NAME)->read_string();
    GB_ERROR  error          = GBT_check_alignment_name(new_ali_name);

    StrArray ali_names;
    selected_alis->get_values(ali_names);

    arb_progress progress("Concatenating alignments", marked_species);
    size_t       ali_count = ali_names.size();

    if (!error && ali_count<2) {
        error = "Not enough alignments selected for concatenation (need at least 2)";
    }
    if (!error) {
        int found[ali_count], missing[ali_count], ali_length[ali_count];

        for (size_t a = 0; a<ali_count; a++) {
            found[a]      = 0;
            missing[a]    = 0;
            ali_length[a] = GBT_get_alignment_len(GLOBAL.gb_main, ali_names[a]);

            if (strcmp(ali_names[a], new_ali_name) == 0) {
                error = "Target alignment may not be one of the source alignments";
            }
        }

        if (!error) {
            char      *ali_separator = aw_root->awar(AWAR_CON_ALIGNMENT_SEPARATOR)->read_string();
            const int  sep_len       = strlen(ali_separator);

            long new_alignment_len = (ali_count-1)*sep_len;
            for (size_t a = 0; a<ali_count; ++a) {
                new_alignment_len += ali_length[a];
            }

            GBDATA *gb_presets          = GBT_get_presets(GLOBAL.gb_main);
            GBDATA *gb_alignment_exists = GB_find_string(gb_presets, "alignment_name", new_ali_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
            GBDATA *gb_new_alignment    = 0;
            char   *seq_type            = aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->read_string();

            if (gb_alignment_exists) {
                // target alignment exists
                if (aw_root->awar(AWAR_CON_ALLOW_OVERWRITE_ALI)->read_int()) { // allow overwrite
                    gb_new_alignment             = GBT_get_alignment(GLOBAL.gb_main, new_ali_name);
                    if (!gb_new_alignment) error = GB_await_error();
                }
                else {
                    error = GBS_global_string("Target alignment '%s' already exists\n(check overwrite-toggle if you really want to overwrite)", new_ali_name);
                }
            }
            else {
                // create new target alignment
                gb_new_alignment             = GBT_create_alignment(GLOBAL.gb_main, new_ali_name, new_alignment_len, 0, 0, seq_type);
                if (!gb_new_alignment) error = GB_await_error();
            }

            if (!error) {
                AW_repeated_question ask_about_missing_alignment;
                bool                 insertGaps = aw_root->awar(AWAR_CON_INSGAPS_FOR_MISS_ALIS)->read_int();

                for (GBDATA *gb_species = GBT_first_marked_species(GLOBAL.gb_main);
                     gb_species && !error;
                     gb_species = GBT_next_marked_species(gb_species))
                {
                    GBS_strstruct *str_seq       = GBS_stropen(new_alignment_len+1); // create output stream
                    int            data_inserted = 0;

                    for (size_t a = 0; a<ali_count; ++a) {
                        if (a) GBS_strcat(str_seq, ali_separator);

                        GBDATA *gb_seq_data = GBT_find_sequence(gb_species, ali_names[a]);
                        if (gb_seq_data) { // found data
                            const char *str_data = GB_read_char_pntr(gb_seq_data);
                            GBS_strcat(str_seq, str_data);
                            ++found[a];
                            ++data_inserted;
                        }
                        else { // missing data
                            if (insertGaps) GBS_chrncat(str_seq, '.', ali_length[a]);
                            ++missing[a];
                        }
                    }

                    if (!data_inserted) {
                        error = GBS_global_string("None of the source alignments had data for species '%s'", GBT_read_name(gb_species));
                    }
                    else {
                        char   *concatenated_ali_seq_data = GBS_strclose(str_seq);
                        GBDATA *gb_data                   = GBT_add_data(gb_species, new_ali_name, "data", GB_STRING);

                        GB_write_string(gb_data, concatenated_ali_seq_data);
                        free(concatenated_ali_seq_data);
                    }
                    progress.inc_and_check_user_abort(error);
                }

                if (!error) {
                    // ............. print missing alignments...........
                    aw_message(GBS_global_string("Concatenation of alignments was performed for %ld species.", marked_species));
                    for (size_t a = 0; a<ali_count; ++a) {
                        aw_message(GBS_global_string("%s: was found in %d species and missing in %d species.", ali_names[a], found[a], missing[a]));
                    }
                }

                if (!error) error = GBT_check_data(GLOBAL.gb_main, new_ali_name); // update alignment info (otherwise create_concatInfo_SAI fails when overwriting an alignment)
                if (!error) error = create_concatInfo_SAI(GLOBAL.gb_main, new_ali_name, ali_separator, ali_names);
            }

            free(seq_type);
            free(ali_separator);
        }
    }

    if (!error) {
        char *nfield = GBS_global_string_copy("%s/data", new_ali_name);
        error        = GBT_add_new_changekey(GLOBAL.gb_main, nfield, GB_STRING);
        free(nfield);
    }
    else {
        progress.done();
    }
    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
    free(new_ali_name);
}

static void addSpeciesToConcatenateList(SpeciesConcatenateList **sclp, GB_CSTR species_name) {

    GBDATA *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);
    GBDATA *gb_species      = GBT_find_species_rel_species_data(gb_species_data, species_name);

    if (gb_species) {
        SpeciesConcatenateList *scl = new SpeciesConcatenateList;

        scl->species      = gb_species;
        scl->species_name = strdup(species_name);
        scl->next         = *sclp;
        *sclp             = scl;
    }
}

static void freeSpeciesConcatenateList(SpeciesConcatenateList *scl) {
    while (scl) {
        SpeciesConcatenateList *next = scl->next;
        free(scl->species_name);
        delete scl;
        scl = next;
    }
}

static GB_ERROR checkAndMergeFields(GBDATA *gb_new_species, GB_ERROR error, SpeciesConcatenateList *scl) {

    char *doneFields = strdup(";name;"); // all fields which are already merged
    int   doneLen    = strlen(doneFields);
    SpeciesConcatenateList *sl = scl;
    int  sl_length = 0; while (scl) { sl_length++; scl=scl->next; } // counting no. of similar species stored in the list
    int *fieldStat = new int[sl_length]; // 0 = not used yet ; -1 = doesn't have field ; 1..n = field content (same number means same content)

    while (sl && !error) { // with all species do..
        char *newFields  = GB_get_subfields(sl->species);
        char *fieldStart = newFields; // points to ; before next field

        while (fieldStart[1] && !error) { // with all subfields of the species do..
            char *fieldEnd = strchr(fieldStart+1, ';');
            nt_assert(fieldEnd);
            char behind = fieldEnd[1]; fieldEnd[1] = 0;

            if (strstr(doneFields, fieldStart)==0) { // field is not merged yet
                char *fieldName = fieldStart+1;
                int   fieldLen  = int(fieldEnd-fieldName);

                nt_assert(fieldEnd[0]==';');
                fieldEnd[0] = 0;

                GBDATA   *gb_field = GB_search(sl->species, fieldName, GB_FIND); // field does to exist (it was found before)
                GB_TYPES  type     = GB_read_type(gb_field);

                if (type==GB_STRING) { // we only merge string fields
                    int i; int doneSpecies = 0; int nextStat = 1;

                    for (i=0; i<sl_length; i++) { fieldStat[i] = 0; } // clear field status

                    while (doneSpecies<sl_length) { // since all species in list were handled
                        SpeciesConcatenateList *sl2 = sl;
                        i = 0;

                        while (sl2) {
                            if (fieldStat[i]==0) {
                                gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                if (gb_field) {
                                    char *content = GB_read_as_string(gb_field);
                                    SpeciesConcatenateList *sl3 = sl2->next;
                                    fieldStat[i] = nextStat;
                                    int j = i+1; doneSpecies++;

                                    while (sl3) {
                                        if (fieldStat[j]==0) {
                                            gb_field = GB_search(sl3->species, fieldName, GB_FIND);
                                            if (gb_field) {
                                                char *content2 = GB_read_as_string(gb_field);
                                                if (strcmp(content, content2)==0) { // if contents are the same, they get the same status
                                                    fieldStat[j] = nextStat;
                                                    doneSpecies++;
                                                }
                                                free(content2);
                                            }
                                            else {
                                                fieldStat[j] = -1;
                                                doneSpecies++;
                                            }
                                        }
                                        sl3 = sl3->next; j++;
                                    }
                                    free(content); nextStat++;
                                }
                                else {
                                    fieldStat[i] = -1; // field does not exist here
                                    doneSpecies++;
                                }
                            }
                            sl2 = sl2->next; i++;
                        }
                        if (!sl2) break;
                    }
                    nt_assert(nextStat!=1); // this would mean that none of the species contained the field
                    {
                        char *new_content     = 0;
                        int   new_content_len = 0; // @@@ useless (0 where used; unused otherwise)

                        if (nextStat==2) { // all species contain same field content or do not have the field
                            SpeciesConcatenateList *sl2 = sl;
                            while (sl2) {
                                gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                if (gb_field) {
                                    new_content = GB_read_as_string(gb_field);
                                    new_content_len = strlen(new_content);
                                    break;
                                }
                                sl2 = sl2->next;
                            }
                        }
                        else { // different field contents
                            int actualStat;
                            for (actualStat=1; actualStat<nextStat; actualStat++) {
                                int names_len = 1; // open bracket
                                SpeciesConcatenateList *sl2 = sl;
                                char *content = 0; i = 0;

                                while (sl2) {
                                    if (fieldStat[i]==actualStat) {
                                        names_len += strlen(sl2->species_name)+1;
                                        if (!content) {
                                            gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                            nt_assert(gb_field);
                                            content = GB_read_as_string(gb_field);
                                        }
                                    }
                                    sl2 = sl2->next; i++;
                                }
                                nt_assert(content);
                                int add_len = names_len+1+strlen(content);
                                char *whole = (char*)malloc(new_content_len+1+add_len+1);
                                nt_assert(whole);
                                char *add = new_content ? whole+sprintf(whole, "%s ", new_content) : whole;
                                sl2 = sl; i = 0;
                                int first = 1;
                                while (sl2) {
                                    if (fieldStat[i]==actualStat) {
                                        add += sprintf(add, "%c%s", first ? '{' : ';', sl2->species_name);
                                        first = 0;
                                    }
                                    sl2 = sl2->next; i++;
                                }
                                add += sprintf(add, "} %s", content);

                                free(content);
                                freeset(new_content, whole);
                                new_content_len = strlen(new_content); // cppcheck-suppress deallocuse
                            }
                        }

                        if (new_content) {
                            error = GBT_write_string(gb_new_species, fieldName, new_content);
                            free(new_content);
                        }
                    }
                }

                // mark field as done:
                char *new_doneFields = (char*)malloc(doneLen+fieldLen+1+1);
                sprintf(new_doneFields, "%s%s;", doneFields, fieldName);
                doneLen += fieldLen+1;
                freeset(doneFields, new_doneFields);
                fieldEnd[0] = ';';
            }
            fieldEnd[1] = behind;
            fieldStart = fieldEnd;
        }
        free(newFields);
        sl = sl->next;
    }
    free(doneFields);
    delete [] fieldStat;

    return error;
}

static GBDATA *concatenateFieldsCreateNewSpecies(AW_window *, GBDATA *gb_species, SpeciesConcatenateList *scl) {
    GB_push_transaction(GLOBAL.gb_main);

    GB_ERROR  error           = 0;
    GBDATA   *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);

    // data needed for name generation
    char *full_name = 0;
    char *acc       = 0;

    // --------------------getting the species related data --------------------

    GBDATA *gb_new_species = 0;

    if (!error) {
        // copy species to create a new species
        gb_new_species = GB_create_container(gb_species_data, "species");
        error          = gb_new_species ? GB_copy(gb_new_species, gb_species) : GB_await_error();

        if (!error) { // write dummy-name (real name written below)
            error = GBT_write_string(gb_new_species, "name", "$currcat$");
        }
    }

    if (!error) { // copy full name
        full_name             = GBT_read_string(gb_species, "full_name");
        if (!full_name) error = GB_await_error();
        else error            = GBT_write_string(gb_new_species, "full_name", full_name);
    }

    if (!error) {
        ConstStrArray ali_names;
        GBT_get_alignment_names(ali_names, GLOBAL.gb_main);

        long id = 0;
        for (SpeciesConcatenateList *speciesList = scl; speciesList; speciesList = speciesList->next) {
            for (int no_of_alignments = 0; ali_names[no_of_alignments]!=0; no_of_alignments++) {
                GBDATA *gb_seq_data = GBT_find_sequence(speciesList->species, ali_names[no_of_alignments]);
                if (gb_seq_data) {
                    const char *seq_data  = GB_read_char_pntr(gb_seq_data);
                    GBDATA *gb_data = GBT_add_data(gb_new_species, ali_names[no_of_alignments], "data", GB_STRING);
                    error           = GB_write_string(gb_data, seq_data);
                    if (!error) id += GBS_checksum(seq_data, 1, ".-");  // creating checksum of the each aligned sequence to generate new accession number
                }
                if (error) error = GB_export_errorf("Can't create alignment '%s'", ali_names[no_of_alignments]);
            }
        }

        if (!error) {
            acc   = GBS_global_string_copy("ARB_%lX", id); // create new accession number
            error = GBT_write_string(gb_new_species, "acc", acc);
        }
    }

    if (!error) error = checkAndMergeFields(gb_new_species, error, scl);

    // now generate new name
    if (!error) {
        char *new_species_name = 0;

        const char *add_field = AW_get_nameserver_addid(GLOBAL.gb_main);
        GBDATA     *gb_addid  = add_field[0] ? GB_entry(gb_new_species, add_field) : 0;
        char       *addid     = 0;
        if (gb_addid) addid   = GB_read_as_string(gb_addid);

        error = AWTC_generate_one_name(GLOBAL.gb_main, full_name, acc, addid, new_species_name);
        if (!error) {   // name was created
            if (GBT_find_species_rel_species_data(gb_species_data, new_species_name) != 0) {
                // if the name is not unique -> create unique name
                UniqueNameDetector und(gb_species_data);
                freeset(new_species_name, AWTC_makeUniqueShortName(new_species_name, und));
                if (!new_species_name) error = GB_await_error();
            }
        }

        if (!error) error = GBT_write_string(gb_new_species, "name", new_species_name); // insert new 'name'

        free(new_species_name);
        free(addid);
    }

    error = GB_end_transaction(GLOBAL.gb_main, error);
    if (error) {
        gb_new_species = 0;
        aw_message(error);
    }

    free(acc);
    free(full_name);

    return gb_new_species;
}

static GB_ERROR checkAndCreateNewField(GBDATA *gb_main, char *new_field_name) {
    GB_ERROR error    = GB_check_key(new_field_name);

    if (error) return error;
    else {
        error = GBT_add_new_changekey(gb_main, new_field_name, GB_STRING);
        if (error) {
            bool overwrite = aw_ask_sure("merge_similar_overwrite_field",
                                         GBS_global_string("\"%s\" field exists! Do you want to overwrite the existing field?", new_field_name));
            if (!overwrite) return error;
        }
    }
    return 0;
}

enum MergeSpeciesType {
    MERGE_SPECIES_SIMPLE,
    MERGE_SPECIES_AND_CONCAT_ALI,
};

static void mergeSimilarSpecies(AW_window *aws, MergeSpeciesType mergeType, AW_selection *selected_alis) {
    nt_assert(correlated(selected_alis, mergeType == MERGE_SPECIES_AND_CONCAT_ALI));

    GB_ERROR     error = NULL;
    arb_progress wrapper;
    {
        AW_root *aw_root          = aws->get_root();
        char    *merge_field_name = aw_root->awar(AWAR_CON_MERGE_FIELD)->read_string();
        char    *new_field_name   = aw_root->awar(AWAR_CON_STORE_SIM_SP_NO)->read_string();

        SpeciesConcatenateList *scl            = 0; // to build list of similar species
        SpeciesConcatenateList *newSpeciesList = 0;  // new SpeciesConcatenateList

        GB_begin_transaction(GLOBAL.gb_main);       // open database for transaction

        error = checkAndCreateNewField(GLOBAL.gb_main, new_field_name);

        PersistentNameServerConnection stayAlive;
        arb_progress progress("Merging similar species", GBT_count_marked_species(GLOBAL.gb_main));
        progress.auto_subtitles("Species");

        for (GBDATA * gb_species = GBT_first_marked_species(GLOBAL.gb_main);
             gb_species && !error;
             gb_species = GBT_next_marked_species(gb_species))
        {
            GBDATA     *gb_species_field = GB_entry(gb_species, merge_field_name);
            const char *name             = GBT_read_name(gb_species);

            if (!gb_species_field) {
                // exit if species doesn't have any data in the selected field
                error = GBS_global_string("Species '%s' does not contain data in selected field '%s'", name, merge_field_name);
            }
            else {
                char *gb_species_field_content = GB_read_string(gb_species_field);
                int   similar_species          = 0;

                for (GBDATA * gb_species_next = GBT_next_marked_species(gb_species);
                     gb_species_next && !error;
                     gb_species_next = GBT_next_marked_species(gb_species_next))
                {
                    GBDATA     *gb_next_species_field = GB_entry(gb_species_next, merge_field_name);
                    const char *next_name             = GBT_read_name(gb_species_next);

                    if (!gb_next_species_field) {
                        // exit if species doesn't have any data in the selected field
                        error = GBS_global_string("Species '%s' does not contain data in selected field '%s'", next_name, merge_field_name);
                    }
                    else {
                        char *gb_next_species_field_content = GB_read_string(gb_next_species_field);

                        if (strcmp(gb_species_field_content, gb_next_species_field_content) == 0) {
                            addSpeciesToConcatenateList(&scl, next_name);
                            GB_write_flag(gb_species_next, 0);
                            ++similar_species;
                            ++progress;
                        }
                        free(gb_next_species_field_content);
                    }
                }

                if (similar_species > 0 && !error) {
                    addSpeciesToConcatenateList(&scl, name);
                    GB_write_flag(gb_species, 0);

                    GBDATA *new_species_created = concatenateFieldsCreateNewSpecies(aws, gb_species, scl);

                    nt_assert(new_species_created);
                    if (new_species_created) {      // create a list of newly created species
                        addSpeciesToConcatenateList(&newSpeciesList, GBT_read_name(new_species_created));
                    }

                    error = GBT_write_int(new_species_created, new_field_name, ++similar_species);
                }

                freeSpeciesConcatenateList(scl); scl = 0;
                free(gb_species_field_content);
            }

            progress.inc_and_check_user_abort(error);
        }

        if (!error) {
            GBT_mark_all(GLOBAL.gb_main, 0);        // unmark all species in the database
            int newSpeciesCount = 0;

            for (; newSpeciesList; newSpeciesList = newSpeciesList->next) { // mark only newly created species
                GB_write_flag(newSpeciesList->species, 1);
                newSpeciesCount++;
            }
            aw_message(GBS_global_string("%i new species were created by taking \"%s\" as a criterion!", newSpeciesCount, merge_field_name));
            freeSpeciesConcatenateList(newSpeciesList);
        }

        free(merge_field_name);
        free(new_field_name);

        GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
    }

    if (mergeType == MERGE_SPECIES_AND_CONCAT_ALI && !error) {
        concatenateAlignments(aws, selected_alis);
    }
}

static AW_window *createMergeSimilarSpeciesWindow(AW_root *aw_root, MergeSpeciesType mergeType, AW_selection *selected_alis) {
    AW_window_simple *aws = new AW_window_simple;

    {
        char       *window_id    = GBS_global_string_copy("MERGE_SPECIES_%i", mergeType);
        const char *window_title = NULL;
        switch (mergeType) {
            case MERGE_SPECIES_SIMPLE:         window_title = "Merge species";         break;
            case MERGE_SPECIES_AND_CONCAT_ALI: window_title = "Merge and concatenate"; break;
        }
        aws->init(aw_root, window_id, window_title);
        free(window_id);
    }
    aws->load_xfig("merge_species.fig");

    aws->callback(makeHelpCallback("merge_species.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("field_select");
    aws->auto_space(0, 0);
    aws->callback(AW_POPDOWN);
    create_selection_list_on_itemfields(GLOBAL.gb_main, aws, AWAR_CON_MERGE_FIELD, SPECIES_get_selector(), FIELD_FILTER_NDS, SF_HIDDEN, "field_select", "sel_merge_field");

    aws->at("store_sp_no");
    aws->label_length(20);
    aws->create_input_field(AWAR_CON_STORE_SIM_SP_NO, 20);

    {
        const char *buttonText = NULL;
        switch (mergeType) {
            case MERGE_SPECIES_SIMPLE:         buttonText = "Merge similar species";                       break;
            case MERGE_SPECIES_AND_CONCAT_ALI: buttonText = "Merge similar species and concat alignments"; break;
        }

        aws->at("merge");
        aws->callback(makeWindowCallback(mergeSimilarSpecies, mergeType, selected_alis));
        aws->create_autosize_button("MERGE_SIMILAR_SPECIES", buttonText, "M");
    }

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    return aws;
}

AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root) {
    static AW_window *aw = 0;
    if (!aw) aw = createMergeSimilarSpeciesWindow(aw_root, MERGE_SPECIES_SIMPLE, NULL);
    return aw;
}

static AW_window *NT_createMergeSimilarSpeciesAndConcatenateWindow(AW_root *aw_root, AW_selection *selected_alis) {
    static AW_window *aw = NULL;
#if defined(ASSERTION_USED)
    static AW_selection *prev_selected_alis = NULL;
#endif

    if (!aw) {
        aw = createMergeSimilarSpeciesWindow(aw_root, MERGE_SPECIES_AND_CONCAT_ALI, selected_alis);
#if defined(ASSERTION_USED)
        prev_selected_alis = selected_alis;
#endif
    }
#if defined(ASSERTION_USED)
    nt_assert(selected_alis == prev_selected_alis); // would need multiple windows in that case
#endif
    return aw;
}

static void useSelectedAlignment(AW_window *aww) {
    AW_root    *root   = aww->get_root();
    const char *selali = root->awar(AWAR_CON_SELECTED_ALI)->read_char_pntr();
    if (selali && strcmp(selali, NO_ALI_SELECTED) != 0) {
        root->awar(AWAR_CON_NEW_ALIGNMENT_NAME)->write_string(selali);
    }
    else {
        aw_message("Select alignment to use in the left alignment list");
    }
}

// ----------------------------Creating concatenation window-----------------------------------------
AW_window *NT_createConcatenationWindow(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CONCAT_ALIGNMENTS", "Concatenate Alignments");
    aws->load_xfig("concatenate.fig");

    aws->auto_space(5, 5);
    aws->button_length(8);

    aws->callback(makeHelpCallback("concatenate.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("dbAligns");
    AW_DB_selection *all_alis = createSelectionList(GLOBAL.gb_main, aws, AWAR_CON_SELECTED_ALI);
    AW_selection    *sel_alis = awt_create_subset_selection_list(aws, all_alis->get_sellist(), "concatAligns", "collect", "sort");

    aws->at("type");
    aws->create_option_menu(AWAR_CON_SEQUENCE_TYPE, true);
    aws->insert_option("DNA", "d", "dna");
    aws->insert_option("RNA", "r", "rna");
    aws->insert_default_option("PROTEIN", "p", "ami");
    aws->update_option_menu();
    aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->add_callback(makeRootCallback(alitype_changed_cb, all_alis));

    aws->at("aliSeparator");
    aws->create_input_field(AWAR_CON_ALIGNMENT_SEPARATOR, 10);

    aws->at("aliName");
    aws->create_input_field(AWAR_CON_NEW_ALIGNMENT_NAME, 25);
    aws->button_length(5);
    aws->callback(useSelectedAlignment);
    aws->create_button("USE", "Use");

    aws->at("overwrite");
    aws->label("Allow to overwrite an existing alignment?");
    aws->create_toggle(AWAR_CON_ALLOW_OVERWRITE_ALI);

    aws->at("insgaps");
    aws->label("Insert gaps for missing alignment data?");
    aws->create_toggle(AWAR_CON_INSGAPS_FOR_MISS_ALIS);

    aws->button_length(22);
    aws->at("go");

    aws->callback(makeWindowCallback(concatenateAlignments, sel_alis));
    aws->create_button("CONCATENATE", "CONCATENATE", "A");

    aws->callback(NT_createMergeSimilarSpeciesWindow);
    aws->create_button("MERGE_SPECIES", "MERGE SIMILAR SPECIES", "M");

    aws->callback(makeCreateWindowCallback(NT_createMergeSimilarSpeciesAndConcatenateWindow, sel_alis));
    aws->create_button("MERGE_CONCATENATE", "MERGE & CONCATENATE", "S");

    return aws;
}
// -------------------------------------------------------------------------------------------------------
