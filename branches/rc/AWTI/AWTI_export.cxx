// ============================================================= //
//                                                               //
//   File      : AWTI_export.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "awti_export.hxx"

#include <seqio.hxx>
#include <awt_filter.hxx>
#include <AP_filter.hxx>
#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_window.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <arb_str.h>
#include <arb_file.h>
#include <arb_strbuf.h>

#define AWAR_EXPORT_FILE           "tmp/export_db/file"
#define AWAR_EXPORT_FORM           "tmp/export/form"
#define AWAR_EXPORT_ALI            "tmp/export/alignment"
#define AWAR_EXPORT_MULTIPLE_FILES "tmp/export/multiple_files"
#define AWAR_EXPORT_MARKED         "export/marked"
#define AWAR_EXPORT_COMPRESS       "export/compress"
#define AWAR_EXPORT_FILTER_PREFIX  "export/filter"
#define AWAR_EXPORT_FILTER_NAME    AWAR_EXPORT_FILTER_PREFIX "/name"
#define AWAR_EXPORT_FILTER_FILTER  AWAR_EXPORT_FILTER_PREFIX "/filter"
#define AWAR_EXPORT_FILTER_ALI     AWAR_EXPORT_FILTER_PREFIX "/alignment"
#define AWAR_EXPORT_CUTSTOP        "export/cutstop"

#define awti_assert(cond) arb_assert(cond)

static void export_go_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL res_from_awt_create_select_filter) {
    awti_assert(!GB_have_error());

    GBDATA           *gb_main = (GBDATA*)cl_gb_main;
    GB_transaction    ta(gb_main);
    adfiltercbstruct *acbs    = (adfiltercbstruct*)res_from_awt_create_select_filter;

    arb_progress progress("Exporting data");

    AW_root  *awr            = aww->get_root();
    char     *formname       = awr->awar(AWAR_EXPORT_FORM"/file_name")->read_string();
    int       multiple       = awr->awar(AWAR_EXPORT_MULTIPLE_FILES)->read_int();
    int       marked_only    = awr->awar(AWAR_EXPORT_MARKED)->read_int();
    int       cut_stop_codon = awr->awar(AWAR_EXPORT_CUTSTOP)->read_int();
    int       compress       = awr->awar(AWAR_EXPORT_COMPRESS)->read_int();
    GB_ERROR  error          = 0;

    char *outname      = awr->awar(AWAR_EXPORT_FILE"/file_name")->read_string();
    char *real_outname = 0;     // with suffix (name of first file if multiple)

    AP_filter *filter = awt_get_filter(acbs);
    char *db_name = awr->awar(AWAR_DB_NAME)->read_string();

    error = SEQIO_export_by_format(gb_main, marked_only, filter, cut_stop_codon, compress,
                                   db_name, formname, outname, multiple, &real_outname);
    free(db_name);
    if (error) aw_message(error);

    if (real_outname) awr->awar(AWAR_EXPORT_FILE"/file_name")->write_string(real_outname);

    AW_refresh_fileselection(awr, AWAR_EXPORT_FILE);

    free(real_outname);
    free(outname);
    free(formname);

}

static void create_export_awars(AW_root *awr, AW_default def) {
    {
        GBS_strstruct path(500);
        path.cat(GB_path_in_arbprop("filter"));
        path.put(':');
        path.cat(GB_path_in_ARBLIB("export"));

        AW_create_fileselection_awars(awr, AWAR_EXPORT_FORM, path.get_data(), ".eft", "*");
        AW_create_fileselection_awars(awr, AWAR_EXPORT_FILE, "",              "",     "noname");
    }

    awr->awar_string(AWAR_EXPORT_ALI, "16s", def);
    awr->awar_int(AWAR_EXPORT_MULTIPLE_FILES, 0, def);

    awr->awar_int(AWAR_EXPORT_MARKED, 1, def); // marked only
    awr->awar_int(AWAR_EXPORT_COMPRESS, 1, def); // vertical gaps
    awr->awar_string(AWAR_EXPORT_FILTER_NAME, "none", def); // no default filter
    awr->awar_string(AWAR_EXPORT_FILTER_FILTER, "", def);
    AW_awar *awar_ali = awr->awar_string(AWAR_EXPORT_FILTER_ALI, "", def);
    awar_ali->map(AWAR_DEFAULT_ALIGNMENT);

    awr->awar_int(AWAR_EXPORT_CUTSTOP, 0, def); // don't cut stop-codon
}


static void export_form_changed_cb(AW_root *aw_root) {
    // called when selected export format changes
    // -> automatically correct filename suffix
    // -> restrict view to suffix

    static char *previous_suffix = 0;

    GB_ERROR  error          = 0;
    AW_awar  *awar_form      = aw_root->awar(AWAR_EXPORT_FORM"/file_name");
    char     *current_format = awar_form->read_string();

    if (current_format) {
        if (GB_is_regularfile(current_format)) {
            char *current_suffix = SEQIO_exportFormat_get_outfile_default_suffix(current_format, error);
            if (!error) {
                // Note: current_suffix may be NULL.. is that ok?

                // modify export filename and view

                AW_awar *awar_filter = aw_root->awar(AWAR_EXPORT_FILE"/filter");
                AW_awar *awar_export = aw_root->awar(AWAR_EXPORT_FILE"/file_name");

                awar_filter->write_string("");

                char *exportname = awar_export->read_string();

                {
                    char *path, *nameOnly, *suffix;
                    GB_split_full_path(exportname, &path, NULL, &nameOnly, &suffix);

                    if (suffix) {
                        if (previous_suffix && ARB_stricmp(suffix, previous_suffix) == 0) freedup(suffix, current_suffix); // remove old suffix
                        else freedup(suffix, GB_append_suffix(suffix, current_suffix)); // don't know existing suffix -> append
                    }
                    else suffix = strdup(current_suffix);

                    const char *new_exportname = GB_concat_path(path, GB_append_suffix(nameOnly, suffix));
                    if (new_exportname) awar_export->write_string(new_exportname);

                    free(suffix);
                    free(nameOnly);
                    free(path);
                }

                free(exportname);

                awar_filter->write_string(current_suffix);

                // remember last applied suffix
                reassign(previous_suffix, current_suffix);
            }

            free(current_suffix);
        }
        free(current_format);
    }

    if (error) aw_message(error);
}

AW_window *create_AWTC_export_window(AW_root *awr, GBDATA *gb_main)
{
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    create_export_awars(awr, AW_ROOT_DEFAULT);

    aws = new AW_window_simple;

    aws->init(awr, "ARB_EXPORT", "ARB EXPORT");
    aws->load_xfig("awt/export_db.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("arb_export.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_fileselection(aws, AWAR_EXPORT_FILE, "f", "PWD",     ANY_DIR,    false); // select export filename
    AW_create_fileselection(aws, AWAR_EXPORT_FORM, "",  "ARBHOME", MULTI_DIRS, false); // select export filter

    aws->get_root()->awar(AWAR_EXPORT_FORM"/file_name")->add_callback(export_form_changed_cb);

    aws->at("allmarked");
    aws->create_option_menu(AWAR_EXPORT_MARKED, true);
    aws->insert_option("all", "a", 0);
    aws->insert_option("marked", "m", 1);
    aws->update_option_menu();

    aws->at("compress");
    aws->create_option_menu(AWAR_EXPORT_COMPRESS, true);
    aws->insert_option("no", "n", 0);
    aws->insert_option("vertical gaps", "v", 1);
    aws->insert_option("all gaps", "a", 2);
    aws->update_option_menu();

    aws->at("seqfilter");
    adfiltercbstruct *filtercd = awt_create_select_filter(aws->get_root(), gb_main, AWAR_EXPORT_FILTER_NAME);
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, filtercd));
    aws->create_button("SELECT_FILTER", AWAR_EXPORT_FILTER_NAME);

    aws->at("cutstop");
    aws->create_toggle(AWAR_EXPORT_CUTSTOP);

    aws->at("multiple");
    aws->create_toggle(AWAR_EXPORT_MULTIPLE_FILES);

    aws->at("go");
    aws->highlight();
    aws->callback(export_go_cb, (AW_CL)gb_main, (AW_CL)filtercd);
    aws->create_button("GO", "GO", "G");

    return aws;
}
