// ================================================================= //
//                                                                   //
//   File      : ed4_plugins.hxx                                     //
//   Purpose   : interface for edit4 plugins                         //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ED4_PLUGINS_HXX
#define ED4_PLUGINS_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

class AW_event;

struct ED4_plugin_host {
    // interface provided for EDIT4 plugins (SECEDIT and RNA3D)

    virtual ~ED4_plugin_host() {}

    virtual AW_root *get_application_root() const = 0;
    virtual GBDATA *get_database() const          = 0;

    virtual bool SAIs_visualized() const                                = 0; // == true -> SECEDIT visualizes SAIs
    virtual const char *get_SAI_background(int start, int end) const    = 0; // get SAI background color
    virtual const char *get_search_background(int start, int end) const = 0; // get background color of search hits

    virtual int get_base_position(int seq_position) const = 0; // transform seq-pos(gaps, [0..N]!) to base-pos(nogaps, [0..N]!) 

    virtual void forward_event(AW_event *event) const = 0; // all events not handled by SECEDIT are forwarded here

    virtual void announce_current_species(const char *species_name) = 0;
};

typedef AW_window *ED4_plugin(ED4_plugin_host&);

#if defined(IN_ARB_EDIT4)
void ED4_start_plugin(AW_window *aw, GBDATA *gb_main, const char *pluginname);
#endif // IN_ARB_EDIT4

#else
#error ed4_plugins.hxx included twice
#endif // ED4_PLUGINS_HXX
