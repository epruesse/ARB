#ifndef probe_design_hxx_included
#define probe_design_hxx_included

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pd_assert(bed) arb_assert(bed)

AW_window *create_probe_design_window( AW_root *root, AW_default def);
AW_window *create_probe_match_window( AW_root *root, AW_default def);
AW_window *create_probe_admin_window( AW_root *root, AW_CL cl_genome_db);
AW_window *create_probe_group_result_window(AW_root *awr, AW_default cl_AW_canvas_ntw);

#define PROBE_DESIGN_EXCEPTION_MAX	1

#define AWAR_PROBE_ADMIN_PT_SERVER "tmp/probe_admin/pt_server"

#endif
