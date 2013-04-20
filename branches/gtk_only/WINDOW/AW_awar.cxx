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

#include <gtk/gtkwidget.h>

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
            case AW_WIDGET_CHOICE_MENU:
                widgetlist->aw->refresh_option_menu((AW_option_menu_struct*)widgetlist->cd);
                break;
            case AW_WIDGET_TOGGLE_FIELD:
                widgetlist->aw->refresh_toggle_field((int)widgetlist->cd);
                break;
            case AW_WIDGET_SELECTION_LIST:
                ((AW_selection_list *)widgetlist->cd)->refresh();
            default:
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


bool AW_awar::allowed_to_run_callbacks = true;

static GB_ERROR AW_MSG_UNMAPPED_AWAR = "Error (unmapped AWAR):\n"
    "You cannot write to this field because it is either deleted or\n"
    "unmapped. Try to select a different item, reselect this and retry.";


#define AWAR_WRITE_ACCESS_FAILED \
    GB_export_errorf("AWAR write access failure. Called %s on awar of type %s", \
                     __PRETTY_FUNCTION__, get_type_name())

GB_ERROR AW_awar::write_as_string(const char* para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar::write_string(const char *para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar::write_int(long para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar::write_float(double para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar::write_pointer(GBDATA *para, bool touch) {
    return AWAR_WRITE_ACCESS_FAILED;
}


#define AWAR_READ_ACCESS_FAILED \
    GB_warningf("AWAR read access failure. Called %s on awar of type %s", \
                __PRETTY_FUNCTION__, get_type_name())

const char *AW_awar::read_char_pntr() {
    AWAR_READ_ACCESS_FAILED;
    return "";
}
double AW_awar::read_float() {
    AWAR_READ_ACCESS_FAILED;
    return 0.;
}
long AW_awar::read_int() {
    AWAR_READ_ACCESS_FAILED;
    return 0;
}

GBDATA *AW_awar::read_pointer() {
    AWAR_READ_ACCESS_FAILED;
    return NULL;
}

char *AW_awar::read_string() {
    AWAR_READ_ACCESS_FAILED;
    return NULL;
}

#define AWAR_TARGET_FAILURE \
    GBK_terminatef("AWAR target access failure. Called %s on awar of type %s", \
                   __PRETTY_FUNCTION__, get_type_name())

AW_awar *AW_awar::add_target_var(char **ppchr) {
    AWAR_TARGET_FAILURE;
    return NULL;
}
AW_awar *AW_awar::add_target_var(float *pfloat) {
    AWAR_TARGET_FAILURE;
    return NULL;
}
AW_awar *AW_awar::add_target_var(long *pint) {
    AWAR_TARGET_FAILURE;
    return NULL;
}

GB_ERROR AW_awar::toggle_toggle() {
    return GB_export_errorf("AWAR toggle access failure. Called %s on awar of type %s", \
                           __PRETTY_FUNCTION__, get_type_name());
}

AW_awar *AW_awar::add_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::add(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar::add_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) {
    return add_callback((AW_RCB)f, cd1, 0);
}

AW_awar *AW_awar::add_callback(void (*f)(AW_root*)) {
    return add_callback((AW_RCB)f, 0, 0);
}

void AW_awar::tie_widget(AW_CL cd1, GtkWidget *widget, AW_widget_type type, AW_window *aww) {
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


AW_awar_int::AW_awar_int(const char *var_name, long var_value, AW_default default_file, AW_root *root) 
  : AW_awar(var_name, root),
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
        if (root) root->changer_of_variable = 0;
        write_int(lo);
    }

    // update targets
    for (unsigned int i=0; i<target_variables.size(); ++i) {
        *target_variables[i] = lo;
    }
}
AW_awar *AW_awar_int::set_minmax(float min, float max) {
    if (min>max) GBK_terminatef("illegal values in set_minmax for AWAR '%s'", awar_name);
    min_value = min;
    max_value = max;
    update();
    return this;
}
GB_ERROR AW_awar_int::write_int(long para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_int(gb_var, para);
    if (!error) update_tmp_state(para == default_value);
    if (touch) GB_touch(gb_var);
    return error;
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

AW_awar_float::AW_awar_float(const char *var_name, double var_value, AW_default default_file, AW_root *root) 
  : AW_awar(var_name, root),
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
        if (root) root->changer_of_variable = 0;
        write_float(fl);
    }

    // update targets
    for (int i=0; i<target_variables.size(); i++) {
        *target_variables[i] = fl;
    }
}
AW_awar *AW_awar_float::set_minmax(float min, float max) {
    if (min>max) GBK_terminatef("illegal values in set_minmax for AWAR '%s'", awar_name);
    min_value = min;
    max_value = max;
    update();
    return this;
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


AW_awar_string::AW_awar_string(const char *var_name, const char* var_value, AW_default default_file, AW_root *root) 
  : AW_awar(var_name, root),
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
        if (root) root->changer_of_variable = 0;
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
    return GB_read_as_string(gb_var);
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

AW_awar_pointer::AW_awar_pointer(const char *var_name, void* var_value, AW_default default_file, AW_root *root) 
  : AW_awar(var_name, root),
    default_value(var_value),
    value(NULL)
{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_POINTER);
    if (GB_is_temporary(gb_origin)) {
        GB_write_pointer(gb_origin, (GBDATA*)var_value);
    }

    this->map(gb_origin);
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


AW_awar::AW_awar(const char *var_name,  AW_root *rooti) 
  : callback_list(NULL),
    refresh_list(NULL),
    in_tmp_branch(var_name && strncmp(var_name, "tmp/", 4) == 0),
    root(rooti),
    gb_var(NULL),
    gb_origin(NULL),
    awar_name(strdup(var_name))
{
}
AW_awar::~AW_awar() {
}
char *AW_awar::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}


void AW_awar::update() {
    aw_assert(is_valid());
    
    do_update();
    update_targets();
    run_callbacks();

    aw_assert(is_valid());
}

void AW_awar::touch() {
    if (gb_var) {
        GB_transaction dummy(gb_var);
        GB_touch(gb_var);
    }
}


void AW_awar::gbdata_changed(GBDATA *, int *cl, GB_CB_TYPE) {
    AW_awar *awar = (AW_awar *)cl;
    awar->update();
}

void AW_awar::gbdata_deleted(GBDATA *gbd, int *cl, GB_CB_TYPE) {
    AW_awar *awar = (AW_awar *)cl;

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

AW_awar *AW_awar::map(AW_default gbd) {
    if (gb_var) {                                   // remove old mapping
      GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, (GB_CB)AW_awar::gbdata_changed, (int *)this);
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
          GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, (GB_CB)AW_awar::gbdata_deleted, (int *)this);
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, (GB_CB)AW_awar::gbdata_changed, (int *)this);
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, (GB_CB)AW_awar::gbdata_deleted, (int *)this);
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

AW_awar *AW_awar::map(AW_awar *dest) {
    return this->map(dest->gb_var);
}
AW_awar *AW_awar::map(const char *awarn) {
    return map(root->awar(awarn));
}

AW_awar *AW_awar::remove_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::remove(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar::remove_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) {
    return remove_callback((AW_RCB) f, cd1, 0);
}
AW_awar *AW_awar::remove_callback(void (*f)(AW_root*)) {
    return remove_callback((AW_RCB) f, 0, 0);
}


AW_awar *AW_awar::set_minmax(float min, float max) {
    GBK_terminatef("set_minmax only applies to numeric awars (in '%s')", awar_name);
    return this;
}

AW_awar *AW_awar::set_srt(const char *srt) {
    GBK_terminatef("set_srt only applies to string awars (in '%s')", awar_name);
    return this;
}



AW_awar *AW_awar::unmap() {
    return this->map(gb_origin);
}

void AW_awar::run_callbacks() {
    if (allowed_to_run_callbacks) AW_root_cblist::call(callback_list, root);
}

void AW_awar::update_targets() {
    // send data to all variables (run update_target on each target)
}

void AW_awar::update_tmp_state(bool has_default_value) {
    if (in_tmp_branch || !gb_origin || 
        GB_is_temporary(gb_origin) == has_default_value)
        return;

    GB_ERROR error = has_default_value ? GB_set_temporary(gb_origin) : GB_clear_temporary(gb_origin);

    if (error) 
      GB_warning(GBS_global_string("Failed to set temporary for AWAR '%s' (Reason: %s)", 
                                   awar_name, error));
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


