// =============================================================== //
//                                                                 //
//   File      : ED4_secedit.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_secedit.hxx"

#include <secedit_extern.hxx>
#include <arbdb.h>
#include <awt_canvas.hxx>

#include "ed4_extern.hxx" // @@@ remove again (functions dont need to be exported)

#include "ed4_class.hxx"


#define e4_assert(bed) arb_assert(bed)

class SEC_host_EDIT4 : public SEC_host {
    AW_root                     *aw_root;
    const ED4_sequence_terminal *seq_term;
    mutable ED4_base_position    base_pos;

public:
    SEC_host_EDIT4(AW_root *aw_root_)
        : aw_root(aw_root_),
          seq_term(NULL)
    {}
    virtual ~SEC_host_EDIT4() {}

    void announce_current_species(const char *species) { seq_term = ED4_find_seq_terminal(species); }
    
    bool SAIs_visualized() const { return ED4_ROOT->visualizeSAI; }
    const char* get_SAI_background(int start, int end) const {
        return ED4_getSaiColorString(aw_root, start, end);
    }
    const char* get_search_background(int start, int end) const {
        return seq_term ? ED4_buildColorString(seq_term, start, end) : 0;
    }

    int get_base_position(int seq_position) const {
        e4_assert(seq_term);
        return base_pos.get_base_position(seq_term, seq_position);
    }

    void forward_event(AW_event *event) const { ED4_remote_event(event); }
};


void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL) {
    static AW_window *aw_sec = 0;

    if (!aw_sec) { // do not open window twice
        static SEC_host_EDIT4 secedit_host(aw->get_root());
        aw_sec = SEC_create_main_window(secedit_host, aw->get_root(), (GBDATA*)cl_gbmain);
        if (!aw_sec) {
            GB_ERROR err = GB_await_error();
            aw_message(GBS_global_string("Couldn't start secondary structure editor.\nReason: %s", err));
            return;
        }
    }
    aw_sec->activate();
}

