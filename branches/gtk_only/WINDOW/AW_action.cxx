#include "aw_action.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_msg.hxx"

#include <arb_msg.h>
#include <arb_string.h>

#include "gtk/gtk.h"

struct AW_action_g_signal_binding {
    GObject    *object;
    const char *sig_name;
    gulong      handler_id;
    AW_action  *action;

    AW_action_g_signal_binding(GtkWidget*, const char*);
    AW_action_g_signal_binding(const AW_action_g_signal_binding&);
    ~AW_action_g_signal_binding();

    AW_action_g_signal_binding& operator=(const AW_action_g_signal_binding&);
    bool operator==(const AW_action_g_signal_binding&) const;

    void connect(AW_action*);
    void disconnect();
    
};

struct AW_action::Pimpl {
    std::list<AW_action_g_signal_binding> gsignals;
};
  

AW_action::AW_action()
    : prvt(new Pimpl)
{
}

AW_action::AW_action(const AW_action& o) 
    : AW_element(o),
      prvt(new Pimpl)
{
    *this = o;
}

AW_action::~AW_action() {
    delete prvt;
}

AW_action& AW_action::operator=(const AW_action& o) {
    // copy element
    AW_element::operator=(o);

    // copy the upstream g_signals we are connected to
    prvt->gsignals = o.prvt->gsignals;

    // connect to g_signals
    for (std::list<AW_action_g_signal_binding>::iterator it = prvt->gsignals.begin();
         it != prvt->gsignals.end(); ++it) {
        it->connect(this);
    }
    
    // connect signals
    dclicked = o.dclicked;
    clicked = o.clicked;
    
    return *this;
}

/** Tests whether two actions are equal ignoring bound widgets */
bool AW_action::equal_nobound(const AW_action& o) const {
    return AW_element::operator==(o)
        && clicked == o.clicked
        && dclicked == o.dclicked;
}

static AW_postcb_cb postcb_cb = NULL;
//static
void AW_action::set_AW_postcb_cb(AW_postcb_cb cb) {
    postcb_cb = cb;
}


/** 
 * Triggers the action
 * This is called
 * - when the user clicks on a connected widget,
 * - from AW_root::pimpl::callallcallbacks and
 * - from check_for_remote_command()
 */
void AW_action::user_clicked(GtkWidget*) {
    AW_root *root = AW_root::SINGLETON;

    // clear and show database error
    if (GB_have_error()) {
        GB_ERROR error = GB_await_error();
        aw_message(GBS_global_string("Warning: unhandled error status\n"
                                     "%s", error));
    }

    if (root->is_help_active()) {
        root->set_help_active(false);
        root->set_cursor(NORMAL_CURSOR);

        if (get_help()) AW_help_popup(NULL, get_help());
        return; // action is done, showing the help
    }

    if (root->is_tracking()) { 
        root->track_action(get_id());
    }

    root->set_cursor(WAIT_CURSOR);

    clicked.emit();

    if (GB_have_error()) {
        GB_ERROR error = GB_await_error();
        aw_message(GBS_global_string("Warning: error while executing action %s\n"
                                     "%s", get_id(), error));
    }
    
    if (!root->is_help_active()) {
        AW_root::SINGLETON->set_cursor(NORMAL_CURSOR);
    }

    // editor wants to know the last window after every CB
    if (clicked.get_last_window() && postcb_cb) {
        postcb_cb(clicked.get_last_window());
    }
}


/////////////////////// binding to GTK / GLIB Signals ////////////////////////

/**
 * Enables/disables this signal. Connected gtk widgets will have
 * their sensitivity changed accordingly.
 */
void AW_action::set_enabled(bool enable) {
    // change status of connected widgets
    bound_set("sensitive", enable, NULL);

    AW_element::set_enabled(enable);
}

void AW_action::bound_set(const char* first_prop, ...) {
    va_list arg_list;

    for (std::list<AW_action_g_signal_binding>::iterator it = prvt->gsignals.begin();
         it != prvt->gsignals.end(); ++it) {
        if (G_IS_OBJECT(it->object)) {
            va_start(arg_list, first_prop);
            g_object_set_valist(G_OBJECT(it->object), first_prop, arg_list);
            va_end(arg_list);
        }
    }
}

/**
 * Connects a signal of a GTK widget to this signal.
 * (i.e. triggering of that signal triggers this signal
 */
void AW_action::bind(GtkWidget* widget, const char* sig) {
    aw_return_if_fail(widget != NULL);
    aw_return_if_fail(sig != NULL);
    
    g_object_set(G_OBJECT(widget), "sensitive", get_enabled(), NULL);

    prvt->gsignals.push_back(AW_action_g_signal_binding(widget, sig));
    prvt->gsignals.back().connect(this);
}

/**
 * Disconnect all signals coming from GtkWidget w
 */
void AW_action::unbind(GtkWidget* widget, const char *sig) {
    aw_return_if_fail(widget != NULL);
    aw_return_if_fail(sig != NULL);

    prvt->gsignals.remove(AW_action_g_signal_binding(widget, sig));
}

AW_action_g_signal_binding::AW_action_g_signal_binding(GtkWidget *w, const char* s) 
    : object(G_OBJECT(w)),
      sig_name(ARB_strdup(s)),
      handler_id(0),
      action(NULL)
{
}

AW_action_g_signal_binding::AW_action_g_signal_binding(const AW_action_g_signal_binding& o) 
    : object(o.object),
      sig_name(ARB_strdup(o.sig_name)),
      handler_id(0),
      action(NULL)
{
}

AW_action_g_signal_binding& 
AW_action_g_signal_binding::operator=(const AW_action_g_signal_binding& o) {
    disconnect();
    object = o.object;
    free((void*)sig_name);
    sig_name = ARB_strdup(o.sig_name);
    if (o.action && o.handler_id) {
        connect(o.action);
    }
    return *this;
}    

static bool _aw_signal_received_from_widget(GtkWidget* w, gpointer data) {
    AW_action* action = (AW_action*) data;
    action->user_clicked(w);
    return true; // hope this is right
}

static void _aw_signal_unref_widget(void* data, GObject* obj) {
    AW_action_g_signal_binding* binding = (AW_action_g_signal_binding*) data;
    binding->handler_id = 0;
    binding->action->unbind(GTK_WIDGET(obj), binding->sig_name);
}


AW_action_g_signal_binding::~AW_action_g_signal_binding() {
    disconnect();
    free((void*)sig_name);
}

bool AW_action_g_signal_binding::operator==(const AW_action_g_signal_binding& o) const {
    return object == o.object && strcmp(sig_name, o.sig_name);
}

void AW_action_g_signal_binding::disconnect() {
    if (!action || !handler_id) return;
    g_signal_handler_disconnect(object, handler_id);
    g_object_weak_unref(object, _aw_signal_unref_widget, this);
    action = NULL;
    handler_id = 0L;
}

void AW_action_g_signal_binding::connect(AW_action* a) {
    if (action) disconnect();
    action = a;

    g_object_weak_ref(object, _aw_signal_unref_widget, this);

    handler_id = g_signal_connect(object, sig_name, 
                                  G_CALLBACK(_aw_signal_received_from_widget), action);
}





#ifdef UNIT_TESTS
#include <test_unit.h>

#endif
