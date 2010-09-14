// =============================================================== //
//                                                                 //
//   File      : ED4_plugins.cxx                                   //
//   Purpose   : implement plugin connector + wrap plugin calls    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <secedit_extern.hxx>
#include <rna3d_extern.hxx>

#include <arbdb.h>

#include "ed4_visualizeSAI.hxx"
#include "ed4_class.hxx"

#define e4_assert(bed) arb_assert(bed)

// ------------------------------------------------------------
//      functions used to implement ED4_plugin_connector

static int has_species_name(ED4_base *base, AW_CL cl_species_name) {
    if (base->is_sequence_terminal()) {
        ED4_sequence_terminal *seq_term = base->to_sequence_terminal();
        const char *species_name = (const char *)cl_species_name;
        return species_name && seq_term && seq_term->species_name && strcmp(species_name, seq_term->species_name)==0;
    }
    return 0;
}

static ED4_sequence_terminal *find_seq_terminal(const char *species_name) {
    ED4_base *base = ED4_ROOT->main_manager->find_first_that(ED4_L_SEQUENCE_STRING, has_species_name, (AW_CL)species_name);
    ED4_sequence_terminal *seq_term = base->to_sequence_terminal();

    return seq_term;
}


// -----------------------------------
//      Implement plugin connector

class ED4_plugin_connector_impl : public ED4_plugin_connector {
    AW_root *aw_root;
    GBDATA  *gb_main;

    const ED4_sequence_terminal *seq_term;
    mutable ED4_base_position    base_pos;

public:
    ED4_plugin_connector_impl(AW_root *aw_root_, GBDATA *gb_main_)
        : aw_root(aw_root_),
          gb_main(gb_main_), 
          seq_term(NULL)
    {}
    virtual ~ED4_plugin_connector_impl() {}

    AW_root *get_application_root() const { return aw_root; }
    GBDATA *get_database() const { return gb_main; }

    void announce_current_species(const char *species) { seq_term = find_seq_terminal(species); }

    bool SAIs_visualized() const { return ED4_ROOT->visualizeSAI; }
    const char* get_SAI_background(int start, int end) const {
        return ED4_getSaiColorString(aw_root, start, end);
    }

    const char* get_search_background(int start, int end) const {
        return
            seq_term
            ? seq_term->results().buildColorString(seq_term, start, end)
            : 0;
    }

    int get_base_position(int seq_position) const {
        e4_assert(seq_term);
        return base_pos.get_base_position(seq_term, seq_position);
    }

    void forward_event(AW_event *event) const { ED4_remote_event(event); }
};

static ED4_plugin_connector& plugin_connector(AW_root *aw_root, GBDATA *gb_main) {
    static ED4_plugin_connector *host = 0;
    if (!host) {
        host = new ED4_plugin_connector_impl(aw_root, gb_main);
    }
    return *host;
}


// ----------------
//      SECEDIT


static void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL) {
    static AW_window *aw_sec = 0;

    if (!aw_sec) { // do not open window twice
        GBDATA                *gb_main   = (GBDATA*)cl_gbmain;
        ED4_plugin_connector&  connector = plugin_connector(aw->get_root(), gb_main);
        
        aw_sec = start_SECEDIT_plugin(connector);
        if (!aw_sec) {
            GB_ERROR err = GB_await_error();
            aw_message(GBS_global_string("Couldn't start SECEDIT\nReason: %s", err));
            return;
        }
    }
    aw_sec->activate();
}

// --------------
//      RNA3D

#if defined(ARB_OPENGL)

static void ED4_RNA3D_start(AW_window *aw, AW_CL cl_gbmain, AW_CL) {
    static AW_window *aw_3d = 0;

    if (!aw_3d) { // do not open window twice
        GBDATA                *gb_main   = (GBDATA*)cl_gbmain;
        ED4_plugin_connector&  connector = plugin_connector(aw->get_root(), gb_main);
        
        aw_3d = start_RNA3D_plugin(connector);
        if (!aw_3d) {
            GB_ERROR err = GB_await_error();
            aw_message(GBS_global_string("Couldn't start RNA3D\nReason: %s", err));
            return;
        }
    }
    aw_3d->activate();
}

#endif // ARB_OPENGL


void ED4_start_plugin(AW_window *aw, AW_CL cl_gbmain, AW_CL cl_pluginname) {
    const char *pluginname = (const char *)cl_pluginname;

    if (strcmp(pluginname, "SECEDIT") == 0) {
        ED4_SECEDIT_start(aw, cl_gbmain, 0);
    }
#if defined(ARB_OPENGL)
    else if (strcmp(pluginname, "RNA3D") == 0) {
        ED4_RNA3D_start(aw, cl_gbmain, 0);
    }
#endif // ARB_OPENGL
    else {
        aw_message(GBS_global_string("Failed to start unknown plugin '%s'", pluginname));
    }
}

