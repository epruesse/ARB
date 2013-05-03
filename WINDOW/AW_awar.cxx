// ============================================================= //
//                                                               //
//   File      : AW_awar.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_gtk_migration_helpers.hxx"
#include "aw_awar.hxx"
#include "aw_awar_impl.hxx"
#include "aw_nawar.hxx" //TODO difference between awar and nawar?
#include "aw_msg.hxx"
#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_select.hxx"
#include "glib-object.h"

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#include <arb_str.h>

#include <algorithm>

#define AWAR_EPS 0.00000001

struct AW_widget_refresh_cb : virtual Noncopyable {
    AW_widget_refresh_cb(AW_widget_refresh_cb *previous, AW_awar *vs, AW_CL cd1, GtkWidget* w, AW_widget_type type, AW_window *awi);
    ~AW_widget_refresh_cb();

    AW_CL           cd;
    AW_awar        *awar;
    GtkWidget      *widget;
    AW_widget_type  widget_type;
    AW_window      *aw;

    AW_widget_refresh_cb *next;
};


static void aw_cp_awar_2_widget_cb(AW_root *root, AW_CL cl_widget_refresh_cb) {
    AW_widget_refresh_cb *widgetlist = (AW_widget_refresh_cb*)cl_widget_refresh_cb;
    if (widgetlist->widget == root->changer_of_variable) {
        root->changer_of_variable = 0;
        root->value_changed = false;
        return;
    }

    {
        char *var_value;
        var_value = widgetlist->awar->read_as_string();

        // und benachrichtigen der anderen
        switch (widgetlist->widget_type) {

            case AW_WIDGET_INPUT_FIELD:
                widgetlist->aw->update_input_field(widgetlist->widget, var_value);
                break;
            case AW_WIDGET_TEXT_FIELD:
                widgetlist->aw->update_text_field(widgetlist->widget, var_value);
                break;
            case AW_WIDGET_TOGGLE:
                widgetlist->aw->update_toggle(widgetlist->widget, var_value, widgetlist->cd);
                break;
            case AW_WIDGET_LABEL_FIELD:
                widgetlist->aw->update_label(widgetlist->widget, var_value);
                break;
            case AW_WIDGET_TOGGLE_FIELD:
                widgetlist->aw->refresh_toggle_field((int)widgetlist->cd);
                break;
            case AW_WIDGET_CHOICE_MENU: // fall-through
            case AW_WIDGET_SELECTION_LIST:
                ((AW_selection_list *)widgetlist->cd)->refresh();
                break;
            case AW_WIDGET_PROGRESS_BAR:
            {
                aw_assert(GB_FLOAT == widgetlist->awar->get_type());
                const double value = widgetlist->awar->read_float();
                widgetlist->aw->update_progress_bar(widgetlist->widget, value);
                break;
            }
            default:
                aw_assert(false);
                break;
        }
        free(var_value);
    }
    root->value_changed = false;     // Maybe value changed is set because Motif calls me
}


AW_widget_refresh_cb::AW_widget_refresh_cb(AW_widget_refresh_cb *previous, AW_awar *vs, AW_CL cd1, GtkWidget *w, AW_widget_type type, AW_window *awi) {
    cd          = cd1;
    widget      = w;
    widget_type = type;
    awar        = vs;
    aw          = awi;
    next        = previous;

    awar->add_callback(aw_cp_awar_2_widget_cb, (AW_CL)this);
}

AW_widget_refresh_cb::~AW_widget_refresh_cb() {
    if (next) delete next;
    awar->remove_callback(aw_cp_awar_2_widget_cb, (AW_CL)this);
}


bool AW_awar_impl::allowed_to_run_callbacks = true;

static GB_ERROR AW_MSG_UNMAPPED_AWAR = "Error (unmapped AWAR):\n"
    "You cannot write to this field because it is either deleted or\n"
    "unmapped. Try to select a different item, reselect this and retry.";


#define AWAR_WRITE_ACCESS_FAILED \
    GB_export_errorf("AWAR write access failure. Called %s on awar %s of type %s", \
                     __PRETTY_FUNCTION__, get_name(), get_type_name())

GB_ERROR AW_awar_impl::write_as_string(const char* para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_as_bool(bool para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_string(const char *para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_int(long para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_float(double para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_pointer(GBDATA *para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}


#define AWAR_READ_ACCESS_FAILED \
    GBK_terminatef("AWAR read access failure. Called %s on awar %s of type %s", \
                __PRETTY_FUNCTION__, get_name(), get_type_name())

const char *AW_awar_impl::read_char_pntr() {
    AWAR_READ_ACCESS_FAILED;
    return "";
}
double AW_awar_impl::read_float() {
    AWAR_READ_ACCESS_FAILED;
    return 0.;
}
double AW_awar_impl::read_as_float() {
    AWAR_READ_ACCESS_FAILED;
    return 0.;
}
bool AW_awar_impl::read_as_bool() {
    AWAR_READ_ACCESS_FAILED;
    return 0.;
}
long AW_awar_impl::read_int() {
    AWAR_READ_ACCESS_FAILED;
    return 0;
}

GBDATA *AW_awar_impl::read_pointer() {
    AWAR_READ_ACCESS_FAILED;
    return NULL;
}

char *AW_awar_impl::read_string() {
    AWAR_READ_ACCESS_FAILED;
    return NULL;
}

#define AWAR_TARGET_FAILURE \
    GBK_terminatef("AWAR target access failure. Called %s on awar of type %s", \
                   __PRETTY_FUNCTION__, get_type_name())

__ATTR__NORETURN AW_awar *AW_awar_impl::add_target_var(char **ppchr) {
    AWAR_TARGET_FAILURE;
    return NULL;
}
__ATTR__NORETURN AW_awar *AW_awar_impl::add_target_var(float *pfloat) {
    AWAR_TARGET_FAILURE;
    return NULL;
}
__ATTR__NORETURN AW_awar *AW_awar_impl::add_target_var(long *pint) {
    AWAR_TARGET_FAILURE;
    return NULL;
}

GB_ERROR AW_awar_impl::toggle_toggle() {
    return GB_export_errorf("AWAR toggle access failure. Called %s on awar of type %s", \
                           __PRETTY_FUNCTION__, get_type_name());
}

AW_awar *AW_awar_impl::add_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::add(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar_impl::add_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) {
    return add_callback((AW_RCB)f, cd1, 0);
}

AW_awar *AW_awar_impl::add_callback(void (*f)(AW_root*)) {
    return add_callback((AW_RCB)f, 0, 0);
}

void AW_awar_impl::tie_widget(AW_CL cd1, GtkWidget *widget, AW_widget_type type, AW_window *aww) {
    refresh_list = new AW_widget_refresh_cb(refresh_list, this, cd1, widget, type, aww);
}

static GBDATA* ensure_gbdata(AW_default gb_main, const char* var_name, GB_TYPES type) {
    aw_assert(var_name && var_name[0] != 0);
#if defined(DEBUG)
    GB_ERROR err = GB_check_hkey(var_name);
    aw_assert(!err);
#endif // DEBUG

    GBDATA *gbd = GB_search(gb_main, var_name, GB_FIND);
    if (gbd && type != GB_read_type(gbd)) {
        GB_warningf("Existing awar '%s' has wrong type (%i instead of %i) - recreating\n",
                    var_name, int(GB_read_type(gbd)), int(type));
        GB_delete(gbd);
        gbd = NULL;
    }
    if (!gbd) {
        gbd = GB_search(gb_main, var_name, type);
        GB_ERROR error = GB_set_temporary(gbd);
        if (error) GB_warningf("AWAR '%s': failed to set temporary on creation (Reason: %s)", var_name, error);
    }
    return gbd;
}


AW_awar_int::AW_awar_int(const char *var_name, long var_value, AW_default default_file, AW_root*) 
  : AW_awar_impl(var_name),
    min_value(0), max_value(0),
    default_value(var_value),
    value(var_value)
{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_INT);
    if (GB_is_temporary(gb_origin)) {
        GB_write_int(gb_origin, var_value);
    } 
    else {
        value = GB_read_int(gb_origin);
    }

    this->map(gb_origin);
    aw_assert(is_valid());
}
AW_awar_int::~AW_awar_int() {
}
void AW_awar_int::do_update() {
    if (min_value == max_value) return;
  
    long lo = read_int();
    if (lo < min_value) lo = min_value;
    if (lo > max_value) lo = max_value;
    if (lo != value) {
        value = lo;
        if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
        write_int(lo);
    }

    // update targets
    for (unsigned int i=0; i<target_variables.size(); ++i) {
        *target_variables[i] = lo;
    }
}
AW_awar* AW_awar_int::set_minmax(float min, float max) {
    if (min>max) GBK_terminatef("illegal values in set_minmax for AWAR '%s'", get_name());
    min_value = min;
    max_value = max;
    update();
    return this;
}
float AW_awar_int::get_min() const {
    if (min_value == max_value) 
        return std::numeric_limits<int>::min();
    else 
        return min_value;
}
float AW_awar_int::get_max() const {
    if (min_value == max_value) 
        return std::numeric_limits<int>::max();
    else 
        return max_value;
}
GB_ERROR AW_awar_int::write_int(long para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_int(gb_var, para);
    if (!error) update_tmp_state(para == default_value);
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar_int::write_float(double para, bool touch) {
    write_int(para, touch);
}
GB_ERROR AW_awar_int::write_as_string(const char *para, bool touch) {
    return write_int(atol(para),touch);
}
long AW_awar_int::read_int() {
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int(gb_var);
}
char *AW_awar_int::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}
AW_awar *AW_awar_int::add_target_var(long *pint) {
    target_variables.push_back(pint);
    *pint = read_int();
    return this;
}
GB_ERROR AW_awar_int::toggle_toggle() {
    return write_int(read_int() ? 0 : 1);
}
bool AW_awar_int::read_as_bool() {
    return read_int() != 0;
}
GB_ERROR AW_awar_int::write_as_bool(bool b, bool touch) {
    return write_int(b ? 1 : 0, touch);
}

AW_awar_float::AW_awar_float(const char *var_name, double var_value, AW_default default_file, AW_root *) 
  : AW_awar_impl(var_name),
    min_value(0.), max_value(0.),
    default_value(var_value),
    value(0.)

{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_FLOAT);
    if (GB_is_temporary(gb_origin)) {
        GB_write_float(gb_origin, var_value);
    }

    this->map(gb_origin);
    aw_assert(is_valid());
}
AW_awar_float::~AW_awar_float() {
}
void AW_awar_float::do_update() {
    if (min_value == max_value) return;
    float fl = read_float();
    if (fl < min_value) fl = min_value + AWAR_EPS;
    if (fl > max_value) fl = max_value - AWAR_EPS;
    if (fl != value) {
        value = fl;
        if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
        write_float(fl);
    }

    // update targets
    for (int i=0; i<target_variables.size(); i++) {
        *target_variables[i] = fl;
    }
}
AW_awar *AW_awar_float::set_minmax(float min, float max) {
    if (min>max) GBK_terminatef("illegal values in set_minmax for AWAR '%s'", get_name());
    min_value = min;
    max_value = max;
    update();
    return this;
}
float AW_awar_float::get_min() const {
    if (min_value == max_value) 
        return std::numeric_limits<float>::min();
    else 
        return min_value;
}
float AW_awar_float::get_max() const {
    if (min_value == max_value) 
        return std::numeric_limits<float>::max();
    else 
        return max_value;
}
GB_ERROR AW_awar_float::write_float(double para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_float(gb_var, para);
    if (!error) update_tmp_state(para == default_value);
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar_float::write_as_string(const char *para, bool touch) {
    return write_float(atof(para),touch);
}
double AW_awar_float::read_float() {
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}
char *AW_awar_float::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}
AW_awar *AW_awar_float::add_target_var(float *pfloat) {
    target_variables.push_back(pfloat);
    *pfloat = read_float();
    return this;
}
GB_ERROR AW_awar_float::toggle_toggle() {
    return write_float((read_float() != 0.0) ? 0.0 : 1.0);
}
bool AW_awar_float::read_as_bool() {
    return read_float() != 0.0;
}
GB_ERROR AW_awar_float::write_as_bool(bool b, bool touch) {
    return write_float(b ? 1.0 : 0.0, touch);
}


AW_awar_string::AW_awar_string(const char *var_name, const char* var_value, AW_default default_file, AW_root *) 
  : AW_awar_impl(var_name),
    srt_program(NULL),
    default_value(nulldup(var_value)),
    value(NULL)
{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_STRING);
    if (GB_is_temporary(gb_origin)) {
        GB_write_string(gb_origin, var_value);
    }

    this->map(gb_origin);
    aw_assert(is_valid());
}
AW_awar_string::~AW_awar_string() {
    free(default_value);
    if (srt_program) free(srt_program);
}
void AW_awar_string::do_update() {
    if (!srt_program) return;

    char *str = read_string();
    char *n   = GBS_string_eval(str, srt_program, 0);
    
    if (!n) GBK_terminatef("SRT ERROR %s %s", srt_program, GB_await_error());
    
    if (strcmp(n, str) != 0) {
        if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
        write_string(n);
    }

    // update targets
    for (int i=0; i<target_variables.size(); i++) {
        freeset(*target_variables[i], n);
    }

    free(n);
    free(str);
}
AW_awar *AW_awar_string::set_srt(const char *srt) {
    freedup(srt_program, srt);
    return this;
}
GB_ERROR AW_awar_string::write_as_string(const char *para, bool touch) {
    return write_string(para,touch);
}
GB_ERROR AW_awar_string::write_string(const char *para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_as_string(gb_var, para);
    if (!error) update_tmp_state(ARB_strNULLcmp(para, default_value));
    if (touch) GB_touch(gb_var);
    return error;
}
char *AW_awar_string::read_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}
const char *AW_awar_string::read_char_pntr() {
    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}
char *AW_awar_string::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}
AW_awar *AW_awar_string::add_target_var(char **ppchr) {
    target_variables.push_back(ppchr);
    freeset(*ppchr, read_string());
    return this;
}
GB_ERROR AW_awar_string::toggle_toggle() {
    char* str = read_string();
    GB_ERROR err = write_string(strcmp("yes", str) ? "yes" : "no");
    free(str);
}
bool AW_awar_string::read_as_bool() {
    return strcasecmp("yes", read_char_pntr()) == 0;
}
GB_ERROR AW_awar_string::write_as_bool(bool b, bool touch) {
    return write_string(b ? "yes" : "no", touch);
}


AW_awar_pointer::AW_awar_pointer(const char *var_name, void* var_value, AW_default default_file, AW_root*) 
  : AW_awar_impl(var_name),
    default_value(var_value),
    value(NULL)
{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_POINTER);
    if (GB_is_temporary(gb_origin)) {
        GB_write_pointer(gb_origin, (GBDATA*)var_value);
    }

    map(gb_origin);
    aw_assert(is_valid());
}
AW_awar_pointer::~AW_awar_pointer() {

}
void AW_awar_pointer::do_update() {
}
GB_ERROR AW_awar_pointer::write_pointer(GBDATA *para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_pointer(gb_var, para);
    if (!error) update_tmp_state(para == default_value);
    if (touch) GB_touch(gb_var);
    return error;
}
GBDATA *AW_awar_pointer::read_pointer() {
    if (!gb_var) return NULL;
    GB_transaction ta(gb_var);
    return GB_read_pointer(gb_var);
}


AW_awar_impl::AW_awar_impl(const char *var_name) 
  : callback_list(NULL),
    refresh_list(NULL),
    in_tmp_branch(var_name && strncmp(var_name, "tmp/", 4) == 0),
    gb_var(NULL),
    gb_origin(NULL)
#ifdef DEBUG
    ,is_global(false)
#endif
{
    awar_name = strdup(var_name);
}
AW_awar_impl::~AW_awar_impl() {
    free(awar_name);
}

char *AW_awar_impl::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}


void AW_awar_impl::update() {
    aw_assert(is_valid());
    
    do_update();
    run_callbacks();

    aw_assert(is_valid());
}

void AW_awar_impl::touch() {
    if (gb_var) {
        GB_transaction dummy(gb_var);
        GB_touch(gb_var);
    }
}

static void _aw_awar_gbdata_changed(GBDATA *, int *cl, GB_CB_TYPE) {
    AW_awar *awar = (AW_awar *)cl;
    awar->update();
}

static void _aw_awar_gbdata_deleted(GBDATA *gbd, int *cl, GB_CB_TYPE) {
    AW_awar_impl *awar = (AW_awar_impl *)cl;

    if (awar->gb_origin == gbd) {
        // make awar zombie
        awar->gb_var    = NULL;
        awar->gb_origin = NULL;
    }
    else {
        aw_assert(awar->gb_var == gbd);             // forgot to remove a callback ?
        awar->gb_var = awar->gb_origin;             // unmap
    }

    awar->update();
}

AW_awar *AW_awar_impl::map(AW_default gbd) {
    if (gb_var) {                                   // remove old mapping
      GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, (GB_CB)_aw_awar_gbdata_changed, (int *)this);
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
          GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, (GB_CB)_aw_awar_gbdata_deleted, (int *)this);
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, (GB_CB)_aw_awar_gbdata_changed, (int *)this);
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, (GB_CB)_aw_awar_gbdata_deleted, (int *)this);
        }
        if (error) aw_message(error);

        gb_var = gbd;
        update();
    }
    else {
        update();
    }

    aw_assert(is_valid());

    return this;
}

AW_awar *AW_awar_impl::map(AW_awar *dest) {
    return map(((AW_awar_impl*)dest)->gb_var);
}
AW_awar *AW_awar_impl::map(const char *awarn) {
    return map(AW_root::SINGLETON->awar(awarn));
}

AW_awar *AW_awar_impl::remove_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::remove(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar_impl::remove_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) {
    return remove_callback((AW_RCB) f, cd1, 0);
}
AW_awar *AW_awar_impl::remove_callback(void (*f)(AW_root*)) {
    return remove_callback((AW_RCB) f, 0, 0);
}


AW_awar *AW_awar_impl::set_minmax(float min, float max) {
    GBK_terminatef("set_minmax only applies to numeric awars (in '%s')", get_name());
    return this;
}

float AW_awar_impl::get_min() const {
    GBK_terminatef("set_minmax only applies to numeric awars (in '%s')", get_name());
    return 0.;
}

float AW_awar_impl::get_max() const {
    GBK_terminatef("set_minmax only applies to numeric awars (in '%s')", get_name());
    return 0.;
}

AW_awar *AW_awar_impl::set_srt(const char *srt) {
    GBK_terminatef("set_srt only applies to string awars (in '%s')", get_name());
    return this;
}



AW_awar *AW_awar_impl::unmap() {
    return this->map(gb_origin);
}

void AW_awar_impl::run_callbacks() {
    if (allowed_to_run_callbacks) AW_root_cblist::call(callback_list, AW_root::SINGLETON);
}

void AW_awar_impl::update_tmp_state(bool has_default_value) {
    if (in_tmp_branch || !gb_origin || 
        GB_is_temporary(gb_origin) == has_default_value)
        return;

    GB_ERROR error = has_default_value ? GB_set_temporary(gb_origin) : GB_clear_temporary(gb_origin);

    if (error) 
      GB_warning(GBS_global_string("Failed to set temporary for AWAR '%s' (Reason: %s)", 
                                   get_name(), error));
}


static void _aw_awar_on_notify(GObject* obj, GParamSpec *pspec, awar_gparam_binding* binding);
static void _aw_awar_notify_gparam(AW_root*, AW_CL data);

/**
 * Creates a binding between the AWAR and a GObject property. Changes in one will be
 * immediately propagated to the other. While creating the binding, the property
 * is set to the value of the AWAR.
 * Standard "translations" exist for G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_INT and
 * G_TYPE_BOOLEAN using the awar read/write accessors. Special translations have to 
 * be implemented with a custom mapper object.
 * FIXME: AWAR should take ownership of the mapper and handle destruction.
 * 
 * @param obj      Pointer to a GObject (e.g. a GtkEntry)
 * @param propname Name of the GObject's property to bind (e.g. "value")
 * @param mapper   Pointer to a mapper object implementing custom translation between the
 *                 AWAR value and the GValue representing the GObjects property.
 */
void AW_awar_impl::bind_value(GObject* obj, const char *propname, AW_awar_gvalue_mapper* mapper) {
    GParamSpec *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), propname);
    aw_assert(NULL != pspec);

    gparam_bindings.push_back(awar_gparam_binding(obj, pspec, this, mapper));

    g_signal_connect(obj, "notify", G_CALLBACK(_aw_awar_on_notify), &gparam_bindings.back());
    add_callback(_aw_awar_notify_gparam, (AW_CL)&gparam_bindings.back());
    
    // update property from awar now:
    _aw_awar_notify_gparam(NULL, (AW_CL) &gparam_bindings.back());
}

static void _aw_awar_on_notify(GObject* obj, GParamSpec *pspec, awar_gparam_binding* binding) {
    // discard notify event if not the property we're registered for:
    if (g_intern_string(pspec->name) != g_intern_string(binding->pspec->name)) return;

    // don't loop (notify causing update causing notify...)
    if (binding->frozen) return;
    binding->frozen = true;

    GValue gval = G_VALUE_INIT;
    g_value_init(&gval, G_PARAM_SPEC_VALUE_TYPE(binding->pspec));
    g_object_get_property(obj, pspec->name, &gval);

    if (binding->mapper) {
        (*binding->mapper)(&gval, binding->awar);
    } 
    else {
        switch(G_VALUE_TYPE(&gval)) {
        case G_TYPE_STRING:
            binding->awar->write_string(g_value_get_string(&gval), true);
            break;
        case G_TYPE_DOUBLE:
            binding->awar->write_float(g_value_get_double(&gval), true);
            break;
        case G_TYPE_INT:
            binding->awar->write_int(g_value_get_int(&gval), true);
            break;
        case G_TYPE_BOOLEAN:
            binding->awar->write_as_bool(g_value_get_boolean(&gval), true);
            break;
        default:
            aw_assert(false);
        }
    }

    binding->frozen = false;
    
    // FIXME: somehow the UserActionTracker in AW_ROOT needs to be informed:
    // if root->is_tracking(), call root->track_awar_change(awar) after
    // setting the value, but before running other callbacks. 

    g_value_unset(&gval);
}

static void _aw_awar_notify_gparam(AW_root*, AW_CL data) {
    awar_gparam_binding* binding = (awar_gparam_binding*) data;

    // don't loop
    if (binding->frozen) return;
    binding->frozen = true;

    GValue gval = G_VALUE_INIT;
    g_value_init(&gval, G_PARAM_SPEC_VALUE_TYPE(binding->pspec));

    if (binding->mapper) {
        if (binding->mapper->operator()(binding->awar, &gval)) {
            g_object_set_property(binding->obj, binding->pspec->name, &gval);
        }
    } else {
        switch(G_VALUE_TYPE(&gval)) {
        case G_TYPE_STRING: {
            char* str = binding->awar->read_as_string();
            g_value_set_string(&gval, str);
            free(str);
            break;
        } 
        case G_TYPE_DOUBLE:
            g_value_set_double(&gval, binding->awar->read_as_float());
            break;
        case G_TYPE_INT:
            g_value_set_int(&gval, binding->awar->read_int());
            break;
        case G_TYPE_BOOLEAN:
            g_value_set_boolean(&gval, binding->awar->read_as_bool());
            break;
        default:
            aw_assert(false);
        }
        g_object_set_property(binding->obj, binding->pspec->name, &gval);
    }

    g_value_unset(&gval);
    binding->frozen = false;
}





// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static int test_cb1_called;
static int test_cb2_called;

static void test_cb1(AW_root *, AW_CL cd1, AW_CL cd2) { test_cb1_called += (cd1+cd2); }
static void test_cb2(AW_root *, AW_CL cd1, AW_CL cd2) { test_cb2_called += (cd1+cd2); }

#define TEST_EXPECT_CBS_CALLED(cbl, c1,c2)              \
    do {                                                \
        test_cb1_called = test_cb2_called = 0;          \
        AW_root_cblist::call(cbl, NULL);                \
        TEST_EXPECT_EQUAL(test_cb1_called, c1);         \
        TEST_EXPECT_EQUAL(test_cb2_called, c2);         \
    } while(0)

void TEST_AW_root_cblist() {
    AW_root_cblist *cb_list = NULL;

    AW_root_callback tcb1(test_cb1, 1, 0);
    AW_root_callback tcb2(test_cb2, 0, 1);
    AW_root_callback wrong_tcb2(test_cb2, 1, 0);

    AW_root_cblist::add(cb_list, tcb1); TEST_EXPECT_CBS_CALLED(cb_list, 1, 0);
    AW_root_cblist::add(cb_list, tcb2); TEST_EXPECT_CBS_CALLED(cb_list, 1, 1);

    AW_root_cblist::remove(cb_list, tcb1);       TEST_EXPECT_CBS_CALLED(cb_list, 0, 1);
    AW_root_cblist::remove(cb_list, wrong_tcb2); TEST_EXPECT_CBS_CALLED(cb_list, 0, 1);
    AW_root_cblist::remove(cb_list, tcb2);       TEST_EXPECT_CBS_CALLED(cb_list, 0, 0);

    AW_root_cblist::add(cb_list, tcb1);
    AW_root_cblist::add(cb_list, tcb1); // add callback twice
    TEST_EXPECT_CBS_CALLED(cb_list, 1, 0);  // should only be called once

    AW_root_cblist::add(cb_list, tcb2);
    AW_root_cblist::clear(cb_list);
    TEST_EXPECT_CBS_CALLED(cb_list, 0, 0); // list clear - nothing should be called
}

#endif // UNIT_TESTS


