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

#include <aw_msg.hxx>
#include <arbdb.h>
#include <arb_defs.h>

#include "ed4_visualizeSAI.hxx"
#include "ed4_class.hxx"

#define e4_assert(bed) arb_assert(bed)

static int has_species_name(ED4_base *base, AW_CL cl_species_name) {
    if (base->is_sequence_terminal()) {
        ED4_sequence_terminal *seq_term = base->to_sequence_terminal();
        const char *species_name = (const char *)cl_species_name;
        return species_name && seq_term && seq_term->species_name && strcmp(species_name, seq_term->species_name)==0;
    }
    return 0;
}

// -----------------
//      ED4_host

class ED4_host : public ED4_plugin_host, virtual Noncopyable {
    AW_root *aw_root;
    GBDATA  *gb_main;

    const ED4_sequence_terminal *seq_term;
    mutable ED4_base_position    base_pos;

public:
    ED4_host(AW_root *aw_root_, GBDATA *gb_main_)
        : aw_root(aw_root_),
          gb_main(gb_main_), 
          seq_term(NULL)
    {}
    virtual ~ED4_host() {}

    AW_root *get_application_root() const { return aw_root; }
    GBDATA *get_database() const { return gb_main; }

    void announce_current_species(const char *species_name) {
        ED4_base *base = ED4_ROOT->main_manager->find_first_that(ED4_L_SEQUENCE_STRING, has_species_name, (AW_CL)species_name);
        seq_term       = base->to_sequence_terminal();
    }

    bool SAIs_visualized() const { return ED4_ROOT->visualizeSAI; }
    const char* get_SAI_background(int start, int end) const {
        return ED4_getSaiColorString(aw_root, start, end);
    }

    const char* get_search_background(int start, int end) const {
        return seq_term
            ? seq_term->results().buildColorString(seq_term, start, end)
            : 0;
    }

    int get_base_position(int seq_position) const {
        e4_assert(seq_term);
        return base_pos.get_base_position(seq_term, seq_position);
    }

    void forward_event(AW_event *event) const { ED4_remote_event(event); }
};

// ---------------
//      PlugIn

class PlugIn {
    char              *name;
    ED4_plugin        *start_plugin;
    mutable AW_window *window;

public:
    PlugIn(const char *name_, ED4_plugin *start_plugin_)
        : name(strdup(name_)),
          start_plugin(start_plugin_),
          window(NULL)
    {}
    PlugIn(const PlugIn& other)
        : name(strdup(other.name)),
          start_plugin(other.start_plugin),
          window(other.window)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(PlugIn);
    ~PlugIn() { free(name); }

    bool has_name(const char *Name) const { return strcmp(Name, name) == 0; }

    GB_ERROR activate(AW_root *aw_root, GBDATA *gb_main) const {
        GB_ERROR error = NULL;
        if (!window) {
            static ED4_plugin_host *host = 0;
            if (!host) host = new ED4_host(aw_root, gb_main);
            window = start_plugin(*host);
            error = window ? NULL : GB_await_error();
            if (error) error = GBS_global_string("Couldn't start plugin '%s'\nReason: %s", name, error);
        }
        if (!error) window->activate();
        return error;
    }
};


static PlugIn registered[] = { // register plugins here
    PlugIn("SECEDIT", start_SECEDIT_plugin),
#if defined(ARB_OPENGL)
    PlugIn("RNA3D", start_RNA3D_plugin),
#endif // ARB_OPENGL
};

static const PlugIn *findPlugin(const char *name) {
    for (size_t plug = 0; plug<ARRAY_ELEMS(registered); ++plug) {
        if (registered[plug].has_name(name)) {
            return &registered[plug];
        }
    }
    return NULL;
}

void ED4_start_plugin(AW_window *aw, AW_CL cl_gbmain, AW_CL cl_pluginname) {
    const char *pluginname = (const char *)cl_pluginname;
    GBDATA     *gb_main    = (GBDATA*)cl_gbmain;

    const PlugIn *plugin = findPlugin(pluginname);

    GB_ERROR error = plugin
        ? plugin->activate(aw->get_root(), gb_main)
        : GBS_global_string("plugin '%s' is unknown", pluginname);
    
    aw_message_if(error);
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_plugin_found() {
    TEST_ASSERT(findPlugin("SECEDIT"));
    TEST_ASSERT(!findPlugin("unknown"));
#if defined(ARB_OPENGL)
    TEST_ASSERT(findPlugin("RNA3D"));
#else
    TEST_ASSERT(!findPlugin("RNA3D"));
#endif
}

#endif // UNIT_TESTS

