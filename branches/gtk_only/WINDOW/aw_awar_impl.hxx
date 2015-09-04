#include <list>

#include "aw_choice.hxx"

class AW_awar_impl;

/**
 * Describes a binding between an AWAR and a GObject property.
 */
struct awar_gparam_binding {
    AW_awar_impl *awar;  // owned by root
    GObject      *obj;   // weak
    GParamSpec   *pspec; // owned by obj
    gulong        handler_id;
    AW_awar_gvalue_mapper *mapper; // owned by us
    bool          frozen;

    awar_gparam_binding(AW_awar_impl* awar_, GObject *obj_)
        : awar(awar_), obj(obj_), 
          pspec(NULL), handler_id(0),
          mapper(NULL), frozen(false) {}
    ~awar_gparam_binding();
    
    /* bindings are equal if obj and awar are equal */
    bool operator==(const awar_gparam_binding& other) 
    { return obj  == other.obj  && awar == other.awar; }
    /* copying just transfers obj and awar */
    awar_gparam_binding(const awar_gparam_binding& other) 
        : awar(other.awar), obj(other.obj),
          pspec(NULL), handler_id(0),
          mapper(NULL), frozen(false){}
    awar_gparam_binding& operator=(const awar_gparam_binding& other) 
    { awar = other.awar, obj=other.obj; return *this;}

    bool connect(const char* propname, AW_awar_gvalue_mapper* mapper);
};

class AW_awar_impl : public AW_awar, virtual Noncopyable {
    bool            in_tmp_branch;
    std::list<awar_gparam_binding> gparam_bindings;

    static bool allowed_to_run_callbacks;

    double callback_time_sum;   // in seconds
    int    callback_time_count; // number of callbacks traced in callback_time_sum

    static void gbdata_changed(GBDATA*, int*, GB_CB_TYPE);
    static void gbdata_deleted(GBDATA*, int*, GB_CB_TYPE);

    void         remove_all_callbacks();
    virtual void remove_all_target_vars() = 0;
    virtual void do_update() = 0;
protected:
    AW_choice_list  choices;

    void update_tmp_state(bool has_default_value);
    AW_awar_impl(const char *var_name);
    virtual ~AW_awar_impl();
    void run_callbacks();

public:
    const char *get_name() const { return awar_name; }

    GBDATA    *gb_origin;                    // this is set ONCE on creation of awar
    GBDATA    *gb_var;                       // if unmapped, points to same DB elem as 'gb_origin'

    bool is_valid() const { return correlated(gb_var, gb_origin); } // both or none NULL

    void bind_value(GObject* obj, const char* propname, AW_awar_gvalue_mapper* mapper) OVERRIDE;
    void unbind(GObject* obj) OVERRIDE;

    void unlink();
    void unlink_from_DB(GBDATA*);
    void set_temp_if_is_default(GBDATA *);

    void update() OVERRIDE;
    void update_choices() OVERRIDE { choices.update(); }

    double mean_callback_time() const OVERRIDE {
        if (callback_time_sum>0) {
            return callback_time_sum / callback_time_count;
        }
        return 0.0;
    }

    virtual AW_choice *add_choice(AW_action&, int) OVERRIDE;
    virtual AW_choice *add_choice(AW_action&, double) OVERRIDE;
    virtual AW_choice *add_choice(AW_action&, const char*) OVERRIDE;

    AW_awar *add_callback(const RootCallback& cb) OVERRIDE;
    AW_awar *remove_callback(const RootCallback& cb) OVERRIDE;

    virtual AW_awar *add_target_var(char **ppchr) OVERRIDE;
    virtual AW_awar *add_target_var(long *pint) OVERRIDE;
    virtual AW_awar *add_target_var(float *pfloat) OVERRIDE;

    virtual AW_awar *set_minmax(float min, float max) OVERRIDE;
    virtual float    get_min() const OVERRIDE;
    virtual float    get_max() const OVERRIDE;
    virtual AW_awar *set_srt(const char *srt) OVERRIDE;

    virtual bool        has_default_value() const OVERRIDE = 0;
    virtual char       *read_string() const       OVERRIDE;
    virtual const char *read_char_pntr() const    OVERRIDE;
    virtual char       *read_as_string() const    OVERRIDE;
    virtual long        read_int() const          OVERRIDE;
    virtual double      read_float() const        OVERRIDE;
    virtual double      read_as_float() const     OVERRIDE;
    virtual GBDATA     *read_pointer() const      OVERRIDE;
    virtual bool        read_as_bool() const      OVERRIDE;

    virtual GB_ERROR write_string(const char *aw_string, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_as_string(const char *aw_string, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_int(long aw_int, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_float(double aw_double, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_pointer(GBDATA *aw_pointer, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_as_bool(bool b, bool touch = false) OVERRIDE;

    virtual GB_ERROR toggle_toggle() OVERRIDE;   // switches between 1/0

    virtual AW_awar *map(const char *awarn) OVERRIDE;
    virtual AW_awar *map(AW_default dest) OVERRIDE; // map to new address
    virtual AW_awar *map(AW_awar *dest) OVERRIDE;   // map to new address
    virtual AW_awar *unmap() OVERRIDE;              // map to original addres
    virtual bool     is_mapped() const OVERRIDE;    // returns true if awar is remapped to new address
    virtual void     touch() OVERRIDE;
};

class AW_awar_int : public AW_awar_impl, virtual Noncopyable {
    long min_value;
    long max_value;
    long default_value;
    long value;
    std::vector<long*> target_variables;
    void     remove_all_target_vars() OVERRIDE;
public:
    AW_awar_int(const char *var_name, long var_value, AW_default default_file, AW_root *root);
    ~AW_awar_int();
    GB_TYPES get_type() const OVERRIDE { return GB_INT; } 
    const char* get_type_name() const { return "AW_awar_int"; }
    void       do_update() OVERRIDE;
    AW_awar*   set_minmax(float min, float max) OVERRIDE;
    float      get_min() const OVERRIDE;
    float      get_max() const OVERRIDE;
    AW_choice *add_choice(AW_action&, int) OVERRIDE;
    
    GB_ERROR  write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR  write_int(long para, bool touch=false) OVERRIDE;
    GB_ERROR  write_float(double para, bool touch=false) OVERRIDE;
    GB_ERROR  write_as_bool(bool b, bool touch=false) OVERRIDE;

    bool      has_default_value() const OVERRIDE;
    long      read_int() const OVERRIDE;
    char*     read_as_string() const OVERRIDE;
    double    read_as_float() const OVERRIDE { return read_int(); };
    bool      read_as_bool() const OVERRIDE;
    AW_awar*  add_target_var(long *pint) OVERRIDE;
    GB_ERROR  toggle_toggle() OVERRIDE;
    GB_ERROR  reset_to_default() OVERRIDE;
};

class AW_awar_float : public AW_awar_impl {
    double min_value;
    double max_value;
    double default_value;
    double value;
    std::vector<float*> target_variables;
    void     remove_all_target_vars() OVERRIDE;
    bool      has_default_value() const OVERRIDE;
public:
    AW_awar_float(const char *var_name, double var_value, AW_default default_file, AW_root *root);
    ~AW_awar_float();
    GB_TYPES get_type() const OVERRIDE { return GB_FLOAT; }
    const char* get_type_name() const { return "AW_awar_float"; }
    void        do_update() OVERRIDE;
    AW_awar*    set_minmax(float min, float max) OVERRIDE;
    float       get_min() const OVERRIDE;
    float       get_max() const OVERRIDE;
    AW_choice  *add_choice(AW_action&, double) OVERRIDE;

    GB_ERROR    write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_float(double para, bool touch=false) OVERRIDE;
    GB_ERROR    write_as_bool(bool b, bool touch=false) OVERRIDE;
    double      read_float() const OVERRIDE;
    double      read_as_float() const OVERRIDE { return read_float(); }
    char*       read_as_string() const OVERRIDE;
    bool        read_as_bool() const OVERRIDE;
    AW_awar*    add_target_var(float *pfloat) OVERRIDE;
    GB_ERROR    toggle_toggle() OVERRIDE;
    GB_ERROR    reset_to_default() OVERRIDE;
};

class AW_awar_string : public AW_awar_impl, virtual Noncopyable {
    char* srt_program;
    char* default_value;
    char* value;
    std::vector<char**> target_variables;
    void     remove_all_target_vars() OVERRIDE;
    bool      has_default_value() const OVERRIDE;
public:
    AW_awar_string(const char *var_name, const char* var_value, AW_default default_file, AW_root *root);
    ~AW_awar_string();
    GB_TYPES    get_type() const OVERRIDE { return GB_STRING; }
    const char* get_type_name() const { return "AW_awar_string"; }
    void        do_update() OVERRIDE;
    AW_awar*    set_srt(const char *srt) OVERRIDE;
    AW_choice  *add_choice(AW_action&, const char*) OVERRIDE;
    GB_ERROR    write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_as_bool(bool b, bool touch=false) OVERRIDE;
    char*       read_string() const OVERRIDE;
    const char* read_char_pntr() const OVERRIDE;
    char*       read_as_string() const OVERRIDE;
    bool        read_as_bool() const OVERRIDE;
    AW_awar*    add_target_var(char **ppchr) OVERRIDE;
    GB_ERROR    toggle_toggle() OVERRIDE;
    GB_ERROR    reset_to_default() OVERRIDE;
};

class AW_awar_pointer : public AW_awar_impl, virtual Noncopyable {
    GBDATA *default_value;
    GBDATA *value;

    void     remove_all_target_vars() OVERRIDE;
    bool      has_default_value() const OVERRIDE;
public:
    AW_awar_pointer(const char *var_name, GBDATA* var_value, AW_default default_file, AW_root *root);
    ~AW_awar_pointer();
    GB_TYPES    get_type() const OVERRIDE { return GB_POINTER; }
    const char* get_type_name() const { return "AW_awar_pointer"; }
    void        do_update() OVERRIDE;
    GB_ERROR    write_pointer(GBDATA* para, bool touch=false) OVERRIDE;
    GBDATA*     read_pointer() const OVERRIDE;
    GB_ERROR    reset_to_default() OVERRIDE;
};


