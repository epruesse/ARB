#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"
#include "aw_modal.hxx"

//TODO comment
AW_cb_struct_guard AW_cb_struct::guard_before = NULL;
AW_cb_struct_guard AW_cb_struct::guard_after  = NULL;
AW_postcb_cb       AW_cb_struct::postcb       = NULL;


AW_cb_struct::AW_cb_struct(AW_window *awi, void (*g)(AW_window*, AW_CL, AW_CL), AW_CL cd1i, AW_CL cd2i, const char *help_texti, class AW_cb_struct *nexti) {
    aw            = awi;
    f             = g;
    cd1           = cd1i;
    cd2           = cd2i;
    help_text     = help_texti;
    pop_up_window = NULL;
    this->next = nexti;

    id = NULL;
}


bool AW_cb_struct::contains(void (*g)(AW_window*, AW_CL, AW_CL)) {
    return (f == g) || (next && next->contains(g));
}

bool AW_cb_struct::is_equal(const AW_cb_struct& other) const {
    bool equal = false;
    if (f == other.f) {                             // same callback function
        equal = (cd1 == other.cd1) && (cd2 == other.cd2);
        if (equal) {
            if (f == AW_POPUP) {
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
    }
    return equal;
}




void AW_cb_struct::run_callback() {
    if (next) next->run_callback();                 // callback the whole list
    if (!f) return;                                 // run no callback

    AW_root *root = aw->get_root();
    if (root->disable_callbacks) {
        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
        // have to disable most callbacks, because they are often called from inside these callbacks
        // (e.g. because some exceptional condition occurred which needs user interaction) and if
        // callbacks weren't disabled, a recursive deadlock occurs.

        // the following callbacks are allowed even if disable_callbacks is true

        bool isModalCallback = (f == AW_CB(message_cb) ||
                                f == AW_CB(input_history_cb) ||
                                f == AW_CB(input_cb));


        bool isPopdown = (f == AW_CB(AW_POPDOWN));
        bool isHelp    = (f == AW_CB(AW_POPUP_HELP));
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
            else if (isHelp) printf("allowed AW_POPUP_HELP\n");
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
    
    if (f == AW_POPUP) {
        if (pop_up_window) { // already exists
            pop_up_window->activate();
        }
        else {
            AW_PPP g = (AW_PPP)cd1;
            if (g) {
                pop_up_window = g(aw->get_root(), cd2, 0);
                pop_up_window->activate();
            }
            else {
                aw_message("not implemented -- please report to devel@arb-home.de");
            }
        }
        if (pop_up_window && pop_up_window->prvt->popup_cb)
            pop_up_window->prvt->popup_cb->run_callback();
    }
    else {
        f(aw, cd1, cd2);
    }

    useraction_done(aw);
}
