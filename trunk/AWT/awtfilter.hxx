#ifndef awtfilter_hxx_included
#define awtfilter_hxx_included

struct adfiltercbstruct {
    AW_window *aws;
    AW_root   *awr;
    GBDATA    *gb_main;
    
    AW_selection_list *id;

    char *def_name;
    char *def_2name;
    char *def_2filter;
    char *def_2alignment;
    char *def_subname;
    char *def_alignment;
    char *def_simplify;
    char *def_source;
    char *def_dest;
    char *def_cancel;
    char *def_filter;
    char *def_min;
    char *def_max;
    char *def_len;
    char  may_be_an_error;

};

#endif
