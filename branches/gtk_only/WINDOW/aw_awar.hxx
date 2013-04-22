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

#pragma once

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

#include "aw_gtk_forward_declarations.hxx"

#include <vector>


// --------------
//      AWARS

class  AW_root_cblist;
struct AW_widget_refresh_cb;

typedef AW_RCB  Awar_CB;
typedef Awar_CB Awar_CB2;
typedef void (*Awar_CB1)(AW_root *, AW_CL);
typedef void (*Awar_CB0)(AW_root *);

enum AW_widget_type {
    AW_WIDGET_INPUT_FIELD,
    AW_WIDGET_TEXT_FIELD,
    AW_WIDGET_LABEL_FIELD,
    AW_WIDGET_CHOICE_MENU,
    AW_WIDGET_TOGGLE_FIELD,
    AW_WIDGET_SELECTION_LIST,
    AW_WIDGET_TOGGLE,
    AW_WIDGET_PROGRESS_BAR
};


class AW_awar : virtual Noncopyable {
    AW_root_cblist       *callback_list;

    AW_widget_refresh_cb *refresh_list;

    bool in_tmp_branch;

    static bool allowed_to_run_callbacks;

#if defined(DEBUG)
    bool is_global;
#endif // DEBUG

    void remove_all_callbacks();
    void remove_all_target_vars();

    void assert_var_type(GB_TYPES target_var_type);

    virtual void update_targets();
    virtual void do_update() = 0;

    static void gbdata_changed(GBDATA*, int*, GB_CB_TYPE);
    static void gbdata_deleted(GBDATA*, int*, GB_CB_TYPE);

protected:
    void update_tmp_state(bool has_default_value);
    AW_awar(const char *var_name, AW_root *root);

public:
    // read only
    class AW_root *root;

    GBDATA *gb_var;                                 // if unmapped, points to same DB elem as 'gb_origin'
    GBDATA *gb_origin;                              // this is set ONCE on creation of awar

    // read only


    char      *awar_name;                    // name of the awar

    void unlink();                                  // unconditionally unlink from DB

    bool unlink_from_DB(GBDATA *gb_main);
    
    void run_callbacks();

    virtual ~AW_awar();

    void tie_widget(AW_CL cd1, GtkWidget* widget, AW_widget_type type, AW_window *aww);

    AW_awar *add_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2);
    AW_awar *add_callback(Awar_CB1 f, AW_CL cd1);
    AW_awar *add_callback(Awar_CB0 f);

    AW_awar *remove_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2);   // remove a callback
    AW_awar *remove_callback(Awar_CB1 f, AW_CL cd1);
    AW_awar *remove_callback(Awar_CB0 f);

    virtual AW_awar *add_target_var(char **ppchr);
    virtual AW_awar *add_target_var(long *pint);
    virtual AW_awar *add_target_var(float *pfloat);
    void    update();       // awar has changed

    virtual AW_awar *set_minmax(float min, float max);
    virtual AW_awar *set_srt(const char *srt);

    AW_awar *map(const char *awarn);
    AW_awar *map(AW_default dest); // map to new address
    AW_awar *map(AW_awar *dest); // map to new address
    AW_awar *unmap();           // map to original address

#if defined(ASSERTION_USED)
    bool is_valid() const { return correlated(gb_var, gb_origin); } // both or none NULL
#endif // ASSERTION_USED

    void get(char **p_string)  { freeset(*p_string, read_string()); } // deletes existing targets !!!
    void get(long *p_int)      { *p_int = (long)read_int(); }
    void get(double *p_double) { *p_double = read_float(); }
    void get(float *p_float)   { *p_float = read_float(); }

    virtual GB_TYPES get_type() const = 0;
    virtual const char* get_type_name() const { return "AW_awar"; }

    virtual char       *read_string();
    virtual const char *read_char_pntr();
    virtual char       *read_as_string();
    virtual long        read_int();
    virtual double      read_float();
    virtual GBDATA     *read_pointer();

    virtual GB_ERROR write_string(const char *aw_string, bool touch = false);
    virtual GB_ERROR write_as_string(const char *aw_string, bool touch = false);
    virtual GB_ERROR write_int(long aw_int, bool touch = false);
    virtual GB_ERROR write_float(double aw_double, bool touch = false);
    virtual GB_ERROR write_pointer(GBDATA *aw_pointer, bool touch = false);

    GB_ERROR write_as(char *aw_value) { return write_as_string(aw_value); }

    // same as write_-versions above, but always touches the database field
    GB_ERROR rewrite_string(const char *aw_string) { return write_string(aw_string, true);}
    GB_ERROR rewrite_as_string(const char *aw_string) { return write_as_string(aw_string, true); }
    GB_ERROR rewrite_int(long aw_int) { return write_int(aw_int, true); }
    GB_ERROR rewrite_float(double aw_double) { return write_float(aw_double, true); }
    GB_ERROR rewrite_pointer(GBDATA *aw_pointer) { return write_pointer(aw_pointer, true); }

    GB_ERROR rewrite_as(char *aw_value) { return rewrite_as_string(aw_value); };

    virtual GB_ERROR toggle_toggle();   // switches between 1/0
    void     touch();

    GB_ERROR make_global() __ATTR__USERESULT;       // should be used by ARB_init_global_awars only
    void set_temp_if_is_default(GBDATA *gb_db);
};
