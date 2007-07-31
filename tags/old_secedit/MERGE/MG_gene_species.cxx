//  ==================================================================== //
//                                                                       //
//    File      : MG_gene_species.cxx                                    //
//    Purpose   : Transfer fields from organism and gene when            //
//                tranferring gene species                               //
//    Time-stamp: <Wed Feb/21/2007 13:26 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2002             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <string.h>
#include <stdlib.h>

#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_config_manager.hxx>

#include <arbdbt.h>

#include <adGene.h>
#include <inline.h>

#include "merge.hxx"

using namespace std;

// for input :
#define AWAR_MERGE_GENE_SPECIES_BASE_TMP "tmp/" AWAR_MERGE_GENE_SPECIES_BASE

#define AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD AWAR_MERGE_GENE_SPECIES_BASE_TMP "current"
#define AWAR_MERGE_GENE_SPECIES_DEST          AWAR_MERGE_GENE_SPECIES_BASE_TMP "dest"

#define AWAR_MERGE_GENE_SPECIES_SOURCE AWAR_MERGE_GENE_SPECIES_BASE_TMP "source"
#define AWAR_MERGE_GENE_SPECIES_METHOD AWAR_MERGE_GENE_SPECIES_BASE_TMP "method"
#define AWAR_MERGE_GENE_SPECIES_ACI    AWAR_MERGE_GENE_SPECIES_BASE_TMP "aci"

#define AWAR_MERGE_GENE_SPECIES_EXAMPLE     AWAR_MERGE_GENE_SPECIES_BASE_TMP "example"
#define AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE AWAR_MERGE_GENE_SPECIES_BASE_TMP "save" // only used to save/load config

// saved awars :
#define AWAR_MERGE_GENE_SPECIES_CREATE_FIELDS AWAR_MERGE_GENE_SPECIES_BASE "activated"
#define AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS   AWAR_MERGE_GENE_SPECIES_BASE "field_defs"

enum CreationMethod  {
    MG_CREATE_COPY_ORGANISM,
    MG_CREATE_COPY_GENE,
    MG_CREATE_USING_ACI_ONLY
};

static AW_default MG_props = 0; // pointer current applications properties database

// --------------------------------------------------------------------------------
//      void MG_create_gene_species_awars(AW_root *aw_root, AW_default aw_def)
// --------------------------------------------------------------------------------
void MG_create_gene_species_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_MERGE_GENE_SPECIES_METHOD, 0, aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_ACI, "", aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD, "", aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_DEST, "", aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_EXAMPLE, "", aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_SOURCE, "", aw_def);

    aw_root->awar_int(AWAR_MERGE_GENE_SPECIES_CREATE_FIELDS, 0, aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS, ";", aw_def);
    aw_root->awar_string(AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE, "", aw_def);

    MG_props = aw_def;
}

#define BUFSIZE 100

inline char *strcpydest(char *dest, const char *src) {
    // like strcpy, but returns pointer to zero terminator
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
    return dest+i;
}

inline const char *field_awar(const char *field_name, const char *subfield) {
    static char        buffer[BUFSIZE];
    static const char *prefix    = AWAR_MERGE_GENE_SPECIES_BASE"def_";
    static int         prefixlen = strlen(prefix);

    char *end = strcpydest(strcpydest(buffer, AWAR_MERGE_GENE_SPECIES_BASE"def_"), field_name);
    *end++ = '/';
    end       = strcpydest(end, subfield);

    mg_assert((end-buffer)<BUFSIZE);

    return buffer;
}

inline const char *current_field_awar(AW_root *aw_root, const char *subfield) {
    static char *cur_field = 0;

    if (cur_field) { free(cur_field); cur_field = 0; }
    cur_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->read_string();

    if (cur_field[0]) return field_awar(cur_field, subfield);
    return 0; // no field definition selected
}

// -------------------------------------------------------------------------------------
//      static void create_awars_for_field(AW_root *aw_root, const char *cur_field)
// -------------------------------------------------------------------------------------
// Note : MG_current_field_def_changed_cb also creates these awars!

static void create_awars_for_field(AW_root *aw_root, const char *cur_field) {
    aw_root->awar_string(field_awar(cur_field, "source"), cur_field, MG_props);
    aw_root->awar_int(field_awar(cur_field, "method"), 1, MG_props);
    aw_root->awar_string(field_awar(cur_field, "aci"), "", MG_props);
}

static char *MG_create_field_content(GBDATA *gb_species, int method, const char *origins_field, const char *aci, GB_ERROR& error, GB_HASH *organism_hash) {
    // does not write to database (only creates the content)
    mg_assert(GEN_is_pseudo_gene_species(gb_species));

    char   *result    = 0;
    GBDATA *gb_origin = 0;

    switch (method) {
        case MG_CREATE_COPY_ORGANISM:
            gb_origin = GEN_find_origin_organism(gb_species, organism_hash);
            break;
        case MG_CREATE_COPY_GENE:
            gb_origin = GEN_find_origin_gene(gb_species, organism_hash);
            break;
        case MG_CREATE_USING_ACI_ONLY:
            break;
    }

    if (gb_origin) {            // read source field
        if (origins_field[0]) {
            GBDATA *gb_field = GB_find(gb_origin, origins_field, 0, down_level);
            if (!gb_field) {
                error = GBS_global_string("Field not found: '%s'", origins_field);
            }
            else {
                result = GB_read_as_string(gb_field);
            }
        }
        else {
            error = "Specify a 'Source field'";
        }
    }

    if (!error) {
        char *aci_result = 0;

        if (method == MG_CREATE_USING_ACI_ONLY) {
            mg_assert(!result);
            aci_result             = GB_command_interpreter(gb_merge, "", aci, gb_species, 0);
            if (!aci_result) error = GB_get_error();
        }
        else {
            if (aci && aci[0]) {
                aci_result = GB_command_interpreter(gb_merge, result ? result : "", aci, gb_origin, 0);
                if (!aci_result) error = GB_get_error();
            }
        }

        if (aci_result) {
            free(result);
            result = aci_result;
        }
    }

    if (error) {
        free(result);
        result = 0;
    }

    return result;
}

GB_ERROR MG_export_fields(AW_root *aw_root, GBDATA *gb_source, GBDATA *gb_dest, GB_HASH *error_suppressor, GB_HASH *source_organism_hash) {
    GB_ERROR error         = 0;
    int      export_fields = aw_root->awar(AWAR_MERGE_GENE_SPECIES_CREATE_FIELDS)->read_int();

    if (export_fields) { // should fields be exported ?
        mg_assert(GEN_is_pseudo_gene_species(gb_source));
        
        char *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();
        char *start                = existing_definitions+1;

        mg_assert(existing_definitions[0] == ';');

        while (!error && start[0]) {      // parse existing definitions and add them to selection list
            char *end     = strchr(start, ';');
            if (!end) end = strchr(start, 0);
            int   len     = end-start;
            if (len<1) break;

            mg_assert(end[0] == ';');
            end[0] = 0;

            // export one field (start contains destination field name)
            {
                create_awars_for_field(aw_root, start);
                int   method = aw_root->awar(field_awar(start, "method"))->read_int();
                char *source = aw_root->awar(field_awar(start, "source"))->read_string();
                char *aci    = aw_root->awar(field_awar(start, "aci"))->read_string();

                char *result = MG_create_field_content(gb_source, method, source, aci, error, source_organism_hash);
                mg_assert(result || error);
                if (result) {
                    GBDATA *gb_export = GB_search(gb_dest, start, GB_STRING);
                    error             = GB_write_string(gb_export, result);

                    free(result);
                }
                else {
                    long error_seen = GBS_read_hash(error_suppressor, error);
#define MAX_EQUAL_WARNINGS 10
                    if (error_seen >= MAX_EQUAL_WARNINGS) {
                        if (error_seen == MAX_EQUAL_WARNINGS) {
                            aw_message(GBS_global_string("More than %i warnings about '%s' (suppressing)", MAX_EQUAL_WARNINGS, error));
                        }
                    }
                    else {
                        GBDATA     *gb_name = GB_find(gb_source, "name", 0, down_level);
                        const char *name    = gb_name ? GB_read_char_pntr(gb_name) : "<unknown species>";

                        aw_message(GBS_global_string("'%s' when exporting %s (continuing)", error, name));
                    }
                    GBS_incr_hash(error_suppressor, error);
                    error = 0;
                }

                free(aci);
                free(source);
            }

            start = end+1;
        }

        free(existing_definitions);
    }

    return 0;
}

static char *MG_create_current_field_content(AW_root *aw_root, GBDATA *gb_species, GB_ERROR& error) {
    int   method        = aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->read_int();
    char *origins_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_SOURCE)->read_string();
    char *aci           = aw_root->awar(AWAR_MERGE_GENE_SPECIES_ACI)->read_string();

    char *result = MG_create_field_content(gb_species, method, origins_field, aci, error, 0);

    free(aci);
    free(origins_field);

    return result;
}

// ---------------------------------------------------------
//      static void MG_update_example(AW_root *aw_root)
// ---------------------------------------------------------
static void MG_update_example(AW_root *aw_root) {
    char     *result       = 0;
    GB_ERROR  error        = 0;
    char     *curr_species = aw_root->awar(MG_left_AWAR_SPECIES_NAME())->read_string();

    if (!curr_species || !curr_species[0]) {
        error = "No species selected.";
    }
    else {
        GB_transaction  dummy(gb_merge);
        GBDATA         *gb_species = GBT_find_species(gb_merge, curr_species);

        if (!gb_species)                                    error = GB_export_error("No such species: '%s'", curr_species);
        else if (!GEN_is_pseudo_gene_species(gb_species))   error = "Selected species is no gene-species";
        else {
            result = MG_create_current_field_content(aw_root, gb_species, error);
        }
    }

    if (!error && !result) error = "no result";
    if (error) {
        free(result);
        result = GBS_global_string_copy("<%s>", error);
    }

    aw_root->awar(AWAR_MERGE_GENE_SPECIES_EXAMPLE)->write_string(result);

    free(result);
}
// -----------------------------------------------------------------------
//      static void check_and_correct_current_field(char*& cur_field)
// -----------------------------------------------------------------------
static void check_and_correct_current_field(char*& cur_field) {
    if (ARB_stricmp(cur_field, "name") == 0 || ARB_stricmp(cur_field, "acc") == 0) {
        aw_message("rules writing to 'name' or 'acc' are not allowed.");

        char *new_cur_field = new char[strlen(cur_field)+3];
        sprintf(new_cur_field, "%s_2", cur_field);
        free(cur_field);
        cur_field           = new_cur_field;
    }
}


static bool allow_callbacks = true;

// -----------------------------------------------------------------------
//      static void MG_current_field_def_changed_cb(AW_root *aw_root)
// -----------------------------------------------------------------------
static void MG_current_field_def_changed_cb(AW_root *aw_root) {
    if (allow_callbacks) {
        allow_callbacks = false;

        char *cur_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->read_string();
        check_and_correct_current_field(cur_field);

        aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->write_string(cur_field);
        aw_root->awar(AWAR_MERGE_GENE_SPECIES_DEST)->write_string(cur_field);

        if (cur_field[0]) {
            const char *awar_name = field_awar(cur_field, "source");

            // read stored source field (if undef default to new value of destination field)
            char *source_field = aw_root->awar_string(awar_name, cur_field, MG_props)->read_string();
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_SOURCE)->write_string(source_field);
            free(source_field);

            // read stored method (if undef then default to currently visible method)
            awar_name      = field_awar(cur_field, "method");
            int def_method = aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->read_int();
            int method     = aw_root->awar_int(awar_name, def_method, MG_props)->read_int();
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->write_int(method);

            // read stored aci (if undef then default to currently visible aci )
            awar_name      = field_awar(cur_field, "aci");
            char *curr_aci = aw_root->awar(AWAR_MERGE_GENE_SPECIES_ACI)->read_string();
            char *aci      = aw_root->awar_string(awar_name, curr_aci, MG_props)->read_string();
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_ACI)->write_string(aci);
            free(aci);
            free(curr_aci);

            MG_update_example(aw_root);
        }

        free(cur_field);

        allow_callbacks = true;
    }
}

// ------------------------------------------------------------------
//      static void MG_source_field_changed_cb(AW_root *aw_root)
// ------------------------------------------------------------------
static void MG_source_field_changed_cb(AW_root *aw_root) {
    if (allow_callbacks) {
        const char *awar_name = current_field_awar(aw_root, "source");
        if (awar_name) { // if a rule is selected
            char *source_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_SOURCE)->read_string();
            int   method       = aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->read_int();

            if (source_field[0] && method == MG_CREATE_USING_ACI_ONLY) {
                aw_message("Source field is not used with this method");
                source_field[0] = 0;
            }

            aw_root->awar(awar_name)->write_string(source_field);
            free(source_field);

            MG_update_example(aw_root);
        }
    }
}
// ----------------------------------------------------------------
//      static void MG_dest_field_changed_cb(AW_root *aw_root)
// ----------------------------------------------------------------
static void MG_dest_field_changed_cb(AW_root *aw_root) {
    if (allow_callbacks) {
        // if this is changed -> a new definition will be generated
        char *dest_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_DEST)->read_string();
        check_and_correct_current_field(dest_field);

        const char *search               = GBS_global_string(";%s;", dest_field);
        char       *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();

        mg_assert(existing_definitions[0] == ';');

        if (strstr(existing_definitions, search) == 0) { // not found -> create a new definition
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)
                ->write_string(GBS_global_string("%s%s;", existing_definitions, dest_field));

        }
        aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->write_string(dest_field);

        free(existing_definitions);
        free(dest_field);

        MG_update_example(aw_root);
    }
}
// ------------------------------------------------------------
//      static void MG_method_changed_cb(AW_root *aw_root)
// ------------------------------------------------------------
static void MG_method_changed_cb(AW_root *aw_root) {
    if (allow_callbacks) {
        const char *awar_name = current_field_awar(aw_root, "method");

        if (awar_name) {
            int method = aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->read_int();
            aw_root->awar(awar_name)->write_int(method);
            if (method == MG_CREATE_USING_ACI_ONLY) {
                aw_root->awar(AWAR_MERGE_GENE_SPECIES_SOURCE)->write_string(""); // clear source field
            }
            MG_update_example(aw_root);
        }
    }
}
// ------------------------------------------------------------------
//      static void MG_delete_selected_field_def(AW_window *aww)
// ------------------------------------------------------------------
static void MG_delete_selected_field_def(AW_window *aww) {
    AW_root *aw_root   = aww->get_root();
    char    *cur_field = aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->read_string();

    if (cur_field[0]) {
        char       *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();
        const char *search               = GBS_global_string(";%s;", cur_field);

        mg_assert(existing_definitions[0] == ';');

        char *found = strstr(existing_definitions, search);
        mg_assert(found);
        if (found) {
            strcpy(found, found+strlen(cur_field)+1); // remove that config
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->write_string(existing_definitions);
            aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->write_string("");
        }

        free(existing_definitions);
    }
    else {
        aw_message("No field selected.");
    }

    free(cur_field);
}
// ---------------------------------------------------------
//      static void MG_aci_changed_cb(AW_root *aw_root)
// ---------------------------------------------------------
static void MG_aci_changed_cb(AW_root *aw_root) {
    if (allow_callbacks) {
        const char *awar_name = current_field_awar(aw_root, "aci");
        if (awar_name) { // if a rule is selected
            char       *aci       = aw_root->awar(AWAR_MERGE_GENE_SPECIES_ACI)->read_string();
            aw_root->awar(awar_name)->write_string(aci);
            free(aci);
            MG_update_example(aw_root);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------
//      static void MG_update_selection_list_on_field_transfers(AW_root *aw_root, AW_CL cl_aww, AW_CL cl_sel_id)
// ------------------------------------------------------------------------------------------------------------------
static void MG_update_selection_list_on_field_transfers(AW_root *aw_root, AW_CL cl_aww, AW_CL cl_sel_id) {
    AW_window         *aww     = (AW_window*)cl_aww;
    AW_selection_list *sel_id  = (AW_selection_list*)cl_sel_id;
//     AW_root           *aw_root = aww->get_root();

    aww->clear_selection_list(sel_id);

    {
        char *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();
        char *start                = existing_definitions+1;

        mg_assert(existing_definitions[0] == ';');

        while (start[0]) {      // parse existing definitions and add them to selection list
            char *end     = strchr(start, ';');
            if (!end) end = strchr(start, 0);
            int   len     = end-start;
            if (len<1) break;

            mg_assert(end[0] == ';');
            end[0] = 0;
            aww->insert_selection(sel_id, start, start);
            start = end+1;
        }

        free(existing_definitions);
    }

    aww->insert_default_selection(sel_id, "", "");
    aww->update_selection_list(sel_id);
}

static void init_gene_species_xfer_fields_subconfig(AWT_config_definition& cdef, char *existing_definitions) {
    AW_root *aw_root = cdef.get_root();

    char *start = existing_definitions+1;
    mg_assert(existing_definitions[0] == ';');

    for (int count = 0; start[0]; ++count) { // parse existing definitions and add them to config
        char *end     = strchr(start, ';');
        if (!end) end = strchr(start, 0);
        int   len     = end-start;
        if (len<1) break;

        mg_assert(end[0] == ';');
        end[0] = 0;

        // add config :
#define add_config(s, id) cdef.add(field_awar(s, id), id, count)

        create_awars_for_field(aw_root, start);

        add_config(start, "source");
        add_config(start, "method");
        add_config(start, "aci");

#undef add_config

        end[0] = ';';

        start = end+1;
    }
}
static void init_gene_species_xfer_fields_config(AWT_config_definition& cdef) {
    cdef.add(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS, "fields");
    cdef.add(AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE, "defs");
}
static char *store_gene_species_xfer_fields(AW_window *aww, AW_CL , AW_CL ) {
    AW_root *aw_root = aww->get_root();
    {
        char                  *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();
        AWT_config_definition  cdef(aw_root);

        init_gene_species_xfer_fields_subconfig(cdef, existing_definitions);

        char *sub_config = cdef.read(); // save single configs to sub_config
        aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE)->write_string(sub_config);

        free(sub_config);
        free(existing_definitions);
    }

    // save AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE and AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS
    AWT_config_definition sub_cdef(aw_root);
    init_gene_species_xfer_fields_config(sub_cdef);
    return sub_cdef.read();
}
static void load_gene_species_xfer_fields(AW_window *aww, const char *stored_string, AW_CL cl_sel_id, AW_CL ) {
    AW_root *aw_root = aww->get_root();

    aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->write_string(""); // de-select

    // Load 'AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS' and 'AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE'
    {
        AWT_config_definition sub_cdef(aw_root);    
        init_gene_species_xfer_fields_config(sub_cdef);
        sub_cdef.write(stored_string);
    }

    {
        char *existing_definitions = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->read_string();
        char *sub_config           = aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_SAVE)->read_string();

        AWT_config_definition cdef(aw_root);
        init_gene_species_xfer_fields_subconfig(cdef, existing_definitions);
        cdef.write(sub_config);

        free(sub_config);
        free(existing_definitions);
    }

    // refresh mask :
    AW_selection_list *sel_id = (AW_selection_list*)cl_sel_id;
    MG_update_selection_list_on_field_transfers(aw_root, (AW_CL)aww, (AW_CL)sel_id);
}

// ---------------------------------------------------------------------------------------
//      AW_window *MG_gene_species_create_field_transfer_def_window(AW_root *aw_root)
// ---------------------------------------------------------------------------------------
AW_window *MG_gene_species_create_field_transfer_def_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    aws = new AW_window_simple;
    aws->init(aw_root, "DEFINE_GENE_SPECIES_FIELDS", "DEFINE FIELDS EXPORTED WITH GENE SPECIES");
    aws->load_xfig("merge/mg_def_gene_species_fields.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"gene_species_field_transfer.hlp");
    aws->create_button("HELP","HELP");

    aws->at("active");
    aws->create_toggle(AWAR_MERGE_GENE_SPECIES_CREATE_FIELDS);

    aws->at("src");
    aws->create_input_field(AWAR_MERGE_GENE_SPECIES_SOURCE);

    aws->at("delete");
    aws->callback(MG_delete_selected_field_def);
    aws->create_button("DELETE", "DELETE");

    aws->at("sel_method");
    aws->create_toggle_field(AWAR_MERGE_GENE_SPECIES_METHOD, 0, "");
    aws->insert_toggle("Copy from organism", "O", MG_CREATE_COPY_ORGANISM);
    aws->insert_toggle("Copy from gene", "G", MG_CREATE_COPY_GENE);
    aws->insert_toggle("Only use ACI below", "A", MG_CREATE_USING_ACI_ONLY);
    aws->update_toggle_field();

    aws->at("aci");
    aws->create_input_field(AWAR_MERGE_GENE_SPECIES_ACI);

    aws->at("dest");
    aws->create_input_field(AWAR_MERGE_GENE_SPECIES_DEST);

    aws->at("example");
    aws->create_text_field(AWAR_MERGE_GENE_SPECIES_EXAMPLE, 40, 3);

    aws->at("fields");
    AW_selection_list *sel_id = aws->create_selection_list(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD, 0, "", 10, 30);

    MG_update_selection_list_on_field_transfers(aw_root, (AW_CL)aws, (AW_CL)sel_id);

    aws->at("save");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "gene_species_field_xfer",
                              store_gene_species_xfer_fields, load_gene_species_xfer_fields, (AW_CL)sel_id, 0);

    // add callbacks for this window :
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_FIELDS_DEFS)->add_callback(MG_update_selection_list_on_field_transfers, (AW_CL)aws, (AW_CL)sel_id);
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_CURRENT_FIELD)->add_callback(MG_current_field_def_changed_cb);
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_SOURCE)->add_callback(MG_source_field_changed_cb);
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_DEST)->add_callback(MG_dest_field_changed_cb);
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_METHOD)->add_callback(MG_method_changed_cb);
    aw_root->awar(AWAR_MERGE_GENE_SPECIES_ACI)->add_callback(MG_aci_changed_cb);
    aw_root->awar(MG_left_AWAR_SPECIES_NAME())->add_callback(MG_update_example);

    return aws;
}





