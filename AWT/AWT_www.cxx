// ================================================================ //
//                                                                  //
//   File      : AWT_www.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt_config_manager.hxx"
#include "awt.hxx"

#include <aw_window.hxx>
#include <aw_global_awars.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>

#include <arbdbt.h>
#include <arb_str.h>
#include <arb_defs.h>

#define WWW_COUNT                10
#define AWAR_WWW_SELECT          "www/url_select"
#define AWAR_WWW_SELECT_TEMPLATE "www/url_%i/select"
#define AWAR_WWW_TEMPLATE        "www/url_%i/srt"
#define AWAR_WWW_DESC_TEMPLATE   "www/url_%i/desc"

inline char *extract_url_host(const char *templ) {
    const char *url_start = strstr(templ, "\"http://");
    if (url_start) {
        const char *host_start = url_start+8;
        const char *slash      = strchr(host_start, '/');

        if (slash) return GB_strpartdup(host_start, slash-1);
    }
    return NULL;
}

inline bool url_host_matches(const char *templ1, const char *templ2) {
    bool  matches = false;
    char *url1    = extract_url_host(templ1);
    if (url1) {
        char *url2 = extract_url_host(templ2);
        matches = url1 && url2 && ARB_stricmp(url1, url2) == 0;
        free(url2);
    }
    free(url1);
    return matches;
}

void awt_create_aww_vars(AW_root *aw_root, AW_default aw_def) {
    struct Example {
        const char *descr;
        const char *templ;
    } example[] = {
        { "EMBL example",   "\"http://www.ebi.ac.uk/ena/data/view/\";readdb(acc)" },
        { "SILVA example",  "\"http://www.arb-silva.de/browser/ssu/\";readdb(acc)" },
        { "Google example", "\"http://www.google.com/search?q=\";readdb(full_name);|srt(\": =+\")" }
    }, empty = { "", "" };

    const int DEFAULT_SELECT = 1; // SILVA
    const int EXAMPLE_COUNT  = ARRAY_ELEMS(example);
    STATIC_ASSERT(EXAMPLE_COUNT <= WWW_COUNT);

    bool example_url_seen[EXAMPLE_COUNT];
    for (int x = 0; x<EXAMPLE_COUNT; ++x) example_url_seen[x] = false;

    AW_awar *awar_templ[WWW_COUNT];
    AW_awar *awar_descr[WWW_COUNT];
    bool     is_empty[WWW_COUNT];

    for (int i = 0; i<WWW_COUNT; ++i) {
        const Example&  curr = i<EXAMPLE_COUNT ? example[i] : empty;
        const char     *awar_name;

        awar_name     = GBS_global_string(AWAR_WWW_TEMPLATE, i);
        awar_templ[i] = aw_root->awar_string(awar_name, curr.templ, aw_def);

        awar_name     = GBS_global_string(AWAR_WWW_DESC_TEMPLATE, i);
        awar_descr[i] = aw_root->awar_string(awar_name, curr.descr, aw_def);

        const char *templ = awar_templ[i]->read_char_pntr();
        const char *descr = awar_descr[i]->read_char_pntr();

        is_empty[i] = !templ[0] && !descr[0];
        if (!is_empty[i]) {
            for (int x = 0; x<EXAMPLE_COUNT; ++x) {
                if (!example_url_seen[x]) {
                    example_url_seen[x] = url_host_matches(templ, example[x].templ);
                }
            }
        }

        awar_name = GBS_global_string(AWAR_WWW_SELECT_TEMPLATE, i);
        aw_root->awar_int(awar_name, 0, aw_def);
    }

    // insert missing examples
    for (int x = 0; x<EXAMPLE_COUNT; ++x) {
        if (!example_url_seen[x]) {
            for (int i = 0; i<WWW_COUNT; ++i) {
                if (is_empty[i]) {
                    awar_templ[i]->write_string(example[x].templ);
                    awar_descr[i]->write_string(example[x].descr);
                    is_empty[i] = false;
                    break;
                }
            }
        }
    }

    aw_root->awar_int(AWAR_WWW_SELECT, DEFAULT_SELECT, aw_def);
    awt_assert(ARB_global_awars_initialized());
}

GB_ERROR awt_open_ACISRT_URL_by_gbd(AW_root *aw_root, GBDATA *gb_main, GBDATA *gbd, const char *name, const char *url_srt) {
    GB_ERROR        error = 0;
    GB_transaction  tscope(gb_main);
    char           *url   = GB_command_interpreter(gb_main, name, url_srt, gbd, 0);

    if (!url) error = GB_await_error();
    else AW_openURL(aw_root, url);

    free(url);

    return error;
}

GB_ERROR awt_openURL_by_gbd(AW_root *aw_root, GBDATA *gb_main, GBDATA *gbd, const char *name) {
    GB_transaction  tscope(gb_main);
    int             url_selected = aw_root->awar(AWAR_WWW_SELECT)->read_int();
    const char     *awar_url     = GBS_global_string(AWAR_WWW_TEMPLATE, url_selected);
    char           *url_srt      = aw_root->awar(awar_url)->read_string();
    GB_ERROR        error        = awt_open_ACISRT_URL_by_gbd(aw_root, gb_main, gbd, name, url_srt);

    free(url_srt);
    return error;
}

static void awt_openDefaultURL_on_species(AW_window *aww, GBDATA *gb_main) {
    GB_transaction  tscope(gb_main);
    AW_root        *aw_root          = aww->get_root();
    GB_ERROR        error            = 0;
    char           *selected_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species       = GBT_find_species(gb_main, selected_species);

    if (!gb_species) {
        error = GBS_global_string("Cannot find species '%s'", selected_species);
    }
    else {
        error = awt_openURL_by_gbd(aw_root, gb_main, gb_species, selected_species);
    }
    if (error) aw_message(error);
    delete selected_species;
}

static void awt_www_select_change(AW_window *aww, int selected) {
    AW_root *aw_root = aww->get_root();
    for (int i=0; i<WWW_COUNT; i++) {
        const char *awar_name = GBS_global_string(AWAR_WWW_SELECT_TEMPLATE, i);
        aw_root->awar(awar_name)->write_int((i==selected) ? 1 : 0);
    }
    aw_root->awar(AWAR_WWW_SELECT)->write_int(selected);
}

static void www_setup_config(AWT_config_definition& cdef) {
    for (int i=0; i<WWW_COUNT; i++) {
        char buf[256];
        sprintf(buf, AWAR_WWW_SELECT_TEMPLATE, i); cdef.add(buf, "active",      i);
        sprintf(buf, AWAR_WWW_DESC_TEMPLATE,   i); cdef.add(buf, "description", i);
        sprintf(buf, AWAR_WWW_TEMPLATE,        i); cdef.add(buf, "template",    i);
    }
}

AW_window *AWT_create_www_window(AW_root *aw_root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "WWW_PROPS", "WWW");
    aws->load_xfig("awt/www.fig");
    aws->auto_space(10, 5);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("props_www.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("action");
    aws->callback(makeWindowCallback(awt_openDefaultURL_on_species, gb_main));
    aws->create_button("WWW", "WWW", "W");

    aws->button_length(13);
    int dummy, closey;
    aws->at_newline();
    aws->get_at_position(&dummy, &closey);
    aws->at_newline();

    aws->create_button(0, "K");

    aws->at_newline();

    int fieldselectx, srtx, descx;


    int i;
    aws->get_at_position(&fieldselectx, &dummy);

    aws->auto_space(10, 2);

    for (i=0; i<WWW_COUNT; i++) {
        char buf[256];
        sprintf(buf, AWAR_WWW_SELECT_TEMPLATE, i);
        aws->callback(makeWindowCallback(awt_www_select_change, i)); // @@@ used as TOGGLE_CLICK_CB (see #559)
        aws->create_toggle(buf);

        sprintf(buf, AWAR_WWW_DESC_TEMPLATE, i);
        aws->get_at_position(&descx, &dummy);
        aws->create_input_field(buf, 15);

        aws->get_at_position(&srtx, &dummy);
        sprintf(buf, AWAR_WWW_TEMPLATE, i);
        aws->create_input_field(buf, 80);

        aws->at_newline();
    }
    aws->at_newline();

    aws->create_input_field(AWAR_WWW_BROWSER, 100);

    aws->at(fieldselectx, closey);

    aws->at_x(fieldselectx);
    aws->create_button(0, "SEL");

    aws->at_x(descx);
    aws->create_button(0, "DESCRIPTION");

    aws->at_x(srtx);
    aws->create_button(0, "URL");

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "www", makeConfigSetupCallback(www_setup_config));

    awt_www_select_change(aws, aw_root->awar(AWAR_WWW_SELECT)->read_int());
    return aws;
}

void AWT_openURL(AW_window *aww, const char *url) {
    AW_openURL(aww->get_root(), url);
}

