// =============================================================== //
//                                                                 //
//   File      : MG_species.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"
#include "MG_adapt_ali.hxx"

#include <AW_rename.hxx>
#include <db_query.h>
#include <db_scanner.hxx>
#include <item_sel_list.h>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <arb_str.h>
#include <arb_strbuf.h>

#define AWAR_SPECIES_SRC AWAR_MERGE_TMP_SRC "name"
#define AWAR_FIELD_SRC   AWAR_MERGE_TMP_SRC "field"
#define AWAR_TAG_SRC     AWAR_MERGE_TMP_SRC "tag"

#define AWAR_SPECIES_DST AWAR_MERGE_TMP_DST "name"
#define AWAR_FIELD_DST   AWAR_MERGE_TMP_DST "field"
#define AWAR_TAG_DST     AWAR_MERGE_TMP_DST "tag"

#define AWAR_TAG_DEL AWAR_MERGE_TMP "tagdel"
#define AWAR_APPEND  AWAR_MERGE_TMP "append"

static DbScanner *scanner_src = 0;
static DbScanner *scanner_dst = 0;

const char *MG_left_AWAR_SPECIES_NAME() { return AWAR_SPECIES_SRC; }

void MG_create_species_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SPECIES_SRC, "", aw_def);
    aw_root->awar_string(AWAR_SPECIES_DST, "", aw_def);
    aw_root->awar_string(AWAR_FIELD_SRC,   "", aw_def);
    aw_root->awar_int(AWAR_APPEND);
}

static void MG_map_species(AW_root *aw_root, AW_CL db_nr) {
    GBDATA     *gb_main   = get_gb_main(db_nr);
    const char *awar_name = db_nr == 1 ? AWAR_SPECIES_SRC : AWAR_SPECIES_DST;
    DbScanner  *scanner   = db_nr == 1 ? scanner_src : scanner_dst;

    GB_transaction ta(gb_main);
    char   *selected_species = aw_root->awar(awar_name)->read_string();
    GBDATA *gb_species       = GBT_find_species(gb_main, selected_species);
    if (gb_species) map_db_scanner(scanner, gb_species, awar_name);
    free(selected_species);
}

static GB_ERROR MG_transfer_fields_info(char *fieldname = NULL) {
    GBDATA   *gb_key_data = GB_search(GLOBAL_gb_src, CHANGE_KEY_PATH, GB_CREATE_CONTAINER);
    GB_ERROR  error       = 0;

    if (!gb_key_data) error = GB_await_error();
    else {
        if (!GB_search(GLOBAL_gb_dst, CHANGE_KEY_PATH, GB_CREATE_CONTAINER)) error = GB_await_error();
        else {
            for (GBDATA *gb_key = GB_entry(gb_key_data, CHANGEKEY);
                 gb_key && !error;
                 gb_key = GB_nextEntry(gb_key))
            {
                GBDATA *gb_key_name = GB_entry(gb_key, CHANGEKEY_NAME);
                if (gb_key_name) {
                    GBDATA *gb_key_type = GB_entry(gb_key, CHANGEKEY_TYPE);
                    if (gb_key_type) {
                        char *name = GB_read_string(gb_key_name);

                        if (!fieldname || strcmp(fieldname, name) == 0) {
                            error = GBT_add_new_changekey(GLOBAL_gb_dst, name, (int)GB_read_int(gb_key_type));
                        }
                        free(name);
                    }
                }
            }
        }
    }
    return error;
}

static GB_ERROR MG_transfer_one_species(AW_root *aw_root, MG_remaps& remap,
                                        GBDATA *gb_species_data1, GBDATA *gb_species_data2,
                                        bool is_genome_db1, bool is_genome_db2,
                                        GBDATA  *gb_species1, const char *name1,
                                        GB_HASH *source_organism_hash, GB_HASH *dest_species_hash,
                                        GB_HASH *error_suppressor)
{
    // copies one species from source to destination DB
    //
    // either 'gb_species1' or 'name1' (and 'gb_species_data1') has to be given
    // 'source_organism_hash' may be NULL, otherwise it's used to search for source organisms (when merging from genome DB)
    // 'dest_species_hash' may be NULL, otherwise created species is stored there

    GB_ERROR error = 0;
    if (gb_species1) {
        name1 = GBT_read_name(gb_species1);
    }
    else {
        mg_assert(name1);
        gb_species1 = GBT_find_species_rel_species_data(gb_species_data1, name1);
        if (!gb_species1) {
            error = GBS_global_string("Could not find species '%s'", name1);
        }
    }

    bool transfer_fields = false;
    if (is_genome_db1) {
        if (is_genome_db2) { // genome -> genome
            if (GEN_is_pseudo_gene_species(gb_species1)) {
                const char *origin        = GEN_origin_organism(gb_species1);
                GBDATA     *dest_organism = dest_species_hash
                    ? (GBDATA*)GBS_read_hash(dest_species_hash, origin)
                    : GEN_find_organism(GLOBAL_gb_dst, origin);

                if (dest_organism) transfer_fields = true;
                else {
                    error = GBS_global_string("Destination DB does not contain '%s's origin-organism '%s'",
                                              name1, origin);
                }
            }
            // else: merge organism ok
        }
        else { // genome -> non-genome
            if (GEN_is_pseudo_gene_species(gb_species1)) transfer_fields = true;
            else {
                error = GBS_global_string("You can't merge organisms (%s) into a non-genome DB.\n"
                                          "Only pseudo-species are possible", name1);
            }
        }
    }
    else {
        if (is_genome_db2) { // non-genome -> genome
            error = GBS_global_string("You can't merge non-genome species (%s) into a genome DB", name1);
        }
        // else: non-genome -> non-genome ok
    }

    GBDATA *gb_species2 = 0;
    if (!error) {
        gb_species2 = dest_species_hash
            ? (GBDATA*)GBS_read_hash(dest_species_hash, name1)
            : GBT_find_species_rel_species_data(gb_species_data2, name1);

        if (gb_species2) error = GB_delete(gb_species2);
    }
    if (!error) {
        gb_species2 = GB_create_container(gb_species_data2, "species");
        if (!gb_species2) error = GB_await_error();
    }
    if (!error) error = GB_copy(gb_species2, gb_species1);
    if (!error && transfer_fields) {
        mg_assert(is_genome_db1);
        error = MG_export_fields(aw_root, gb_species1, gb_species2, error_suppressor, source_organism_hash);
    }
    if (!error) GB_write_flag(gb_species2, 1);
    if (!error) error = MG_transfer_all_alignments(&remap, gb_species1, gb_species2);
    if (!error && dest_species_hash) GBS_write_hash(dest_species_hash, name1, (long)gb_species2);

    return error;
}

static const char *get_reference_species_names(AW_root *awr) {
    return awr->awar(AWAR_REMAP_SPECIES_LIST)->read_char_pntr();
}
static bool adaption_enabled(AW_root *awr) {
    return awr->awar(AWAR_REMAP_ENABLE)->read_int() != 0;
}

static void MG_transfer_selected_species(AW_window *aww) {
    if (MG_copy_and_check_alignments(aww) != 0) return;

    AW_root  *aw_root = aww->get_root();
    char     *source  = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
    GB_ERROR  error   = NULL;

    if (!source || !source[0]) {
        error = "Please select a species in the left list";
    }
    else {
        arb_progress progress("Transferring selected species");

        error             = GB_begin_transaction(GLOBAL_gb_src);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dst);

        if (!error) {
            MG_remaps rm(GLOBAL_gb_src, GLOBAL_gb_dst, adaption_enabled(aw_root), get_reference_species_names(aw_root));

            GBDATA *gb_species_data1 = GBT_get_species_data(GLOBAL_gb_src);
            GBDATA *gb_species_data2 = GBT_get_species_data(GLOBAL_gb_dst);

            bool is_genome_db1 = GEN_is_genome_db(GLOBAL_gb_src, -1);
            bool is_genome_db2 = GEN_is_genome_db(GLOBAL_gb_dst, -1);

            error = MG_transfer_one_species(aw_root, rm,
                                            gb_species_data1, gb_species_data2,
                                            is_genome_db1, is_genome_db2,
                                            NULL, source,
                                            NULL, NULL,
                                            NULL);
        }

        if (!error) error = MG_transfer_fields_info();

        error = GB_end_transaction(GLOBAL_gb_src, error);
        error = GB_end_transaction(GLOBAL_gb_dst, error);
    }

    if (error) aw_message(error);
}

static void MG_transfer_species_list(AW_window *aww) {
    if (MG_copy_and_check_alignments(aww) != 0) return;

    GB_ERROR error = NULL;
    GB_begin_transaction(GLOBAL_gb_src);
    GB_begin_transaction(GLOBAL_gb_dst);

    bool is_genome_db1 = GEN_is_genome_db(GLOBAL_gb_src, -1);
    bool is_genome_db2 = GEN_is_genome_db(GLOBAL_gb_dst, -1);

    GB_HASH *error_suppressor     = GBS_create_hash(50, GB_IGNORE_CASE);
    GB_HASH *dest_species_hash    = GBT_create_species_hash(GLOBAL_gb_dst);
    GB_HASH *source_organism_hash = is_genome_db1 ? GBT_create_organism_hash(GLOBAL_gb_src) : 0;

    AW_root   *aw_root = aww->get_root();
    MG_remaps  rm(GLOBAL_gb_src, GLOBAL_gb_dst, adaption_enabled(aw_root), get_reference_species_names(aw_root));

    GBDATA *gb_species1;
    arb_progress progress("Transferring listed species", mg_count_queried(GLOBAL_gb_src));
    
    for (gb_species1 = GBT_first_species(GLOBAL_gb_src);
         gb_species1;
         gb_species1 = GBT_next_species(gb_species1))
    {
        if (IS_QUERIED_SPECIES(gb_species1)) {
            GBDATA *gb_species_data2 = GBT_get_species_data(GLOBAL_gb_dst);

            error = MG_transfer_one_species(aw_root, rm,
                                            NULL, gb_species_data2,
                                            is_genome_db1, is_genome_db2,
                                            gb_species1, NULL,
                                            source_organism_hash, dest_species_hash,
                                            error_suppressor);

            progress.inc_and_check_user_abort(error);
        }
    }

    GBS_free_hash(dest_species_hash);
    if (source_organism_hash) GBS_free_hash(source_organism_hash);
    GBS_free_hash(error_suppressor);

    if (!error) error = MG_transfer_fields_info();

    error = GB_end_transaction(GLOBAL_gb_src, error);
    GB_end_transaction_show_error(GLOBAL_gb_dst, error, aw_message);
}

static void MG_transfer_fields_cb(AW_window *aww) {
    if (MG_copy_and_check_alignments(aww) != 0) return;

    char     *field  = aww->get_root()->awar(AWAR_FIELD_SRC)->read_string();
    long      append = aww->get_root()->awar(AWAR_APPEND)->read_int();
    GB_ERROR  error  = 0;

    if (field[0] == 0) {
        error = "Please select a field you want to transfer";
    }
    else if (strcmp(field, "name") == 0) {
        error = "Transferring the 'name' field is forbidden";
    }
    else {
        GB_begin_transaction(GLOBAL_gb_src);
        GB_begin_transaction(GLOBAL_gb_dst);

        GBDATA *gb_dest_species_data  = GBT_get_species_data(GLOBAL_gb_dst);
        bool    transfer_of_alignment = GBS_string_matches(field, "ali_*/data", GB_MIND_CASE);

        arb_progress progress("Transferring fields of listed", mg_count_queried(GLOBAL_gb_src));

        AW_root   *aw_root = aww->get_root();
        MG_remaps  rm(GLOBAL_gb_src, GLOBAL_gb_dst, adaption_enabled(aw_root), get_reference_species_names(aw_root));

        for (GBDATA *gb_species1 = GBT_first_species(GLOBAL_gb_src);
             gb_species1 && !error;
             gb_species1 = GBT_next_species(gb_species1))
        {
            if (IS_QUERIED_SPECIES(gb_species1)) {
                const char *name1       = GBT_read_name(gb_species1);
                GBDATA     *gb_species2 = GB_find_string(gb_dest_species_data, "name", name1, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

                if (!gb_species2) {
                    gb_species2             = GB_create_container(gb_dest_species_data, "species");
                    if (!gb_species2) error = GB_await_error();
                    else error              = GBT_write_string(gb_species2, "name", name1);
                }
                else {
                    gb_species2 = GB_get_father(gb_species2);
                }

                if (!error) {
                    GBDATA *gb_field1 = GB_search(gb_species1, field, GB_FIND);
                    GBDATA *gb_field2 = GB_search(gb_species2, field, GB_FIND);
                    bool    use_copy  = true;

                    if (gb_field2 && gb_field1) {
                        GB_TYPES type1 = GB_read_type(gb_field1);
                        GB_TYPES type2 = GB_read_type(gb_field2);

                        if ((type1==type2) && (GB_DB != type1)) {
                            if (append && type1 == GB_STRING) {
                                char *s1 = GB_read_string(gb_field1);
                                char *s2 = GB_read_string(gb_field2);

                                if (!s1 || !s2) error = GB_await_error();
                                else {
                                    if (!GBS_find_string(s2, s1, 0)) {
                                        error = GB_write_string(gb_field2, GBS_global_string("%s %s", s2, s1));
                                        if (!error) GB_write_flag(gb_species2, 1);
                                    }
                                }

                                free(s1);
                                free(s2);
                            }
                            else { // not GB_STRING
                                error = GB_copy(gb_field2, gb_field1);
                                if (!error) GB_write_flag(gb_species2, 1);
                                if (transfer_of_alignment && !error) {
                                    error = MG_transfer_all_alignments(&rm, gb_species1, gb_species2);
                                }
                            }
                            use_copy = false;
                        }
                    }

                    if (use_copy) {
                        if (gb_field2) {
                            if (gb_field1 && !append) error = GB_delete(gb_field2);
                        }
                        if (gb_field1 && !error) {
                            GB_TYPES type1        = GB_read_type(gb_field1);
                            gb_field2             = GB_search(gb_species2, field, type1);
                            if (!gb_field2) error = GB_await_error();
                            else     error        = GB_copy(gb_field2, gb_field1);
                        }
                    }
                }
                progress.inc_and_check_user_abort(error);
            }
        }

        if (!error) error = MG_transfer_fields_info(field);

        error = GB_end_transaction(GLOBAL_gb_src, error);
        error = GB_end_transaction(GLOBAL_gb_dst, error);
    }

    if (error) aw_message(error);
    free(field);
}

static AW_window *MG_transfer_fields(AW_root *aw_root)
{
    GB_transaction    dummy(GLOBAL_gb_src);
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "MERGE_TRANSFER_FIELD", "TRANSFER FIELD");
    aws->load_xfig("merge/mg_transfield.fig");
    aws->button_length(13);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("go");
    aws->callback(MG_transfer_fields_cb);
    aws->create_button("GO", "GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_spec_sel_field.hlp");
    aws->create_button("HELP", "HELP");

    aws->at("append");
    aws->create_toggle(AWAR_APPEND);

    create_selection_list_on_itemfields(GLOBAL_gb_src,
                                            aws, AWAR_FIELD_SRC,
                                            FIELD_FILTER_NDS,
                                            "scandb", "rescandb", SPECIES_get_selector(), 20, 10);

    return (AW_window*)aws;
}

static void MG_move_field_cb(AW_window *aww) {
    if (MG_copy_and_check_alignments(aww) != 0) return;

    AW_root  *aw_root = aww->get_root();
    char     *field   = aww->get_root()->awar(AWAR_FIELD_SRC)->read_string();
    GB_ERROR  error   = 0;

    if (field[0] == 0) {
        error = "Please select a field to transfer";
    }
    else if (strcmp(field, "name") == 0) {
        error = "You are not allowed to transfer the 'name' field";
    }
    else {
        arb_progress progress("Cross Move field");
        error             = GB_begin_transaction(GLOBAL_gb_src);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dst);

        if (!error) {
            GBDATA *gb_species1;
            GBDATA *gb_species2;
            {
                char *source = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
                char *dest   = aw_root->awar(AWAR_SPECIES_DST)->read_string();

                gb_species1 = GBT_find_species(GLOBAL_gb_src, source);
                gb_species2 = GBT_find_species(GLOBAL_gb_dst, dest);

                free(dest);
                free(source);
            }

            if (!gb_species1) error = "Please select a species in left hitlist";
            if (!gb_species2) error = "Please select a species in right hitlist";

            if (!error) {
                GBDATA *gb_field1 = GB_search(gb_species1, field, GB_FIND);
                if (!gb_field1) error = GBS_global_string("Species 1 has no field '%s'", field);

                if (!error) {
                    GB_TYPES  type1     = GB_read_type(gb_field1);
                    GBDATA   *gb_field2 = GB_search(gb_species2, field, GB_FIND);

                    if (gb_field2) {
                        int type2 = GB_read_type(gb_field2);

                        if ((type1==type2) && (GB_DB != type1)) {
                            error = GB_copy(gb_field2, gb_field1);
                        }
                        else { // remove dest. if type mismatch or container
                            error     = GB_delete(gb_field2);
                            gb_field2 = 0; // trigger copy
                        }
                    }

                    if (!error && !gb_field2) { // destination missing or removed
                        gb_field2             = GB_search(gb_species2, field, type1);
                        if (!gb_field2) error = GB_await_error();
                        else error            = GB_copy(gb_field2, gb_field1);
                    }
                }
                if (!error) GB_write_flag(gb_species2, 1);
            }
        }
        if (!error) error = MG_transfer_fields_info(field);

        error = GB_end_transaction(GLOBAL_gb_src, error);
        error = GB_end_transaction(GLOBAL_gb_dst, error);
    }

    if (error) aw_message(error);

    free(field);
}

static AW_window *create_mg_move_fields(AW_root *aw_root)
{
    GB_transaction dummy(GLOBAL_gb_src);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_CROSS_MOVE_FIELD", "CROSS MOVE FIELD");
    aws->load_xfig("merge/mg_movefield.fig");
    aws->button_length(13);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("go");
    aws->callback(MG_move_field_cb);
    aws->create_button("GO", "GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"movefield.hlp");
    aws->create_button("HELP", "HELP");


    create_selection_list_on_itemfields(GLOBAL_gb_src,
                                            aws, AWAR_FIELD_SRC,
                                            FIELD_FILTER_NDS,
                                            "scandb", "rescandb", SPECIES_get_selector(), 20, 10);

    return (AW_window*)aws;
}

static void MG_merge_tagged_field_cb(AW_window *aww) {
    GB_transaction ta_merge(GLOBAL_gb_src);
    GB_ERROR       error = GB_begin_transaction(GLOBAL_gb_dst);

    if (!error) {
        AW_root *awr      = aww->get_root();
        char    *f1       = awr->awar(AWAR_FIELD_SRC)->read_string();
        char    *f2       = awr->awar(AWAR_FIELD_DST)->read_string();
        char    *tag1     = awr->awar(AWAR_TAG_SRC)->read_string();
        char    *tag2     = awr->awar(AWAR_TAG_DST)->read_string();
        char    *tag_del1 = awr->awar(AWAR_TAG_DEL)->read_string();

        arb_progress progress("Merging tagged fields", mg_count_queried(GLOBAL_gb_src));
        
        GBDATA *gb_dest_species_data     = GBT_get_species_data(GLOBAL_gb_dst);
        if (!gb_dest_species_data) error = GB_await_error();
        else {
            for (GBDATA * gb_species1 = GBT_first_species(GLOBAL_gb_src);
                 gb_species1 && !error;
                 gb_species1 = GBT_next_species(gb_species1))
            {
                if (IS_QUERIED_SPECIES(gb_species1)) {
                    char *name       = GBT_read_string(gb_species1, "name");
                    if (!name) error = GB_await_error();
                    else {
                        GBDATA *gb_species2     = GBT_find_or_create_species_rel_species_data(gb_dest_species_data, name);
                        if (!gb_species2) error = GB_await_error();
                        else {
                            char *s1              = GBT_readOrCreate_string(gb_species1, f1, "");
                            char *s2              = GBT_readOrCreate_string(gb_species2, f2, "");
                            if (!s1 || !s2) error = GB_await_error();
                            else {
                                char *sum       = GBS_merge_tagged_strings(s1, tag1, tag_del1, s2, tag2, tag_del1);
                                if (!sum) error = GB_await_error();
                                else  error     = GBT_write_string(gb_species2, f2, sum);
                                free(sum);
                            }
                            free(s2);
                            free(s1);
                        }
                    }
                    free(name);
                    progress.inc_and_check_user_abort(error);
                }
            }
        }

        free(tag_del1);
        free(tag2);
        free(tag1);
        free(f2);
        free(f1);
    }
    GB_end_transaction_show_error(GLOBAL_gb_dst, error, aw_message);
}

static AW_window *create_mg_merge_tagged_fields(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    GB_transaction dummy(GLOBAL_gb_src);

    aw_root->awar_string(AWAR_FIELD_SRC, "full_name");
    aw_root->awar_string(AWAR_FIELD_DST, "full_name");

    aw_root->awar_string(AWAR_TAG_SRC, "S");
    aw_root->awar_string(AWAR_TAG_DST, "D");

    aw_root->awar_string(AWAR_TAG_DEL, "S*");

    aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_TAGGED_FIELDS", "MERGE TAGGED FIELDS");
    aws->load_xfig("merge/mg_mergetaggedfield.fig");
    aws->button_length(13);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("go");
    aws->callback(MG_merge_tagged_field_cb);
    aws->create_button("GO", "GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mergetaggedfield.hlp");
    aws->create_button("HELP", "HELP");

    aws->at("tag1");    aws->create_input_field(AWAR_TAG_SRC, 5);
    aws->at("tag2");    aws->create_input_field(AWAR_TAG_DST, 5);

    aws->at("del1");    aws->create_input_field(AWAR_TAG_DEL, 5);

    create_selection_list_on_itemfields(GLOBAL_gb_src, aws, AWAR_FIELD_SRC, FIELD_FILTER_NDS, "fields1", 0, SPECIES_get_selector(), 20, 10);
    create_selection_list_on_itemfields(GLOBAL_gb_dst,  aws, AWAR_FIELD_DST, FIELD_FILTER_NDS, "fields2", 0, SPECIES_get_selector(), 20, 10);

    return (AW_window*)aws;
}

static GB_ERROR MG_equal_alignments(bool autoselect_equal_alignment_name) {
    //! Make the alignment names equal
    ConstStrArray M_alignment_names;
    ConstStrArray D_alignment_names;
    GBT_get_alignment_names(M_alignment_names, GLOBAL_gb_src);
    GBT_get_alignment_names(D_alignment_names, GLOBAL_gb_dst);

    GB_ERROR error = 0;
    if (M_alignment_names[0] == 0) {
        error =  GB_export_error("No source sequences found");
    }
    else {
        char       *type = GBT_get_alignment_type_string(GLOBAL_gb_src, M_alignment_names[0]);
        const char *dest = 0;

        for (int d = D_alignment_names.size()-1; d>0; --d) {
            char *type2 = GBT_get_alignment_type_string(GLOBAL_gb_dst, D_alignment_names[d]);
            if (strcmp(type, type2) != 0) D_alignment_names.remove(d--);
            free(type2);
        }

        int d = D_alignment_names.size();
        switch (d) {
            case 0:
                error = GB_export_errorf("Cannot find a target alignment with a type of '%s'\n"
                                         "You should create one first or select a different alignment type\n"
                                         "during sequence import", type);
                break;
            case 1:
                dest = D_alignment_names[0];
                break;

            default:
                if (autoselect_equal_alignment_name) {
                    for (int i = 0; i<d; ++i) {
                        if (ARB_stricmp(M_alignment_names[0], D_alignment_names[i]) == 0) {
                            dest = D_alignment_names[i];
                            break;
                        }
                    }
                }

                if (!dest) {
                    GBS_strstruct *str = GBS_stropen(100);

                    for (int i=0; i<d; i++) {
                        GBS_strcat(str, D_alignment_names[i]);
                        GBS_chrcat(str, ',');
                    }
                    GBS_strcat(str, "ABORT");

                    char *b = GBS_strclose(str);
                    int aliid = aw_question(NULL,
                                            "There are more than one possible alignment targets\n"
                                            "Choose one destination alignment or ABORT", b);
                    free(b);

                    if (aliid >= d) {
                        error = "Operation Aborted";
                        break;
                    }
                    dest = D_alignment_names[aliid];
                }
                break;
        }
        if (!error && dest && strcmp(M_alignment_names[0], dest) != 0) {
            error = GBT_rename_alignment(GLOBAL_gb_src, M_alignment_names[0], dest, 1, 1);
            if (error) {
                error = GBS_global_string("Failed to rename alignment '%s' to '%s' (Reason: %s)",
                                          M_alignment_names[0], dest, error);
            }
            else {
                GBT_add_new_changekey(GLOBAL_gb_src, GBS_global_string("%s/data", dest), GB_STRING);
            }
        }
        free(type);
    }

    return error;
}

GB_ERROR MG_simple_merge(AW_root *awr) {
    //! Merge the sequences of two databases

    static char *m_name = 0;

    GB_HASH *D_species_hash = 0;

    GB_push_my_security(GLOBAL_gb_src);
    GB_push_my_security(GLOBAL_gb_dst);
    GB_begin_transaction(GLOBAL_gb_src);
    GB_begin_transaction(GLOBAL_gb_dst);

    GB_ERROR error = MG_equal_alignments(true);
    if (!error) {
        GBDATA *M_species_data = GBT_get_species_data(GLOBAL_gb_src);
        GBDATA *D_species_data = GBT_get_species_data(GLOBAL_gb_dst);

        freenull(m_name);

        {
            long M_species_count = GB_number_of_subentries(M_species_data);
            long D_species_count = GB_number_of_subentries(D_species_data);

            // create hash containing all species from gb_dest,
            // but sized to hold all species from both DBs:
            D_species_hash = GBT_create_species_hash_sized(GLOBAL_gb_dst, M_species_count+D_species_count);
        }

        bool overwriteall  = false;
        bool autorenameall = false;
        
        for (GBDATA *M_species = GB_entry(M_species_data, "species"); M_species; M_species = GB_nextEntry(M_species)) {
            GBDATA *M_name = GB_search(M_species, "name", GB_STRING);

            freeset(m_name, GB_read_string(M_name));

            int  count = 1;
            bool retry;
            do {
                retry = false;
                count++;

                GBDATA *D_species = (GBDATA*)GBS_read_hash(D_species_hash, m_name);
                if (D_species) {
                    if (overwriteall) {
                        error = GB_delete(D_species);
                    }
                    else if (autorenameall) {
                        GB_ERROR  dummy;
                        char     *newname = AWTC_create_numbered_suffix(D_species_hash, m_name, dummy);

                        mg_assert(newname);
                        freeset(m_name, newname);
                    }
                    else {
                        switch (aw_question("merge_existing_species",
                                            GBS_global_string("Warning:  There is a name conflict for species '%s'\n"
                                                              "  You may:\n"
                                                              "  - Overwrite existing species\n"
                                                              "  - Overwrite all species with name conflicts\n"
                                                              "  - Not import species\n"
                                                              "  - Rename imported species\n"
                                                              "  - Automatically rename species (append .NUM)\n"
                                                              "  - Abort everything", m_name),
                                            "overwrite, overwrite all, don't import, rename, auto-rename, abort")) {
                            case 1:
                                overwriteall = true;
                            case 0:
                                GB_delete(D_species);
                                break;

                            case 2:
                                continue;

                            case 3: {
                                GB_ERROR  warning;          // duplicated species warning (does not apply here)
                                char     *autoname = AWTC_create_numbered_suffix(D_species_hash, m_name, warning);

                                if (!autoname) autoname = strdup(m_name);
                                freeset(m_name, aw_input("Rename species", "Enter new name of species", autoname));
                                free(autoname);
                                retry = true;
                                break;
                            }
                            case 4:
                                autorenameall = true;
                                retry = true;
                                break;

                            case 5:
                                error = "Operation aborted";
                                break;
                        }
                    }
                }
            } while (retry);

            if (!error) {
                GBDATA *D_species     = GB_create_container(D_species_data, "species");
                if (!D_species) error = GB_await_error();
                else {
                    error             = GB_copy_with_protection(D_species, M_species, true);
                    if (!error) GB_write_flag(D_species, 1); // mark species
                    if (!error) error = GB_write_usr_private(D_species, 255); // put in hitlist
                    if (!error) error = GBT_write_string(D_species, "name", m_name);
                }

                GBS_write_hash(D_species_hash, m_name, (long)D_species);
            }

            if (error) break;
        }
    }

    if (D_species_hash) GBS_free_hash(D_species_hash);

    if (!error) error = MG_transfer_fields_info();
    if (!error) awr->awar(AWAR_SPECIES_NAME)->write_string(m_name);

    error = GB_end_transaction(GLOBAL_gb_src, error);
    GB_end_transaction_show_error(GLOBAL_gb_dst, error, aw_message);

    GB_pop_my_security(GLOBAL_gb_src);
    GB_pop_my_security(GLOBAL_gb_dst);

    return error;
}

// -----------------------------
//      MG_species_selector

static void mg_select_species1(GBDATA*,  AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES_SRC)->write_string(item_name);
}
static void mg_select_species2(GBDATA*,  AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES_DST)->write_string(item_name);
}

static GBDATA *mg_get_first_species_data1(GBDATA *, AW_root *, QUERY_RANGE) {
    return GBT_get_species_data(GLOBAL_gb_src);
}
static GBDATA *mg_get_first_species_data2(GBDATA *, AW_root *, QUERY_RANGE) {
    return GBT_get_species_data(GLOBAL_gb_dst);
}

static GBDATA *mg_get_selected_species1(GBDATA * /* gb_main */, AW_root *aw_root) {
    GB_transaction dummy(GLOBAL_gb_src);
    char   *species_name            = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
    GBDATA *gb_species              = 0;
    if (species_name[0]) gb_species = GBT_find_species(GLOBAL_gb_src, species_name);
    free(species_name);
    return gb_species;
}
static GBDATA *mg_get_selected_species2(GBDATA * /* gb_main */, AW_root *aw_root) {
    GB_transaction dummy(GLOBAL_gb_dst);
    char   *species_name            = aw_root->awar(AWAR_SPECIES_DST)->read_string();
    GBDATA *gb_species              = 0;
    if (species_name[0]) gb_species = GBT_find_species(GLOBAL_gb_dst, species_name);
    free(species_name);
    return gb_species;
}

static MutableItemSelector MG_species_selector[2];

static void mg_initialize_species_selectors() {
    static int initialized = 0;
    if (!initialized) {
        MG_species_selector[0] = MG_species_selector[1] = SPECIES_get_selector();

        for (int s = 0; s <= 1; ++s) {
            MutableItemSelector& sel = MG_species_selector[s];

            sel.update_item_awars        = s ? mg_select_species2 : mg_select_species1;
            sel.get_first_item_container = s ? mg_get_first_species_data2 : mg_get_first_species_data1;
            sel.get_selected_item = s ? mg_get_selected_species2 : mg_get_selected_species1;
        }

        initialized = 1;
    }
}

AW_window *MG_merge_species_cb(AW_root *awr) {
    static AW_window_simple_menu *aws = 0;
    if (aws) return (AW_window *)aws;

    awr->awar_string(AWAR_REMAP_SPECIES_LIST, "ecoli", GLOBAL_gb_dst);
    awr->awar_int(AWAR_REMAP_ENABLE, 0, GLOBAL_gb_dst);

    aws = new AW_window_simple_menu;
    aws->init(awr, "MERGE_TRANSFER_SPECIES", "TRANSFER SPECIES");
    aws->load_xfig("merge/species.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_species.hlp");
    aws->create_button("HELP", "HELP", "H");

    mg_initialize_species_selectors();

    {
        QUERY::query_spec awtqs(MG_species_selector[0]);
        aws->create_menu("DB_I_Expert", "D");

        awtqs.gb_main                = GLOBAL_gb_src;
        awtqs.gb_ref                 = GLOBAL_gb_dst;
        awtqs.expect_hit_in_ref_list = false;
        awtqs.species_name           = AWAR_SPECIES_SRC;
        awtqs.tree_name              = 0;               // no selected tree here -> can't use tree related ACI commands without specifying a tree
        awtqs.select_bit             = 1;
        awtqs.ere_pos_fig            = "ere1";
        awtqs.by_pos_fig             = "by1";
        awtqs.qbox_pos_fig           = "qbox1";
        awtqs.rescan_pos_fig         = "rescan1";
        awtqs.key_pos_fig            = 0;
        awtqs.query_pos_fig          = "content1";
        awtqs.result_pos_fig         = "result1";
        awtqs.count_pos_fig          = "count1";
        awtqs.do_query_pos_fig       = "doquery1";
        awtqs.config_pos_fig         = "doconfig1";
        awtqs.do_mark_pos_fig        = 0;
        awtqs.do_unmark_pos_fig      = 0;
        awtqs.do_delete_pos_fig      = "dodelete1";
        awtqs.do_set_pos_fig         = "doset1";
        awtqs.do_refresh_pos_fig     = "dorefresh1";
        awtqs.open_parser_pos_fig    = "openparser1";
        awtqs.use_menu               = 1;

        create_query_box(aws, &awtqs, "db1");

        DbScanner *scanner = create_db_scanner(GLOBAL_gb_src, aws, "box1", 0, 0, 0, DB_SCANNER, 0, 0, FIELD_FILTER_NDS, awtqs.get_queried_itemtype());
        scanner_src = scanner;
        aws->get_root()->awar(AWAR_SPECIES_SRC)->add_callback(MG_map_species, 1);
    }
    {
        QUERY::query_spec awtqs(MG_species_selector[1]);
        aws->create_menu("DB_II_Expert", "B");

        awtqs.gb_main                = GLOBAL_gb_dst;
        awtqs.gb_ref                 = GLOBAL_gb_src;
        awtqs.expect_hit_in_ref_list = true;
        awtqs.species_name           = AWAR_SPECIES_DST;
        awtqs.select_bit             = 1;
        awtqs.ere_pos_fig            = "ere2";
        awtqs.by_pos_fig             = "by2";
        awtqs.qbox_pos_fig           = "qbox2";
        awtqs.rescan_pos_fig         = "rescan2";
        awtqs.key_pos_fig            = 0;
        awtqs.query_pos_fig          = "content2";
        awtqs.result_pos_fig         = "result2";
        awtqs.count_pos_fig          = "count2";
        awtqs.do_query_pos_fig       = "doquery2";
        awtqs.config_pos_fig         = "doconfig2";
        awtqs.do_mark_pos_fig        = 0;
        awtqs.do_unmark_pos_fig      = 0;
        awtqs.do_delete_pos_fig      = "dodelete2";
        awtqs.do_set_pos_fig         = "doset2";
        awtqs.do_refresh_pos_fig     = "dorefresh2";
        awtqs.open_parser_pos_fig    = "openparser2";
        awtqs.use_menu               = 1;

        create_query_box(aws, &awtqs, "db2");

        DbScanner *scanner = create_db_scanner(GLOBAL_gb_dst, aws, "box2", 0, 0, 0, DB_SCANNER, 0, 0, FIELD_FILTER_NDS, awtqs.get_queried_itemtype());
        scanner_dst = scanner;
        aws->get_root()->awar(AWAR_SPECIES_DST)->add_callback(MG_map_species, 2);
    }

    // big transfer buttons:
    aws->button_length(13);
    {
        aws->shadow_width(3);

        aws->at("transsspec");
        aws->callback(MG_transfer_selected_species);
        aws->create_button("TRANSFER_SELECTED_DELETE_DUPLICATED",
                           "TRANSFER\nSELECTED\nSPECIES\n\nDELETE\nDUPLICATE\nIN DB II", "T");

        aws->at("translspec");
        aws->callback(MG_transfer_species_list);
        aws->create_button("TRANSFER_LISTED_DELETE_DUPLI",
                           "TRANSFER\nLISTED\nSPECIES\n\nDELETE\nDUPLICATES\nIN DB II", "T");

        aws->at("transfield");
        aws->callback(AW_POPUP, (AW_CL)MG_transfer_fields, 0);
        aws->create_button("TRANSFER_FIELD_OF_LISTED_DELETE_DUPLI",
                           "TRANSFER\nFIELD\nOF LISTED\nSPECIES\n\nDELETE\nDUPLICATES\nIN DB II", "T");

        aws->shadow_width(1);
    }

    // adapt alignments
    aws->button_length(7);
    aws->at("adapt");
    aws->create_toggle(AWAR_REMAP_ENABLE);

    aws->at("reference");
    aws->create_text_field(AWAR_REMAP_SPECIES_LIST);

    aws->at("pres_sel");
    aws->callback((AW_CB1)AW_POPUP, (AW_CL)MG_select_preserves_cb);
    aws->create_button("SELECT", "SELECT", "S");

    // top icon
    aws->button_length(0);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_species.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    aws->create_menu("DB1->DB2", "-");
    aws->insert_menu_topic("compare_field_of_listed",   "Compare a field of listed species ...", "C", "checkfield.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_mg_check_fields, 0);
    aws->insert_menu_topic("move_field_of_selected",    "Move one field of selected left species to same field of selected right species", "M",
                            "movefield.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_mg_move_fields, 0);
    aws->insert_menu_topic("merge_field_of_listed_to_new_field", "Merge field of listed species of DB1 with different fields of same species of DB2 ", "D",
                            "mergetaggedfield.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_mg_merge_tagged_fields, 0);

    aws->sep______________();
    aws->insert_menu_topic("def_gene_species_field_xfer", "Define field transfer for gene-species", "g", "gene_species_field_transfer.hlp",
                           AWM_ALL, AW_POPUP, (AW_CL)MG_gene_species_create_field_transfer_def_window, 0);

    return aws;
}

