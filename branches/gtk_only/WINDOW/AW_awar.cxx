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

GB_ERROR AW_awar::write_string(const char *para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_string(gb_var, para);
    if (!error) update_tmp_state_during_change();
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar::write_as_string(const char *para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_as_string(gb_var, para);
    if (!error) update_tmp_state_during_change();
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar::write_int(long para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_int(gb_var, para);
    if (!error) update_tmp_state_during_change();
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar::write_float(double para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_float(gb_var, para);
    if (!error) update_tmp_state_during_change();
    if (touch) GB_touch(gb_var);
    return error;
}
GB_ERROR AW_awar::write_pointer(GBDATA *para, bool touch) {
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_pointer(gb_var, para);
    if (!error) update_tmp_state_during_change();
    if (touch) GB_touch(gb_var);
    return error;
}


char *AW_awar::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

const char *AW_awar::read_char_pntr() {
    aw_assert(variable_type == GB_STRING);

    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}

double AW_awar::read_float() {
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}

long AW_awar::read_int() {
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int(gb_var);
}

GBDATA *AW_awar::read_pointer() {
    if (!gb_var) return NULL;
    GB_transaction ta(gb_var);
    return GB_read_pointer(gb_var);
}

char *AW_awar::read_string() {
    aw_assert(variable_type == GB_STRING);

    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

void AW_awar::touch() {
    if (gb_var) {
        GB_transaction dummy(gb_var);
        GB_touch(gb_var);
    }
}



void AW_awar::untie_all_widgets() {
    delete refresh_list; refresh_list = NULL;
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

AW_awar *AW_awar::add_target_var(char **ppchr) {
    assert_var_type(GB_STRING);
    target_variables.push_back((void*)ppchr);
    update_target(ppchr);
    return this;
}

AW_awar *AW_awar::add_target_var(float *pfloat) {
    assert_var_type(GB_FLOAT);
    target_variables.push_back((void*)pfloat);
    update_target(pfloat);
    return this;
}

AW_awar *AW_awar::add_target_var(long *pint) {
    assert_var_type(GB_INT);
    target_variables.push_back((void*)pint);
    update_target(pint);
    return this;
}

void AW_awar::tie_widget(AW_CL cd1, GtkWidget *widget, AW_widget_type type, AW_window *aww) {
    refresh_list = new AW_widget_refresh_cb(refresh_list, this, cd1, widget, type, aww);
}

// extern "C"
static void AW_var_gbdata_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    AW_awar *awar = (AW_awar *)cl;
    awar->update();
}


static void AW_var_gbdata_callback_delete_intern(GBDATA *gbd, int *cl) {
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

AW_awar::AW_awar(GB_TYPES var_type, const char *var_name,
                 const char *var_value, double var_double_value,
                 AW_default default_file, AW_root *rooti) 
  : variable_type(var_type),
    min_value(0.), 
    max_value(0.),
    srt_program(NULL),
    callback_list(NULL),
    target_variables(),
    refresh_list(NULL),
    in_tmp_branch(var_name && strncmp(var_name, "tmp/", 4) == 0),
    root(rooti),
    gb_var(NULL),
    gb_origin(NULL),
    awar_name(strdup(var_name))
{
    aw_assert(var_name && var_name[0] != 0);
#if defined(DEBUG)
    GB_ERROR err = GB_check_hkey(var_name);
    aw_assert(!err);
#endif // DEBUG


    // store default-value in member
    switch (variable_type) {
        case GB_STRING:  default_value.s = nulldup(var_value); break;
        case GB_INT:     default_value.l = (long)var_value; break;
        case GB_FLOAT:   default_value.d = var_double_value; break;
        case GB_POINTER: default_value.p = (GBDATA*)var_value; break;
        default: aw_assert(0); break;
    }
  
    GB_transaction ta(default_file);

    // 
    GBDATA *gb_def  = GB_search(default_file, var_name, GB_FIND);

    if (gb_def && variable_type != GB_read_type(gb_def)) {
        GB_warningf("Existing awar '%s' has wrong type (%i instead of %i) - recreating\n",
                    var_name, int(GB_read_type(gb_def)), int(variable_type));
        GB_delete(gb_def);
        gb_def = NULL;
    }

    if (!gb_def) { // set AWAR to default value
#if defined(DUMP_AWAR_CHANGES)
        fprintf(stderr, "creating awar '%s' with type %i and default value '%s'\n", 
                var_name, variable_type, default_value.s);
#endif // DUMP_AWAR_CHANGES

        gb_def = GB_search(default_file, var_name, variable_type);

        switch (variable_type) {
            case GB_STRING:
                GB_write_string(gb_def, default_value.s);
                break;
            case GB_INT: 
                GB_write_int(gb_def, default_value.l);
                break;
            case GB_FLOAT:
                GB_write_float(gb_def, default_value.d);
                break;
            case GB_POINTER: 
                GB_write_pointer(gb_def, default_value.p);
                break;
            default:
                GB_warningf("AWAR '%s' cannot be created because of disallowed type", var_name);
                break;
        }

        GB_ERROR error = GB_set_temporary(gb_def);
        if (error) GB_warningf("AWAR '%s': failed to set temporary on creation (Reason: %s)", var_name, error);
    }

    this->gb_origin = gb_def;
    this->map(gb_def);

    aw_assert(is_valid());
}


void AW_awar::update() {
    bool fix_value = false;

    aw_assert(is_valid());

    if (gb_var && ((min_value != max_value) || srt_program)) {
        switch (variable_type) {
            case GB_INT: {
                long lo = read_int();
                if (lo < min_value -.5) {
                    fix_value = true;
                    lo = (int)(min_value + 0.5);
                }
                if (lo > max_value + .5) {
                    fix_value = true;
                    lo = (int)(max_value + 0.5);
                }
                if (fix_value) {
                    if (root) root->changer_of_variable = 0;
                    write_int(lo);
                }
                break;
            }
            case GB_FLOAT: {
                float fl = read_float();
                if (fl < min_value) {
                    fix_value = true;
                    fl = min_value +AWAR_EPS;
                }
                if (fl > max_value) {
                    fix_value = true;
                    fl = max_value-AWAR_EPS;
                }
                if (fix_value) {
                    if (root) root->changer_of_variable = 0;
                    write_float(fl);
                }
                break;
            }
            case GB_STRING: {
                char *str = read_string();
                char *n   = GBS_string_eval(str, srt_program, 0);

                if (!n) GBK_terminatef("SRT ERROR %s %s", srt_program, GB_await_error());

                if (strcmp(n, str) != 0) {
                    fix_value = true;
                    if (root) root->changer_of_variable = 0;
                    write_string(n);
                }
                free(n);
                free(str);
                break;
            }
            default:
                break;
        }
    }

    this->update_targets();
    this->run_callbacks();

    aw_assert(is_valid());
}


void AW_awar::update_target(void *pntr) {
    // send data to all variables
    if (!pntr) return;
    switch (variable_type) {
        case GB_STRING: this->get((char **)pntr); break;
        case GB_FLOAT:  this->get((float *)pntr); break;
        case GB_INT:    this->get((long *)pntr); break;
        default: aw_assert(0); GB_warning("Unknown awar type"); break;
    }
}



void AW_awar::assert_var_type(GB_TYPES wanted_type) {
    if (wanted_type != variable_type) {
        GBK_terminatef("AWAR '%s' has wrong type (got=%i, expected=%i)",
                       awar_name, variable_type, wanted_type);
    }
}

// extern "C"
static void AW_var_gbdata_callback_delete(GBDATA *gbd, int *cl, GB_CB_TYPE) {
    AW_var_gbdata_callback_delete_intern(gbd, cl);
}

AW_awar *AW_awar::map(AW_default gbd) {
    if (gb_var) {                                   // remove old mapping
        GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this);
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
            GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this);
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this);
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this);
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
    if (variable_type == GB_STRING) GBK_terminatef("set_minmax does not apply to string AWAR '%s'", awar_name);
    if (min>max) GBK_terminatef("illegal values in set_minmax for AWAR '%s'", awar_name);

    min_value = min;
    max_value = max;

    update(); // corrects wrong default value
    return this;
}

AW_awar *AW_awar::set_srt(const char *srt) {
    assert_var_type(GB_STRING);
    srt_program = srt;
    return this;
}

GB_ERROR AW_awar::toggle_toggle() {
    char *var = this->read_as_string();
    GB_ERROR    error = 0;
    if (var[0] == '0' || var[0] == 'n') {
        switch (this->variable_type) {
            case GB_STRING:     error = this->write_string("yes"); break;
            case GB_INT:        error = this->write_int(1); break;
            case GB_FLOAT:      error = this->write_float(1.0); break;
            default: break;
        }
    }
    else {
        switch (this->variable_type) {
            case GB_STRING:     error = this->write_string("no"); break;
            case GB_INT:        error = this->write_int(0); break;
            case GB_FLOAT:      error = this->write_float(0.0); break;
            default: break;
        }
    }
    free(var);
    return error;
}

AW_awar *AW_awar::unmap() {
    return this->map(gb_origin);
}

void AW_awar::run_callbacks() {
    if (allowed_to_run_callbacks) AW_root_cblist::call(callback_list, root);
}

void AW_awar::update_targets() {
    // send data to all variables (run update_target on each target)
    std::for_each(target_variables.begin(), target_variables.end(),
                  std::bind1st(std::mem_fun(&AW_awar::update_target), this));
}

void AW_awar::update_tmp_state_during_change() {
    // here it's known that the awar is inside the correct DB (main-DB or prop-DB)

    if (has_managed_tmp_state()) {
        aw_assert(GB_get_transaction_level(gb_origin) != 0);
        // still should be inside change-TA (or inside TA opened in update_tmp_state())
        // (or in no-TA-mode; as true for properties)

        bool has_default_value = false;
        switch (variable_type) {
            case GB_STRING:  has_default_value = ARB_strNULLcmp(GB_read_char_pntr(gb_origin), default_value.s) == 0; break;
            case GB_INT:     has_default_value = GB_read_int(gb_origin)     == default_value.l; break;
            case GB_FLOAT:   has_default_value = GB_read_float(gb_origin)   == default_value.d; break;
            case GB_POINTER: has_default_value = GB_read_pointer(gb_origin) == default_value.p; break;
            default: aw_assert(0); GB_warning("Unknown awar type"); break;
        }

        if (GB_is_temporary(gb_origin) != has_default_value) {
            GB_ERROR error = has_default_value ? GB_set_temporary(gb_origin) : GB_clear_temporary(gb_origin);
            if (error) GB_warning(GBS_global_string("Failed to set temporary for AWAR '%s' (Reason: %s)", awar_name, error));
        }
    }
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


