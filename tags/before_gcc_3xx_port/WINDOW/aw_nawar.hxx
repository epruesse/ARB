#ifndef aw_nawar_hxx_included
#define aw_nawar_hxx_included

/*************************************************************************/
struct AW_var_callback {
    AW_var_callback( void (*vc_cb)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 , AW_var_callback *next);

    void            (*value_changed_cb)(AW_root*,AW_CL,AW_CL);
    AW_CL            value_changed_cb_cd1;
    AW_CL            value_changed_cb_cd2;
    AW_var_callback *next;

    void            run_callback(AW_root *root);
    // runs the whole list in reverse order !!!!
};

/*************************************************************************/
struct AW_var_target {
    AW_var_target( void *pntr, AW_var_target *next);
    void          *pointer;
    AW_var_target *next;
    // runs the whole list in reverse order !!!!
};


void aw_update_awar_window_geometry(AW_root *awr);


#endif
