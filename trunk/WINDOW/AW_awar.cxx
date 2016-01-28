// =============================================================== //
//                                                                 //
//   File      : AW_awar.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_nawar.hxx"
#include "aw_awar.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"
#include "aw_window.hxx"
#include "aw_select.hxx"
#include <arb_str.h>
#include <arb_file.h>
#include <ad_cb.h>
#include <list>
#include <sys/stat.h>
#include <climits>
#include <cfloat>

#if defined(DEBUG)
// uncomment next line to dump all awar-changes to stderr
// #define DUMP_AWAR_CHANGES
#endif // DEBUG

#define AWAR_EPS 0.00000001

#if defined(DUMP_AWAR_CHANGES)
#define AWAR_CHANGE_DUMP(name, where, format) fprintf(stderr, "change awar '%s' " where "(" format ")\n", name, para)
#else
#define AWAR_CHANGE_DUMP(name, where, format)
#endif // DEBUG

#if defined(ASSERTION_USED)
bool AW_awar::deny_read = false;
bool AW_awar::deny_write = false;
#endif

struct AW_widget_refresh_cb : virtual Noncopyable {
    AW_widget_refresh_cb(AW_widget_refresh_cb *previous, AW_awar *vs, AW_CL cd1, Widget w, AW_widget_type type, AW_window *awi);
    ~AW_widget_refresh_cb();

    AW_CL           cd;
    AW_awar        *awar;
    Widget          widget;
    AW_widget_type  widget_type;
    AW_window      *aw;

    AW_widget_refresh_cb *next;
};


static void aw_cp_awar_2_widget_cb(AW_root *root, AW_widget_refresh_cb *widgetlist) {
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
                break;
            case AW_WIDGET_SCALER:
                widgetlist->aw->update_scaler(widgetlist->widget, widgetlist->awar, (AW_ScalerType)widgetlist->cd);
                break;
            default:
                break;
        }
        free(var_value);
    }
    root->value_changed = false;     // Maybe value changed is set because Motif calls me
}


AW_widget_refresh_cb::AW_widget_refresh_cb(AW_widget_refresh_cb *previous, AW_awar *vs, AW_CL cd1, Widget w, AW_widget_type type, AW_window *awi) {
    cd          = cd1;
    widget      = w;
    widget_type = type;
    awar        = vs;
    aw          = awi;
    next        = previous;

    awar->add_callback(makeRootCallback(aw_cp_awar_2_widget_cb, this));
}

AW_widget_refresh_cb::~AW_widget_refresh_cb() {
    if (next) delete next;
    awar->remove_callback(makeRootCallback(aw_cp_awar_2_widget_cb, this));
}

AW_var_target::AW_var_target(void* pntr, AW_var_target *nexti) {
    next    = nexti;
    pointer = pntr;
}


bool AW_awar::allowed_to_run_callbacks = true;

static GB_ERROR AW_MSG_UNMAPPED_AWAR = "Error (unmapped AWAR):\n"
    "You cannot write to this field because it is either deleted or\n"
    "unmapped. Try to select a different item, reselect this and retry.";

#define concat(x, y) x##y

#define WRITE_BODY(self,format,func)                    \
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;           \
    aw_assert(!deny_write);                             \
    GB_transaction ta(gb_var);                          \
    AWAR_CHANGE_DUMP(awar_name, #self, format);         \
    GB_ERROR error = func(gb_var, para);                \
    if (!error) update_tmp_state_during_change()

#define WRITE_SKELETON(self,type,format,func)           \
    GB_ERROR AW_awar::self(type para) {                 \
        WRITE_BODY(self, format, func);                 \
        return error;                                   \
    }                                                   \
    GB_ERROR AW_awar::concat(re,self)(type para) {      \
        WRITE_BODY(self, format, func);                 \
        GB_touch(gb_var);                               \
        return error;                                   \
    }

WRITE_SKELETON(write_string, const char*, "%s", GB_write_string) // defines rewrite_string
WRITE_SKELETON(write_int, long, "%li", GB_write_int) // defines rewrite_int
WRITE_SKELETON(write_float, float, "%f", GB_write_float) // defines rewrite_float
WRITE_SKELETON(write_as_string, const char*, "%s", GB_write_autoconv_string) // defines rewrite_as_string // @@@ rename -> write_autoconvert_string?
WRITE_SKELETON(write_pointer, GBDATA*, "%p", GB_write_pointer) // defines rewrite_pointer

#undef WRITE_SKELETON
#undef concat
#undef AWAR_CHANGE_DUMP


char *AW_awar::read_as_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

const char *AW_awar::read_char_pntr() const {
    aw_assert(!deny_read);
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}

float AW_awar::read_float() const {
    aw_assert(!deny_read);
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}

long AW_awar::read_int() const {
    aw_assert(!deny_read);
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int(gb_var);
}

GBDATA *AW_awar::read_pointer() const {
    aw_assert(!deny_read);
    if (!gb_var) return NULL;
    GB_transaction ta(gb_var);
    return GB_read_pointer(gb_var);
}

char *AW_awar::read_string() const {
    aw_assert(!deny_read);
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

void AW_awar::touch() {
    aw_assert(!deny_write);
    if (gb_var) {
        GB_transaction ta(gb_var);
        GB_touch(gb_var);
    }
}
GB_ERROR AW_awar::reset_to_default() {
    aw_assert(!deny_write);
    GB_ERROR error = NULL;
    switch (variable_type) {
        case AW_STRING:  error = write_string (default_value.s); break;
        case AW_INT:     error = write_int    (default_value.l); break;
        case AW_FLOAT:   error = write_float  (default_value.f); break;
        case AW_POINTER: error = write_pointer(default_value.p); break;
        default: aw_assert(0); break;
    }
    return error;
}

void AW_awar::untie_all_widgets() {
    delete refresh_list; refresh_list = NULL;
}

AW_awar *AW_awar::add_callback(const RootCallback& rcb) {
    AW_root_cblist::add(callback_list, rcb);
    return this;
}

AW_awar *AW_awar::add_target_var(char **ppchr) {
    assert_var_type(AW_STRING);
    target_list = new AW_var_target((void *)ppchr, target_list);
    update_target(target_list);
    return this;
}

AW_awar *AW_awar::add_target_var(float *pfloat) {
    assert_var_type(AW_FLOAT);
    target_list = new AW_var_target((void *)pfloat, target_list);
    update_target(target_list);
    return this;
}

AW_awar *AW_awar::add_target_var(long *pint) {
    assert_var_type(AW_INT);
    target_list = new AW_var_target((void *)pint, target_list);
    update_target(target_list);
    return this;
}

void AW_awar::tie_widget(AW_CL cd1, Widget widget, AW_widget_type type, AW_window *aww) {
    refresh_list = new AW_widget_refresh_cb(refresh_list, this, cd1, widget, type, aww);
}


AW_VARIABLE_TYPE AW_awar::get_type() const {
    return this->variable_type;
}


// extern "C"
static void AW_var_gbdata_callback(GBDATA*, AW_awar *awar) {
    awar->update();
}


static void AW_var_gbdata_callback_delete_intern(GBDATA *gbd, AW_awar *awar) {
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

AW_awar::AW_awar(AW_VARIABLE_TYPE var_type, const char *var_name,
                 const char *var_value, float var_float_value,
                 AW_default default_file, AW_root *rooti)
    : callback_list(NULL),
      target_list(NULL),
      refresh_list(NULL),
      callback_time_sum(0.0),
      callback_time_count(0),
#if defined(DEBUG)
      is_global(false),
#endif
      root(rooti),
      gb_var(NULL)
{
    pp.f.min = 0;
    pp.f.max = 0;
    pp.srt   = NULL;

    GB_transaction ta(default_file);

    aw_assert(var_name && var_name[0] != 0);

#if defined(DEBUG)
    GB_ERROR err = GB_check_hkey(var_name);
    aw_assert(!err);
#endif // DEBUG

    awar_name      = strdup(var_name);
    GBDATA *gb_def = GB_search(default_file, var_name, GB_FIND);

    in_tmp_branch = strncmp(var_name, "tmp/", 4) == 0;

    GB_TYPES wanted_gbtype = (GB_TYPES)var_type;

    if (gb_def) { // use value stored in DB
        GB_TYPES gbtype = GB_read_type(gb_def);

        if (gbtype != wanted_gbtype) {
            GB_warningf("Existing awar '%s' has wrong type (%i instead of %i) - recreating\n",
                        var_name, int(gbtype), int(wanted_gbtype));
            GB_delete(gb_def);
            gb_def = 0;
        }
    }

    // store default-value in member
    switch (var_type) {
        case AW_STRING:  default_value.s = nulldup(var_value); break;
        case AW_INT:     default_value.l = (long)var_value; break;
        case AW_FLOAT:   default_value.f = var_float_value; break;
        case AW_POINTER: default_value.p = (GBDATA*)var_value; break;
        default: aw_assert(0); break;
    }

    if (!gb_def) { // set AWAR to default value
        gb_def = GB_search(default_file, var_name, wanted_gbtype);

        switch (var_type) {
            case AW_STRING:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_string '%s' with default value '%s'\n", var_name, default_value.s);
#endif // DUMP_AWAR_CHANGES
                GB_write_string(gb_def, default_value.s);
                break;

            case AW_INT: {
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_int '%s' with default value '%li'\n", var_name, default_value.l);
#endif // DUMP_AWAR_CHANGES
                GB_write_int(gb_def, default_value.l);
                break;
            }
            case AW_FLOAT:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_float '%s' with default value '%f'\n", var_name, default_value.d);
#endif // DUMP_AWAR_CHANGES
                GB_write_float(gb_def, default_value.f);
                break;

            case AW_POINTER: {
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_pointer '%s' with default value '%p'\n", var_name, default_value.p);
#endif // DUMP_AWAR_CHANGES
                GB_write_pointer(gb_def, default_value.p);
                break;
            }
            default:
                GB_warningf("AWAR '%s' cannot be created because of disallowed type", var_name);
                break;
        }

        GB_ERROR error = GB_set_temporary(gb_def);
        if (error) GB_warningf("AWAR '%s': failed to set temporary on creation (Reason: %s)", var_name, error);
    }

    variable_type = var_type;
    gb_origin     = gb_def;
    this->map(gb_def);

    aw_assert(is_valid());
}


void AW_awar::update() {
    bool fix_value = false;

    aw_assert(is_valid());

    if (gb_var && ((pp.f.min != pp.f.max) || pp.srt)) {
        switch (variable_type) {
            case AW_INT: {
                long lo = read_int();
                if (lo < pp.f.min -.5) {
                    fix_value = true;
                    lo = (int)(pp.f.min + 0.5);
                }
                if (lo>pp.f.max + .5) {
                    fix_value = true;
                    lo = (int)(pp.f.max + 0.5);
                }
                if (fix_value) {
                    if (root) root->changer_of_variable = 0;
                    write_int(lo);
                }
                break;
            }
            case AW_FLOAT: {
                float fl = read_float();
                if (fl < pp.f.min) {
                    fix_value = true;
                    fl = pp.f.min+AWAR_EPS;
                }
                if (fl>pp.f.max) {
                    fix_value = true;
                    fl = pp.f.max-AWAR_EPS;
                }
                if (fix_value) {
                    if (root) root->changer_of_variable = 0;
                    write_float(fl);
                }
                break;
            }
            case AW_STRING: {
                char *str = read_string();
                char *n   = GBS_string_eval(str, pp.srt, 0);

                if (!n) GBK_terminatef("SRT ERROR %s %s", pp.srt, GB_await_error());

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

    if (!fix_value) { // function was already recalled by write_xxx() above
        this->update_targets();
        this->run_callbacks();
    }

    aw_assert(is_valid());
}


void AW_awar::update_target(AW_var_target *pntr) {
    // send data to all variables
    if (!pntr->pointer) return;
    switch (variable_type) {
        case AW_STRING: this->get((char **)pntr->pointer); break;
        case AW_FLOAT:  this->get((float *)pntr->pointer); break;
        case AW_INT:    this->get((long *)pntr->pointer); break;
        default: aw_assert(0); GB_warning("Unknown awar type"); break;
    }
}



void AW_awar::assert_var_type(AW_VARIABLE_TYPE wanted_type) {
    if (wanted_type != variable_type) {
        GBK_terminatef("AWAR '%s' has wrong type (got=%i, expected=%i)",
                       awar_name, variable_type, wanted_type);
    }
}

// extern "C"
static void AW_var_gbdata_callback_delete(GBDATA *gbd, AW_awar *awar) {
    AW_var_gbdata_callback_delete_intern(gbd, awar);
}

AW_awar *AW_awar::map(AW_default gbd) {
    if (gb_var) {                                   // remove old mapping
        GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, makeDatabaseCallback(AW_var_gbdata_callback, this));
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
            GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, makeDatabaseCallback(AW_var_gbdata_callback_delete, this));
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, makeDatabaseCallback(AW_var_gbdata_callback, this));
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, makeDatabaseCallback(AW_var_gbdata_callback_delete, this));
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

AW_awar *AW_awar::remove_callback(const RootCallback& rcb) {
    AW_root_cblist::remove(callback_list, rcb);
    return this;
}

AW_awar *AW_awar::set_minmax(float min, float max) {
    if (variable_type == AW_STRING) GBK_terminatef("set_minmax does not apply to string AWAR '%s'", awar_name);
    if (min>=max) {
        // 'min>max'  would set impossible limits
        // 'min==max' would remove limits, which is not allowed!
        GBK_terminatef("illegal values in set_minmax for AWAR '%s'", awar_name);
    }

    pp.f.min = min;
    pp.f.max = max;
    update(); // corrects wrong default value
    return this;
}

float AW_awar::get_min() const {
    if (variable_type == AW_STRING) GBK_terminatef("get_min does not apply to string AWAR '%s'", awar_name);
    bool isSet = (pp.f.min != pp.f.max); // as used in AW_awar::update
    if (!isSet) {
        aw_assert(float(INT_MIN)>=-FLT_MAX);
        if (variable_type == AW_INT) return float(INT_MIN);
        aw_assert(variable_type == AW_FLOAT);
        return -FLT_MAX;
    }
    return pp.f.min;
}
float AW_awar::get_max() const {
    if (variable_type == AW_STRING) GBK_terminatef("get_max does not apply to string AWAR '%s'", awar_name);
    bool isSet = (pp.f.min != pp.f.max); // as used in AW_awar::update
    if (!isSet) {
        aw_assert(float(INT_MAX)<=FLT_MAX);
        if (variable_type == AW_INT) return float(INT_MAX);
        aw_assert(variable_type == AW_FLOAT);
        return FLT_MAX;
    }
    return pp.f.max;
}

AW_awar *AW_awar::set_srt(const char *srt) {
    assert_var_type(AW_STRING);
    pp.srt = srt;
    return this;
}

GB_ERROR AW_awar::toggle_toggle() {
    char *var = this->read_as_string();
    GB_ERROR    error = 0;
    if (var[0] == '0' || var[0] == 'n') {
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("yes"); break;
            case AW_INT:        error = this->write_int(1); break;
            case AW_FLOAT:      error = this->write_float(1.0); break;
            default: break;
        }
    }
    else {
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("no"); break;
            case AW_INT:        error = this->write_int(0); break;
            case AW_FLOAT:      error = this->write_float(0.0); break;
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
    if (allowed_to_run_callbacks) {
        clock_t start = clock();

        AW_root_cblist::call(callback_list, root);

        clock_t end = clock();

        if (start<end) { // ignore overflown
            double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

            if (callback_time_count>19 && (callback_time_count%2) == 0) {
                callback_time_count = callback_time_count/2;
                callback_time_sum   = callback_time_sum/2.0;
            }

            callback_time_sum += cpu_time_used;
            callback_time_count++;
        }
    }
}


void AW_awar::update_targets() {
    // send data to all variables
    AW_var_target*pntr;
    for (pntr = target_list; pntr; pntr = pntr->next) {
        update_target(pntr);
    }
}


void AW_awar::remove_all_callbacks() {
    AW_root_cblist::clear(callback_list);
}

// --------------------------------------------------------------------------------

inline bool member_of_DB(GBDATA *gbd, GBDATA *gb_main) {
    return gbd && GB_get_root(gbd) == gb_main;;
}

void AW_awar::unlink() {
    aw_assert(this->is_valid());
    remove_all_callbacks();
    remove_all_target_vars();
    gb_origin = NULL;                               // make zombie awar
    unmap();                                        // unmap to nothing
    aw_assert(is_valid());
}

bool AW_awar::unlink_from_DB(GBDATA *gb_main) {
    bool make_zombie  = false;
    bool mapped_to_DB = member_of_DB(gb_var, gb_main);
    bool origin_in_DB = member_of_DB(gb_origin, gb_main);

    aw_assert(is_valid());

    if (mapped_to_DB) {
        if (origin_in_DB) make_zombie = true;
        else unmap();
    }
    else if (origin_in_DB) {
        // origin is in DB, current mapping is not
        // -> remap permanentely
        gb_origin = gb_var;
    }

    if (make_zombie) unlink();

    aw_assert(is_valid());

    return make_zombie;
}

// --------------------------------------------------------------------------------



void AW_awar::remove_all_target_vars() {
    while (target_list) {
        AW_var_target *tar = target_list;
        target_list        = tar->next;
        delete tar;
    }
}


void AW_awar::update_tmp_state_during_change() {
    // here it's known that the awar is inside the correct DB (main-DB or prop-DB)

    if (has_managed_tmp_state()) {
        aw_assert(GB_get_transaction_level(gb_origin) != 0);
        // still should be inside change-TA (or inside TA opened in update_tmp_state())
        // (or in no-TA-mode; as true for properties)

        bool has_default_value = false;
        switch (variable_type) {
            case AW_STRING:  has_default_value = ARB_strNULLcmp(GB_read_char_pntr(gb_origin), default_value.s) == 0; break;
            case AW_INT:     has_default_value = GB_read_int(gb_origin)     == default_value.l; break;
            case AW_FLOAT:   has_default_value = GB_read_float(gb_origin)   == default_value.f; break;
            case AW_POINTER: has_default_value = GB_read_pointer(gb_origin) == default_value.p; break;
            default: aw_assert(0); GB_warning("Unknown awar type"); break;
        }

        if (GB_is_temporary(gb_origin) != has_default_value) {
            GB_ERROR error = has_default_value ? GB_set_temporary(gb_origin) : GB_clear_temporary(gb_origin);
            if (error) GB_warning(GBS_global_string("Failed to set temporary for AWAR '%s' (Reason: %s)", awar_name, error));
        }
    }
}

void AW_awar::set_temp_if_is_default(GBDATA *gb_db) {
    if (has_managed_tmp_state() && member_of_DB(gb_origin, gb_db)) { // ignore awars in "other" DB (main-DB vs properties)
        aw_assert(GB_get_transaction_level(gb_origin)<1); // make sure allowed_to_run_callbacks works as expected

        allowed_to_run_callbacks = false;                 // avoid AWAR-change-callbacks
        {
            GB_transaction ta(gb_origin);
            update_tmp_state_during_change();
        }
        allowed_to_run_callbacks = true;
    }
}

AW_awar::~AW_awar() {
    unlink();
    untie_all_widgets();
    if (variable_type == AW_STRING) free(default_value.s);
    free(awar_name);
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

    RootCallback tcb1(test_cb1, 1, 0);
    RootCallback tcb2(test_cb2, 0, 1);
    RootCallback wrong_tcb2(test_cb2, 1, 0);

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
TEST_PUBLISH(TEST_AW_root_cblist);

#endif // UNIT_TESTS


