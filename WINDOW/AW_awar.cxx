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
#include "aw_msg.hxx"
#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_select.hxx"
#include "aw_choice.hxx"
#include "glib-object.h"

#include <ad_cb.h>
#include <arb_str.h>

#include <algorithm>
#include <limits>

#if !GLIB_CHECK_VERSION(2,30,0)
#  define G_VALUE_INIT  { 0, { { 0 } } }
#endif

#define AWAR_EPS 0.00000001

// --------------------------------------------------------------------------------
// AW_awar_impl -- default methods with warning/failure messages

bool AW_awar_impl::allowed_to_run_callbacks = true;

#if defined(ASSERTION_USED)
bool AW_awar::deny_read = false;
bool AW_awar::deny_write = false;
#endif

static GB_ERROR AW_MSG_UNMAPPED_AWAR = "Error (unmapped AWAR):\n"
    "You cannot write to this field because it is either deleted or\n"
    "unmapped. Try to select a different item, reselect this and retry.";


#define AWAR_WRITE_ACCESS_FAILED \
    GB_export_errorf("AWAR write access failure. Called %s on awar %s of type %s", \
                     __PRETTY_FUNCTION__, get_name(), get_type_name())

GB_ERROR AW_awar_impl::write_as_string(const char*, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_as_bool(bool, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_string(const char *, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_int(long, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_float(float, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}
GB_ERROR AW_awar_impl::write_pointer(GBDATA *, bool) {
    return AWAR_WRITE_ACCESS_FAILED;
}

#pragma GCC diagnostic ignored "-Wmissing-noreturn"

#define AWAR_READ_ACCESS_FAILED                                                 \
    GBK_terminatef("AWAR read access failure. Called %s on awar %s of type %s", \
                __PRETTY_FUNCTION__, get_name(), get_type_name())

const char *AW_awar_impl::read_char_pntr() const {
    AWAR_READ_ACCESS_FAILED;
    return NULL; //is never reached, only exists to avoid compiler warning
}
float AW_awar_impl::read_float() const {
    AWAR_READ_ACCESS_FAILED;
    return 0.0f;//is never reached, only exists to avoid compiler warning
}
float AW_awar_impl::read_as_float() const {
    AWAR_READ_ACCESS_FAILED;
    return 0.0f; //is never reached, only exists to avoid compiler warning
}
 bool AW_awar_impl::read_as_bool() const {
    AWAR_READ_ACCESS_FAILED;
    return false; //is never reached, only exists to avoid compiler warning
}
long AW_awar_impl::read_int() const {
    AWAR_READ_ACCESS_FAILED;
    return 0;//is never reached, only exists to avoid compiler warning
}
GBDATA *AW_awar_impl::read_pointer() const {
    AWAR_READ_ACCESS_FAILED;
    return NULL; //is never reached, only exists to avoid compiler warning
}
char *AW_awar_impl::read_string() const {
    AWAR_READ_ACCESS_FAILED;
    return NULL; //is never reached, only exists to avoid compiler warning
}

#define AWAR_TARGET_FAILURE \
    GBK_terminatef("AWAR target access failure. Called %s on awar of type %s", \
                   __PRETTY_FUNCTION__, get_type_name())

AW_awar *AW_awar_impl::add_target_var(char **) {
    AWAR_TARGET_FAILURE;
    return NULL;//is never reached, only exists to avoid compiler warning
}
AW_awar *AW_awar_impl::add_target_var(float *) {
    AWAR_TARGET_FAILURE;
    return NULL;//is never reached, only exists to avoid compiler warning
}
AW_awar *AW_awar_impl::add_target_var(long *) {
    AWAR_TARGET_FAILURE;
    return NULL; //is never reached, only exists to avoid compiler warning
}
GB_ERROR AW_awar_impl::toggle_toggle() {
    AWAR_TARGET_FAILURE;
    return 0;
}
AW_awar *AW_awar_impl::set_minmax(float, float) {
    AWAR_TARGET_FAILURE;
    return NULL; //is never reached, only exists to avoid compiler warning
}
float AW_awar_impl::get_min() const {
    AWAR_TARGET_FAILURE;
    return 0.0f;//is never reached, only exists to avoid compiler warning
}
float AW_awar_impl::get_max() const {
    AWAR_TARGET_FAILURE;
    return 0.0f; //is never reached, only exists to avoid compiler warning
}
AW_awar *AW_awar_impl::set_srt(const char *) {
    AWAR_TARGET_FAILURE;
    return NULL; //is never reached, only exists to avoid compiler warning
}

#define AWAR_CHOICE_FAILURE \
    GBK_terminatef("AWAR choice definition. Called %s on awar of type %s", \
                   __PRETTY_FUNCTION__, get_type_name())

AW_choice* AW_awar_impl::add_choice(AW_action&, const char*) {
    AWAR_CHOICE_FAILURE;
    return NULL;
}
AW_choice* AW_awar_impl::add_choice(AW_action&, int) {
    AWAR_CHOICE_FAILURE;
    return NULL;
}
AW_choice* AW_awar_impl::add_choice(AW_action&, float) {
    AWAR_CHOICE_FAILURE;
    return NULL;
}

// --------------------------------------------------------------------------------
// callback handling

void AW_awar_impl::run_callbacks() {
    if (allowed_to_run_callbacks) {
        clock_t start = clock();

        changed.emit();

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

AW_awar *AW_awar_impl::add_callback(const RootCallback& rcb) {
    changed.connect(rcb);
    return this;
}
AW_awar *AW_awar_impl::remove_callback(const RootCallback& rcb) {
    changed.disconnect(rcb);
    return this;
}

void AW_awar_impl::remove_all_callbacks() {
    changed.clear();
}

// --------------------------------------------------------------------------------

/**
 * Helper method for constructors
 * Makes sure that the GBDATA entry exists and has the right type
 * by (re)creating it if necessary. If it has to create an entry, the entry
 * will be made temporary.
 */
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


// --------------------------------------------------------------------------------

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
    unlink();
}

void AW_awar_int::do_update() {
    long lo = read_int();

    // fix min/max
    if (min_value != max_value) {
        if (lo < min_value) lo = min_value;
        if (lo > max_value) lo = max_value;
        if (lo != value) {
            value = lo;
            if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
            write_int(lo);
        }
    }

    // update targets
    for (unsigned int i=0; i<target_variables.size(); ++i) {
        *target_variables[i] = lo;
    }
}

AW_choice* AW_awar_int::add_choice(AW_action& act, int val) {
    return choices.add_choice(act, (int32_t)val); // @@@ dangerous cast (check input 'val' for overflow here)
}

AW_awar* AW_awar_int::set_minmax(float min, float max) {
    long lmin = min;
    long lmax = max;
    if (lmin>=lmax) {
        // 'min>max'  would set impossible limits
        // 'min==max' would remove limits, which is not allowed!
        GBK_terminatef("illegal values in set_minmax for AWAR '%s'", get_name());
    }
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

GB_ERROR AW_awar_int::write_int(long para, bool do_touch) {
    aw_assert(!deny_write);
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_int(gb_var, para);
    if (!error) update_tmp_state(has_default_value());
    if (do_touch) GB_touch(gb_var);
    return error;
}

GB_ERROR AW_awar_int::write_float(float para, bool do_touch) {
    return write_int(para, do_touch);
}

GB_ERROR AW_awar_int::write_as_string(const char *para, bool do_touch) {
    return write_int(atol(para), do_touch);
}

bool AW_awar_int::has_default_value() const {
    return read_int() == default_value;
}

long AW_awar_int::read_int() const {
    aw_assert(!deny_read);
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int(gb_var);
}

char *AW_awar_int::read_as_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return ARB_strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

AW_awar *AW_awar_int::add_target_var(long *pint) {
    target_variables.push_back(pint);
    *pint = read_int();
    return this;
}

void AW_awar_int::remove_all_target_vars() {
    target_variables.clear();
}

GB_ERROR AW_awar_int::toggle_toggle() {
    return write_int(read_int() ? 0 : 1);
}

bool AW_awar_int::read_as_bool() const {
    return read_int() != 0;
}

GB_ERROR AW_awar_int::write_as_bool(bool b, bool do_touch) {
    return write_int(b ? 1 : 0, do_touch);
}

GB_ERROR AW_awar_int::reset_to_default() {
    return has_default_value() ? NULL : write_int(default_value, true);
}

// --------------------------------------------------------------------------------

AW_awar_float::AW_awar_float(const char *var_name, float var_value, AW_default default_file, AW_root *) 
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
    unlink();
}

void AW_awar_float::do_update() {
    float fl = read_float();

    if (min_value != max_value) {
        // @@@ using AWAR_EPS here seems strange to me -> try without!
        if (fl < min_value) fl = min_value + AWAR_EPS;
        if (fl > max_value) fl = max_value - AWAR_EPS;
        if (fl != value) {
            value = fl;
            if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
            write_float(fl);
        }
    }

    // update targets
    for (unsigned int i=0; i<target_variables.size(); i++) {
        *target_variables[i] = fl;
    }
}

AW_choice* AW_awar_float::add_choice(AW_action& act, float val) {
    return choices.add_choice(act, val);
}

AW_awar *AW_awar_float::set_minmax(float min, float max) {
    if (min>=max) {
        // 'min>max'  would set impossible limits
        // 'min==max' would remove limits, which is not allowed!
        GBK_terminatef("illegal values in set_minmax for AWAR '%s'", get_name());
    }
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

GB_ERROR AW_awar_float::write_float(float para, bool do_touch) {
    aw_assert(!deny_write);
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_float(gb_var, para);
    if (!error) update_tmp_state(has_default_value());
    if (do_touch) GB_touch(gb_var);
    return error;
}

GB_ERROR AW_awar_float::write_as_string(const char *para, bool do_touch) {
    return write_float(atof(para),do_touch);
}

bool AW_awar_float::has_default_value() const {
    return read_float() == default_value;
}

float AW_awar_float::read_float() const {
    aw_assert(!deny_read);
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}

char *AW_awar_float::read_as_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return ARB_strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

AW_awar *AW_awar_float::add_target_var(float *pfloat) {
    target_variables.push_back(pfloat);
    *pfloat = read_float();
    return this;
}

void AW_awar_float::remove_all_target_vars() {
    target_variables.clear();
}


GB_ERROR AW_awar_float::toggle_toggle() {
    return write_float((read_float() != 0.0) ? 0.0 : 1.0);
}

bool AW_awar_float::read_as_bool() const {
    return read_float() != 0.0;
}

GB_ERROR AW_awar_float::write_as_bool(bool b, bool do_touch) {
    return write_float(b ? 1.0 : 0.0, do_touch);
}

GB_ERROR AW_awar_float::reset_to_default() {
    return has_default_value() ? NULL : write_float(default_value, true);
}

// --------------------------------------------------------------------------------

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
    unlink();
    free(default_value);
    if (srt_program) free(srt_program);
}

void AW_awar_string::do_update() {
    char *str = read_string();

    // apply srt
    if (srt_program) {
        char *n   = GBS_string_eval(str, srt_program, 0);
    
        if (!n) GBK_terminatef("SRT ERROR %s %s", srt_program, GB_await_error());
        
        if (strcmp(n, str) != 0) {
            if (AW_root::SINGLETON) AW_root::SINGLETON->changer_of_variable = 0;
            write_string(n);
        }
        free(str);
        str = n;
    }

    // update targets
    for (unsigned int i=0; i<target_variables.size(); i++) {
        freedup(*target_variables[i], str);
    }
    free(str);
}

AW_choice* AW_awar_string::add_choice(AW_action& act, const char* val) {
    return choices.add_choice(act, val);
}

AW_awar *AW_awar_string::set_srt(const char *srt) {
    freedup(srt_program, srt);
    return this;
}

GB_ERROR AW_awar_string::write_as_string(const char *para, bool do_touch) {
    return write_string(para,do_touch);
}

GB_ERROR AW_awar_string::write_string(const char *para, bool do_touch) {
    aw_assert(!deny_write);
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_autoconv_string(gb_var, para); // @@@ GB_write_string should do here
    if (!error) update_tmp_state(has_default_value());
    if (do_touch) GB_touch(gb_var);
    return error;
}

bool AW_awar_string::has_default_value() const { 
    return 0 == ARB_strNULLcmp(default_value, read_char_pntr()); 
}

char *AW_awar_string::read_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return ARB_strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

const char *AW_awar_string::read_char_pntr() const {
    aw_assert(!deny_read);
    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}

char *AW_awar_string::read_as_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return ARB_strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

AW_awar *AW_awar_string::add_target_var(char **ppchr) {
    target_variables.push_back(ppchr);
    freeset(*ppchr, read_string());
    return this;
}

void AW_awar_string::remove_all_target_vars() {
    target_variables.clear();
}

GB_ERROR AW_awar_string::toggle_toggle() {
    char* str = read_string();
    GB_ERROR err = write_string(strcmp("yes", str) ? "yes" : "no");
    free(str);
    return err;
}

bool AW_awar_string::read_as_bool() const {
    return strcasecmp("yes", read_char_pntr()) == 0;
}

GB_ERROR AW_awar_string::write_as_bool(bool b, bool do_touch) {
    return write_string(b ? "yes" : "no", do_touch);
}

GB_ERROR AW_awar_string::reset_to_default() {
    return has_default_value() ? NULL : write_string(default_value, true);
}

// --------------------------------------------------------------------------------

AW_awar_pointer::AW_awar_pointer(const char *var_name, GBDATA* var_value, AW_default default_file, AW_root*) 
  : AW_awar_impl(var_name),
    default_value(var_value),
    value(NULL)
{
    GB_transaction ta(default_file);
    gb_origin = ensure_gbdata(default_file, var_name, GB_POINTER);
    if (GB_is_temporary(gb_origin)) {
        GB_write_pointer(gb_origin, var_value);
    }

    map(gb_origin);
    aw_assert(is_valid());
}

AW_awar_pointer::~AW_awar_pointer() {
    unlink();
}

void AW_awar_pointer::do_update() {
}

void AW_awar_pointer::remove_all_target_vars() {}

GB_ERROR AW_awar_pointer::write_pointer(GBDATA *para, bool do_touch) {
    aw_assert(!deny_write);
    if (!gb_var) return AW_MSG_UNMAPPED_AWAR;
    GB_transaction ta(gb_var);
    GB_ERROR error = GB_write_pointer(gb_var, para);
    if (!error) update_tmp_state(has_default_value());
    if (do_touch) GB_touch(gb_var);
    return error;
}

bool AW_awar_pointer::has_default_value() const {
    return default_value == (void*)read_pointer(); 
}

GBDATA *AW_awar_pointer::read_pointer() const {
    aw_assert(!deny_read);
    if (!gb_var) return NULL;
    GB_transaction ta(gb_var);
    return GB_read_pointer(gb_var);
}

GB_ERROR AW_awar_pointer::reset_to_default() {
    aw_assert(!deny_write);
    return has_default_value() ? NULL : write_pointer(default_value, true);
}

// --------------------------------------------------------------------------------

AW_awar_impl::AW_awar_impl(const char *var_name)
    : in_tmp_branch(var_name && strncmp(var_name, "tmp/", 4) == 0),
      callback_time_sum(0.0),
      callback_time_count(0),
      choices(this),
      gb_origin(NULL),
      gb_var(NULL)
{
    awar_name = ARB_strdup(var_name);
}

AW_awar_impl::~AW_awar_impl() {
    free(awar_name);
}

char *AW_awar_impl::read_as_string() const {
    aw_assert(!deny_read);
    if (!gb_var) return ARB_strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

void AW_awar_impl::update() {
    aw_assert(is_valid());
    
    do_update();
    run_callbacks();
    choices.update();

    aw_assert(is_valid());
}

void AW_awar_impl::touch() {
    aw_assert(!deny_write);
    if (gb_var) {
        GB_transaction ta(gb_var);
        GB_touch(gb_var);
    }
}

static void _aw_awar_gbdata_changed(GBDATA *, AW_awar_impl *awar) {
    awar->update();
}

static void _aw_awar_gbdata_deleted(GBDATA *gbd, AW_awar_impl *awar) {
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
        GB_remove_callback(gb_var, GB_CB_CHANGED, makeDatabaseCallback(_aw_awar_gbdata_changed, this));
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
            GB_remove_callback(gb_var, GB_CB_DELETE, makeDatabaseCallback(_aw_awar_gbdata_deleted, this));
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback(gbd, GB_CB_CHANGED, makeDatabaseCallback(_aw_awar_gbdata_changed, this));
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback(gbd, GB_CB_DELETE, makeDatabaseCallback(_aw_awar_gbdata_deleted, this));
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
    return map(DOWNCAST(AW_awar_impl*, dest)->gb_var);
}
AW_awar *AW_awar_impl::map(const char *awarn) {
    return map(AW_root::SINGLETON->awar(awarn));
}
AW_awar *AW_awar_impl::unmap() {
    return this->map(gb_origin);
}
bool AW_awar_impl::is_mapped() const {
    return gb_var != gb_origin;
}

void AW_awar_impl::unlink() {
    aw_assert(this->is_valid());
    remove_all_callbacks();
    remove_all_target_vars();
    gb_origin = NULL;                               // make zombie awar
    unmap();                                        // unmap to nothing
    aw_assert(is_valid());
}

void AW_awar_impl::unlink_from_DB(GBDATA *gb_main) {
    bool mapped_to_DB = gb_var && GB_get_root(gb_var) == gb_main;
    bool origin_in_DB = gb_origin && GB_get_root(gb_origin) == gb_main;

    aw_assert(is_valid());

    if (mapped_to_DB) {
        if (origin_in_DB) unlink();
        else unmap();
    }
    else if (origin_in_DB) {
        // origin is in DB, current mapping is not
        // -> remap permanentely
        gb_origin = gb_var;
    }

    aw_assert(is_valid());
}

void AW_awar_impl::update_tmp_state(bool has_default_value_) {
    if (in_tmp_branch || !gb_origin || 
        GB_is_temporary(gb_origin) == has_default_value_)
        return;

    GB_ERROR error = has_default_value_ ? GB_set_temporary(gb_origin) : GB_clear_temporary(gb_origin);

    if (error) 
      GB_warning(GBS_global_string("Failed to set temporary for AWAR '%s' (Reason: %s)", 
                                   get_name(), error));
}

void AW_awar_impl::set_temp_if_is_default(GBDATA *gb_db) {
    // ignore awars in "other" DB (main-DB vs properties)
    if (!in_tmp_branch || !gb_origin || GB_get_root(gb_origin) != gb_db) 
        return;
    
    aw_assert(GB_get_transaction_level(gb_origin)<1); // make sure allowed_to_run_callbacks works as expected

    allowed_to_run_callbacks = false;                 // avoid AWAR-change-callbacks
    {
        GB_transaction ta(gb_origin);
        update_tmp_state(has_default_value());
    }
    allowed_to_run_callbacks = true;
}


// --------------------------------------------------------------------------------
// AW_awar binding with GObject properties

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
 *                 Ownership is transfered, mapper will be deleted with binding.
 */
void AW_awar_impl::bind_value(GObject* obj, const char *propname, AW_awar_gvalue_mapper* mapper) {
    awar_gparam_binding binding(this, obj);

    // Remove previous binding of GObject to an AWAR if present
    AW_awar *awar_prev = (AW_awar*)g_object_get_data(obj, "AWAR");
    if (awar_prev) {
        aw_debug("AWAR: overwriting binding?!");
        awar_prev->unbind(obj);
    }

    // Create binding
    gparam_bindings.push_back(awar_gparam_binding(this, obj));
    if (!gparam_bindings.back().connect(propname, mapper)) {
        gparam_bindings.pop_back();
        return;
    }

    // Store reference to us in gobject_data
    g_object_set_data(obj, "AWAR", (gpointer)this);
}

/**
 * Destroys a previously created binding.
 */
void AW_awar_impl::unbind(GObject* obj) {
    gparam_bindings.remove(awar_gparam_binding(this, obj));
}

static void _aw_awar_on_notify(GObject* obj, GParamSpec *pspec, awar_gparam_binding* binding);
static void _aw_awar_on_destroy(void* binding, GObject* obj);
static void _aw_awar_notify_gparam(AW_root*, awar_gparam_binding *data);

/**
 * Connects a binding to callbacks:
 *  - awar change
 *  - notify on gobject
 *  - destroy on gobject
 * @return false if binding failed
 */
bool awar_gparam_binding::connect(const char* propname_, AW_awar_gvalue_mapper* mapper_) {
    mapper = mapper_;

    aw_return_val_if_fail(G_IS_OBJECT(obj), false);
    aw_return_val_if_fail(propname_ != NULL, false);

    pspec =  g_object_class_find_property(G_OBJECT_GET_CLASS(obj), propname_);
    aw_return_val_if_fail(pspec != NULL, false);

    // create weak reference to object and register handler
    // for destruction of object (need to safeguard against notifying non-extant object
    g_object_weak_ref(obj, _aw_awar_on_destroy, this);

    // update property from awar
    _aw_awar_notify_gparam(NULL, this);

    // connect gparam -> awar
    handler_id = g_signal_connect(obj, "notify", G_CALLBACK(_aw_awar_on_notify), this);
    // connect awar -> gparam
    awar->add_callback(makeRootCallback(_aw_awar_notify_gparam, this));
    
    return true;
}

awar_gparam_binding::~awar_gparam_binding() {
    awar->remove_callback(makeRootCallback(_aw_awar_notify_gparam, this));

    if (handler_id) {
        g_signal_handler_disconnect(obj, handler_id);
        g_object_weak_unref(obj, _aw_awar_on_destroy, this);
    }
    if (mapper) 
        delete mapper;
}

static void _aw_awar_on_destroy(void* binding, GObject *obj) {
    ((awar_gparam_binding*)binding)->handler_id = 0; // handler is already gone
    ((awar_gparam_binding*)binding)->awar->unbind(obj);
}

static void _aw_awar_on_notify(GObject* obj, GParamSpec *pspec, awar_gparam_binding* binding) {
    // discard notify event if not the property we're registered for:
    if (g_intern_string(pspec->name) != g_intern_string(binding->pspec->name)) return;

    // don't loop (notify causing update causing notify...)
    if (binding->frozen) return;
    
    GValue gval = G_VALUE_INIT;
    g_value_init(&gval, G_PARAM_SPEC_VALUE_TYPE(binding->pspec));
    g_object_get_property(obj, pspec->name, &gval);

    binding->frozen = true;
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

    // unfreeze before emitting changed signal, connected handlers may actually
    // want to undo a user change (e.g. fake radio buttons in editor NDS)
    binding->frozen = false; 

    binding->awar->changed_by_user.emit();

    // FIXME: somehow the UserActionTracker in AW_ROOT needs to be informed:
    // if root->is_tracking(), call root->track_awar_change(awar) after
    // setting the value, but before running other callbacks.
    // Notes:
    // * AWAR changes should only be "tracked" if the value was changed
    //   and the change was directly triggered by the user.
    // * track_awar_change has to be called outside any database TA!

    g_value_unset(&gval);
}

static void _aw_awar_notify_gparam(AW_root*, awar_gparam_binding* binding) {
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
        case G_TYPE_DOUBLE: {
            g_value_set_double(&gval, binding->awar->read_float());
            break;
        }
        case G_TYPE_INT: {
            g_value_set_int(&gval, binding->awar->read_int());
            break;
        }
        case G_TYPE_BOOLEAN: {
            g_value_set_boolean(&gval, binding->awar->read_as_bool()); 
            break;
        }
        default:
            aw_assert(false);
        }
        g_object_set_property(binding->obj, binding->pspec->name, &gval);
    }

    g_value_unset(&gval);
    binding->frozen = false;
}

// --------------------------------------------------------------------------------
// debug helper

// dump CB for AW_awar_dump_changes
static void _dump_changes(AW_root*, AW_awar* awar) {
    char* val = awar->read_as_string();
    printf("AWAR %s changed: %s\n", awar->get_name(), val);
    free(val);
}

/**
 * Print all changes to awar @param name to console
 */
void AW_awar_dump_changes(const char* name) {
    AW_awar* awar = AW_root::SINGLETON->awar(name);
    awar->add_callback(makeRootCallback(_dump_changes, awar));
    char* val = awar->read_as_string();
    printf("AWAR %s initial value: %s\n", awar->get_name(), val);
    free(val);
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>


#endif // UNIT_TESTS


