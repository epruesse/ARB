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

#include "aw_signal.hxx"

#include "aw_gtk_forward_declarations.hxx"

#include <vector>
#include <list>


// --------------
//      AWARS

class  AW_root_cblist;

class AW_awar;
class AW_choice;

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
    AW_signal changed;
    AW_signal changed_by_user;
    AW_signal dclicked;

    virtual ~AW_awar() {};

    virtual GB_TYPES get_type() const = 0;
    virtual const char* get_type_name() const { return "AW_awar"; }
    virtual const char* get_name() const { return awar_name; }

#if defined(ASSERTION_USED)
    static bool deny_read;  // true -> make awar reads fail
    static bool deny_write; // true -> make awar writes fail
#endif

    virtual void bind_value(GObject* obj, const char* propname, AW_awar_gvalue_mapper* mapper=NULL) = 0;
    virtual void unbind(GObject* obj) = 0;

    virtual AW_choice* add_choice(AW_action&, int)         = 0;
    virtual AW_choice* add_choice(AW_action&, double)      = 0;
    virtual AW_choice* add_choice(AW_action&, const char*) = 0;

    virtual void update_choices() = 0;

    // add/remove callbacks
    virtual AW_awar *add_callback(const RootCallback& cb)    = 0;
    virtual AW_awar *remove_callback(const RootCallback& cb) = 0;

    AW_awar *add_callback(RootCallbackSimple f)    { return add_callback(makeRootCallback(f)); }
    AW_awar *remove_callback(RootCallbackSimple f) { return remove_callback(makeRootCallback(f)); }

    // target vars
    virtual AW_awar *add_target_var(char **ppchr) = 0;
    virtual AW_awar *add_target_var(long *pint) = 0;
    virtual AW_awar *add_target_var(float *pfloat) = 0;
    virtual void     update() = 0;       // awar has changed

    virtual double mean_callback_time() const = 0;

    // limits
    virtual AW_awar *set_minmax(float min, float max) = 0;  // min<max !!!
    virtual float    get_min() const = 0;
    virtual float    get_max() const = 0;
    virtual AW_awar *set_srt(const char *srt) = 0;

    AW_awar *set_min(float min) { return set_minmax(min, get_max()); }

    // ARBDB mapping
    virtual AW_awar *map(const char *awarn) = 0;
    virtual AW_awar *map(AW_default dest)   = 0; // map to new address
    virtual AW_awar *map(AW_awar *dest)     = 0; // map to new address
    virtual AW_awar *unmap()                = 0; // map to original address
    virtual bool is_mapped() const          = 0; // returns true if awar is remapped to new address

    // read access
    virtual bool        has_default_value() const = 0;
    virtual char       *read_string() const       = 0;
    virtual const char *read_char_pntr() const    = 0;
    virtual char       *read_as_string() const    = 0;
    virtual long        read_int() const          = 0;
    virtual double      read_float() const        = 0;
    virtual double      read_as_float() const     = 0;
    virtual GBDATA     *read_pointer() const      = 0;
    virtual bool        read_as_bool() const      = 0;

    void get(char **p_string) const { freeset(*p_string, read_string()); } // deletes existing targets !!!
    void get(long *p_int) const { *p_int = (long)read_int(); }
    void get(double *p_double) const { *p_double = read_float(); }
    void get(float *p_float) const { *p_float = read_float(); }

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

    virtual GB_ERROR reset_to_default() = 0;

    GB_ERROR make_global() __ATTR__USERESULT;       // should be used by ARB_init_global_awars only
};

void AW_awar_dump_changes(const char* name);
