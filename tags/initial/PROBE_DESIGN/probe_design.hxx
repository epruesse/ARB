#ifndef probe_design_hxx_included
#define probe_design_hxx_included

#ifndef NDEBUG
# define pd_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define pd_assert(bed)
#endif

void probe_design_build_pt_server_choices(AW_window *aws,const char *var, AW_BOOL sel_list);

AW_window *create_probe_design_window( AW_root *root, AW_default def);
AW_window *create_probe_match_window( AW_root *root, AW_default def);
AW_window *create_probe_admin_window( AW_root *root, AW_default def);
AW_window *create_probe_group_result_window(AW_root *awr, AW_default cl_AW_canvas_ntw);

#define PROBE_DESIGN_EXCEPTION_MAX	1

#define AWAR_PROBE_ADMIN_PT_SERVER "tmp/probe_admin/pt_server"


#endif
