#include <list>

class AW_awar_impl;

/**
 * Describes a binding between an AWAR and a GObject property.
 */
struct awar_gparam_binding {
    GObject      *obj;
    GParamSpec   *pspec;
    AW_awar_impl *awar;
    AW_awar_gvalue_mapper *mapper;
    bool          frozen;
    awar_gparam_binding(GObject *obj_, GParamSpec *pspec_, AW_awar_impl* awar_, 
                        AW_awar_gvalue_mapper* mapper_)
        : obj(obj_), pspec(pspec_),  awar(awar_), mapper(mapper_), frozen(false) {}
};

class AW_awar_impl : public AW_awar {
    AW_root_cblist       *callback_list;
    AW_widget_refresh_cb *refresh_list;
    bool in_tmp_branch;
    std::list<awar_gparam_binding> gparam_bindings;

    static bool allowed_to_run_callbacks;

#if defined(DEBUG)
    bool is_global;
#endif // DEBUG

    virtual void do_update() = 0;

    static void gbdata_changed(GBDATA*, int*, GB_CB_TYPE);
    static void gbdata_deleted(GBDATA*, int*, GB_CB_TYPE);

protected:
    void update_tmp_state(bool has_default_value);
    AW_awar_impl(const char *var_name);
    virtual ~AW_awar_impl();
    void run_callbacks();

public:
    const char *get_name() const { return awar_name; }

    GBDATA    *gb_origin;                    // this is set ONCE on creation of awar
    GBDATA    *gb_var;                       // if unmapped, points to same DB elem as 'gb_origin'

#if defined(ASSERTION_USED)
    bool is_valid() const { return correlated(gb_var, gb_origin); } // both or none NULL
#endif // ASSERTION_USED

    void bind_value(GObject* obj, const char* propname, AW_awar_gvalue_mapper* mapper) OVERRIDE;
    void tie_widget(AW_CL cd1, GtkWidget* widget, AW_widget_type type, AW_window *aww) OVERRIDE;

    void     update() OVERRIDE;
    
    AW_awar *add_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2) OVERRIDE;
    AW_awar *add_callback(Awar_CB1 f, AW_CL cd1) OVERRIDE;
    AW_awar *add_callback(Awar_CB0 f) OVERRIDE;
    AW_awar *remove_callback(Awar_CB2 f, AW_CL cd1, AW_CL cd2) OVERRIDE;   // remove a callback
    AW_awar *remove_callback(Awar_CB1 f, AW_CL cd1) OVERRIDE;
    AW_awar *remove_callback(Awar_CB0 f) OVERRIDE;

    virtual AW_awar *add_target_var(char **ppchr) OVERRIDE;
    virtual AW_awar *add_target_var(long *pint) OVERRIDE;
    virtual AW_awar *add_target_var(float *pfloat) OVERRIDE;

    virtual AW_awar *set_minmax(float min, float max) OVERRIDE;
    virtual float    get_min() const OVERRIDE;
    virtual float    get_max() const OVERRIDE;
    virtual AW_awar *set_srt(const char *srt) OVERRIDE;

    virtual char       *read_string() OVERRIDE;
    virtual const char *read_char_pntr() OVERRIDE;
    virtual char       *read_as_string() OVERRIDE;
    virtual long        read_int() OVERRIDE;
    virtual double      read_float() OVERRIDE;
    virtual double      read_as_float() OVERRIDE;
    virtual GBDATA     *read_pointer() OVERRIDE;
    virtual bool        read_as_bool() OVERRIDE;

    virtual GB_ERROR write_string(const char *aw_string, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_as_string(const char *aw_string, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_int(long aw_int, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_float(double aw_double, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_pointer(GBDATA *aw_pointer, bool touch = false) OVERRIDE;
    virtual GB_ERROR write_as_bool(bool b, bool touch = false) OVERRIDE;

    virtual GB_ERROR toggle_toggle() OVERRIDE;   // switches between 1/0

    virtual AW_awar *map(const char *awarn) OVERRIDE;
    virtual AW_awar *map(AW_default dest) OVERRIDE; // map to new address
    virtual AW_awar *map(AW_awar *dest) OVERRIDE; // map to new address
    virtual AW_awar *unmap() OVERRIDE;           // map to original addres
    virtual void     touch() OVERRIDE;

};

class AW_awar_int : public AW_awar_impl {
    long min_value;
    long max_value;
    long default_value;
    long value;
    std::vector<long*> target_variables;
public:
    AW_awar_int(const char *var_name, long var_value, AW_default default_file, AW_root *root);
    ~AW_awar_int();
    GB_TYPES get_type() const OVERRIDE { return GB_INT; } 
    const char* get_type_name() const { return "AW_awar_int"; }
    void      do_update() OVERRIDE;
    AW_awar*  set_minmax(float min, float max) OVERRIDE;
    float     get_min() const OVERRIDE;
    float     get_max() const OVERRIDE;
    
    GB_ERROR  write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR  write_int(long para, bool touch=false) OVERRIDE;
    GB_ERROR  write_float(double para, bool touch=false) OVERRIDE;
    GB_ERROR  write_as_bool(bool b, bool touch=false) OVERRIDE;
    long      read_int() OVERRIDE;
    char*     read_as_string() OVERRIDE;
    double    read_as_float() OVERRIDE { return read_int(); };
    bool      read_as_bool() OVERRIDE;
    AW_awar*  add_target_var(long *pint) OVERRIDE;
    GB_ERROR  toggle_toggle() OVERRIDE;
};

class AW_awar_float : public AW_awar_impl {
    double min_value;
    double max_value;
    double default_value;
    double value;
    std::vector<float*> target_variables;
public:
    AW_awar_float(const char *var_name, double var_value, AW_default default_file, AW_root *root);
    ~AW_awar_float();
    GB_TYPES get_type() const OVERRIDE { return GB_FLOAT; }
    const char* get_type_name() const { return "AW_awar_float"; }
    void        do_update() OVERRIDE;
    AW_awar*    set_minmax(float min, float max) OVERRIDE;
    float       get_min() const OVERRIDE;
    float       get_max() const OVERRIDE;
    GB_ERROR    write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_float(double para, bool touch=false) OVERRIDE;
    GB_ERROR    write_as_bool(bool b, bool touch=false) OVERRIDE;
    double      read_float() OVERRIDE;
    double      read_as_float() OVERRIDE { return read_float(); }
    char*       read_as_string() OVERRIDE;
    bool        read_as_bool() OVERRIDE;
    AW_awar*    add_target_var(float *pfloat) OVERRIDE;
    GB_ERROR    toggle_toggle() OVERRIDE;
};

class AW_awar_string : public AW_awar_impl {
    char* srt_program;
    char* default_value;
    char* value;
    std::vector<char**> target_variables;
public:
    AW_awar_string(const char *var_name, const char* var_value, AW_default default_file, AW_root *root);
    ~AW_awar_string();
    GB_TYPES    get_type() const OVERRIDE { return GB_STRING; }
    const char* get_type_name() const { return "AW_awar_string"; }
    void        do_update() OVERRIDE;
    AW_awar*    set_srt(const char *srt) OVERRIDE;
    GB_ERROR    write_as_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_string(const char* para, bool touch=false) OVERRIDE;
    GB_ERROR    write_as_bool(bool b, bool touch=false) OVERRIDE;
    char*       read_string() OVERRIDE;
    const char* read_char_pntr() OVERRIDE;
    char*       read_as_string() OVERRIDE;
    bool        read_as_bool() OVERRIDE;
    AW_awar*    add_target_var(char **ppchr) OVERRIDE;
    GB_ERROR    toggle_toggle() OVERRIDE;
};

class AW_awar_pointer : public AW_awar_impl {
    void *default_value;
    void *value;
public:
    AW_awar_pointer(const char *var_name, void* var_value, AW_default default_file, AW_root *root);
    ~AW_awar_pointer();
    GB_TYPES    get_type() const OVERRIDE { return GB_POINTER; }
    const char* get_type_name() const { return "AW_awar_pointer"; }
    void        do_update() OVERRIDE;
    GB_ERROR    write_pointer(GBDATA* para, bool touch=false) OVERRIDE;
    GBDATA*     read_pointer() OVERRIDE;
};
