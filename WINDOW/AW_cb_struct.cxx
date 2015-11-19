// =============================================================== //
//                                                                 //
//   File      : AW_cb_struct.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_window_Xm.hxx"
#include "aw_msg.hxx"

#if defined(DEBUG)
// #define TRACE_CALLBACKS
#endif // DEBUG

AW_cb_struct_guard AW_cb::guard_before = NULL;
AW_cb_struct_guard AW_cb::guard_after  = NULL;
AW_postcb_cb       AW_cb::postcb       = NULL;


AW_cb::AW_cb(AW_window *awi, AW_CB g, AW_CL cd1i, AW_CL cd2i, const char *help_texti, class AW_cb *nexti)
    : cb(makeWindowCallback(g, cd1i, cd2i))
{
    aw         = awi;
    help_text  = help_texti;
    this->next = nexti;

    id = NULL;
}

AW_cb::AW_cb(AW_window *awi, const WindowCallback& wcb, const char *help_texti, class AW_cb *nexti)
    : cb(wcb)
{
    aw         = awi;
    help_text  = help_texti;
    this->next = nexti;

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
    if (root->disable_callbacks) {
        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
        // have to disable most callbacks, because they are often called from inside these callbacks
        // (e.g. because some exceptional condition occurred which needs user interaction) and if
        // callbacks weren't disabled, a recursive deadlock occurs.

        // the following callbacks are allowed even if disable_callbacks is true

        bool isModalCallback = (f == AW_CB(message_cb) ||
                                f == AW_CB(input_history_cb) ||
                                f == AW_CB(input_cb)         ||
                                f == AW_CB(file_selection_cb));

        bool isPopdown = (f == AW_CB(AW_POPDOWN));
        bool isHelp    = (f == AW_CB(AW_help_popup));
        bool allow     = isModalCallback || isHelp || isPopdown;

        bool isInfoResizeExpose = false;

        if (!allow) {
            isInfoResizeExpose = aw->is_expose_callback(AW_INFO_AREA, f) || aw->is_resize_callback(AW_INFO_AREA, f);
            allow              = isInfoResizeExpose;
        }

        if (!allow) {
            // do not change position of modal dialog, when one of the following callbacks happens - just raise it
            // (other callbacks like pressing a button, will position the modal dialog under mouse)
            bool onlyRaise =
                aw->is_expose_callback(AW_MIDDLE_AREA, f) ||
                aw->is_focus_callback(f) ||
                root->is_focus_callback((AW_RCB)f) ||
                aw->is_resize_callback(AW_MIDDLE_AREA, f);

            if (root->current_modal_window) { 
                AW_window *awmodal = root->current_modal_window;

                AW_PosRecalc prev = awmodal->get_recalc_pos_atShow();
                if (onlyRaise) awmodal->recalc_pos_atShow(AW_KEEP_POS);
                awmodal->activate();
                awmodal->recalc_pos_atShow(prev);
            }
            else {
                aw_message("Internal error (callback suppressed when no modal dialog active)");
                aw_assert(0);
            }
#if defined(TRACE_CALLBACKS)
            printf("suppressing callback %p\n", f);
#endif // TRACE_CALLBACKS
            return; // suppress the callback!
        }
#if defined(TRACE_CALLBACKS)
        else {
            if (isModalCallback) printf("allowed modal callback %p\n", f);
            else if (isPopdown) printf("allowed AW_POPDOWN\n");
            else if (isHelp) printf("allowed AW_help_popup\n");
            else if (isInfoResizeExpose) printf("allowed expose/resize infoarea\n");
            else printf("allowed other (unknown) callback %p\n", f);
        }
#endif // TRACE_CALLBACKS
    }
    else {
#if defined(TRACE_CALLBACKS)
        printf("Callbacks are allowed (executing %p)\n", f);
#endif // TRACE_CALLBACKS
    }

    useraction_init();
    cb(aw);
    useraction_done(aw);
}
