// ================================================================= //
//                                                                   //
//   File      : aw_awar.hxx                                         //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_AWAR_HXX
#define AW_AWAR_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

// --------------
//      AWARS

class  AW_root_cblist;
struct AW_var_target;
struct AW_widget_refresh_cb;

// @@@ [CB] eliminate decls below? just use AW_RCB instead // @@@ duplicated in cb.h
typedef AW_RCB  Awar_CB;
typedef Awar_CB Awar_CB2;
typedef         void (*Awar_CB1)(AW_root *, AW_CL);
typedef         void (*Awar_CB0)(AW_root *);

enum AW_widget_type {
    AW_WIDGET_INPUT_FIELD,
    AW_WIDGET_TEXT_FIELD,
    AW_WIDGET_LABEL_FIELD,
    AW_WIDGET_CHOICE_MENU,
    AW_WIDGET_TOGGLE_FIELD,
    AW_WIDGET_SELECTION_LIST,
    AW_WIDGET_TOGGLE,
    AW_WIDGET_SCALER,
};


class AW_awar : virtual Noncopyable {
    struct {
        struct {
            float min;
            float max;
        } f;
        const char *srt;
    } pp;

    AW_root_cblist       *callback_list;
    AW_var_target        *target_list;
    AW_widget_refresh_cb *refresh_list;

    union {
        char   *s;
        double  d;
        long    l;
        GBDATA *p;
    } default_value;

    bool in_tmp_branch;

    static bool allowed_to_run_callbacks;

    double callback_time_sum;   // in seconds
    int    callback_time_count; // number of callbacks traced in callback_time_sum

#if defined(DEBUG)
    bool is_global;
#endif // DEBUG

    void remove_all_callbacks();
    void remove_all_target_vars();

    void assert_var_type(AW_VARIABLE_TYPE target_var_type);

    bool has_managed_tmp_state() const { return !in_tmp_branch && gb_origin; }

    void update_tmp_state_during_change();

public:
    // read only
    class AW_root *root;

    GBDATA *gb_var;                                 // if unmapped, points to same DB elem as 'gb_origin'
    GBDATA *gb_origin;                              // this is set ONCE on creation of awar

    // read only

    AW_VARIABLE_TYPE  variable_type;                // type of the awar
    char             *awar_name;                    // name of the awar

#if defined(ASSERTION_USED)
    static bool deny_read;  // true -> make awar reads fail
    static bool deny_write; // true -> make awar writes fail
#endif

    void unlink();                                  // unconditionally unlink from DB

    bool unlink_from_DB(GBDATA *gb_main);

    void run_callbacks();
    double mean_callback_time() const {
        if (callback_time_sum>0) {
            return callback_time_sum / callback_time_count;
        }
        return 0.0;
    }

    void update_target(AW_var_target*pntr);
    void update_targets();

    AW_awar(AW_VARIABLE_TYPE var_type, const char *var_name, const char *var_value, double var_double_value, AW_default default_file, AW_root *root);
    ~AW_awar();

    void tie_widget(AW_CL cd1, Widget widget, AW_widget_type type, AW_window *aww);
    void untie_all_widgets();

    AW_awar *add_callback(const RootCallback& cb);
    AW_awar *add_callback(Awar_CB0 f) { return add_callback(makeRootCallback(f)); }
    AW_awar *add_callback(Awar_CB1 f, AW_CL cd1) __ATTR__DEPRECATED_TODO("pass RootCallback") { return add_callback(makeRootCallback(f, cd1)); }
    AW_awar *add_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass RootCallback") { return add_callback(makeRootCallback(f, cd1, cd2)); }

    AW_awar *remove_callback(const RootCallback& cb);
    AW_awar *remove_callback(Awar_CB0 f) { return remove_callback(makeRootCallback(f)); }
    AW_awar *remove_callback(Awar_CB1 f, AW_CL cd1) __ATTR__DEPRECATED_TODO("pass RootCallback") { return remove_callback(makeRootCallback(f, cd1)); }
    AW_awar *remove_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2) __ATTR__DEPRECATED_TODO("pass RootCallback") { return remove_callback(makeRootCallback(f, cd1, cd2)); }

    AW_awar *add_target_var(char **ppchr);
    AW_awar *add_target_var(long *pint);
    AW_awar *add_target_var(float *pfloat);
    void    update();       // awar has changed

    AW_awar *set_minmax(float min, float max); // min<max !!!
    float    get_min() const;
    float    get_max() const;
    AW_awar *set_min(float min) { return set_minmax(min, get_max()); }
    AW_awar *set_srt(const char *srt);

    AW_awar *map(const char *awarn);
    AW_awar *map(AW_default dest); // map to new address
    AW_awar *map(AW_awar *dest);   // map to new address
    AW_awar *unmap();              // map to original address
    bool is_mapped() const { return gb_var != gb_origin; }

#if defined(ASSERTION_USED)
    bool is_valid() const { return correlated(gb_var, gb_origin); } // both or none NULL
#endif // ASSERTION_USED

    void get(char **p_string)  { freeset(*p_string, read_string()); } // deletes existing targets !!!
    void get(long *p_int)      { *p_int = (long)read_int(); }
    void get(double *p_double) { *p_double = read_float(); }
    void get(float *p_float)   { *p_float = read_float(); }

    AW_VARIABLE_TYPE get_type() const;

    char       *read_string() const;
    const char *read_char_pntr() const;
    char       *read_as_string() const;
    long        read_int() const;
    double      read_float() const;
    GBDATA     *read_pointer() const;

    GB_ERROR write_string(const char *aw_string);
    GB_ERROR write_as_string(const char *aw_string);
    GB_ERROR write_int(long aw_int);
    GB_ERROR write_float(double aw_double);
    GB_ERROR write_pointer(GBDATA *aw_pointer);

    GB_ERROR write_as(char *aw_value) { return write_as_string(aw_value); };

    // same as write_-versions above, but always touches the database field
    GB_ERROR rewrite_string(const char *aw_string);
    GB_ERROR rewrite_as_string(const char *aw_string);
    GB_ERROR rewrite_int(long aw_int);
    GB_ERROR rewrite_float(double aw_double);
    GB_ERROR rewrite_pointer(GBDATA *aw_pointer);

    GB_ERROR rewrite_as(char *aw_value) { return rewrite_as_string(aw_value); };

    GB_ERROR toggle_toggle();   // switches between 1/0
    void     touch();

    GB_ERROR reset_to_default();

    GB_ERROR make_global() __ATTR__USERESULT;       // should be used by ARB_init_global_awars only
    void set_temp_if_is_default(GBDATA *gb_db);
};



#else
#error aw_awar.hxx included twice
#endif // AW_AWAR_HXX

