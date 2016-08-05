// =============================================================== //
//                                                                 //
//   File      : probe_design.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "SaiProbeVisualization.hxx"
#include "probe_match_parser.hxx"

#include <PT_com.h>
#include <PT_server.h> // needed for DOMAIN_MIN_LENGTH
#include <client.h>
#include <servercntrl.h>
#include <probe_gui.hxx>
#include <probe_local.hxx>

#include <GEN.hxx>
#include <TreeCallbacks.hxx>

#include <iupac.h>
#include <awt_config_manager.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_misc.hxx>

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_select.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_edit.hxx>
#include <rootAsWin.h>

#include <adGene.h>

#include <arb_progress.h>
#include <arb_strbuf.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <arb_misc.h>

#include "probe_collection.hxx"

// general awars

#define AWAR_PROBE_CREATE_GENE_SERVER "tmp/probe_admin/gene_server" // whether to create a gene pt server

// probe match awars

// #define AWAR_PD_MATCH_ITEM     AWAR_SPECIES_NAME
#define AWAR_PD_SELECTED_MATCH "tmp/probe_design/match"
#define AWAR_PD_MATCH_RESOLVE  "tmp/probe_design/match_resolve" // for IUPAC resolution

#define AWAR_PD_MATCH_SORTBY     "probe_match/sort_by" // weighted mismatches
#define AWAR_PD_MATCH_COMPLEMENT "probe_match/complement" // check reverse complement too
#define AWAR_PD_MATCH_MARKHITS   "probe_match/mark_hits" // mark hitten species in database
#define AWAR_PD_MATCH_WRITE2TMP  "probe_match/write_2_tmp" // write result to field tmp
#define AWAR_PD_MATCH_AUTOMATCH  "probe_match/auto_match" // auto match probes when target string changes

#define AWAR_PD_MATCH_NHITS    "tmp/probe_match/nhits" // display the 'number of hits'

// probe design awars

#define AWAR_PD_DESIGN_CLIPRESULT "probe_design/CLIPRESULT" // 'length of output' (how many probes will get designed)
#define AWAR_PD_DESIGN_MISHIT     "probe_design/MISHIT"     // 'non group hits'
#define AWAR_PD_DESIGN_MAXBOND    "probe_design/MAXBOND"    // max hairpinbonds ?
#define AWAR_PD_DESIGN_MINTARGETS "probe_design/MINTARGETS" // 'min. group hits (%)'

#define AWAR_PD_DESIGN_MIN_LENGTH "probe_design/PROBELENGTH"      // min. length of probe
#define AWAR_PD_DESIGN_MAX_LENGTH "probe_design/PROBEMAXLENGTH"   // max. length of probe (or empty)

#define AWAR_PD_DESIGN_MIN_TEMP     "probe_design/MINTEMP"     // temperature (min)
#define AWAR_PD_DESIGN_MAX_TEMP     "probe_design/MAXTEMP"     // temperature (max)
#define AWAR_PD_DESIGN_MIN_GC       "probe_design/MINGC"       // GC content (min)
#define AWAR_PD_DESIGN_MAX_GC       "probe_design/MAXGC"       // GC content (max)
#define AWAR_PD_DESIGN_MIN_ECOLIPOS "probe_design/MINECOLI"    // ecolipos (min)
#define AWAR_PD_DESIGN_MAX_ECOLIPOS "probe_design/MAXECOLI"    // ecolipos (max)

#define AWAR_PD_DESIGN_GENE "probe_design/gene" // generate probes for genes ?

// probe design/match (expert window)
#define AWAR_PD_COMMON_EXP_BONDS_FORMAT "probe_design/bonds/pos%i"  // format to generate bond awar names
#define AWAR_PD_COMMON_EXP_SPLIT        "probe_design/SPLIT"

#define AWAR_PD_DESIGN_EXP_DTEDGE "probe_design/DTEDGE"
#define AWAR_PD_DESIGN_EXP_DT     "probe_design/DT"

#define AWAR_PD_MATCH_NMATCHES   "probe_match/nmatches"
#define AWAR_PD_MATCH_LIM_NMATCH "probe_match/lim_nmatch"
#define AWAR_PD_MATCH_MAX_RES    "probe_match/maxres"

// --------------------------------
// probe collection awars

// probe collection window
#define AWAR_PC_TARGET_STRING      "probe_collection/target_string"
#define AWAR_PC_TARGET_NAME        "probe_collection/target_name"
#define AWAR_PC_MATCH_WEIGHTS      "probe_collection/match_weights/pos"
#define AWAR_PC_MATCH_WIDTH        "probe_collection/match_weights/width"
#define AWAR_PC_MATCH_BIAS         "probe_collection/match_weights/bias"
#define AWAR_PC_AUTO_MATCH         "probe_collection/auto"
#define AWAR_PC_CURRENT_COLLECTION "probe_collection/current"

#define AWAR_PC_SELECTED_PROBE     "tmp/probe_collection/probe"
#define AWAR_PC_MATCH_NHITS        "tmp/probe_collection/nhits"

// probe collection matching control parameters
#define AWAR_PC_MISMATCH_THRESHOLD "probe_collection/mismatch_threshold"

#define REPLACE_TARGET_CONTROL_CHARS ":#=_:\\:=_"

// ----------------------------------------

static saiProbeData *g_spd = 0;

static struct {
    aisc_com          *link;
    T_PT_LOCS          locs;
    T_PT_MAIN          com;
    AW_window_simple  *win;
    AW_selection_list *resultList; // @@@ replace by TypedSelectionList?
} PD;

struct ProbeMatchEventParam : virtual Noncopyable {
    GBDATA            *gb_main;
    AW_selection_list *selection_id;                // may be NULL
    int               *counter;                     // may be NULL (if != NULL -> afterwards contains number of hits)

    // results set by probe_match_event:
    std::string hits_summary; // shown in probe match window
    bool        refresh_sai_display;

    void init_results() {
        refresh_sai_display = false;
    }

    ProbeMatchEventParam(GBDATA *gb_main_, int *counter_)         : gb_main(gb_main_), selection_id(NULL), counter(counter_) { init_results(); }
    ProbeMatchEventParam(GBDATA *gb_main_, AW_selection_list *id) : gb_main(gb_main_), selection_id(id),   counter(NULL)     { init_results(); }
};

struct AutoMatchSettings {
    ProbeMatchEventParam *event_param; // not owned!
    bool                  disable;

    AutoMatchSettings(ProbeMatchEventParam *event_param_, bool disable_)
        : event_param(event_param_), disable(disable_)
    {}
    AutoMatchSettings()
        : event_param(NULL) , disable(true)
    {}

    bool initialized() const { return event_param != NULL; }
};

static AutoMatchSettings auto_match_cb_settings;

// prototypes:
static void probe_match_event_using_awars(AW_root *root, ProbeMatchEventParam *event_param);

static void auto_match_cb(AW_root *root) {
    if (!auto_match_cb_settings.disable) {
        char *ts = root->awar(AWAR_TARGET_STRING)->read_string();
        if (strlen(ts) > 0) {
            probe_match_event_using_awars(root, auto_match_cb_settings.event_param);
        }
        free(ts);
    }
}

static const char *auto_match_sensitive_awars[] = {
    AWAR_TARGET_STRING,
    AWAR_PT_SERVER,
    AWAR_PD_MATCH_COMPLEMENT,
    AWAR_MAX_MISMATCHES,
    AWAR_PD_MATCH_SORTBY,
    0
};

static void auto_match_changed(AW_root *root) {
    static bool callback_active = false;
    int         autoMatch       = root->awar(AWAR_PD_MATCH_AUTOMATCH)->read_int();

    if (autoMatch) {
        if (!callback_active) {
            for (int i = 0; auto_match_sensitive_awars[i]; ++i) {
                root->awar(auto_match_sensitive_awars[i])->add_callback(auto_match_cb);
            }
        }
    }
    else {
        if (callback_active) {
            for (int i = 0; auto_match_sensitive_awars[i]; ++i) {
                root->awar(auto_match_sensitive_awars[i])->remove_callback(auto_match_cb);
            }
        }
    }
    callback_active = bool(autoMatch);
}

static void enable_auto_match_cb(AW_root *root, ProbeMatchEventParam *event_param) {
    if (event_param == NULL && auto_match_cb_settings.initialized()) {
        // "partially" enable (w/o ProbeMatchEventParam) is only done
        // if not already "fully enabled"
        return;
    }

    auto_match_cb_settings = AutoMatchSettings(event_param, false);
    auto_match_changed(root);
}

static void popup_match_window_cb(AW_window *aww, GBDATA *gb_main) { // @@@ shall be called by auto_match_cb (if never opened b4)
    AW_root   *root         = aww->get_root();
    AW_window *match_window = create_probe_match_window(root, gb_main);
    match_window->activate();
    root->awar(AWAR_TARGET_STRING)->touch(); // force re-match
}

// --------------------------------------------------------------------------------

static void popup_probe_design_result_window(AW_window *aww, GBDATA *gb_main) {
    if (!PD.win) {
        AW_root *root = aww->get_root();

        PD.win = new AW_window_simple;
        PD.win->init(root, "PD_RESULT", "PD RESULT");
        PD.win->load_xfig("pd_reslt.fig");

        PD.win->button_length(6);
        PD.win->auto_space(10, 10);

        PD.win->at("help");
        PD.win->callback(makeHelpCallback("probedesignresult.hlp"));
        PD.win->create_button("HELP", "HELP", "H");

        PD.win->at("result");
        PD.resultList = PD.win->create_selection_list(AWAR_TARGET_STRING, 40, 5, false);
        const StorableSelectionList *storable_result_list = new StorableSelectionList(TypedSelectionList("prb", PD.resultList, "designed probes", "designed")); // @@@ make member of PD ?

        PD.resultList->clear();
        PD.resultList->insert_default("No probes designed yet", "");

        PD.win->at("buttons");

        PD.win->callback(AW_POPDOWN);
        PD.win->create_button("CLOSE", "CLOSE", "C");

        PD.win->callback(makeWindowCallback(awt_clear_selection_list_cb, PD.resultList));
        PD.win->create_button("CLEAR", "CLEAR", "R");

        PD.win->callback(makeCreateWindowCallback(create_load_box_for_selection_lists, storable_result_list));
        PD.win->create_button("LOAD", "LOAD", "L");

        PD.win->callback(makeCreateWindowCallback(create_save_box_for_selection_lists, storable_result_list));
        PD.win->create_button("SAVE", "SAVE", "S");

        PD.win->callback(makeWindowCallback(create_print_box_for_selection_lists, &storable_result_list->get_typedsellist()));
        PD.win->create_button("PRINT", "PRINT", "P");

        PD.win->callback(makeWindowCallback(popup_match_window_cb, gb_main));
        PD.win->create_button("MATCH", "MATCH", "M");

        PD.win->label("Auto match");
        PD.win->create_toggle(AWAR_PD_MATCH_AUTOMATCH);

        enable_auto_match_cb(root, NULL);
    }
    PD.win->activate();
}

static int init_local_com_struct() {
    const char *user = GB_getenvUSER();

    if (aisc_create(PD.link, PT_MAIN, PD.com,
                    MAIN_LOCS, PT_LOCS, PD.locs,
                    LOCS_USER, user,
                    NULL)) {
        return 1;
    }
    return 0;
}

static const char *PD_probe_pt_look_for_server(int serverNumber, GB_ERROR& error) {
    // return PT server info string (see GBS_read_arb_tcp for details)
    // or NULL (in this case 'error' is set)

    // DRY vs  ../TOOLS/arb_probe.cxx@AP_probe_pt_look_for_server
    // DRY vs  ../MULTI_PROBE/MP_noclass.cxx@MP_probe_pt_look_for_server

    const char *result     = 0;
    const char *server_tag = GBS_ptserver_tag(serverNumber);

    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag);
    if (!error) {
        result             = GBS_read_arb_tcp(server_tag);
        if (!result) error = GB_await_error();
    }
    pd_assert(contradicted(result, error));
    return result;
}

static GB_ERROR species_requires(GBDATA *gb_species, const char *whats_required) {
    return GBS_global_string("Species '%s' needs %s", GBT_read_name(gb_species), whats_required);
}

static GB_ERROR gene_requires(GBDATA *gb_gene, const char *whats_required) {
    GBDATA *gb_species = GB_get_grandfather(gb_gene);
    pd_assert(gb_species);
    return GBS_global_string("Gene '%s' of organism '%s' needs %s", GBT_read_name(gb_gene), GBT_read_name(gb_species), whats_required);
}

static GB_ERROR pd_get_the_names(GBDATA *gb_main, bytestring &bs, bytestring &checksum) {
    GBS_strstruct *names     = GBS_stropen(1024);
    GBS_strstruct *checksums = GBS_stropen(1024);
    GB_ERROR       error     = 0;

    GB_begin_transaction(gb_main);

    char *use = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species)) {
        GBDATA *gb_name = GB_entry(gb_species, "name");
        if (!gb_name) { error = species_requires(gb_species, "name"); break; }

        GBDATA *gb_data = GBT_find_sequence(gb_species, use);
        if (!gb_data) { error = species_requires(gb_species, GBS_global_string("data in '%s'", use)); break; }

        GBS_intcat(checksums, GBS_checksum(GB_read_char_pntr(gb_data), 1, ".-"));
        GBS_strcat(names, GB_read_char_pntr(gb_name));
        GBS_chrcat(checksums, '#');
        GBS_chrcat(names, '#');
    }

    GBS_str_cut_tail(names, 1); // remove trailing '#'
    GBS_str_cut_tail(checksums, 1); // remove trailing '#'

    free(use);

    bs.size = GBS_memoffset(names)+1;
    bs.data = GBS_strclose(names);

    checksum.size = GBS_memoffset(checksums)+1;
    checksum.data = GBS_strclose(checksums);

    GB_commit_transaction(gb_main);

    return error;
}

static GB_ERROR pd_get_the_gene_names(GBDATA *gb_main, bytestring &bs, bytestring &checksum) {
    GBS_strstruct *names     = GBS_stropen(1024);
    GBS_strstruct *checksums = GBS_stropen(1024);
    GB_ERROR       error     = 0;

    GB_begin_transaction(gb_main);
    const char *use = GENOM_ALIGNMENT; // gene pt server is always build on 'ali_genom'

    for (GBDATA *gb_species = GEN_first_organism(gb_main); gb_species && !error; gb_species = GEN_next_organism(gb_species)) {
        const char *species_name = 0;
        {
            GBDATA *gb_data = GBT_find_sequence(gb_species, use);
            if (!gb_data) { error = species_requires(gb_species, GBS_global_string("data in '%s'", use)); break; }

            GBDATA *gb_name = GB_search(gb_species, "name", GB_FIND);
            if (!gb_name) { error = species_requires(gb_species, "name"); break; }
            species_name = GB_read_char_pntr(gb_name);
        }

        for (GBDATA *gb_gene = GEN_first_marked_gene(gb_species); gb_gene && !error; gb_gene = GEN_next_marked_gene(gb_gene)) {
            const char *gene_name = 0;
            {
                GBDATA *gb_gene_name = GB_entry(gb_gene, "name");
                if (!gb_gene_name) { error = gene_requires(gb_gene, "name"); break; }
                gene_name = GB_read_char_pntr(gb_gene_name);
            }

            {
                char *gene_seq       = GBT_read_gene_sequence(gb_gene, false, 0);
                if (!gene_seq) error = GB_await_error();
                else {
                    long        CheckSum = GBS_checksum(gene_seq, 1, ".-");
                    const char *id       = GBS_global_string("%s/%s", species_name, gene_name);

                    GBS_intcat(checksums, CheckSum);
                    GBS_strcat(names, id);
                    GBS_chrcat(checksums, '#');
                    GBS_chrcat(names, '#');

                    free(gene_seq);
                }
            }
        }
    }

    GBS_str_cut_tail(names, 1); // remove trailing '#'
    GBS_str_cut_tail(checksums, 1); // remove trailing '#'

    if (error) {
        GBS_strforget(names);
        GBS_strforget(checksums);
    }
    else {
        bs.size = GBS_memoffset(names)+1;
        bs.data = GBS_strclose(names);

        checksum.size = GBS_memoffset(checksums)+1;
        checksum.data = GBS_strclose(checksums);
    }
    error = GB_end_transaction(gb_main, error);
    return error;
}

inline const char *bond_awar_name(int i) {
    return GBS_global_string(AWAR_PD_COMMON_EXP_BONDS_FORMAT, i);
}

struct ProbeCommonSettings {
    float split;
    float bonds[16];

    explicit ProbeCommonSettings(AW_root *root)
        : split(root->awar(AWAR_PD_COMMON_EXP_SPLIT)->read_float())
    {
        for (int i=0; i<16; i++) {
            bonds[i] = root->awar(bond_awar_name(i))->read_float();
        }
    }
};

static int probe_common_send_data(const ProbeCommonSettings& commonSettings) {
    // send data common to probe-design AND -match
    if (aisc_put(PD.link, PT_LOCS, PD.locs,
                 LOCS_SPLIT, (double)commonSettings.split,
                 NULL))
        return 1;

    for (int i=0; i<16; i++) {
        if (aisc_put(PD.link, PT_LOCS, PD.locs,
                     PT_INDEX,     (long)i,
                     LOCS_BONDVAL, (double)commonSettings.bonds[i],
                     NULL))
            return 1;
    }
    return 0;
}
static int probe_design_send_data(AW_root *root, const T_PT_PDC& pdc) {
    if (aisc_put(PD.link, PT_PDC, pdc,
                 PDC_DTEDGE,     (double)root->awar(AWAR_PD_DESIGN_EXP_DTEDGE)->read_float()*100.0,
                 PDC_DT,         (double)root->awar(AWAR_PD_DESIGN_EXP_DT)->read_float()*100.0,
                 PDC_CLIPRESULT, (long)root->awar(AWAR_PD_DESIGN_CLIPRESULT)->read_int(),
                 NULL))
        return 1;

    return probe_common_send_data(ProbeCommonSettings(root));
}

struct ProbeMatchSettings : public ProbeCommonSettings {
    int serverNumber;
    int nmatches;
    int nlimit;
    int maxhits;
    int complement;
    int sortBy;
    int maxMismatches;
    int markHits;
    int write2tmp;

    std::string targetString;

    explicit ProbeMatchSettings(AW_root *root)
        : ProbeCommonSettings(root),
          serverNumber(root->awar(AWAR_PT_SERVER)->read_int()),
          nmatches(root->awar(AWAR_PD_MATCH_NMATCHES)->read_int()),
          nlimit(root->awar(AWAR_PD_MATCH_LIM_NMATCH)->read_int()),
          maxhits(root->awar(AWAR_PD_MATCH_MAX_RES)->read_int()),
          complement(root->awar(AWAR_PD_MATCH_COMPLEMENT)->read_int()),
          sortBy(root->awar(AWAR_PD_MATCH_SORTBY)->read_int()),
          maxMismatches(root->awar(AWAR_MAX_MISMATCHES)->read_int()),
          markHits(root->awar(AWAR_PD_MATCH_MARKHITS)->read_int()),
          write2tmp(root->awar(AWAR_PD_MATCH_WRITE2TMP)->read_int()),
          targetString(root->awar(AWAR_TARGET_STRING)->read_char_pntr())
    {}
};

static int probe_match_send_data(const ProbeMatchSettings& matchSettings) {
    if (aisc_put(PD.link, PT_LOCS, PD.locs,
                 LOCS_MATCH_N_ACCEPT, (long)matchSettings.nmatches,
                 LOCS_MATCH_N_LIMIT,  (long)matchSettings.nlimit,
                 LOCS_MATCH_MAX_HITS, (long)matchSettings.maxhits,
                 NULL))
        return 1;

    return probe_common_send_data(matchSettings);
}

static int ecolipos2int(const char *awar_val) {
    int i = atoi(awar_val);
    return i>0 ? i : -1;
}

static char *readableUnknownNames(const ConstStrArray& unames) {
    GBS_strstruct readable(100);

    int ulast = unames.size()-1;
    int umax  = ulast <= 10 ? ulast : 10;
    for (int u = 0; u <= umax; ++u) {
        if (u) readable.cat(u == ulast ? " and " : ", ");
        readable.cat(unames[u]);
    }

    if (umax<ulast) readable.nprintf(30, " (and %i other)", ulast-umax);

    return readable.release();
}

static void probe_design_event(AW_window *aww, GBDATA *gb_main) {
    AW_root     *root  = aww->get_root();
    T_PT_PDC     pdc;
    T_PT_TPROBE  tprobe;
    bytestring   bs;
    bytestring   check;
    GB_ERROR     error = 0;

    arb_progress progress("Probe design");
    progress.subtitle("Connecting PT-server");

    {
        const char *servername = PD_probe_pt_look_for_server(root->awar(AWAR_PT_SERVER)->read_int(), error);
        if (servername) {
            PD.link = aisc_open(servername, PD.com, AISC_MAGIC_NUMBER, &error);
            if (!PD.link) error = "can't contact PT server";
            PD.locs.clear();
        }
    }

    if (!error && init_local_com_struct()) {
        error = "Cannot contact to probe server: Connection Refused";
    }

    bool design_gene_probes = root->awar(AWAR_PD_DESIGN_GENE)->read_int();
    if (design_gene_probes) {
        GB_transaction ta(gb_main);
        if (!GEN_is_genome_db(gb_main, -1)) design_gene_probes = false;
    }

    if (!error) {
        if (design_gene_probes) { // design probes for genes
            error = pd_get_the_gene_names(gb_main, bs, check);
        }
        else {
            error = pd_get_the_names(gb_main, bs, check);
        }
    }

    if (error) {
        aw_message(error);
        return;
    }

    progress.subtitle("probe design running");

    if (aisc_create(PD.link, PT_LOCS, PD.locs,
                    LOCS_PROBE_DESIGN_CONFIG, PT_PDC, pdc,
                    PDC_MIN_PROBELEN, (long)root->awar(AWAR_PD_DESIGN_MIN_LENGTH)->read_int(),
                    PDC_MAX_PROBELEN, (long)root->awar(AWAR_PD_DESIGN_MAX_LENGTH)->read_int(),
                    PDC_MINTEMP,      (double)root->awar(AWAR_PD_DESIGN_MIN_TEMP)->read_float(),
                    PDC_MAXTEMP,      (double)root->awar(AWAR_PD_DESIGN_MAX_TEMP)->read_float(),
                    PDC_MINGC,        (double)root->awar(AWAR_PD_DESIGN_MIN_GC)->read_float()/100.0,
                    PDC_MAXGC,        (double)root->awar(AWAR_PD_DESIGN_MAX_GC)->read_float()/100.0,
                    PDC_MAXBOND,      (double)root->awar(AWAR_PD_DESIGN_MAXBOND)->read_int(),
                    PDC_MIN_ECOLIPOS, (long)ecolipos2int(root->awar(AWAR_PD_DESIGN_MIN_ECOLIPOS)->read_char_pntr()),
                    PDC_MAX_ECOLIPOS, (long)ecolipos2int(root->awar(AWAR_PD_DESIGN_MAX_ECOLIPOS)->read_char_pntr()),
                    PDC_MISHIT,       (long)root->awar(AWAR_PD_DESIGN_MISHIT)->read_int(),
                    PDC_MINTARGETS,   (double)root->awar(AWAR_PD_DESIGN_MINTARGETS)->read_float()/100.0,
                    NULL))
    {
        aw_message("Connection to PT_SERVER lost (1)");
        return;
    }

    if (probe_design_send_data(root, pdc)) {
        aw_message("Connection to PT_SERVER lost (1)");
        return;
    }

    aisc_put(PD.link, PT_PDC, pdc,
             PDC_NAMES, &bs,
             PDC_CHECKSUMS, &check,
             NULL);


    // Get the unknown names
    bytestring unknown_names;
    if (aisc_get(PD.link, PT_PDC, pdc,
                 PDC_UNKNOWN_NAMES, &unknown_names,
                 NULL))
    {
        aw_message("Connection to PT_SERVER lost (1)");
        return;
    }

    bool abort = false;

    if (unknown_names.size>1) {
        ConstStrArray unames_array;
        GBT_split_string(unames_array, unknown_names.data, "#", true);

        char *readable_unames = readableUnknownNames(unames_array);

        if (design_gene_probes) { // updating sequences of missing genes is not possible with gene PT server
            aw_message(GBS_global_string("Your PT server is not up to date or wrongly chosen!\n"
                                         "It knows nothing about the following genes:\n"
                                         "\n"
                                         "  %s\n"
                                         "\n"
                                         "You have to rebuild the PT server.",
                                         readable_unames));
            abort = true;
        }
        else if (aw_question("ptserver_add_unknown",
                             GBS_global_string("Your PT server is not up to date or wrongly chosen!\n"
                                               "It knows nothing about the following species:\n"
                                               "\n"
                                               "  %s\n"
                                               "\n"
                                               "You may now temporarily add these sequences for probe design.\n",
                                               readable_unames),
                             "Add and continue,Abort"))
        {
            abort = true;
        }
        else {
            GB_transaction ta(gb_main);

            char *ali_name = GBT_get_default_alignment(gb_main);

            for (size_t u = 0; !abort && u<unames_array.size(); ++u) {
                const char *uname      = unames_array[u];
                GBDATA     *gb_species = GBT_find_species(gb_main, uname);
                if (!gb_species) {
                    aw_message(GBS_global_string("Species '%s' not found", uname));
                    abort = true;
                }
                else {
                    GBDATA *data = GBT_find_sequence(gb_species, ali_name);
                    if (!data) {
                        aw_message(GB_export_errorf("Species '%s' has no sequence belonging to alignment '%s'", uname, ali_name));
                        abort = true;
                    }
                    else {
                        T_PT_SEQUENCE pts;

                        bytestring bs_seq;
                        bs_seq.data = (char*)GB_read_char_pntr(data);
                        bs_seq.size = GB_read_string_count(data)+1;

                        aisc_create(PD.link, PT_PDC, pdc,
                                    PDC_SEQUENCE, PT_SEQUENCE, pts,
                                    SEQUENCE_SEQUENCE, &bs_seq,
                                    NULL);
                    }
                }
            }
            free(ali_name);
        }
        free(readable_unames);
        free(unknown_names.data);
    }

    if (!abort) {
        aisc_put(PD.link, PT_PDC, pdc,
                 PDC_GO, (long)0,
                 NULL);

        progress.subtitle("Reading results from server");
        {
            char *locs_error = 0;
            if (aisc_get(PD.link, PT_LOCS, PD.locs,
                         LOCS_ERROR, &locs_error,
                         NULL))
            {
                aw_message("Connection to PT_SERVER lost (1)");
                abort = true;
            }
            else if (*locs_error) {
                aw_message(locs_error);
                abort = true;
            }
            else {
                free(locs_error);
            }
        }
    }

    if (!abort) {
        aisc_get(PD.link, PT_PDC, pdc,
                 PDC_TPROBE, tprobe.as_result_param(),
                 NULL);

        popup_probe_design_result_window(aww, gb_main);
        PD.resultList->clear();

        {
            char *match_info = 0;
            aisc_get(PD.link, PT_PDC, pdc,
                     PDC_INFO_HEADER, &match_info,
                     NULL);

            char *s = strtok(match_info, "\n");
            while (s) {
                PD.resultList->insert(s, "");
                s = strtok(0, "\n");
            }
            free(match_info);
        }

        while (tprobe.exists()) {
            T_PT_TPROBE  tprobe_next;
            char        *match_info = 0;

            if (aisc_get(PD.link, PT_TPROBE, tprobe,
                         TPROBE_NEXT, tprobe_next.as_result_param(),
                         TPROBE_INFO, &match_info,
                         NULL)) break;

            tprobe.assign(tprobe_next);

            char *probe, *space;
            probe = strpbrk(match_info, "acgtuACGTU");
            if (probe) space = strchr(probe, ' ');
            if (probe && space) {
                *space = 0; probe = strdup(probe); *space=' ';
            }
            else {
                probe = strdup("");
            }
            PD.resultList->insert(match_info, probe);
            free(probe);
            free(match_info);
        }
        PD.resultList->insert_default("", "");
        PD.resultList->update();
    }

    aisc_close(PD.link, PD.com); PD.link = 0;
    return;
}

static bool allow_probe_match_event = true;

static __ATTR__USERESULT GB_ERROR probe_match_event(const ProbeMatchSettings& matchSettings, ProbeMatchEventParam *event_param) {
    GB_ERROR error = NULL;
    if (allow_probe_match_event) {
#if defined(ASSERTION_USED)
        // runtime check against awar usage when NOT called from probe match window!
        typedef SmartPtr< LocallyModify<bool> > Maybe;

        Maybe dontReadAwars;
        Maybe dontWriteAwars;

        if (!event_param || !event_param->selection_id) {
            dontReadAwars = new LocallyModify<bool>(AW_awar::deny_read, true);
            dontWriteAwars = new LocallyModify<bool>(AW_awar::deny_write, true);
        }
#endif

        AW_selection_list *selection_id = event_param ? event_param->selection_id : NULL;
        int               *counter      = event_param ? event_param->counter : NULL;
        GBDATA            *gb_main      = event_param ? event_param->gb_main : NULL;
        int                show_status  = 0;
        int                extras       = 1;        // mark species and write to temp fields

        if (!gb_main) { error = "Please open probe match window once to enable auto-match"; }

        SmartPtr<arb_progress> progress;

        if (!error) {
            const char *servername = PD_probe_pt_look_for_server(matchSettings.serverNumber, error);

            if (!error) {
                if (selection_id) {
                    selection_id->clear();
                    pd_assert(!counter);
                    show_status = 1;
                }
                else if (counter) {
                    extras = 0;
                }

                if (show_status) {
                    progress = new arb_progress("Probe match");
                    progress->subtitle("Connecting PT-server");
                }

                PD.link = aisc_open(servername, PD.com, AISC_MAGIC_NUMBER, &error);
                if (!error && !PD.link) error = "Cannot contact PT-server";
                PD.locs.clear();
            }
        }

        if (!error && init_local_com_struct())              error = "Cannot contact PT-server (2)";
        if (!error && probe_match_send_data(matchSettings)) error = "Connection to PT_SERVER lost (2)";

        const char *probe = matchSettings.targetString.c_str();
        if (!error) {
            if (show_status) progress->subtitle("Probe match running");

            if (aisc_nput(PD.link, PT_LOCS, PD.locs,
                          LOCS_MATCH_REVERSED,       (long)matchSettings.complement, // @@@ use of complement/reverse looks suspect -> check (all uses of LOCS_MATCH_REVERSED and LOCS_MATCH_COMPLEMENT)
                          LOCS_MATCH_COMPLEMENT,     (long)0,
                          LOCS_MATCH_SORT_BY,        (long)matchSettings.sortBy,
                          LOCS_MATCH_MAX_MISMATCHES, (long)matchSettings.maxMismatches,
                          LOCS_SEARCHMATCH,          probe,
                          NULL))
            {
                error = "Connection to PT_SERVER lost (2)";
            }
            else {
                delete(g_spd);              // delete previous probe data
                g_spd = new saiProbeData;
                transferProbeData(g_spd);

                g_spd->setProbeTarget(probe);
            }
        }

        bytestring bs;
        bs.data = 0;

        long matches_truncated = 0;
        if (!error) {
            if (show_status) progress->subtitle("Reading results");

            T_PT_MATCHLIST  match_list;
            long            match_list_cnt    = 0;
            char           *locs_error        = 0;

            if (aisc_get(PD.link, PT_LOCS, PD.locs,
                         LOCS_MATCH_LIST,        match_list.as_result_param(),
                         LOCS_MATCH_LIST_CNT,    &match_list_cnt,
                         LOCS_MATCH_STRING,      &bs,
                         LOCS_MATCHES_TRUNCATED, &matches_truncated,
                         LOCS_ERROR,             &locs_error,
                         NULL))
            {
                error = "Connection to PT_SERVER lost (3)";
            }
            else {
                if (locs_error) {
                    if (*locs_error) error = GBS_static_string(locs_error);
                    free(locs_error);
                }
                else {
                    error = "Missing status from server (connection aborted?)";
                }
            }

            event_param->hits_summary = GBS_global_string(matches_truncated ? "[more than %li]" : "%li", match_list_cnt);
            if (matches_truncated) {
                aw_message("Too many matches, displaying a random digest.\n" // @@@ should better be handled by caller
                           "Increase the limit in the expert window.");
            }
        }

        long mcount                = 0;
        long unknown_species_count = 0;
        long unknown_gene_count    = 0;

        if (!error) {
            char        toksep[2]  = { 1, 0 };
            char       *strtok_ptr = 0; // stores strtok position
            const char *hinfo      = strtok_r(bs.data, toksep, &strtok_ptr);

            bool              gene_flag = false;
            ProbeMatchParser *parser    = 0;
            char             *result    = (char*)malloc(1024);


            if (hinfo) {
                g_spd->setHeadline(hinfo);
                parser = new ProbeMatchParser(probe, hinfo);
                error  = parser->get_error();
                if (!error) gene_flag = parser->is_gene_result();
            }

            if (selection_id) {
                int width         = 0;
                if (parser && !error) width = parser->get_probe_region_offset()+2+10; // 2 cause headline is shorter and 10 for match prefix region

                const char *searched = GBS_global_string("%-*s%s", width, "Searched for ", probe);
                selection_id->insert(searched, probe);
                if (hinfo) selection_id->insert(hinfo, "");
            }


            // clear all marks and delete all 'tmp' entries

            int mark        = extras && matchSettings.markHits;
            int write_2_tmp = extras && matchSettings.write2tmp;

            GB_push_transaction(gb_main);

            GBDATA *gb_species_data = GBT_get_species_data(gb_main);
            if (mark && !error) {
                if (show_status) progress->subtitle(gene_flag ? "Unmarking all species and genes" : "Unmarking all species");
                for (GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
                     gb_species;
                     gb_species = GBT_next_marked_species(gb_species))
                {
                    GB_write_flag(gb_species, 0);
                }

                if (gene_flag) {
                    // unmark genes of ALL species
                    for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
                         gb_species;
                         gb_species = GBT_next_species(gb_species))
                    {
                        GBDATA *genData = GEN_find_gene_data(gb_species);
                        if (genData) {
                            for (GBDATA *gb_gene = GEN_first_marked_gene(gb_species);
                                 gb_gene;
                                 gb_gene = GEN_next_marked_gene(gb_gene))
                            {
                                GB_write_flag(gb_gene, 0);
                            }
                        }
                    }
                }
            }
            if (write_2_tmp && !error) {
                if (show_status) progress->subtitle("Deleting old 'tmp' fields");
                for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
                     gb_species;
                     gb_species = GBT_next_species(gb_species))
                {
                    GBDATA *gb_tmp = GB_entry(gb_species, "tmp");
                    if (gb_tmp) GB_delete(gb_tmp);
                    if (gene_flag) {
                        for (GBDATA *gb_gene = GEN_first_gene(gb_species);
                             gb_gene;
                             gb_gene = GEN_next_gene(gb_species))
                        {
                            gb_tmp = GB_entry(gb_gene, "tmp");
                            if (gb_tmp) GB_delete(gb_tmp);
                        }
                    }
                }
            }

            // read results from pt-server :

            if (!error) {
                if (show_status) progress->subtitle("Parsing results");

                g_spd->probeSpecies.clear();
                g_spd->probeSeq.clear();

                if (gene_flag) {
                    if (!GEN_is_genome_db(gb_main, -1)) {
                        error = "Wrong PT-Server chosen (selected one is built for genes)";
                    }
                }
            }

            const char *match_name = 0;
            while (hinfo && !error && (match_name = strtok_r(0, toksep, &strtok_ptr))) {
                const char *match_info = strtok_r(0, toksep, &strtok_ptr);
                if (!match_info) break;

                pd_assert(parser);
                ParsedProbeMatch  ppm(match_info, *parser);
                char             *gene_str = 0;

                if (gene_flag) {
                    gene_str = ppm.get_column_content("genename", true);
                }

                if (!error) {
                    char flags[]       = "xx";
                    GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, match_name);

                    if (gb_species) {
                        GBDATA *gb_gene = 0;
                        if (gene_flag && strncmp(gene_str, "intergene_", 10) != 0) { // real gene
                            gb_gene = GEN_find_gene(gb_species, gene_str);
                            if (!gb_gene) {
                                aw_message(GBS_global_string("Gene '%s' not found in organism '%s'", gene_str, match_name));
                            }
                        }

                        if (mark) {
                            GB_write_flag(gb_species, 1);
                            flags[0] = '*';
                            if (gb_gene) {
                                GB_write_flag(gb_gene, 1);
                                flags[1] = '*';
                            }
                            else {
                                flags[1] = '?'; // no gene
                            }
                        }
                        else {
                            flags[0] = " *"[GB_read_flag(gb_species)];
                            flags[1] = " *?"[gb_gene ? GB_read_flag(gb_gene) : 2];
                        }

                        if (write_2_tmp) {
                            // write or append to field 'tmp'

                            GBDATA   *gb_tmp = 0;
                            GB_ERROR  error2 = 0;
                            bool      append = true;
                            {
                                GBDATA *gb_parent = gene_flag ? gb_gene : gb_species;
                                gb_tmp            = GB_search(gb_parent, "tmp", GB_FIND);
                                if (!gb_tmp) {
                                    append              = false;
                                    gb_tmp              = GB_search(gb_parent, "tmp", GB_STRING);
                                    if (!gb_tmp) error2 = GB_await_error();
                                }
                            }

                            if (!error2) {
                                pd_assert(gb_tmp);
                                if (append) {
                                    char *prevContent = GB_read_string(gb_tmp);
                                    if (!prevContent) {
                                        error2 = GB_await_error();
                                    }
                                    else {
                                        char *concatenatedContent = (char *)malloc(strlen(prevContent)+1+strlen(match_info)+1);
                                        sprintf(concatenatedContent, "%s\n%s", prevContent, match_info);
                                        error2 = GB_write_string(gb_tmp, concatenatedContent);
                                        free(concatenatedContent);
                                        free(prevContent);
                                    }
                                }
                                else {
                                    error2 = GB_write_string(gb_tmp, match_info);
                                }
                            }

                            if (error2) aw_message(error2);
                        }
                    }
                    else {
                        flags[0] = flags[1] = '?'; // species does not exist
                        unknown_species_count++;
                    }


                    if (gene_flag) {
                        sprintf(result, "%s %s", flags, match_info+1); // both flags (skip 1 space from match info to keep alignment)
                        char *gene_match_name = new char[strlen(match_name) + strlen(gene_str)+2];
                        sprintf(gene_match_name, "%s/%s", match_name, gene_str);
                        if (selection_id) selection_id->insert(result, gene_match_name);   // @@@ wert fuer awar eintragen
                    }
                    else {
                        sprintf(result, "%c %s", flags[0], match_info); // only first flag ( = species related)
                        if (selection_id) selection_id->insert(result, match_name);   // @@@ wert fuer awar eintragen

                        // storing probe data into linked lists
                        g_spd->probeSeq.push_back(std::string(match_info));
                        g_spd->probeSpecies.push_back(std::string(match_name));
                    }
                    mcount++;
                }

                free(gene_str);
            }

            if (error) error = GBS_static_string(error); // make static copy (error may be freed by delete parser)
            delete parser;
            free(result);

            GB_pop_transaction(gb_main);
        }

        if (error) {
            if (event_param) event_param->hits_summary = "[none]"; // clear hits
        }
        else {
            if (unknown_species_count>0) {
                error = GBS_global_string("%li matches hit unknown species -- PT-server is out-of-date or build upon a different database", unknown_species_count);
            }
            if (unknown_gene_count>0 && !error) {
                error = GBS_global_string("%li matches hit unknown genes -- PT-server is out-of-date or build upon a different database", unknown_gene_count);
            }

            if (selection_id) {     // if !selection_id then probe match window is not opened
                pd_assert(g_spd);
                event_param->refresh_sai_display = true; // want refresh of SAI/Probe window
            }
        }

        if (counter) *counter = mcount;

        aisc_close(PD.link, PD.com);
        PD.link = 0;

        if (selection_id) {
            const char *last_line                 = 0;
            if (error) last_line                  = GBS_global_string("****** Error: %s *******", error);
            else if (matches_truncated) last_line = "****** List is truncated *******";
            else        last_line                 = "****** End of List *******";

            selection_id->insert_default(last_line, "");
            selection_id->update();
        }

        free(bs.data);
    }
    return error;
}

static void probe_match_event_using_awars(AW_root *root, ProbeMatchEventParam *event_param) {
    if (allow_probe_match_event) {
        GB_ERROR error = probe_match_event(ProbeMatchSettings(root), event_param);
        aw_message_if(error);

        LocallyModify<bool> flag(allow_probe_match_event, false);

        if (event_param) {
            if (event_param->refresh_sai_display) {
                root->awar(AWAR_SPV_DB_FIELD_NAME)->touch(); // force refresh of SAI/Probe window
            }
            if (event_param->selection_id) {
                root->awar(AWAR_PD_MATCH_NHITS)->write_string(event_param->hits_summary.c_str()); // update hits in probe match window
            }
        }
        root->awar(AWAR_TREE_REFRESH)->touch(); // refresh tree
    }
}

static void probe_match_all_event(AW_window *aww, AW_selection_list *iselection_id, GBDATA *gb_main) {
    AW_selection_list_iterator selentry(iselection_id);
    arb_progress               progress("Matching all resolved strings", iselection_id->size());
    ProbeMatchSettings         matchSettings(aww->get_root());
    const bool                 got_result = selentry;

    while (selentry) {
        const char *entry          = selentry.get_value(); // @@@ rename -> probe
        matchSettings.targetString = entry;

        int                  counter = -1;
        ProbeMatchEventParam match_event(gb_main, &counter);
        GB_ERROR             error   = probe_match_event(matchSettings, &match_event);

        // write # of hits to list entries:
        {
#define DISP_FORMAT "%7i %s"
            const char *displayed;
            if (error) {
                displayed = GBS_global_string(DISP_FORMAT " (Error: %s)", 0, entry, error);
            }
            else {
                displayed = GBS_global_string(DISP_FORMAT, counter, entry);
            }
            selentry.set_displayed(displayed);
#undef DISP_FORMAT
        }

        ++selentry;
        progress.inc();
    }

    if (got_result) {
        iselection_id->sort(true, true);
        iselection_id->update();
    }
}

static void resolved_probe_chosen(AW_root *root) {
    char *string = root->awar(AWAR_PD_MATCH_RESOLVE)->read_string();
    root->awar(AWAR_TARGET_STRING)->write_string(string);
}

static void selected_match_changed_cb(AW_root *root) {
    // this gets called when ever the selected probe match changes
    char *temp;
    char *selected_match = root->awar(AWAR_PD_SELECTED_MATCH)->read_string();

    if (strchr(selected_match, '/')) { // "organism/gene"
        temp = strtok(selected_match, "/");
        root->awar(AWAR_SPECIES_NAME)->write_string(temp);
        temp = strtok(NULL, " /\n");
        root->awar(AWAR_GENE_NAME)->write_string(temp);
    }
    else {
        root->awar(AWAR_SPECIES_NAME)->write_string(selected_match);
    }

    {
        LocallyModify<bool> flag(allow_probe_match_event, false); // avoid recursion
        root->awar(AWAR_TARGET_STRING)->touch(); // forces editor to jump to probe match in gene
    }

    free(selected_match);
}

static void probelength_changed_cb(AW_root *root, bool maxChanged) {
    static bool avoid_recursion = false;
    if (!avoid_recursion) {
        AW_awar *awar_minl = root->awar(AWAR_PD_DESIGN_MIN_LENGTH);
        AW_awar *awar_maxl = root->awar(AWAR_PD_DESIGN_MAX_LENGTH);

        int minl = awar_minl->read_int();
        int maxl = awar_maxl->read_int();

        if (minl>maxl) {
            if (maxChanged) awar_minl->write_int(maxl);
            else            awar_maxl->write_int(minl);
        }
    }
}

static void minmax_awar_pair_changed_cb(AW_root *root, bool maxChanged, const char *minAwarName, const char *maxAwarName) {
    static bool avoid_recursion = false;
    if (!avoid_recursion) {
        LocallyModify<bool> flag(avoid_recursion, true);

        AW_awar *awar_min = root->awar(minAwarName);
        AW_awar *awar_max = root->awar(maxAwarName);

        float currMin = awar_min->read_float();
        float currMax = awar_max->read_float();

        if (currMin>currMax) { // invalid -> correct
            if (maxChanged) awar_min->write_float(currMax);
            else            awar_max->write_float(currMin);
        }
    }
}
static void gc_minmax_changed_cb(AW_root *root, bool maxChanged) {
    minmax_awar_pair_changed_cb(root, maxChanged, AWAR_PD_DESIGN_MIN_GC, AWAR_PD_DESIGN_MAX_GC);
}
static void temp_minmax_changed_cb(AW_root *root, bool maxChanged) {
    minmax_awar_pair_changed_cb(root, maxChanged, AWAR_PD_DESIGN_MIN_TEMP, AWAR_PD_DESIGN_MAX_TEMP);
}

void create_probe_design_variables(AW_root *root, AW_default props, AW_default db) {
    PD.win = 0;        // design result window not created

    root->awar_string(AWAR_SPECIES_NAME,         "", props);
    root->awar_string(AWAR_PD_SELECTED_MATCH,    "", props)->add_callback(selected_match_changed_cb);
    root->awar_float (AWAR_PD_DESIGN_EXP_DTEDGE, .5, props);
    root->awar_float (AWAR_PD_DESIGN_EXP_DT,     .5, props);

    double default_bonds[16] = {
        0.0, 0.0, 0.5, 1.1,
        0.0, 0.0, 1.5, 0.0,
        0.5, 1.5, 0.4, 0.9,
        1.1, 0.0, 0.9, 0.0
    };

    for (int i=0; i<16; i++) {
        root->awar_float(bond_awar_name(i), default_bonds[i], props)->set_minmax(0, 3.0);
    }
    root->awar_float(AWAR_PD_COMMON_EXP_SPLIT,  .5, props);
    root->awar_float(AWAR_PD_DESIGN_EXP_DTEDGE, .5, props);
    root->awar_float(AWAR_PD_DESIGN_EXP_DT,     .5, props);

    root->awar_int  (AWAR_PD_DESIGN_CLIPRESULT, 50,   props)->set_minmax(0, 1000);
    root->awar_int  (AWAR_PD_DESIGN_MISHIT,     0,    props)->set_minmax(0, 100000);
    root->awar_int  (AWAR_PD_DESIGN_MAXBOND,    4,    props)->set_minmax(0, 20);
    root->awar_float(AWAR_PD_DESIGN_MINTARGETS, 50.0, props)->set_minmax(0, 100);

    AW_awar *awar_min_len = root->awar_int(AWAR_PD_DESIGN_MIN_LENGTH, 18, props);
    awar_min_len->set_minmax(DOMAIN_MIN_LENGTH, 100)->add_callback(makeRootCallback(probelength_changed_cb, false));
    root->awar_int(AWAR_PD_DESIGN_MAX_LENGTH, awar_min_len->read_int(), props)->set_minmax(DOMAIN_MIN_LENGTH, 100)->add_callback(makeRootCallback(probelength_changed_cb, true));

    root->awar_float(AWAR_PD_DESIGN_MIN_TEMP,     30.0,   props)->set_minmax(0, 1000)->add_callback(makeRootCallback(temp_minmax_changed_cb, false));
    root->awar_float(AWAR_PD_DESIGN_MAX_TEMP,     100.0,  props)->set_minmax(0, 1000)->add_callback(makeRootCallback(temp_minmax_changed_cb, true));

    root->awar_float(AWAR_PD_DESIGN_MIN_GC, 50.0,  props)->add_callback(makeRootCallback(gc_minmax_changed_cb, false));
    root->awar_float(AWAR_PD_DESIGN_MAX_GC, 100.0, props)->add_callback(makeRootCallback(gc_minmax_changed_cb, true));

    gc_minmax_changed_cb(root, false);
    gc_minmax_changed_cb(root, true);

    root->awar_string(AWAR_PD_DESIGN_MIN_ECOLIPOS, "", props);
    root->awar_string(AWAR_PD_DESIGN_MAX_ECOLIPOS, "", props);

    root->awar_int(AWAR_PT_SERVER,      0, props);
    root->awar_int(AWAR_PD_DESIGN_GENE, 0, props);

    root->awar_int(AWAR_PD_MATCH_MARKHITS,   1, props);
    root->awar_int(AWAR_PD_MATCH_SORTBY,     0, props);
    root->awar_int(AWAR_PD_MATCH_WRITE2TMP,  0, props);
    root->awar_int(AWAR_PD_MATCH_COMPLEMENT, 0, props);

    root->awar_int   (AWAR_MAX_MISMATCHES, 0, db)->set_minmax(0, 200);
    root->awar_string(AWAR_TARGET_STRING,  0, db);

    root->awar_string(AWAR_PD_MATCH_NHITS,      "[none]", props);
    root->awar_int   (AWAR_PD_MATCH_NMATCHES,   1,        props);
    root->awar_int   (AWAR_PD_MATCH_LIM_NMATCH, 4,        props);
    root->awar_int   (AWAR_PD_MATCH_MAX_RES,    1000000,  props);

    root->awar_int(AWAR_PD_MATCH_AUTOMATCH, 0, props)->add_callback(auto_match_changed);

    root->awar_string(AWAR_PD_MATCH_RESOLVE, "", props)->add_callback(resolved_probe_chosen);
    root->awar_string(AWAR_ITARGET_STRING,   "", db);

    root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER,    0, db);
    root->awar_int(AWAR_PROBE_CREATE_GENE_SERVER, 0, db);

    root->awar_string(AWAR_SPV_SAI_2_PROBE,    "",     db); // name of SAI selected in list
    root->awar_string(AWAR_SPV_DB_FIELD_NAME,  "name", db); // name of displayed species field
    root->awar_int   (AWAR_SPV_DB_FIELD_WIDTH, 10,     db); // width of displayed species field
    root->awar_string(AWAR_SPV_ACI_COMMAND,    "",     db); // User defined or pre-defined ACI command to display

    root->awar_int(AWAR_PC_MATCH_NHITS,     0, db);
    root->awar_int(AWAR_PC_AUTO_MATCH,      0, props);

    root->awar_string(AWAR_PC_TARGET_STRING,  "", db)->set_srt(REPLACE_TARGET_CONTROL_CHARS);
    root->awar_string(AWAR_PC_TARGET_NAME,    "", db)->set_srt(REPLACE_TARGET_CONTROL_CHARS);
    root->awar_string(AWAR_PC_SELECTED_PROBE, "", db);

    root->awar_float(AWAR_PC_MATCH_WIDTH, 1.0, db)->set_minmax(0.01, 100.0);
    root->awar_float(AWAR_PC_MATCH_BIAS,  0.0, db)->set_minmax(-1.0, 1.0);

    root->awar_float(AWAR_PC_MISMATCH_THRESHOLD, 0.0, db)->set_minmax(0, 100); // Note: limits will be modified by probe_match_with_specificity_event

    float default_weights[16] = {0.0};
    float default_width = 1.0;
    float default_bias  = 0.0;

    ArbProbeCollection& g_probe_collection = get_probe_collection();
    g_probe_collection.getParameters(default_weights, default_width, default_bias);
    g_probe_collection.clear();

    char buffer[256] = {0};

    for (int i = 0; i < 16 ; i++) {
        sprintf(buffer, AWAR_PC_MATCH_WEIGHTS"%i", i);
        AW_awar *awar = root->awar_float(buffer, 0.0, db);

        awar->set_minmax(0, 10);
        default_weights[i] = awar->read_float();
    }

    g_probe_collection.setParameters(default_weights, default_width, default_bias);

    // read probes from DB and add them to collection
    {
        AW_awar *awar_current = root->awar_string(AWAR_PC_CURRENT_COLLECTION, "", db);
        char    *current      = awar_current->read_string();

        if (current && current[0]) {
            // Note: target sequences/names do not contain '#'/':' (see REPLACE_TARGET_CONTROL_CHARS)
            ConstStrArray probes;
            GBT_splitNdestroy_string(probes, current, "#", true);

            for (size_t i = 0; i<probes.size(); ++i) {
                const char *probe = probes[i];
                const char *colon = strchr(probe, ':');

                if (colon) {
                    char       *name = GB_strpartdup(probe, colon-1);
                    const char *seq  = colon+1;
                    g_probe_collection.add(name, seq);
                    free(name);
                }
                else {
                    aw_message(GBS_global_string("Saved probe ignored: has wrong format ('%s', expected 'name:seq')", probe));
                }
            }
        }
        free(current);
    }
    root->awar_string(AWAR_SPV_SELECTED_PROBE, "",     db); // For highlighting the selected PROBE
}

static AW_window *create_probe_expert_window(AW_root *root, bool for_design) {
    AW_window_simple *aws = new AW_window_simple;
    if (for_design) {
        aws->init(root, "PD_exp", "Probe Design (Expert)");
        aws->load_xfig("pd_spec.fig");
    }
    else {
        aws->init(root, "PM_exp", "Probe Match (Expert)");
        aws->load_xfig("pm_spec.fig");
    }

    aws->label_length(30);
    aws->button_length(10);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback(for_design ? "pd_spec_param.hlp" : "pm_spec_param.hlp")); // uses_hlp_res("pm_spec_param.hlp", "pd_spec_param.hlp"); see ../SOURCE_TOOLS/check_resources.pl@uses_hlp_res
    aws->at("help");
    aws->create_button("HELP", "HELP", "C");

    for (int i=0; i<16; i++) { // bond matrix
        char bondAt[3];
        sprintf(bondAt, "%i", i);
        aws->at(bondAt);

        aws->create_input_field(bond_awar_name(i), 4);
    }

    aws->sens_mask(AWM_EXP);
    aws->at("split"); aws->create_input_field(AWAR_PD_COMMON_EXP_SPLIT,  6);

    if (for_design) {
        aws->at("dt_edge"); aws->create_input_field(AWAR_PD_DESIGN_EXP_DTEDGE, 6);
        aws->at("dt");      aws->create_input_field(AWAR_PD_DESIGN_EXP_DT,     6);
        aws->sens_mask(AWM_ALL);
    }
    else {
        aws->at("nmatches");   aws->create_input_field(AWAR_PD_MATCH_NMATCHES,   3);
        aws->at("lim_nmatch"); aws->create_input_field(AWAR_PD_MATCH_LIM_NMATCH, 3);
        aws->sens_mask(AWM_ALL);
        aws->at("max_res");    aws->create_input_field(AWAR_PD_MATCH_MAX_RES,    14);
    }

    return aws;
}

static AWT_config_mapping_def probe_design_mapping_def[] = {
    // main window:
    { AWAR_PD_DESIGN_CLIPRESULT,   "clip" },
    { AWAR_PD_DESIGN_MISHIT,       "mishit" },
    { AWAR_PD_DESIGN_MAXBOND,      "maxbond" },
    { AWAR_PD_DESIGN_MINTARGETS,   "mintarget" },
    { AWAR_PD_DESIGN_MIN_LENGTH,   "probelen" },
    { AWAR_PD_DESIGN_MAX_LENGTH,   "probemaxlen" },
    { AWAR_PD_DESIGN_MIN_TEMP,     "mintemp" },
    { AWAR_PD_DESIGN_MAX_TEMP,     "maxtemp" },
    { AWAR_PD_DESIGN_MIN_GC,       "mingc" },
    { AWAR_PD_DESIGN_MAX_GC,       "maxgc" },
    { AWAR_PD_DESIGN_MIN_ECOLIPOS, "minecoli" },
    { AWAR_PD_DESIGN_MAX_ECOLIPOS, "maxecoli" },
    { AWAR_PD_DESIGN_GENE,         "gene" },

    // expert window:
    { AWAR_PD_DESIGN_EXP_DTEDGE,   "dtedge" },
    { AWAR_PD_DESIGN_EXP_DT,       "dt" },

    { 0, 0 }
};

static void setup_probe_config(AWT_config_definition& cdef, const AWT_config_mapping_def *mapping) {
    cdef.add(mapping);

    // entries common for both expert windows (design + match)
    cdef.add(AWAR_PD_COMMON_EXP_SPLIT, "split");
    for (int i = 0; i<16; ++i) {
        cdef.add(bond_awar_name(i), "bond", i);
    }
}

AW_window *create_probe_design_window(AW_root *root, GBDATA *gb_main) {
    bool is_genom_db;
    {
        GB_transaction ta(gb_main);
        is_genom_db = GEN_is_genome_db(gb_main, -1);
    }

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "PROBE_DESIGN", "PROBE DESIGN");

    aws->load_xfig("pd_main.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("probedesign.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->callback(makeWindowCallback(probe_design_event, gb_main));
    aws->at("design");
    aws->highlight();
    aws->create_button("GO", "GO", "G");

    aws->callback(makeWindowCallback(popup_probe_design_result_window, gb_main));
    aws->at("result");
    aws->create_button("RESULT", "RESULT", "S");

    aws->callback(makeCreateWindowCallback(create_probe_expert_window, true));
    aws->at("expert");
    aws->create_button("EXPERT", "EXPERT", "S");

    aws->at("pt_server");
    aws->label("PT-Server:");
    awt_create_PTSERVER_selection_button(aws, AWAR_PT_SERVER);

    aws->at("lenout");   aws->create_input_field(AWAR_PD_DESIGN_CLIPRESULT, 6);
    aws->at("mishit");   aws->create_input_field(AWAR_PD_DESIGN_MISHIT,     6);
    aws->sens_mask(AWM_EXP);
    aws->at("maxbonds"); aws->create_input_field(AWAR_PD_DESIGN_MAXBOND,    6);
    aws->sens_mask(AWM_ALL);
    aws->at("minhits");  aws->create_input_field(AWAR_PD_DESIGN_MINTARGETS, 6);

    aws->at("minlen"); aws->create_input_field(AWAR_PD_DESIGN_MIN_LENGTH,   5);
    aws->at("maxlen"); aws->create_input_field(AWAR_PD_DESIGN_MAX_LENGTH,   5);
    aws->at("mint");   aws->create_input_field(AWAR_PD_DESIGN_MIN_TEMP,     5);
    aws->at("maxt");   aws->create_input_field(AWAR_PD_DESIGN_MAX_TEMP,     5);
    aws->at("mingc");  aws->create_input_field(AWAR_PD_DESIGN_MIN_GC,       5);
    aws->at("maxgc");  aws->create_input_field(AWAR_PD_DESIGN_MAX_GC,       5);
    aws->at("minpos"); aws->create_input_field(AWAR_PD_DESIGN_MIN_ECOLIPOS, 5);
    aws->at("maxpos"); aws->create_input_field(AWAR_PD_DESIGN_MAX_ECOLIPOS, 5);

    if (is_genom_db) {
        aws->at("gene");
        aws->label("Gene probes?");
        aws->create_toggle(AWAR_PD_DESIGN_GENE);
    }

    aws->at("save");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "probe_design", makeConfigSetupCallback(setup_probe_config, probe_design_mapping_def));

    return aws;
}

// -------------------------------------------------------------------

inline void my_strupr(char *s) {
    pd_assert(s);
    for (int i=0; s[i]; i++) {
        s[i] = toupper(s[i]);
    }
}

static void resolve_IUPAC_target_string(AW_root *root, AW_selection_list *selection_id, GBDATA *gb_main) {
    selection_id->clear();

    GB_alignment_type ali_type = GBT_get_alignment_type(gb_main, GBT_get_default_alignment(gb_main));

    GB_ERROR err = 0;
    if (ali_type != GB_AT_RNA && ali_type!=GB_AT_DNA) {
        err = "Wrong alignment type";
        GB_clear_error();
    }
    else {
        int   index   = ali_type==GB_AT_RNA ? 1 : 0;
        char *istring = root->awar(AWAR_ITARGET_STRING)->read_string();

        if (istring && istring[0]) { // contains sth?
            my_strupr(istring);

            int bases_to_resolve = 0;
            char *istr = istring;
            int istring_length = strlen(istring);

            for (;;) {
                char i = *istr++;
                if (!i) break;
                if (i=='?') continue; // ignore '?'

                int idx = i-'A';
                if (idx<0 || idx>=26 || iupac::nuc_group[idx][index].members==0) {
                    err = GBS_global_string("Illegal character '%c' in IUPAC-String", i);
                    break;
                }

                if (iupac::nuc_group[idx][index].count>1) {
                    bases_to_resolve++;
                }
            }

            if (!err) {
                int *offsets_to_resolve = new int[bases_to_resolve];
                int resolutions = 1;
                {
                    istr = istring;
                    int offset = 0;
                    int offset_count = 0;
                    for (;;) {
                        char i = *istr++;
                        if (!i) break;

                        if (i!='?') {
                            int idx = iupac::to_index(i);
                            pd_assert(iupac::nuc_group[idx][index].members);

                            if (iupac::nuc_group[idx][index].count>1) {
                                offsets_to_resolve[offset_count++] = offset; // store string offsets of non-unique base-codes
                                resolutions *= iupac::nuc_group[idx][index].count; // calc # of resolutions
                            }
                        }
                        offset++;
                    }
                }

                {
                    int *resolution_idx = new int[bases_to_resolve];
                    int *resolution_max_idx = new int[bases_to_resolve];
                    {
                        int i;
                        for (i=0; i<bases_to_resolve; i++) {
                            resolution_idx[i] = 0;
                            int idx = iupac::to_index(istring[offsets_to_resolve[i]]);
                            resolution_max_idx[i] = iupac::nuc_group[idx][index].count-1;
                        }
                    }

                    char *buffer = new char[istring_length+1];
                    int not_last = resolutions-1;

                    for (;;) {
                        // create string according to resolution_idx[]:
                        int i;

                        memcpy(buffer, istring, istring_length+1);
                        for (i=0; i<bases_to_resolve; i++) {
                            int off = offsets_to_resolve[i];
                            int idx = iupac::to_index(istring[off]);

                            pd_assert(iupac::nuc_group[idx][index].members);
                            buffer[off] = iupac::nuc_group[idx][index].members[resolution_idx[i]];
                        }

                        selection_id->insert(buffer, buffer);
                        not_last--;

                        // permute indices:
                        int nidx = bases_to_resolve-1;
                        int done = 0;
                        while (!done && nidx>=0) {
                            if (resolution_idx[nidx]<resolution_max_idx[nidx]) {
                                resolution_idx[nidx]++;
                                done = 1;
                                break;
                            }
                            nidx--;
                        }
                        if (!done) break; // we did all permutations!

                        nidx++; // do not touch latest incremented index
                        while (nidx<bases_to_resolve) resolution_idx[nidx++] = 0; // zero all other indices
                    }

                    delete [] buffer;
                    delete [] resolution_max_idx;
                    delete [] resolution_idx;

                    pd_assert(!err);
                    selection_id->insert_default("", "");
                }

                delete [] offsets_to_resolve;
            }
        }
        else { // empty input
            selection_id->insert_default("", "");
        }
    }
    if (err) selection_id->insert_default(err, "");
    selection_id->update();
}

enum ModMode { TS_MOD_CLEAR, TS_MOD_REV_COMPL, TS_MOD_COMPL };

static void modify_target_string(AW_window *aww, GBDATA *gb_main, ModMode mod_mode) {
    AW_root  *root          = aww->get_root();
    char     *target_string = root->awar(AWAR_TARGET_STRING)->read_string();
    GB_ERROR  error         = 0;

    if (mod_mode == TS_MOD_CLEAR) target_string[0] = 0;
    else {
        GB_transaction    ta(gb_main);
        GB_alignment_type ali_type = GBT_get_alignment_type(gb_main, GBT_get_default_alignment(gb_main));
        if (ali_type == GB_AT_UNKNOWN) {
            error = GB_await_error();
        }
        else if (mod_mode == TS_MOD_REV_COMPL) {
            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "reverse-complement");
            if (!error) GBT_reverseComplementNucSequence(target_string, strlen(target_string), T_or_U);
        }
        else if (mod_mode == TS_MOD_COMPL) {
            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "complement");
            if (!error) freeset(target_string, GBT_complementNucSequence(target_string, strlen(target_string), T_or_U));
        }
    }

    if (error) aw_message(error);
    else root->awar(AWAR_TARGET_STRING)->write_string(target_string);
    free(target_string);
}

static void clear_itarget(AW_window *aww) {
    aww->get_root()->awar(AWAR_ITARGET_STRING)->write_string("");
}

static AW_window *create_IUPAC_resolve_window(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "PROBE_MATCH_RESOLVE_IUPAC", "Resolve IUPAC for Probe Match");
    aws->load_xfig("pd_match_iupac.fig");

    aws->button_length(11);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("pd_match_iupac.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("istring");
    aws->create_input_field(AWAR_ITARGET_STRING, 32);

    aws->at("iresult");
    AW_selection_list *iselection_id;
    iselection_id = aws->create_selection_list(AWAR_PD_MATCH_RESOLVE, 32, 15, true);
    iselection_id->insert_default("---empty---", "");

    // add callback for automatic decomposition of AWAR_ITARGET_STRING:
    RootCallback  upd_cb       = makeRootCallback(resolve_IUPAC_target_string, iselection_id, gb_main);
    AW_awar      *awar_itarget = root->awar(AWAR_ITARGET_STRING);
    awar_itarget->add_callback(upd_cb);
    root->awar(AWAR_DEFAULT_ALIGNMENT)->add_callback(upd_cb);

    aws->at("match_all");
    aws->callback(makeWindowCallback(probe_match_all_event, iselection_id, gb_main));
    aws->create_button("MATCH_ALL", "Match all", "M");

    aws->at("clear");
    aws->callback(clear_itarget);
    aws->create_button("CLEAR", "Clear", "r");

    aws->at("iupac_info");
    aws->callback(AWT_create_IUPAC_info_window);
    aws->create_button("IUPAC_INFO", "IUPAC info", "I");

    awar_itarget->touch(); // force initial refresh for saved value

    return aws;
}

static void popupSaiProbeMatchWindow(AW_window *aw, GBDATA *gb_main) {
    static AW_window *aw_saiProbeMatch = 0;

    if (!aw_saiProbeMatch) aw_saiProbeMatch = createSaiProbeMatchWindow(aw->get_root(), gb_main);
    if (g_spd) transferProbeData(g_spd); // transferring probe data to saiProbeMatch function

    aw_saiProbeMatch->activate();
}

static AWT_config_mapping_def probe_match_mapping_def[] = {
    // main window:
    { AWAR_TARGET_STRING,       "target" },
    { AWAR_PD_MATCH_COMPLEMENT, "complement" },
    { AWAR_PD_MATCH_MARKHITS,   "markhits" },
    { AWAR_PD_MATCH_WRITE2TMP,  "writetmp" },
    { AWAR_PD_MATCH_AUTOMATCH,  "automatch" },

    // expert window:
    { AWAR_PD_MATCH_NMATCHES,   "nmatches" },
    { AWAR_PD_MATCH_LIM_NMATCH, "limitnmatch" },
    { AWAR_PD_MATCH_MAX_RES,    "maxresults" },

    { 0, 0 }
};


AW_window *create_probe_match_window(AW_root *root, GBDATA *gb_main) {
    static AW_window_simple *aws = 0; // the one and only probe match window
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "PROBE_MATCH", "PROBE MATCH");
        aws->load_xfig("pd_match.fig");

        aws->auto_space(5, 5);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("probematch.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("string");
        aws->create_input_field(AWAR_TARGET_STRING, 32);

        AW_selection_list *selection_id;
        aws->at("result");
        selection_id = aws->create_selection_list(AWAR_PD_SELECTED_MATCH, 110, 15, true);
        selection_id->insert_default("****** No results yet *******", "");  // if list is empty -> crashed if new species was selected in ARB_EDIT4

        static SmartPtr<TypedSelectionList> typed_selection = new TypedSelectionList("match", selection_id, "probe match", "probe_match");
        aws->at("print");
        aws->callback(makeWindowCallback(create_print_box_for_selection_lists, &*typed_selection));
        aws->create_button("PRINT", "PRINT", "P");

        aws->at("matchSai");
        aws->help_text("saiProbe.hlp");
        aws->callback(makeWindowCallback(popupSaiProbeMatchWindow, gb_main));
        aws->create_button("MATCH_SAI", "Match SAI", "S");

        aws->callback(makeCreateWindowCallback(create_probe_expert_window, false));
        aws->at("expert");
        aws->create_button("EXPERT", "EXPERT", "X");

        aws->at("pt_server");
        awt_create_PTSERVER_selection_button(aws, AWAR_PT_SERVER);

        aws->at("complement");
        aws->create_toggle(AWAR_PD_MATCH_COMPLEMENT);

        aws->at("mark");
        aws->create_toggle(AWAR_PD_MATCH_MARKHITS);

        aws->at("weighted");
        aws->create_toggle(AWAR_PD_MATCH_SORTBY);

        aws->at("mismatches");
        aws->create_input_field_with_scaler(AWAR_MAX_MISMATCHES, 5, 200, AW_SCALER_EXP_LOWER);

        aws->at("tmp");
        aws->create_toggle(AWAR_PD_MATCH_WRITE2TMP);

        aws->at("nhits");
        aws->create_button(0, AWAR_PD_MATCH_NHITS);

        aws->button_length(9);

        ProbeMatchEventParam *event_param = new ProbeMatchEventParam(gb_main, selection_id);
        aws->callback(RootAsWindowCallback::simple(probe_match_event_using_awars, event_param));
        aws->at("match");
        aws->create_button("MATCH", "MATCH", "D");

        aws->at("auto");
        aws->label("Auto");
        aws->create_toggle(AWAR_PD_MATCH_AUTOMATCH);
        enable_auto_match_cb(root, event_param);

        aws->callback(makeWindowCallback(modify_target_string, gb_main, TS_MOD_CLEAR));
        aws->at("clear");
        aws->create_button("CLEAR", "Clear", "0");

        aws->callback(makeWindowCallback(modify_target_string, gb_main, TS_MOD_REV_COMPL));
        aws->at("revcompl");
        aws->create_button("REVCOMPL", "RevCompl", "R");

        aws->callback(makeWindowCallback(modify_target_string, gb_main, TS_MOD_COMPL));
        aws->at("compl");
        aws->create_button("COMPL", "Compl", "C");

        aws->callback(makeCreateWindowCallback(create_IUPAC_resolve_window, gb_main));
        aws->at("iupac");
        aws->create_button("IUPAC", "IUPAC", "I");

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "probe_match", makeConfigSetupCallback(setup_probe_config, probe_match_mapping_def));
    }
    return aws;
}

static void pd_start_pt_server(AW_window *aww) {
    const char *server_tag = GBS_ptserver_tag(aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int());
    arb_progress progress("Connecting PT-server");
    GB_ERROR error = arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag);
    if (error) aw_message(error);
}

static void pd_kill_pt_server(AW_window *aww, bool kill_all) {
    if (aw_ask_sure("kill_ptserver",
                    GBS_global_string("Are you sure to stop %s", kill_all ? "all servers" : "that server")))
    {
        long min = 0;
        long max = 0;

        if (kill_all) {
            const char * const *pt_servers = GBS_get_arb_tcp_entries("ARB_PT_SERVER*");
            while (pt_servers[max]) max++;
        }
        else {
            min = max = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int(); // selected server
        }

        arb_progress progress("Stopping PT-servers..", max-min+1);
        GB_ERROR     error = NULL;

        for (int i = min; i <= max && !error;  i++) {
            char *choice = GBS_ptserver_id_to_choice(i, 0);
            if (!choice) {
                error = GB_await_error();
            }
            else {
                progress.subtitle(GBS_global_string("Trying to stop '%s'", choice));

                const char *server_tag = GBS_ptserver_tag(i);
                GB_ERROR    kill_error = arb_look_and_kill_server(AISC_MAGIC_NUMBER, server_tag);

                if (kill_error) aw_message(GBS_global_string("Could not stop '%s' (Reason: %s)", choice, kill_error));
                else aw_message(GBS_global_string("Stopped '%s'", choice));

                free(choice);
            }
            progress.inc_and_check_user_abort(error);
        }
        progress.done();
        aw_message_if(error);
    }
}

static const char *ptserver_directory_info_command(const char *dirname, const char *directory) {
    return GBS_global_string("echo 'Contents of directory %s:'; echo; (cd \"%s\"; ls -hl); echo; "
                             "echo 'Available disk space in %s:'; echo; df -h \"%s\"; echo; ",
                             dirname, directory,
                             dirname, directory);

}

static void pd_query_pt_server(AW_window *aww) {
    const char *server_tag = GBS_ptserver_tag(aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int());

    GBS_strstruct *strstruct = GBS_stropen(1024);
    GBS_strcat(strstruct, ptserver_directory_info_command("ARBHOME/lib/pts", "$ARBHOME/lib/pts"));

    const char *ARB_LOCAL_PTS = ARB_getenv_ignore_empty("ARB_LOCAL_PTS");
    if (ARB_LOCAL_PTS) GBS_strcat(strstruct, ptserver_directory_info_command("ARB_LOCAL_PTS", "$ARB_LOCAL_PTS")); // only defined if called via 'arb'-script
    else               GBS_strcat(strstruct, ptserver_directory_info_command("HOME/.arb_pts", "${HOME}/.arb_pts"));

    GBS_strcat(strstruct, "echo 'Running ARB programs:'; echo; ");

    GB_ERROR error = NULL;
    {
        const char *socketid = GBS_read_arb_tcp(server_tag);
        if (!socketid) {
            error = GB_await_error();
        }
        else {
            char *arb_who = createCallOnSocketHost(socketid, "$ARBHOME/bin/", "arb_who", WAIT_FOR_TERMINATION, NULL);
            GBS_strcat(strstruct, arb_who);
            free(arb_who);
        }
    }

    char *sys         = GBS_strclose(strstruct);
    if (!error) error = GB_xcmd(sys, true, false);
    free(sys);

    aw_message_if(error);
}

static void pd_export_pt_server(AW_window *aww, GBDATA *gb_main) {
    AW_root  *awr   = aww->get_root();
    GB_ERROR  error = 0;

    bool create_gene_server = awr->awar(AWAR_PROBE_CREATE_GENE_SERVER)->read_int();
    {
        GB_transaction ta(gb_main);
        if (create_gene_server && !GEN_is_genome_db(gb_main, -1)) create_gene_server = false;

        // check alignment first
        if (create_gene_server) {
            GBDATA *gb_ali     = GBT_get_alignment(gb_main, GENOM_ALIGNMENT);
            if (!gb_ali) error = GB_await_error();
        }
        else { // normal pt server
            char              *use     = GBT_get_default_alignment(gb_main);
            GB_alignment_type  alitype = GBT_get_alignment_type(gb_main, use);
            if (alitype == GB_AT_AA) error = "The PT-server does only support RNA/DNA sequence data";
            free(use);
        }
    }

    long        server_num = awr->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    const char *server_tag  = GBS_ptserver_tag(server_num);

    if (!error &&
        aw_question("update_ptserver",
                    "This function will send your loaded database to the pt_server,\n"
                    "which will need a long time (up to several hours) to analyse the data.\n"
                    "Until the new server has analyzed all data, no server functions are available.\n\n"
                    "Note 1: You must have the write permissions to do that ($ARBHOME/lib/pts/xxx))\n"
                    "Note 2: The server will do the job in background,\n"
                    "        quitting this program won't affect the server",
                    "Cancel,Do it"))
    {
        arb_progress progress("Updating PT-server");
        progress.subtitle("Stopping PT-server");
        arb_look_and_kill_server(AISC_MAGIC_NUMBER, server_tag);

        const char *ipPort = GBS_read_arb_tcp(server_tag);
        const char *file   = NULL;
        if (!ipPort) error = GB_await_error();
        else {
            file = GBS_scan_arb_tcp_param(ipPort, "-d");
            GBS_add_ptserver_logentry(GBS_global_string("Started build of '%s'", file));

            char *db_name = awr->awar(AWAR_DB_PATH)->read_string();
            GBS_add_ptserver_logentry(GBS_global_string("Exporting DB '%s'", db_name));
            free(db_name);
        }

        if (!error) {
            progress.subtitle("Exporting the database");
            {
                const char *mode = GB_supports_mapfile() ? "mbf" : "bf"; // save PT-server database with Fastload file (if supported)
                if (create_gene_server) {
                    char *temp_server_name = GBS_string_eval(file, "*.arb=*_temp.arb", 0);
                    error = GB_save_as(gb_main, temp_server_name, mode);

                    if (!error) {
                        // convert database (genes -> species)
                        progress.subtitle("Preparing DB for gene PT server");
                        GBS_add_ptserver_logentry("Preparing DB for gene PT server");
                        char *command = GBS_global_string_copy("$ARBHOME/bin/arb_gene_probe %s %s", temp_server_name, file);
                        error = GBK_system(command);
                        if (error) error = GBS_global_string("Couldn't convert database for gene pt server\n(Reason: %s)", error);
                        free(command);
                    }
                    free(temp_server_name);
                }
                else { // normal pt_server
                    error = GB_save_as(gb_main, file, mode);
                }
            }
        }

        if (!error) { // set pt-server database file to same permissions as pts directory
            char *dir = const_cast<char*>(strrchr(file, '/'));
            if (dir) {
                *dir = 0;
                long modi = GB_mode_of_file(file);
                *dir = '/';
                modi &= 0666;
                error = GB_set_mode_of_file(file, modi);
            }
        }

        if (!error) {
            progress.subtitle("Start PT-server (builds in background)");
            error = arb_start_server(server_tag, 1);
        }
    }
    if (error) aw_message(error);
}

static void pd_view_pt_log(AW_window *) {
    AW_edit(GBS_ptserver_logname());
}

AW_window *create_probe_admin_window(AW_root *root, GBDATA *gb_main) {
    bool is_genom_db;
    {
        GB_transaction ta(gb_main);
        is_genom_db = GEN_is_genome_db(gb_main, -1);
    }

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "PT_SERVER_ADMIN", "PT_SERVER ADMIN");

    aws->load_xfig("pd_admin.fig");


    aws->callback(makeHelpCallback("probeadmin.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->button_length(18);

    aws->at("pt_server");
    awt_create_PTSERVER_selection_list(aws, AWAR_PROBE_ADMIN_PT_SERVER);

    aws->at("start");
    aws->callback(pd_start_pt_server);
    aws->create_button("START_SERVER", "Start server");

    aws->at("kill");
    aws->callback(makeWindowCallback(pd_kill_pt_server, false));
    aws->create_button("KILL_SERVER", "Stop server");

    aws->at("query");
    aws->callback(pd_query_pt_server);
    aws->create_button("CHECK_SERVER", "Check server");

    aws->at("kill_all");
    aws->callback(makeWindowCallback(pd_kill_pt_server, true));
    aws->create_button("KILL_ALL_SERVERS", "Stop all servers");

    aws->at("edit");
    aws->callback(makeWindowCallback(awt_edit_arbtcpdat_cb, gb_main));
    aws->create_button("CREATE_TEMPLATE", "Configure");

    aws->at("log");
    aws->callback(pd_view_pt_log);
    aws->create_button("EDIT_LOG", "View logfile");

    aws->at("export");
    aws->callback(makeWindowCallback(pd_export_pt_server, gb_main));
    aws->create_button("UPDATE_SERVER", "Build server");

    if (is_genom_db) {
        aws->at("gene_server");
        aws->label("Gene server?");
        aws->create_toggle(AWAR_PROBE_CREATE_GENE_SERVER);
    }

    return aws;
}

// ----------------------------------------------------------------------------

static AW_window *create_probe_match_specificity_control_window(AW_root *root) {
    static AW_window_simple *aws = NULL;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "MATCH_DISPLAYCONTROL", "MATCH DISPLAY CONTROL");

        aws->auto_space(10, 10);
        aws->label_length(35);

        const int FIELDSIZE  = 5;
        const int SCALERSIZE = 500;

        aws->label("Mismatch threshold");
        aws->create_input_field_with_scaler(AWAR_PC_MISMATCH_THRESHOLD, FIELDSIZE, SCALERSIZE);

        aws->at_newline();

        aws->label("Clade marked threshold");
        aws->create_input_field_with_scaler(AWAR_DTREE_GROUP_MARKED_THRESHOLD, FIELDSIZE, SCALERSIZE);

        aws->at_newline();

        aws->label("Clade partially marked threshold");
        aws->create_input_field_with_scaler(AWAR_DTREE_GROUP_PARTIALLY_MARKED_THRESHOLD, FIELDSIZE, SCALERSIZE);

        aws->at_newline();

        aws->callback(TREE_create_marker_settings_window);
        aws->create_autosize_button("MARKER_SETTINGS", "Marker display settings", "d");

        aws->at_newline();
    }

    return aws;
}

// ----------------------------------------------------------------------------

struct ArbPM_Context {
    AW_selection_list *probes_id;
    AWT_canvas        *ntw;

    ArbPM_Context()
        : probes_id(NULL),
          ntw(NULL)
    {}
};

static ArbPM_Context PM_Context;

static void       probe_match_update_probe_list(ArbPM_Context *pContext);
static AW_window *create_probe_collection_window(AW_root *root, ArbPM_Context *pContext);

struct ArbWriteFile_Context {
    FILE         *pFile;
    arb_progress *pProgress;
    int           nLastPercent;

    ArbWriteFile_Context()
        : pFile(NULL),
          pProgress(NULL),
          nLastPercent(0)
    {}
};

// ----------------------------------------------------------------------------

static bool probe_match_with_specificity_enum_callback(void *pVoidContext, const char *pResult, bool bIsComment, int nItem, int nItems) {
    ArbWriteFile_Context *pContext = (ArbWriteFile_Context*)pVoidContext;
    int                   nPercent = (int)(100.0 * nItem / nItems);

    if (!bIsComment) {
        if (pContext->nLastPercent != nPercent) {
            pContext->nLastPercent = nPercent;
            pContext->pProgress->inc();
        }
    }

    if (pContext->pFile != 0) {
        // Update status - put after matching cause matching writes its own status messages
        fprintf(pContext->pFile, "%s\n", pResult);
    }

    return pContext->pProgress->aborted();
}

// ----------------------------------------------------------------------------

static void probe_match_with_specificity_edit_event() {
    AW_edit(get_results_manager().resultsFileName(), 0, 0, 0);
}

// ----------------------------------------------------------------------------

class GetMatchesContext { // @@@ merge with ProbeCollDisplay?
    float mismatchThreshold;
    int   nProbes;

    typedef ArbMatchResultPtrByStringMultiMap          MatchMap;
    typedef ArbMatchResultPtrByStringMultiMapConstIter MatchMapIter;

    const MatchMap& results;
public:
    GetMatchesContext(float misThres, int numProbes)
        : mismatchThreshold(misThres),
          nProbes(numProbes),
          results(get_results_manager().resultsMap())
    {}

    void detect(std::string speciesName, NodeMarkers& matches) const {
        std::pair<MatchMapIter,MatchMapIter> iter = results.equal_range(speciesName);

        for (; iter.first != iter.second ; ++iter.first) {
            const ArbMatchResult *pMatchResult = iter.first->second;
            if (pMatchResult->weight() <= mismatchThreshold) {
                int nProbe = pMatchResult->index();
                matches.incMarker(nProbe);
            }
        }
        matches.incNodeSize();
    }
};

static SmartPtr<GetMatchesContext> getMatchesContext;

class ProbeCollDisplay : public MarkerDisplay {
    ArbProbe *find_probe(int markerIdx) const {
        const ArbProbePtrList&   rProbeList = get_probe_collection().probeList();
        ArbProbePtrListConstIter wanted     = rProbeList.begin();

        if (markerIdx>=0 && markerIdx<int(rProbeList.size())) advance(wanted, markerIdx);
        else wanted = rProbeList.end();

        return wanted == rProbeList.end() ? NULL : *wanted;
    }

public:
    ProbeCollDisplay(int numProbes)
        : MarkerDisplay(numProbes)
    {}

    // MarkerDisplay interface
    const char *get_marker_name(int markerIdx) const OVERRIDE {
        ArbProbe *probe = find_probe(markerIdx);
        return probe ? probe->name().c_str() : GBS_global_string("<invalid probeindex %i>", markerIdx);
    }
    void retrieve_marker_state(const char *speciesName, NodeMarkers& matches) OVERRIDE {
        getMatchesContext->detect(speciesName, matches);
    }

    void handle_click(int markerIdx, AW_MouseButton, AWT_graphic_exports&) OVERRIDE {
        // select probe in selection list
        ArbProbe *probe = find_probe(markerIdx);
        if (probe) AW_root::SINGLETON->awar(AWAR_PC_SELECTED_PROBE)->write_string(probe->sequence().c_str());
#if defined(DEBUG)
        else fprintf(stderr, "ProbeCollDisplay::handle_click: no probe found for markerIdx=%i\n", markerIdx);
#endif
    }
};

inline bool displays_probeColl_markers(MarkerDisplay *md) { return dynamic_cast<ProbeCollDisplay*>(md); }

static void refresh_matchedProbesDisplay_cb(AW_root *root, AWT_canvas *ntw) {
    // setup parameters for display of probe collection matches and trigger tree refresh
    LocallyModify<bool> flag(allow_probe_match_event, false);

    AWT_graphic_tree *agt     = DOWNCAST(AWT_graphic_tree*, ntw->gfx);
    bool              display = get_results_manager().hasResults();

    MarkerDisplay *markerDisplay = agt->get_marker_display();
    bool           redraw        = false;
    if (display) {
        size_t probesCount = get_probe_collection().probeList().size();
        getMatchesContext  = new GetMatchesContext(root->awar(AWAR_PC_MISMATCH_THRESHOLD)->read_float(), probesCount);

        if (displays_probeColl_markers(markerDisplay) && probesCount == size_t(markerDisplay->size())) {
            markerDisplay->flush_cache();
        }
        else {
            agt->set_marker_display(new ProbeCollDisplay(get_probe_collection().probeList().size()));
        }
        redraw = true;
    }
    else {
        if (displays_probeColl_markers(markerDisplay)) {
            agt->hide_marker_display();
            redraw = true;
        }
    }

    if (redraw) root->awar(AWAR_TREE_REFRESH)->touch();
}

// ----------------------------------------------------------------------------

static void probe_match_with_specificity_event(AW_root *root, AWT_canvas *ntw) {
    if (allow_probe_match_event) {
        GB_ERROR error = NULL;

        ProbeMatchSettings matchSettings(root);
        matchSettings.markHits  = 0;
        matchSettings.write2tmp = 0;

        ArbMatchResultsManager& g_results_manager = get_results_manager();
        g_results_manager.reset();

        // Using g_probe_collection instance of ArbProbeCollection, need to loop
        // through all the probes in the collect and then call probe_match_event,
        // collating the results as we go. The results can be obtained from the
        // g_spd->probeSeq list.
        const ArbProbePtrList& rProbeList = get_probe_collection().probeList();

        const int nItems      = rProbeList.size();
        int       nItem       = 1;
        int       nHits       = 0;
        int       nProbeIndex = 0;

        // This extra scope needed to ensure the arb_progress object is released
        // prior to the next one being used to show progress on write results to file
        {
            arb_progress progress("Matching probe collection", nItems);

            ArbProbeCollection& g_probe_collection = get_probe_collection();

            for (ArbProbePtrListConstIter ProbeIter = rProbeList.begin() ; ProbeIter != rProbeList.end() && !error; ++ProbeIter) {
                const ArbProbe *pProbe = *ProbeIter;

                if (pProbe != 0) {
                    int                nMatches;
                    ArbMatchResultSet *pResultSet = g_results_manager.addResultSet(pProbe);

                    // Update status - put after matching cause matching writes its own status messages
                    progress.subtitle(GBS_global_string("Matching probe %i of %i", nItem, nItems));

                    // Perform match on pProbe
                    matchSettings.targetString  = pProbe->sequence();
                    matchSettings.maxMismatches = pProbe->allowedMismatches();

                    int                   counter = -1;
                    ProbeMatchEventParam  match_event(ntw->gb_main, &counter);

                    error = probe_match_event(matchSettings, &match_event);

                    pResultSet->initialise(pProbe, nProbeIndex);
                    nProbeIndex++;

                    if (g_spd && g_spd->getHeadline() && !error) {
                        ProbeMatchParser parser(pProbe->sequence().c_str(), g_spd->getHeadline());

                        if (parser.get_error()) {
                            error = parser.get_error();
                        }
                        else {
                            int nStartFullName  = 0;
                            int nEndFullName    = 0;

                            parser.getColumnRange("fullname", &nStartFullName, &nEndFullName);

                            pResultSet->headline(g_spd->getHeadline(), nEndFullName);

                            // Collate match results
                            nMatches = g_spd->probeSeq.size();

                            for (int cn = 0 ; cn < nMatches && !error ; cn++) {
                                const std::string& sResult = g_spd->probeSeq[cn];

                                ParsedProbeMatch parsed(sResult.c_str(), parser);

                                if (parsed.get_error()) {
                                    error = parsed.get_error();
                                }
                                else {
                                    char       *pName      = parsed.get_column_content("name", true);
                                    char       *pFullName  = parsed.get_column_content("fullname", true);
                                    const char *pMatchPart = parsed.get_probe_region();

                                    pResultSet->add(pName,
                                                    pFullName,
                                                    pMatchPart,
                                                    sResult.c_str(),
                                                    g_probe_collection.matchWeighting());

                                    free(pName);
                                    free(pFullName);
                                }
                            }
                        }
                    }

                    if (error) error = GBS_global_string("while matching probe #%i: %s", nItem, error);

                    nItem++;
                }
                progress.inc_and_check_user_abort(error);
            }
        }

        if (!error) {
            ArbWriteFile_Context Context;

            arb_progress progress("Writing results to file", 100);
            progress.subtitle(g_results_manager.resultsFileName());

            Context.pFile         = fopen(g_results_manager.resultsFileName(), "w");
            Context.pProgress     = &progress;
            Context.nLastPercent  = 0;

            nHits = g_results_manager.enumerate_results(probe_match_with_specificity_enum_callback, (void*)&Context);

            fclose(Context.pFile);

            if (!error) error = progress.error_if_aborted();
            progress.done();
        }

        root->awar(AWAR_PC_MATCH_NHITS)->write_int(nHits);

        if (error) {
            aw_message(error);

            // Clear the results set
            g_results_manager.reset();
        }
        else {
            // Open the Probe Match Specificity dialog to interactively show how the
            // matches target the phylongeny
            create_probe_match_specificity_control_window(root)->show();

            double oldMaxWeight = g_results_manager.maximumWeight();
            g_results_manager.updateResults();
            double newMaxWeight = g_results_manager.maximumWeight();

            if (newMaxWeight != oldMaxWeight) {
                // set new limits for scaler and force current value into limits
                newMaxWeight = std::max(newMaxWeight, 0.000001); // avoid invalid limits
                root->awar(AWAR_PC_MISMATCH_THRESHOLD)->set_minmax(0.0, newMaxWeight)->touch();
            }
        }

        refresh_matchedProbesDisplay_cb(root, ntw);
    }
}

static void auto_match_cb(AW_root *root, AWT_canvas *ntw) {
    if (root->awar(AWAR_PC_AUTO_MATCH)->read_int()) {
        probe_match_with_specificity_event(root, ntw);
    }
}
static void trigger_auto_match(AW_root *root) {
    root->awar(AWAR_PC_AUTO_MATCH)->touch();
}

// ----------------------------------------------------------------------------

static void probe_forget_matches_event(AW_window *aww, ArbPM_Context *pContext) {
    AW_root *root = aww->get_root();

    get_results_manager().reset();
    root->awar(AWAR_PC_MATCH_NHITS)->write_int(0);
    refresh_matchedProbesDisplay_cb(root, pContext->ntw);
}

static void selected_probe_changed_cb(AW_root *root) {
    char *pSequence = root->awar(AWAR_PC_SELECTED_PROBE)->read_string();
    if (pSequence) {
        const ArbProbe *pProbe = get_probe_collection().find(pSequence);
        if (pProbe) {
            const char *seq = pProbe->sequence().c_str();
            root->awar(AWAR_PC_TARGET_STRING)->write_string(seq);
            root->awar(AWAR_PC_TARGET_NAME)  ->write_string(pProbe->name().c_str());

            root->awar(AWAR_TARGET_STRING)->write_string(seq); // copy to probe match & match in edit4
        }
    }
    free(pSequence);
}

static void target_string_changed_cb(AW_root *root) {
    char *pSequence = root->awar(AWAR_PC_TARGET_STRING)->read_string();
    if (pSequence && pSequence[0]) {
        const ArbProbe *pProbe = get_probe_collection().find(pSequence);
        if (pProbe) root->awar(AWAR_PC_SELECTED_PROBE)->write_string(pSequence);
    }
    free(pSequence);
}

static void match_changed_cb(AW_root *root) {
    // automatically adapt to last probe matched
    // (also adapts to last selected designed probe)
    char *pSequence = root->awar(AWAR_TARGET_STRING)->read_string();
    if (pSequence && pSequence[0]) {
        AW_awar *awar_target = root->awar(AWAR_PC_TARGET_STRING);
        if (strcmp(awar_target->read_char_pntr(), pSequence) != 0) {
            root->awar(AWAR_PC_TARGET_NAME)->write_string(""); // clear name
        }
        awar_target->write_string(pSequence);
    }
    free(pSequence);
}

// ----------------------------------------------------------------------------

AW_window *create_probe_match_with_specificity_window(AW_root *root, AWT_canvas *ntw) {
    static AW_window_simple *aws = 0; // the one and only probeSpec window

    if (!aws) {
        root->awar(AWAR_PC_MISMATCH_THRESHOLD)->add_callback(makeRootCallback(refresh_matchedProbesDisplay_cb, ntw));

        aws = new AW_window_simple;

        aws->init(root, "PROBE_MATCH_WITH_SPECIFICITY", "PROBE MATCH WITH SPECIFICITY");

        aws->load_xfig("pd_match_with_specificity.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("probespec.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        AW_selection_list *probes_id;

        aws->at("probes");
        probes_id = aws->create_selection_list(AWAR_PC_SELECTED_PROBE, 110, 10, false);
        probes_id->insert_default("", "");

        PM_Context.probes_id = probes_id;
        PM_Context.ntw       = ntw;

        aws->callback(makeWindowCallback(probe_match_with_specificity_edit_event));
        aws->at("results");
        aws->create_button("RESULTS", "RESULTS", "R");

        aws->at("pt_server");
        awt_create_PTSERVER_selection_button(aws, AWAR_PT_SERVER);

        aws->at("nhits");
        aws->create_button(0, AWAR_PC_MATCH_NHITS);

        aws->callback(makeCreateWindowCallback(create_probe_collection_window, &PM_Context));
        aws->at("edit");
        aws->create_button("EDIT", "EDIT", "E");

        aws->callback(RootAsWindowCallback::simple(probe_match_with_specificity_event, ntw));
        aws->at("match");
        aws->create_button("MATCH", "MATCH", "M");

        aws->callback(makeWindowCallback(probe_forget_matches_event, &PM_Context));
        aws->at("forget");
        aws->create_button("FORGET", "FORGET", "F");

        aws->at("auto");
        aws->label("Auto match");
        aws->create_toggle(AWAR_PC_AUTO_MATCH);

        AW_awar *awar_automatch = root->awar(AWAR_PC_AUTO_MATCH);
        awar_automatch->add_callback(makeRootCallback(auto_match_cb, ntw));

        aws->callback(makeCreateWindowCallback(create_probe_match_specificity_control_window));
        aws->at("control");
        aws->create_autosize_button("CONTROL", "Display control", "D");

        probe_match_update_probe_list(&PM_Context);

        root->awar(AWAR_PC_SELECTED_PROBE)->add_callback(makeRootCallback(selected_probe_changed_cb));
        root->awar(AWAR_PC_TARGET_STRING)->add_callback(makeRootCallback(target_string_changed_cb));
        root->awar(AWAR_TARGET_STRING)->add_callback(makeRootCallback(match_changed_cb));

        awar_automatch->touch(); // automatically run match if 'auto-match' is checked at startup
    }

    return aws;
}

// ----------------------------------------------------------------------------

struct ArbPC_Context {
    AW_selection_list *selection_id;
    ArbPM_Context     *PM_Context;

    ArbPC_Context()
        : selection_id(NULL),
          PM_Context(NULL)
    {}
};

static ArbPC_Context PC_Context;

// ----------------------------------------------------------------------------

static void save_probe_list_to_DB(const ArbProbePtrList& rProbeList, AW_root *root) {
    std::string saved;
    for (ArbProbePtrListConstIter ProbeIter = rProbeList.begin() ; ProbeIter != rProbeList.end() ; ++ProbeIter) {
        const ArbProbe *pProbe = *ProbeIter;
        if (pProbe) {
            // Note: target sequences/names do not contain '#'/':' (see REPLACE_TARGET_CONTROL_CHARS)
            saved = saved + '#'+pProbe->name()+':'+pProbe->sequence();
        }
    }

    root->awar(AWAR_PC_CURRENT_COLLECTION)->write_string(saved.empty() ? "" : saved.c_str()+1);
}

static void show_probes_in_sellist(const ArbProbePtrList& rProbeList, AW_selection_list *sellist) {
    sellist->clear();
    sellist->insert_default("", "");
    for (ArbProbePtrListConstIter ProbeIter = rProbeList.begin() ; ProbeIter != rProbeList.end() ; ++ProbeIter) {
        const ArbProbe *pProbe = *ProbeIter;
        if (pProbe) {
            sellist->insert(pProbe->displayName().c_str(), pProbe->sequence().c_str());
        }
    }
    sellist->update();
}

static void load_probe_collection(AW_window *aww, ArbPC_Context *Context, const char * const *awar_filename) {
    char *pFileName = aww->get_root()->awar(*awar_filename)->read_string();

    ArbProbeCollection ProbeCollection;
    std::string        sErrorMessage;

    if (ProbeCollection.openXML(pFileName, sErrorMessage)) {
        int    cn;
        char   buffer[256] = {0};
        float  weights[16] = {0.0};
        float  dWidth      = 1.0;
        float  dBias       = 0.0;

        ArbProbeCollection& g_probe_collection = get_probe_collection();
        g_probe_collection = ProbeCollection;

        g_probe_collection.getParameters(weights, dWidth, dBias);

        AW_root *root = AW_root::SINGLETON;
        root->awar(AWAR_PC_MATCH_WIDTH)->write_float(dWidth);
        root->awar(AWAR_PC_MATCH_BIAS)->write_float(dBias);

        for (cn = 0; cn < 16 ; cn++) {
            sprintf(buffer, AWAR_PC_MATCH_WEIGHTS"%i", cn);

            root->awar(buffer)->write_float(weights[cn]);
        }

        const ArbProbePtrList& rProbeList = g_probe_collection.probeList();

        save_probe_list_to_DB(rProbeList, root);
        show_probes_in_sellist(rProbeList, Context->selection_id);

        probe_match_update_probe_list(Context->PM_Context);
        aww->hide();
        trigger_auto_match(root);
    }
    else {
        // Print error message
        aw_message(sErrorMessage.c_str());
    }

    free(pFileName);
}

// ----------------------------------------------------------------------------

static void probe_collection_update_parameters() {
    AW_root *root = AW_root::SINGLETON;

    int cn;
    char buffer[256] = {0};

    float weights[16] = {0.0};
    float dWidth = root->awar(AWAR_PC_MATCH_WIDTH)->read_float();
    float dBias  = root->awar(AWAR_PC_MATCH_BIAS)->read_float();

    for (cn = 0; cn < 16 ; cn++) {
        sprintf(buffer, AWAR_PC_MATCH_WEIGHTS"%i", cn);

        weights[cn] = root->awar(buffer)->read_float();
    }

    ArbProbeCollection& g_probe_collection = get_probe_collection();
    g_probe_collection.setParameters(weights, dWidth, dBias);

    save_probe_list_to_DB(g_probe_collection.probeList(), root);
}

// ----------------------------------------------------------------------------

static void save_probe_collection(AW_window *aww, const char * const *awar_filename) {
    char *pFileName = aww->get_root()->awar(*awar_filename)->read_string();

    struct stat   FileStatus;
    int           nResult = ::stat(pFileName, &FileStatus);
    bool          bWrite  = true;

    if (nResult == 0) {
        bWrite = (aw_question("probe_collection_save", "File already exists. Overwrite?", "YES,NO") == 0);
    }

    if (bWrite) {
        probe_collection_update_parameters();
        get_probe_collection().saveXML(pFileName);

        aww->hide();
    }

    free(pFileName);
}

// ----------------------------------------------------------------------------

static void add_probe_to_collection_event(AW_window *aww, ArbPC_Context *pContext) {
    AW_selection_list *selection_id = pContext->selection_id;
    if (selection_id) {
        AW_root *root      = aww->get_root();
        char    *pSequence = root->awar(AWAR_PC_TARGET_STRING)->read_string();
        char    *pName     = root->awar(AWAR_PC_TARGET_NAME)->read_string();

        GB_ERROR error = NULL;
        if (!pSequence || !pSequence[0]) {
            error = "Please enter a target string";
        }
        else if (get_probe_collection().find(pSequence)) {
            error = "Target string already in collection";
        }

        if (!error) {
            const ArbProbe *pProbe = 0;

            if (get_probe_collection().add(pName, pSequence, &pProbe) && (pProbe != 0)) {
                selection_id->insert(pProbe->displayName().c_str(), pSequence);
                selection_id->update();
                probe_match_update_probe_list(pContext->PM_Context);
                probe_collection_update_parameters();

                root->awar(AWAR_PC_SELECTED_PROBE)->write_string(pSequence);
                trigger_auto_match(root);
            }
            else {
                error = "failed to add probe";
            }
        }

        aw_message_if(error);

        free(pName);
        free(pSequence);
    }
}

static void modify_probe_event(AW_window *aww, ArbPC_Context *pContext) {
    AW_selection_list *selection_id = pContext->selection_id;
    if (selection_id) {
        AW_root *root            = aww->get_root();
        AW_awar *awar_selected   = root->awar(AWAR_PC_SELECTED_PROBE);
        char    *oldSequence     = awar_selected->read_string();
        char    *pSequence       = root->awar(AWAR_PC_TARGET_STRING)->read_string();
        char    *pName           = root->awar(AWAR_PC_TARGET_NAME)->read_string();
        bool     sequenceChanged = false;

        GB_ERROR error = NULL;
        if (!pSequence || !pSequence[0]) {
            error = "Please enter a target string";
        }
        else if (!oldSequence || !oldSequence[0]) {
            error = "Please select probe to modify";
        }
        else {
            sequenceChanged = strcmp(oldSequence, pSequence) != 0;
            if (sequenceChanged) { // sequence changed -> test vs duplicate
                if (get_probe_collection().find(pSequence)) {
                    error = "Target string already in collection";
                }
            }
        }

        if (!error) {
            const ArbProbe *pProbe = 0;

            if (get_probe_collection().replace(oldSequence, pName, pSequence, &pProbe) && (pProbe != 0)) {
                AW_selection_list_iterator entry(selection_id, selection_id->get_index_of_selected());
                entry.set_displayed(pProbe->displayName().c_str());
                entry.set_value(pSequence);
                selection_id->update();

                probe_match_update_probe_list(pContext->PM_Context);
                probe_collection_update_parameters();

                awar_selected->write_string(pSequence);
                trigger_auto_match(root);
            }
            else {
                error = "failed to replace probe";
            }
        }

        aw_message_if(error);

        free(pName);
        free(pSequence);
        free(oldSequence);
    }
}

static void remove_probe_from_collection_event(AW_window *aww, ArbPC_Context *pContext) {
    AW_selection_list *selection_id = pContext->selection_id;
    if (selection_id) {
        AW_root *root      = aww->get_root();
        char    *pSequence = root->awar(AWAR_PC_SELECTED_PROBE)->read_string();
        if (pSequence) {
            const ArbProbe *pProbe = get_probe_collection().find(pSequence);
            if (pProbe) {
                int idx = selection_id->get_index_of_selected();
                selection_id->delete_element_at(idx);

                if (selection_id->size() < 1) {
                    selection_id->insert_default("", "");
                }
                selection_id->update();
                selection_id->select_element_at(idx); // select next probe for deletion

                get_probe_collection().remove(pSequence);
                probe_match_update_probe_list(pContext->PM_Context);
                probe_collection_update_parameters();

                trigger_auto_match(root);
            }
            free(pSequence);
        }
    }
}

// ----------------------------------------------------------------------------

static AW_window *probe_collection_load_prompt(AW_root *root, ArbPC_Context *pContext) {
    static char *awar_name = NULL; // do not free, bound to callback
    return awt_create_load_box(root, "Load", "probe collection",
                               ".", "xpc", &awar_name,
                               makeWindowCallback(load_probe_collection, pContext, (const char*const*)&awar_name));
}
static AW_window *probe_collection_save_prompt(AW_root *root) {
    static char *awar_name = NULL; // do not free, bound to callback
    return awt_create_load_box(root, "Save", "probe collection",
                               ".", "xpc", &awar_name,
                               makeWindowCallback(save_probe_collection, (const char*const*)&awar_name));
}

// ----------------------------------------------------------------------------

static void probe_match_update_probe_list(ArbPM_Context *pContext) {
    if (pContext) {
        AW_selection_list *sellist = pContext->probes_id;
        if (sellist) show_probes_in_sellist(get_probe_collection().probeList(), sellist);
    }
}
static void clear_probe_collection_event(AW_window *aww, ArbPC_Context *pContext) {
    if (get_probe_collection().clear()) {
        probe_match_update_probe_list(pContext->PM_Context);
        show_probes_in_sellist(get_probe_collection().probeList(), pContext->selection_id);
        trigger_auto_match(aww->get_root());
    }
}

// ----------------------------------------------------------------------------

static void probe_collection_close(AW_window *aww) {
    probe_collection_update_parameters();
    aww->hide();
}

// ----------------------------------------------------------------------------

static AW_window *create_probe_collection_window(AW_root *root, ArbPM_Context *pContext) {
    static AW_window_simple *aws = 0;     // the one and only probe match window
    char                     buffer[256];

    pd_assert(pContext);
    PC_Context.PM_Context = pContext;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "PROBE_COLLECTION", "PROBE COLLECTION");

        aws->load_xfig("pd_match_probe_collection.fig");

        aws->callback(probe_collection_close);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("probespec.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        AW_selection_list *selection_id;

        aws->at("probes");
        selection_id = aws->create_selection_list(AWAR_PC_SELECTED_PROBE, 110, 10, false);
        selection_id->insert_default("", "");

        PC_Context.selection_id = selection_id;

        aws->at("string");
        aws->create_input_field(AWAR_PC_TARGET_STRING, 32);

        aws->at("name");
        aws->create_input_field(AWAR_PC_TARGET_NAME, 32);

        aws->callback(makeWindowCallback(add_probe_to_collection_event, &PC_Context));
        aws->at("add");
        aws->create_button("ADD", "ADD", "A");

        aws->callback(makeWindowCallback(modify_probe_event, &PC_Context));
        aws->at("modify");
        aws->create_button("MODIFY", "MODIFY", "M");

        aws->callback(makeWindowCallback(remove_probe_from_collection_event, &PC_Context));
        aws->at("remove");
        aws->create_button("REMOVE", "REMOVE", "R");

        aws->callback(makeCreateWindowCallback(probe_collection_load_prompt, &PC_Context));
        aws->at("open");
        aws->create_button("LOAD", "LOAD", "L");

        aws->callback(makeCreateWindowCallback(probe_collection_save_prompt));
        aws->at("save");
        aws->create_button("SAVE", "SAVE", "S");

        aws->callback(makeWindowCallback(clear_probe_collection_event, &PC_Context));
        aws->at("clear");
        aws->create_button("CLEAR", "CLEAR", "L");

        for (int i = 0 ; i < 16 ; i++) {
            sprintf(buffer, "%i", i);
            aws->at(buffer);
            sprintf(buffer, AWAR_PC_MATCH_WEIGHTS"%i", i);
            aws->create_input_field(buffer, 4);
            root->awar(buffer)->add_callback(trigger_auto_match);
        }

        aws->at("width");
        aws->create_input_field(AWAR_PC_MATCH_WIDTH, 5);
        root->awar(AWAR_PC_MATCH_WIDTH)->add_callback(trigger_auto_match);

        aws->at("bias");
        aws->create_input_field(AWAR_PC_MATCH_BIAS, 5);
        root->awar(AWAR_PC_MATCH_BIAS)->add_callback(trigger_auto_match);

        show_probes_in_sellist(get_probe_collection().probeList(), selection_id);
    }
    return aws;
}
