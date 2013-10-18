#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"

#if defined(DEBUG)
// #define TRACE_CALLBACKS
#endif // DEBUG

//TODO comment
AW_cb_struct_guard AW_cb::guard_before = NULL;
AW_cb_struct_guard AW_cb::guard_after  = NULL;
AW_postcb_cb       AW_cb::postcb       = NULL;


AW_cb::AW_cb(AW_window *awi, void (*g)(AW_window*, AW_CL, AW_CL), AW_CL cd1i, AW_CL cd2i, const char *help_texti, class AW_cb *nexti)
    : cb(makeWindowCallback(g, cd1i, cd2i))
{
    aw            = awi;
    help_text     = help_texti;
    pop_up_window = NULL;
    this->next    = nexti;

    id = NULL;
}

AW_cb::AW_cb(AW_window *awi, const WindowCallback& wcb, const char *help_texti, class AW_cb *nexti)
    : cb(wcb)
{
    aw            = awi;
    help_text     = help_texti;
    pop_up_window = NULL;
    this->next    = nexti;

    id = NULL;
}


bool AW_cb::contains(AW_CB g) {
    return (AW_CB(cb.callee()) == g) || (next && next->contains(g));
}

bool AW_cb::is_equal(const AW_cb& other) const {
    bool equal = cb == other.cb;
    if (equal) {
        if (AW_CB(cb.callee()) == AW_POPUP) {
            equal = aw->get_root() == other.aw->get_root();
        }
        else {
            equal = aw == other.aw;
            if (!equal) {
                equal = aw->get_root() == other.aw->get_root();
#if defined(DEBUG) && 0
                if (equal) {
                    fprintf(stderr,
                            "callback '%s' instantiated twice with different windows (w1='%s' w2='%s') -- assuming the callbacks are equal\n",
                            id, aw->get_window_id(), other.aw->get_window_id());
                }
#endif // DEBUG
            }
        }
    }
    return equal;
}


void AW_cb::run_callbacks() {
    if (next) next->run_callbacks();                // callback the whole list

    AW_CB f = AW_CB(cb.callee());
    aw_assert(f);

    AW_root *root = aw->get_root();

    useraction_init();
    
    if (f == AW_POPUP) {
        if (pop_up_window) { // already exists
            pop_up_window->activate();
        }
        else {
            AW_PPP g = AW_PPP(cb.inspect_CD1());
            if (g) {
                pop_up_window = g(aw->get_root(), cb.inspect_CD2(), 0);
                pop_up_window->activate();
            }
            else {
                aw_message("not implemented -- please report to devel@arb-home.de");
            }
        }
    }
    else {
        cb(aw);
    }

    useraction_done(aw);
}
