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
#include "aw_root.hxx"
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

#include "aw_gtk_forward_declarations.hxx"

#include <vector>
#include <list>


// --------------
//      AWARS

class  AW_root_cblist;

// @@@ [CB] eliminate decls below? just use AW_RCB instead
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
    AW_WIDGET_PROGRESS_BAR
};

class AW_awar;

/**
 * Interface for mappers between AW_awar and GValue. 
 * (see AW_awar::bind())
 */
struct AW_awar_gvalue_mapper {
    virtual bool operator()(GValue*, AW_awar*) = 0;
    virtual bool operator()(AW_awar*, GValue*) = 0;
    virtual ~AW_awar_gvalue_mapper() {}
};

class AW_awar : virtual Noncopyable {
public:
    char      *awar_name; // deprecated -- use get_name();

    virtual ~AW_awar() {};

    virtual GB_TYPES get_type() const = 0;
    virtual const char* get_type_name() const { return "AW_awar"; }
    virtual const char* get_name() const { return awar_name; }


    virtual void bind_value(GObject* obj, const char* propname,
                            AW_awar_gvalue_mapper* mapper=NULL) = 0;
    virtual void unbind(GObject* obj) = 0;

    // add/remove callbacks
    virtual AW_awar *add_callback(const RootCallback& cb)    = 0;
    virtual AW_awar *remove_callback(const RootCallback& cb) = 0;

    // wrappers (deprecated!)
    AW_awar *add_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2)    { return add_callback(makeRootCallback(f, cd1, cd2)); }
    AW_awar *add_callback(Awar_CB1 f, AW_CL cd1)               { return add_callback(Awar_CB2(f), cd1, 0); }
    AW_awar *add_callback(Awar_CB0 f)                          { return add_callback(Awar_CB1(f), 0); }
    AW_awar *remove_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2) { return remove_callback(makeRootCallback(f, cd1, cd2)); }
    AW_awar *remove_callback(Awar_CB1 f, AW_CL cd1)            { return remove_callback(Awar_CB2(f), cd1, 0); }
    AW_awar *remove_callback(Awar_CB0 f)                       { return remove_callback(Awar_CB1(f), 0); }

    // target vars
    virtual AW_awar *add_target_var(char **ppchr) = 0;
    virtual AW_awar *add_target_var(long *pint) = 0;
    virtual AW_awar *add_target_var(float *pfloat) = 0;
    virtual void     update() = 0;       // awar has changed

    // limits
    virtual AW_awar *set_minmax(float min, float max) = 0;
    virtual float    get_min() const = 0;
    virtual float    get_max() const = 0;
    virtual AW_awar *set_srt(const char *srt) = 0;

    // ARBDB mapping
    virtual AW_awar *map(const char *awarn) = 0;
    virtual AW_awar *map(AW_default dest) = 0; // map to new address
    virtual AW_awar *map(AW_awar *dest) = 0; // map to new address
    virtual AW_awar *unmap() = 0;           // map to original address

    // read access
    virtual bool        has_default_value() = 0;
    virtual char       *read_string() = 0;
    virtual const char *read_char_pntr() = 0;
    virtual char       *read_as_string() = 0;
    virtual long        read_int() = 0;
    virtual double      read_float() = 0;
    virtual double      read_as_float() = 0;
    virtual GBDATA     *read_pointer() = 0;
    virtual bool        read_as_bool() = 0;
    void get(char **p_string)  { freeset(*p_string, read_string()); } // deletes existing targets !!!
    void get(long *p_int)      { *p_int = (long)read_int(); }
    void get(double *p_double) { *p_double = read_float(); }
    void get(float *p_float)   { *p_float = read_float(); }

    // write access
    virtual GB_ERROR write_string(const char *aw_string, bool touch = false) = 0;
    virtual GB_ERROR write_as_string(const char *aw_string, bool touch = false) = 0;
    virtual GB_ERROR write_int(long aw_int, bool touch = false) = 0;
    virtual GB_ERROR write_float(double aw_double, bool touch = false) = 0;
    virtual GB_ERROR write_pointer(GBDATA *aw_pointer, bool touch = false) = 0;
    virtual GB_ERROR write_as_bool(bool b, bool touch = false) = 0;
    GB_ERROR write_as(char *aw_value) { return write_as_string(aw_value); }

    // same as write_-versions above, but always touches the database field
    GB_ERROR rewrite_string(const char *aw_string) { return write_string(aw_string, true);}
    GB_ERROR rewrite_as_string(const char *aw_string) { return write_as_string(aw_string, true); }
    GB_ERROR rewrite_int(long aw_int) { return write_int(aw_int, true); }
    GB_ERROR rewrite_float(double aw_double) { return write_float(aw_double, true); }
    GB_ERROR rewrite_pointer(GBDATA *aw_pointer) { return write_pointer(aw_pointer, true); }

    GB_ERROR rewrite_as(char *aw_value) { return rewrite_as_string(aw_value); };

    virtual GB_ERROR toggle_toggle() = 0;   // switches between 1/0
    virtual void     touch() = 0;

    GB_ERROR make_global() __ATTR__USERESULT;       // should be used by ARB_init_global_awars only
    void set_temp_if_is_default(GBDATA *gb_db);
};
