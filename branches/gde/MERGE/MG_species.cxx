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
#include <arb_global_defs.h>

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

static void MG_map_species(AW_root *aw_root, int db_nr) {
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
                                        GBDATA *gb_src_species_data, GBDATA *gb_dst_species_data,
                                        bool src_is_genome, bool dst_is_genome,
                                        GBDATA  *gb_src_species, const char *src_name,
                                        GB_HASH *source_organism_hash, GB_HASH *dest_species_hash,
                                        GB_HASH *error_suppressor)
{
    // copies one species from source to destination DB
    //
    // either 'gb_src_species' or 'src_name' (and 'gb_src_species_data') has to be given
    // 'source_organism_hash' may be NULL, otherwise it's used to search for source organisms (when merging from genome DB)
    // 'dest_species_hash' may be NULL, otherwise created species is stored there

    GB_ERROR error = 0;
    if (gb_src_species) {
        src_name = GBT_read_name(gb_src_species);
    }
    else {
        mg_assert(src_name);
        gb_src_species = GBT_find_species_rel_species_data(gb_src_species_data, src_name);
        if (!gb_src_species) {
            error = GBS_global_string("Could not find species '%s'", src_name);
        }
    }

    bool transfer_fields = false;
    if (src_is_genome) {
        if (dst_is_genome) { // genome -> genome
            if (GEN_is_pseudo_gene_species(gb_src_species)) {
                const char *origin        = GEN_origin_organism(gb_src_species);
                GBDATA     *dest_organism = dest_species_hash
                    ? (GBDATA*)GBS_read_hash(dest_species_hash, origin)
                    : GEN_find_organism(GLOBAL_gb_dst, origin);

                if (dest_organism) transfer_fields = true;
                else {
                    error = GBS_global_string("Destination DB does not contain '%s's origin-organism '%s'",
                                              src_name, origin);
                }
            }
            // else: merge organism ok
        }
        else { // genome -> non-genome
            if (GEN_is_pseudo_gene_species(gb_src_species)) transfer_fields = true;
            else {
                error = GBS_global_string("You can't merge organisms (%s) into a non-genome DB.\n"
                                          "Only pseudo-species are possible", src_name);
            }
        }
    }
    else {
        if (dst_is_genome) { // non-genome -> genome
            error = GBS_global_string("You can't merge non-genome species (%s) into a genome DB", src_name);
        }
        // else: non-genome -> non-genome ok
    }

    GBDATA *gb_dst_species = 0;
    if (!error) {
        gb_dst_species = dest_species_hash
            ? (GBDATA*)GBS_read_hash(dest_species_hash, src_name)
            : GBT_find_species_rel_species_data(gb_dst_species_data, src_name);

        if (gb_dst_species) error = GB_delete(gb_dst_species);
    }
    if (!error) {
        gb_dst_species = GB_create_container(gb_dst_species_data, "species");
        if (!gb_dst_species) error = GB_await_error();
    }
    if (!error) error = GB_copy(gb_dst_species, gb_src_species);
    if (!error && transfer_fields) {
        mg_assert(src_is_genome);
        error = MG_export_fields(aw_root, gb_src_species, gb_dst_species, error_suppressor, source_organism_hash);
    }
    if (!error) GB_write_flag(gb_dst_species, 1);
    if (!error) error = MG_transfer_all_alignments(&remap, gb_src_species, gb_dst_species);
    if (!error && dest_species_hash) GBS_write_hash(dest_species_hash, src_name, (long)gb_dst_species);

    return error;
}

static const char *get_reference_species_names(AW_root *awr) {
    return awr->awar(AWAR_REMAP_SPECIES_LIST)->read_char_pntr();
}
static bool adaption_enabled(AW_root *awr) {
    return awr->awar(AWAR_REMAP_ENABLE)->read_int() != 0;
}

static void MG_transfer_selected_species(AW_window *aww) {
    if (MG_copy_and_check_alignments() != 0) return;

    AW_root  *aw_root  = aww->get_root();
    char     *src_name = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
    GB_ERROR  error    = NULL;

    if (!src_name || !src_name[0]) {
        error = "Please select a species in the left list";
    }
    else {
        arb_progress progress("Transferring selected species");

        error             = GB_begin_transaction(GLOBAL_gb_src);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dst);

        if (!error) {
            MG_remaps rm(GLOBAL_gb_src, GLOBAL_gb_dst, adaption_enabled(aw_root), get_reference_species_names(aw_root));

            GBDATA *gb_src_species_data = GBT_get_species_data(GLOBAL_gb_src);
            GBDATA *gb_dst_species_data = GBT_get_species_data(GLOBAL_gb_dst);

            bool src_is_genome = GEN_is_genome_db(GLOBAL_gb_src, -1);
            bool dst_is_genome = GEN_is_genome_db(GLOBAL_gb_dst, -1);

            error = MG_transfer_one_species(aw_root, rm,
                                            gb_src_species_data, gb_dst_species_data,
                                            src_is_genome, dst_is_genome,
                                            NULL, src_name,
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
    if (MG_copy_and_check_alignments() != 0) return;

    GB_ERROR error = NULL;
    GB_begin_transaction(GLOBAL_gb_src);
    GB_begin_transaction(GLOBAL_gb_dst);

    bool src_is_genome = GEN_is_genome_db(GLOBAL_gb_src, -1);
    bool dst_is_genome = GEN_is_genome_db(GLOBAL_gb_dst, -1);

    GB_HASH *error_suppressor     = GBS_create_hash(50, GB_IGNORE_CASE);
    GB_HASH *dest_species_hash    = GBT_create_species_hash(GLOBAL_gb_dst);
    GB_HASH *source_organism_hash = src_is_genome ? GBT_create_organism_hash(GLOBAL_gb_src) : 0;

    AW_root   *aw_root = aww->get_root();
    MG_remaps  rm(GLOBAL_gb_src, GLOBAL_gb_dst, adaption_enabled(aw_root), get_reference_species_names(aw_root));

    GBDATA *gb_src_species;
    arb_progress progress("Transferring listed species", mg_count_queried(GLOBAL_gb_src));
    
    for (gb_src_species = GBT_first_species(GLOBAL_gb_src);
         gb_src_species;
         gb_src_species = GBT_next_species(gb_src_species))
    {
        if (IS_QUERIED_SPECIES(gb_src_species)) {
            GBDATA *gb_dst_species_data = GBT_get_species_data(GLOBAL_gb_dst);

            error = MG_transfer_one_species(aw_root, rm,
                                            NULL, gb_dst_species_data,
                                            src_is_genome, dst_is_genome,
                                            gb_src_species, NULL,
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
    if (MG_copy_and_check_alignments() != 0) return;

    char     *field  = aww->get_root()->awar(AWAR_FIELD_SRC)->read_string();
    long      append = aww->get_root()->awar(AWAR_APPEND)->read_int();
    GB_ERROR  error  = 0;

    if (strcmp(field, NO_FIELD_SELECTED) == 0) {
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

        for (GBDATA *gb_src_species = GBT_first_species(GLOBAL_gb_src);
             gb_src_species && !error;
             gb_src_species = GBT_next_species(gb_src_species))
        {
            if (IS_QUERIED_SPECIES(gb_src_species)) {
                const char *src_name       = GBT_read_name(gb_src_species);
                GBDATA     *gb_dst_species = GB_find_string(gb_dest_species_data, "name", src_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

                if (!gb_dst_species) {
                    gb_dst_species             = GB_create_container(gb_dest_species_data, "species");
                    if (!gb_dst_species) error = GB_await_error();
                    else error                 = GBT_write_string(gb_dst_species, "name", src_name);
                }
                else {
                    gb_dst_species = GB_get_father(gb_dst_species);
                }

                if (!error) {
                    GBDATA *gb_src_field = GB_search(gb_src_species, field, GB_FIND);
                    GBDATA *gb_dst_field = GB_search(gb_dst_species, field, GB_FIND);
                    bool    use_copy     = true;

                    if (gb_dst_field && gb_src_field) {
                        GB_TYPES src_type = GB_read_type(gb_src_field);
                        GB_TYPES dst_type = GB_read_type(gb_dst_field);

                        if ((src_type==dst_type) && (GB_DB != src_type)) {
                            if (append && src_type == GB_STRING) {
                                char *src_val = GB_read_string(gb_src_field);
                                char *dst_val = GB_read_string(gb_dst_field);

                                if (!src_val || !dst_val) error = GB_await_error();
                                else {
                                    if (!GBS_find_string(dst_val, src_val, 0)) {
                                        error = GB_write_string(gb_dst_field, GBS_global_string("%s %s", dst_val, src_val));
                                        if (!error) GB_write_flag(gb_dst_species, 1);
                                    }
                                }

                                free(src_val);
                                free(dst_val);
                            }
                            else { // not GB_STRING
                                error = GB_copy(gb_dst_field, gb_src_field);
                                if (!error) GB_write_flag(gb_dst_species, 1);
                                if (transfer_of_alignment && !error) {
                                    error = MG_transfer_all_alignments(&rm, gb_src_species, gb_dst_species);
                                }
                            }
                            use_copy = false;
                        }
                    }

                    if (use_copy) {
                        if (gb_dst_field) {
                            if (gb_src_field && !append) error = GB_delete(gb_dst_field);
                        }
                        if (gb_src_field && !error) {
                            GB_TYPES src_type        = GB_read_type(gb_src_field);
                            gb_dst_field             = GB_search(gb_dst_species, field, src_type);
                            if (!gb_dst_field) error = GB_await_error();
                            else     error           = GB_copy(gb_dst_field, gb_src_field);
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

static AW_window *MG_create_transfer_fields_window(AW_root *aw_root) {
    GB_transaction    ta(GLOBAL_gb_src);
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "MERGE_XFER_FIELD_OF_LISTED", "Transfer field of listed");
    aws->load_xfig("merge/mg_transfield.fig");

    aws->button_length(10);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_xfer_field_of_listed.hlp"));
    aws->create_button("HELP", "HELP");

    aws->at("append");
    aws->label("Append data?");
    aws->create_toggle(AWAR_APPEND);

    create_selection_list_on_itemfields(GLOBAL_gb_src, aws, AWAR_FIELD_SRC, true, SPECIES_get_selector(), FIELD_FILTER_NDS, SF_STANDARD, "scandb", 20, 10, "sel_field");

    aws->at("go");
    aws->callback(MG_transfer_fields_cb);
    aws->create_button("GO", "GO");

    return aws;
}

static void MG_transfer_single_field_cb(AW_window *aww) {
    if (MG_copy_and_check_alignments() != 0) return;

    AW_root  *aw_root = aww->get_root();
    char     *field   = aww->get_root()->awar(AWAR_FIELD_SRC)->read_string();
    GB_ERROR  error   = 0;

    if (strcmp(field, NO_FIELD_SELECTED) == 0) {
        error = "Please select a field to transfer";
    }
    else if (strcmp(field, "name") == 0) {
        error = "You are not allowed to transfer the 'name' field";
    }
    else {
        arb_progress progress("Cross copy field");
        error             = GB_begin_transaction(GLOBAL_gb_src);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dst);

        if (!error) {
            GBDATA *gb_src_species;
            GBDATA *gb_dst_species;
            {
                char *src_name = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
                char *dst_name = aw_root->awar(AWAR_SPECIES_DST)->read_string();

                gb_src_species = GBT_find_species(GLOBAL_gb_src, src_name);
                gb_dst_species = GBT_find_species(GLOBAL_gb_dst, dst_name);

                free(dst_name);
                free(src_name);
            }

            if (!gb_src_species) error = "Please select a species in left hitlist";
            if (!gb_dst_species) error = "Please select a species in right hitlist";

            if (!error) {
                GBDATA *gb_src_field = GB_search(gb_src_species, field, GB_FIND);
                if (!gb_src_field) error = GBS_global_string("Source species has no field '%s'", field);

                if (!error) {
                    GB_TYPES  src_type     = GB_read_type(gb_src_field);
                    GBDATA   *gb_dst_field = GB_search(gb_dst_species, field, GB_FIND);

                    if (gb_dst_field) {
                        int dst_type = GB_read_type(gb_dst_field);

                        if ((src_type==dst_type) && (GB_DB != src_type)) {
                            error = GB_copy(gb_dst_field, gb_src_field);
                        }
                        else { // remove dest. if type mismatch or container
                            error        = GB_delete(gb_dst_field);
                            gb_dst_field = 0; // trigger copy
                        }
                    }

                    if (!error && !gb_dst_field) { // destination missing or removed
                        gb_dst_field             = GB_search(gb_dst_species, field, src_type);
                        if (!gb_dst_field) error = GB_await_error();
                        else error               = GB_copy(gb_dst_field, gb_src_field);
                    }
                }
                if (!error) GB_write_flag(gb_dst_species, 1);
            }
        }
        if (!error) error = MG_transfer_fields_info(field);

        error = GB_end_transaction(GLOBAL_gb_src, error);
        error = GB_end_transaction(GLOBAL_gb_dst, error);
    }

    if (error) aw_message(error);

    free(field);
}

static AW_window *create_mg_transfer_single_field_window(AW_root *aw_root) {
    GB_transaction ta(GLOBAL_gb_src);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_XFER_SINGLE_FIELD", "Transfer field of selected");
    aws->load_xfig("merge/mg_transfield.fig");
    aws->button_length(10);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_xfer_field_of_sel.hlp"));
    aws->create_button("HELP", "HELP");

    create_selection_list_on_itemfields(GLOBAL_gb_src, aws, AWAR_FIELD_SRC, true, SPECIES_get_selector(), FIELD_FILTER_NDS, SF_STANDARD, "scandb", 20, 10, "sel_field");

    aws->at("go");
    aws->callback(MG_transfer_single_field_cb);
    aws->create_button("GO", "GO");

    return aws;
}

static void MG_merge_tagged_field_cb(AW_window *aww) {
    GB_transaction ta_merge(GLOBAL_gb_src);
    GB_ERROR       error = GB_begin_transaction(GLOBAL_gb_dst);

    if (!error) {
        AW_root *awr = aww->get_root();

        char *src_field = awr->awar(AWAR_FIELD_SRC)->read_string();
        char *dst_field = awr->awar(AWAR_FIELD_DST)->read_string();
        char *src_tag   = awr->awar(AWAR_TAG_SRC)->read_string();
        char *dst_tag   = awr->awar(AWAR_TAG_DST)->read_string();
        char *del_tag   = awr->awar(AWAR_TAG_DEL)->read_string();

        arb_progress progress("Merging tagged fields", mg_count_queried(GLOBAL_gb_src));

        GBDATA *gb_dest_species_data     = GBT_get_species_data(GLOBAL_gb_dst);
        if (!gb_dest_species_data) error = GB_await_error();
        else {
            for (GBDATA *gb_src_species = GBT_first_species(GLOBAL_gb_src);
                 gb_src_species && !error;
                 gb_src_species = GBT_next_species(gb_src_species))
            {
                if (IS_QUERIED_SPECIES(gb_src_species)) {
                    char *name       = GBT_read_string(gb_src_species, "name");
                    if (!name) error = GB_await_error();
                    else {
                        GBDATA *gb_dst_species     = GBT_find_or_create_species_rel_species_data(gb_dest_species_data, name);
                        if (!gb_dst_species) error = GB_await_error();
                        else {
                            char *src_val = GBT_readOrCreate_string(gb_src_species, src_field, "");
                            char *dst_val = GBT_readOrCreate_string(gb_dst_species, dst_field, "");

                            if (!src_val || !dst_val) error = GB_await_error();
                            else {
                                char *sum = GBS_merge_tagged_strings(src_val, src_tag, del_tag,
                                                                     dst_val, dst_tag, del_tag);

                                if (!sum) error = GB_await_error();
                                else error      = GBT_write_string(gb_dst_species, dst_field, sum);
                                free(sum);
                            }
                            free(dst_val);
                            free(src_val);
                        }
                    }
                    free(name);
                    progress.inc_and_check_user_abort(error);
                }
            }
        }

        free(del_tag);
        free(dst_tag);
        free(src_tag);
        free(dst_field);
        free(src_field);
    }
    GB_end_transaction_show_error(GLOBAL_gb_dst, error, aw_message);
}

static AW_window *create_mg_merge_tagged_fields_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    GB_transaction ta(GLOBAL_gb_src);

    aw_root->awar_string(AWAR_FIELD_SRC, "full_name");
    aw_root->awar_string(AWAR_FIELD_DST, "full_name");

    aw_root->awar_string(AWAR_TAG_SRC, "S");
    aw_root->awar_string(AWAR_TAG_DST, "D");

    aw_root->awar_string(AWAR_TAG_DEL, "S*");

    aws = new AW_window_simple;
    aws->init(aw_root, "MERGE_TAGGED_FIELD", "Merge tagged field");
    aws->load_xfig("merge/mg_mergetaggedfield.fig");
    aws->button_length(13);

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("go");
    aws->callback(MG_merge_tagged_field_cb);
    aws->create_button("GO", "GO");

    aws->at("help");
    aws->callback(makeHelpCallback("mergetaggedfield.hlp"));
    aws->create_button("HELP", "HELP");

    aws->at("tag1");    aws->create_input_field(AWAR_TAG_SRC, 5);
    aws->at("tag2");    aws->create_input_field(AWAR_TAG_DST, 5);

    aws->at("del1");    aws->create_input_field(AWAR_TAG_DEL, 5);

    create_selection_list_on_itemfields(GLOBAL_gb_src, aws, AWAR_FIELD_SRC, true, SPECIES_get_selector(), FIELD_FILTER_NDS, SF_STANDARD, "fields1", 20, 10, "source_field");
    create_selection_list_on_itemfields(GLOBAL_gb_dst, aws, AWAR_FIELD_DST, true, SPECIES_get_selector(), FIELD_FILTER_NDS, SF_STANDARD, "fields2", 20, 10, "target_field");

    return aws;
}

static GB_ERROR MG_equal_alignments(bool autoselect_equal_alignment_name) {
    //! Make the alignment names equal
    ConstStrArray S_alignment_names;
    ConstStrArray D_alignment_names;
    GBT_get_alignment_names(S_alignment_names, GLOBAL_gb_src);
    GBT_get_alignment_names(D_alignment_names, GLOBAL_gb_dst);

    GB_ERROR error = 0;
    if (S_alignment_names[0] == 0) {
        error =  GB_export_error("No source sequences found");
    }
    else {
        char       *src_type = GBT_get_alignment_type_string(GLOBAL_gb_src, S_alignment_names[0]);
        const char *dest     = 0;

        for (int d = D_alignment_names.size()-1; d>0; --d) {
            char *dst_type = GBT_get_alignment_type_string(GLOBAL_gb_dst, D_alignment_names[d]);
            if (strcmp(src_type, dst_type) != 0) D_alignment_names.remove(d--);
            free(dst_type);
        }

        int d = D_alignment_names.size();
        switch (d) {
            case 0:
                error = GB_export_errorf("Cannot find a target alignment with a type of '%s'\n"
                                         "You should create one first or select a different alignment type\n"
                                         "during sequence import", src_type);
                break;
            case 1:
                dest = D_alignment_names[0];
                break;

            default:
                if (autoselect_equal_alignment_name) {
                    for (int i = 0; i<d; ++i) {
                        if (ARB_stricmp(S_alignment_names[0], D_alignment_names[i]) == 0) {
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
        if (!error && dest && strcmp(S_alignment_names[0], dest) != 0) {
            error = GBT_rename_alignment(GLOBAL_gb_src, S_alignment_names[0], dest, 1, 1);
            if (error) {
                error = GBS_global_string("Failed to rename alignment '%s' to '%s' (Reason: %s)",
                                          S_alignment_names[0], dest, error);
            }
            else {
                GBT_add_new_changekey(GLOBAL_gb_src, GBS_global_string("%s/data", dest), GB_STRING);
            }
        }
        free(src_type);
    }

    return error;
}

GB_ERROR MERGE_sequences_simple(AW_root *awr) {
    //! Merge the sequences of two databases

    static char *s_name = 0;

    GB_HASH *D_species_hash = 0;

    GB_push_my_security(GLOBAL_gb_src);
    GB_push_my_security(GLOBAL_gb_dst);
    GB_begin_transaction(GLOBAL_gb_src);
    GB_begin_transaction(GLOBAL_gb_dst);

    GB_ERROR error = MG_equal_alignments(true);
    if (!error) {
        GBDATA *S_species_data = GBT_get_species_data(GLOBAL_gb_src);
        GBDATA *D_species_data = GBT_get_species_data(GLOBAL_gb_dst);

        freenull(s_name);

        {
            long S_species_count = GB_number_of_subentries(S_species_data);
            long D_species_count = GB_number_of_subentries(D_species_data);

            // create hash containing all species from gb_dst,
            // but sized to hold all species from both DBs:
            D_species_hash = GBT_create_species_hash_sized(GLOBAL_gb_dst, S_species_count+D_species_count);
        }

        bool overwriteall  = false;
        bool autorenameall = false;
        
        for (GBDATA *S_species = GB_entry(S_species_data, "species"); S_species; S_species = GB_nextEntry(S_species)) {
            GBDATA *S_name = GB_search(S_species, "name", GB_STRING);

            freeset(s_name, GB_read_string(S_name));

            int  count = 1;
            bool retry;
            do {
                retry = false;
                count++;

                GBDATA *D_species = (GBDATA*)GBS_read_hash(D_species_hash, s_name);
                if (D_species) {
                    if (overwriteall) {
                        error = GB_delete(D_species);
                    }
                    else if (autorenameall) {
                        GB_ERROR  dummy;
                        char     *newname = AWTC_create_numbered_suffix(D_species_hash, s_name, dummy);

                        mg_assert(newname);
                        freeset(s_name, newname);
                    }
                    else {
                        switch (aw_question("merge_existing_species",
                                            GBS_global_string("Warning: There is an ID conflict for species '%s'\n"
                                                              "  You may:\n"
                                                              "  - Overwrite existing species\n"
                                                              "  - Overwrite all species with ID conflicts\n"
                                                              "  - Not import species\n"
                                                              "  - Enter ID for imported species\n"
                                                              "  - Automatically create unique species IDs (append .NUM)\n"
                                                              "  - Abort everything", s_name),
                                            "overwrite, overwrite all, don't import, create ID, auto-create IDs, abort")) {
                            case 1:
                                overwriteall = true;
                            case 0:
                                GB_delete(D_species);
                                break;

                            case 2:
                                continue;

                            case 3: {
                                GB_ERROR  warning;          // duplicated species warning (does not apply here)
                                char     *autoname = AWTC_create_numbered_suffix(D_species_hash, s_name, warning);

                                if (!autoname) autoname = strdup(s_name);
                                freeset(s_name, aw_input("Species ID", "Enter new species ID", autoname));
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
                    error = GB_copy_with_protection(D_species, S_species, true);

                    if (!error) {
                        GB_write_flag(D_species, 1);          // mark species
                        GB_raise_user_flag(D_species, GB_USERFLAG_QUERY); // put in search&query hitlist
                        error = GBT_write_string(D_species, "name", s_name);
                    }
                }

                GBS_write_hash(D_species_hash, s_name, (long)D_species);
            }

            if (error) break;
        }
    }

    if (D_species_hash) GBS_free_hash(D_species_hash);

    if (!error) error = MG_transfer_fields_info();
    if (!error) awr->awar(AWAR_SPECIES_NAME)->write_string(s_name);

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
    GB_transaction ta(GLOBAL_gb_src);
    char   *species_name            = aw_root->awar(AWAR_SPECIES_SRC)->read_string();
    GBDATA *gb_species              = 0;
    if (species_name[0]) gb_species = GBT_find_species(GLOBAL_gb_src, species_name);
    free(species_name);
    return gb_species;
}
static GBDATA *mg_get_selected_species2(GBDATA * /* gb_main */, AW_root *aw_root) {
    GB_transaction ta(GLOBAL_gb_dst);
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

AW_window *MG_create_merge_species_window(AW_root *awr, bool dst_is_new) {
    GB_ERROR error = MG_expect_renamed();
    if (error) {
        aw_message(error);
        return NULL; // deny to open window before user has renamed species
    }

    awr->awar_string(AWAR_REMAP_SPECIES_LIST, "ecoli", GLOBAL_gb_dst);
    awr->awar_int(AWAR_REMAP_ENABLE, 0, GLOBAL_gb_dst);

    AW_window_simple_menu *aws = new AW_window_simple_menu;
    aws->init(awr, "MERGE_TRANSFER_SPECIES", "TRANSFER SPECIES");
    aws->load_xfig("merge/species.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_species.hlp"));
    aws->create_button("HELP", "HELP", "H");

    mg_initialize_species_selectors();

    {
        QUERY::query_spec awtqs(MG_species_selector[0]);
        aws->create_menu("Source-DB", "S");

        awtqs.gb_main                = GLOBAL_gb_src;
        awtqs.gb_ref                 = GLOBAL_gb_dst;
        awtqs.expect_hit_in_ref_list = false;
        awtqs.species_name           = AWAR_SPECIES_SRC;
        awtqs.tree_name              = 0;               // no selected tree here -> can't use tree related ACI commands without specifying a tree
        awtqs.select_bit             = GB_USERFLAG_QUERY;
        awtqs.ere_pos_fig            = "ere1";
        awtqs.by_pos_fig             = "by1";
        awtqs.qbox_pos_fig           = "qbox1";
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
        aws->get_root()->awar(AWAR_SPECIES_SRC)->add_callback(makeRootCallback(MG_map_species, 1));
    }
    {
        QUERY::query_spec awtqs(MG_species_selector[1]);
        aws->create_menu("Target-DB", "T");

        awtqs.gb_main                = GLOBAL_gb_dst;
        awtqs.gb_ref                 = GLOBAL_gb_src;
        awtqs.expect_hit_in_ref_list = true;
        awtqs.species_name           = AWAR_SPECIES_DST;
        awtqs.select_bit             = GB_USERFLAG_QUERY;
        awtqs.ere_pos_fig            = "ere2";
        awtqs.by_pos_fig             = "by2";
        awtqs.qbox_pos_fig           = "qbox2";
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
        aws->get_root()->awar(AWAR_SPECIES_DST)->add_callback(makeRootCallback(MG_map_species, 2));
    }

    // big transfer buttons:
    aws->button_length(13);
    {
        aws->shadow_width(3);

        aws->at("transsspec");
        aws->callback(MG_transfer_selected_species);
        aws->create_button("TRANSFER_SELECTED_DELETE_DUPLICATED",
                           "Transfer\nselected\nspecies\n\nDelete\nduplicate\nin target DB", "T");

        aws->at("translspec");
        aws->callback(MG_transfer_species_list);
        aws->create_button("TRANSFER_LISTED_DELETE_DUPLI",
                           "Transfer\nlisted\nspecies\n\nDelete\nduplicates\nin target DB", "T");

        aws->at("transfield");
        aws->help_text("mg_xfer_field_of_listed.hlp");
        aws->callback(MG_create_transfer_fields_window);
        aws->create_button("TRANSFER_FIELD_OF_LISTED_DELETE_DUPLI",
                           "Transfer\nfield\nof listed\nspecies\n\nDelete\nduplicates\nin target DB", "T");

        aws->shadow_width(1);
    }

    // adapt alignments
    {
        if (dst_is_new) {
            aws->sens_mask(AWM_DISABLED); // if dest DB is new = > adaption impossible
            awr->awar(AWAR_REMAP_ENABLE)->write_int(0); // disable adaption
        }

        aws->button_length(7);
        aws->at("adapt");
        aws->create_toggle(AWAR_REMAP_ENABLE);

        aws->at("reference");
        aws->create_text_field(AWAR_REMAP_SPECIES_LIST);

        aws->at("pres_sel");
        aws->callback(MG_create_preserves_selection_window);
        aws->create_button("SELECT", "SELECT", "S");

        aws->sens_mask(AWM_ALL);
    }

    // top icon
    aws->button_length(0);
    aws->at("icon");
    aws->callback(makeHelpCallback("mg_species.hlp"));
    aws->create_button("HELP_MERGE", "#merge/icon.xpm");

    aws->create_menu("Source->Target", "g");
    aws->insert_menu_topic("compare_field_of_listed",            "Compare a field of listed species ...",         "C", "checkfield.hlp",           AWM_ALL, create_mg_check_fields_window);
    aws->insert_menu_topic("move_field_of_selected",             "Transfer single field of selected species ...", "M", "mg_xfer_field_of_sel.hlp", AWM_ALL, create_mg_transfer_single_field_window);
    aws->insert_menu_topic("merge_field_of_listed_to_new_field", "Merge tagged field ...",                        "D", "mergetaggedfield.hlp",     AWM_ALL, create_mg_merge_tagged_fields_window);
    aws->sep______________();
    aws->insert_menu_topic("def_gene_species_field_xfer", "Define field transfer for gene-species", "g", "gene_species_field_transfer.hlp", AWM_ALL, MG_gene_species_create_field_transfer_def_window);


    return aws;
}

