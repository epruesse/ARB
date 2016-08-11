// ================================================================ //
//                                                                  //
//   File      : AWTC_submission.cxx                                //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_edit.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>

#include <arbdbt.h>
#include <arb_strbuf.h>
#include <arb_strarray.h>

#define awtc_assert(bed) arb_assert(bed)

#define AWAR_SUBMIT_PARSER "tmp/submission/parser"
#define AWAR_SUBMIT_PARSED "tmp/submission/parsed"

#define AWAR_SUBMIT_SOURCE "submission/source"
#define AWAR_SUBMIT_FILE   "submission/file"
#define AWAR_SUBMIT_PRIVAT "submission/privat"

void AWTC_create_submission_variables(AW_root *root, AW_default db1) {
    root->awar_string(AWAR_SUBMIT_SOURCE, "",                         db1);
    root->awar_string(AWAR_SUBMIT_PARSER, "press READ INFO",          db1);
    root->awar_string(AWAR_SUBMIT_PARSED, "",                         db1);
    root->awar_string(AWAR_SUBMIT_FILE,   "",                         db1);
    root->awar_string(AWAR_SUBMIT_PRIVAT, "$(ADDRESS)=TUM Munich:\n", db1);
}

static void ed_calltexe_event(AW_window *aww, const char *varname) {
    AW_edit(aww->get_root()->awar(varname)->read_char_pntr());
}

static AW_window *create_calltexe_window(AW_root *root, const char *varname) {
    AW_window_simple *aws = new AW_window_simple;
    {
        char *var_id    = GBS_string_2_key(varname);
        char *window_id = GBS_global_string_copy("SUBM_TEXTEDIT_%s", var_id);
        aws->init(root, window_id, "START TEXT EDITOR");
        free(window_id);
        free(var_id);
    }

    aws->load_xfig("calltexe.fig");
    aws->label_length(18);
    aws->button_length(8);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");

    aws->at("file");
    aws->create_input_field(varname, 34);

    aws->at("edit");
    aws->callback(makeWindowCallback(ed_calltexe_event, varname));
    aws->create_button("EDIT", "EDIT", "E");

    return aws;
}

static void ed_submit_info_event_rm(char *string)
{
    char *p = string;
    int c;
    while ((c=*p) != 0) {
        if (c==':') *p = ';';
        if (c=='=') *p = '-';
        p++;
    }
}

static void ed_submit_info_event(AW_window *aww, GBDATA *gb_main) {
    AW_root *aw_root = aww->get_root();
    
    GB_transaction  ta(gb_main);
    char           *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species   = GBT_find_species(gb_main, species_name);
    GBS_strstruct  *strstruct    = GBS_stropen(1000);

    if (gb_species) {
        for (GBDATA *gb_entry = GB_child(gb_species); gb_entry; gb_entry = GB_nextChild(gb_entry)) {
            int type = GB_read_type(gb_entry);
            switch (type) {
                case GB_STRING:
                case GB_INT:
                case GB_BITS:
                case GB_FLOAT: {
                    char *key = GB_read_key(gb_entry);

                    GBS_strcat(strstruct, "$(");
                    GBS_strcat(strstruct, key);
                    GBS_strcat(strstruct, ")=");
                    free(key);

                    key = GB_read_as_string(gb_entry);
                    ed_submit_info_event_rm(key);
                    GBS_strcat(strstruct, key);
                    free(key);

                    GBS_strcat(strstruct, ":\n");
                    break;
                }
                default:
                    break;
            }
        }
        char *seq_info = GBS_string_eval(" ", "*="
                                         "$(SEQ_LEN)\\=*(|sequence|len(.-))\\:\n"
                                         "$(SEQ_A)\\=*(|sequence|count(aA))\\:\n"
                                         "$(SEQ_C)\\=*(|sequence|count(cC))\\:\n"
                                         "$(SEQ_G)\\=*(|sequence|count(gG))\\:\n"
                                         "$(SEQ_T)\\=*(|sequence|count(tT))\\:\n"
                                         "$(SEQ_U)\\=*(|sequence|count(uU))\\:\n"
                                         "$(SEQUENCE)\\=*(|sequence|remove(.-)|format_sequence(firsttab\\=12;tab\\=12;width\\=60;numleft;gap\\=10))\\:\n"
                                         , gb_species);
        if (seq_info) {
            GBS_strcat(strstruct, seq_info);
            free(seq_info);
        }
        else {
            aw_message(GB_await_error());
        }
    }
    else {
        GBS_strcat(strstruct, "Species not found");
    }

    char *parser = GBS_strclose(strstruct);
    aw_root->awar(AWAR_SUBMIT_PARSER)->write_string(parser);

    free(parser);
    free(species_name);
}

static void ed_save_var_to_file(AW_window *aww, const char *data_var, const char *file_var) {
    AW_root *aw_root   = aww->get_root();
    char    *data      = aw_root->awar(data_var)->read_string();
    char    *file_name = aw_root->awar(file_var)->read_string();
    FILE    *out       = fopen(file_name, "w");
    
    if (out) {
        fprintf(out, "%s", data);
        fclose(out);
    }
    else {
        aw_message(GB_IO_error("saving info", file_name));
    }
    free(file_name);
    free(data);
}
static void ed_submit_parse_event(AW_window *aww)
{
    AW_root *aw_root = aww->get_root();
    char *parser = aw_root->awar(AWAR_SUBMIT_PARSER)->read_string();
    char    *dest;
    char    *dest2;
    char    *privat;
    char    *p, *d;
    int c;

    for (d = p = parser; *p; p++) {
        if ((c=*p)==':') {
            if (p[1] == '\n') p++; // skip newline
        }
        *(d++) = c;
    }
    *d = 0;
    char *sub_file = aw_root->awar(AWAR_SUBMIT_SOURCE)->read_string();
    char *source = GB_read_file(sub_file);
    if (source) {
        dest = GBS_string_eval(source, parser, 0);
        if (!dest) dest = ARB_strdup(GB_await_error());
    }
    else {
        dest = GBS_global_string_copy("submission form not found\n(Reason: %s)", GB_await_error());
    }

    awtc_assert(dest); // should contain partly filled form or error message

    privat = aw_root->awar(AWAR_SUBMIT_PRIVAT)->read_string();
    for (d = p = privat; *p; p++) {
        if ((c=*p)==':') {
            if (p[1] == '\n') p++; // skip newline
        }
        *(d++) = c;
    }
    *d = 0;

    dest2             = GBS_string_eval(dest, privat, 0);
    if (!dest2) dest2 = ARB_strdup(GB_await_error());

    aw_root->awar(AWAR_SUBMIT_PARSED)->write_string(dest2);

    free(dest);
    free(dest2);
    free(privat);
    free(source);
    free (sub_file);
    free(parser);
}

static void ed_submit_gen_event(AW_window *aww)
{
    AW_root *aw_root = aww->get_root();
    char buffer[256];
    char *name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    sprintf(buffer, "%s.submit", name);
    free(name);
    aw_root->awar(AWAR_SUBMIT_FILE)->write_string(buffer);
}


AW_window *AWTC_create_submission_window(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "SUBMISSION", "SUBMISSION");

    aws->load_xfig("submiss.fig");
    aws->label_length(18);
    aws->button_length(8);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");

    aws->callback(makeHelpCallback("submission.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(15);

    aws->at("privat");
    aws->create_text_field(AWAR_SUBMIT_PRIVAT, 80, 5);

    aws->at("parsed_info");
    aws->create_text_field(AWAR_SUBMIT_PARSER, 80, 6);

    aws->at("parsed");
    aws->create_text_field(AWAR_SUBMIT_PARSED, 80, 13);

    aws->at("species");
    aws->label("Species Name:");
    aws->create_input_field(AWAR_SPECIES_NAME, 12);

    aws->at("submission");
    {
        StrArray submits;
        GBS_read_dir(submits, GB_path_in_ARBLIB("submit"), NULL);

        if (!submits.empty()) {
            aws->label("Select a Form");
            aws->create_option_menu(AWAR_SUBMIT_SOURCE, false);
            for (int i = 0; submits[i]; ++i) {
                aws->insert_option(submits[i], "", submits[i]);
            }
            aws->insert_default_option("default", "d", "default");
            aws->update_option_menu();
        }
    }

    aws->at("gen");
    aws->label("Gen File Name");
    aws->callback(ed_submit_gen_event);
    aws->create_button("GEN_FILE_NAME", "CREATE NAME", "C");

    aws->at("file");
    aws->label("or enter");
    aws->create_input_field(AWAR_SUBMIT_FILE, 30);

    aws->at("info");
    aws->callback(makeWindowCallback(ed_submit_info_event, gb_main));
    aws->create_button("READ_INFO", "READ INFO", "R");

    aws->at("parse");
    aws->callback(ed_submit_parse_event);
    aws->create_button("FILL_OUT_FORM", "FILL THE FORM", "F");

    aws->at("write");
    aws->callback(makeWindowCallback(ed_save_var_to_file, AWAR_SUBMIT_PARSED, AWAR_SUBMIT_FILE));
    aws->create_button("SAVE", "SAVE TO FILE", "S");

    aws->button_length(20);
    aws->at("edit");
    aws->callback(makeCreateWindowCallback(create_calltexe_window, AWAR_SUBMIT_SOURCE));
    aws->create_button("EDIT_FORM", "EDIT FORM", "R");

    aws->at("editresult");
    aws->callback(makeCreateWindowCallback(create_calltexe_window, AWAR_SUBMIT_FILE));
    aws->create_button("EDIT_SAVED", "EDIT SAVED", "S");

    aws->at("privatlabel");
    aws->create_button(0, "Your private data");

    return aws;
}
