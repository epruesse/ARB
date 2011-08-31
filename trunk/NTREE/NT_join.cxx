// =============================================================== //
//                                                                 //
//   File      : NT_join.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <item_sel_list.h>
#include <awt_sel_boxes.hxx>
#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <arbdbt.h>

#define nt_assert(bed) arb_assert(bed)

extern GBDATA *GLOBAL_gb_main;

#define AWAR_SPECIES_JOIN_FIELD "/tmp/NT/species_join/field"
#define AWAR_SPECIES_JOIN_SEP   "/tmp/NT/species_join/separator"
#define AWAR_SPECIES_JOIN_SEP2  "/tmp/NT/species_join/separator_sequences"

GB_ERROR nt_species_join(GBDATA *dest, GBDATA *source, int deep, char *sep, char *sep2) {
    GB_TYPES dtype = GB_read_type(dest);
    GB_TYPES stype = GB_read_type(source);
    if (dtype != stype) return 0;

    GB_ERROR error = 0;

    switch (dtype) {
        case GB_DB: {
            GBDATA     *gb_source_field;
            const char *source_field;
            GBDATA     *gb_dest_field;

            for (gb_source_field = GB_child(source);
                 !error && gb_source_field;
                 gb_source_field = GB_nextChild(gb_source_field))
            {
                source_field = GB_read_key_pntr(gb_source_field);
                if (!strcmp(source_field, "name")) continue;

                gb_dest_field = GB_entry(dest, source_field);
                if (gb_dest_field) { // if destination exists -> recurse
                    error = nt_species_join(gb_dest_field, gb_source_field, 0, sep, sep2);
                }
                else {
                    GB_TYPES type = GB_read_type(gb_source_field);

                    if (type == GB_DB) {
                        GBDATA *gb_dest_container     = GB_create_container(dest, source_field);
                        if (!gb_dest_container) error = GB_await_error();
                        else    error                 = nt_species_join(gb_dest_container, gb_source_field, 0, sep, sep2);
                    }
                    else {
                        gb_dest_field             = GB_create(dest, source_field, GB_read_type(gb_source_field));
                        if (!gb_dest_field) error = GB_await_error();
                        else error                = GB_copy(gb_dest_field, gb_source_field);
                    }
                }
            }
            break;
        }

        case GB_STRING: {
            char *sf = GB_read_string(source);
            char *df = GB_read_string(dest);
            if (!strcmp(sf, df)) {
                free(sf);
                free(df);
                break;
            }
            char *s = sep;
            const char *spacers = " ";
            if (deep) {
                s = sep2;
                spacers = ".- ";
            }
            int i;
            // remove trailing spacers;
            for (i = strlen(df)-1; i>=0; i--) {
                if (strchr(spacers, df[i])) df[i] = 0;
            }
            // remove leading spacers
            int end = strlen(sf);
            for (i=0; i<end; i++) {
                if (!strchr(spacers, sf[i])) break;
            }
            char *str = new char [strlen(sf) + strlen(df) + strlen(s) + 1];
            sprintf(str, "%s%s%s", df, s, sf+i);
            error = GB_write_string(dest, str);
            delete [] str;
            free(sf); free(df);
            break;
        }
        default: {
            break;
        }
    }
    return error;
}

void species_rename_join(AW_window *aww) {
    char     *field = aww->get_root()->awar(AWAR_SPECIES_JOIN_FIELD)->read_string();
    char     *sep   = aww->get_root()->awar(AWAR_SPECIES_JOIN_SEP)->read_string();
    char     *sep2  = aww->get_root()->awar(AWAR_SPECIES_JOIN_SEP2)->read_string();
    GB_ERROR  error = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GB_HASH *hash = GBS_create_hash(1000, GB_MIND_CASE);
        long     maxs = GBT_count_marked_species(GLOBAL_gb_main);
        
        arb_progress progress("Joining species", maxs);

        GBDATA *gb_next = 0;
        for (GBDATA *gb_species = GBT_first_marked_species(GLOBAL_gb_main);
             gb_species && !error;
             gb_species = gb_next)
        {
            gb_next = GBT_next_marked_species(gb_species);

            GBDATA *gb_field = GB_entry(gb_species, field);
            if (gb_field) {
                const char *field_value = GB_read_char_pntr(gb_field);
                GBDATA     *gb_old      = (GBDATA *)GBS_read_hash(hash, field_value);

                if (!gb_old) {
                    GBS_write_hash(hash, field_value, (long)gb_species);
                }
                else {
                    error = nt_species_join(gb_old, gb_species, 0, sep, sep2);
                    if (!error) error = GB_delete(gb_species);
                }
            }
            progress.inc_and_check_user_abort(error);
        }

        GBS_free_hash(hash);
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);

    free(sep2);
    free(sep);
    free(field);
}


AW_window *create_species_join_window(AW_root *root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    root->awar_string(AWAR_SPECIES_JOIN_FIELD, "name", AW_ROOT_DEFAULT);
    root->awar_string(AWAR_SPECIES_JOIN_SEP, "#", AW_ROOT_DEFAULT);
    root->awar_string(AWAR_SPECIES_JOIN_SEP2, "#", AW_ROOT_DEFAULT);

    aws = new AW_window_simple;
    aws->init(root, "JOIN_SPECIES", "JOIN SPECIES");
    aws->load_xfig("join_species.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(AW_POPUP_HELP, (AW_CL)"species_join.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("sym");
    aws->create_input_field(AWAR_SPECIES_JOIN_SEP);

    aws->at("symseq");
    aws->create_input_field(AWAR_SPECIES_JOIN_SEP2);

    aws->at("go");
    aws->callback(species_rename_join);
    aws->help_text("species_join.hlp");
    aws->create_button("GO", "GO", "G");

    create_selection_list_on_itemfields(GLOBAL_gb_main,
                                            aws, AWAR_SPECIES_JOIN_FIELD,
                                            FIELD_FILTER_NDS,
                                            "field", 0, SPECIES_get_selector(), 20, 10);

    return (AW_window *)aws;
}
