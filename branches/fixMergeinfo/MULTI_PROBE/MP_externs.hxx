// ============================================================= //
//                                                               //
//   File      : MP_externs.hxx                                  //
//   Purpose   : global functions (inside MULTI_PROBE)           //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MP_EXTERNS_HXX
#define MP_EXTERNS_HXX

class AW_window;
class AW_root;
class arb_progress;

typedef long AW_CL;             // generic client data type (void *)

void MP_show_probes_in_tree(AW_window *aww);
void MP_show_probes_in_tree_move(AW_window *aww, AW_CL cl_backward, AW_CL cl_result_probes_list);
void MP_popup_result_window(AW_window *aww);
void MP_delete_selected(AW_window*, AW_CL cl_sellist);
void MP_result_chosen(AW_window *aww);
void MP_close_main(AW_window *aww);
void MP_group_all_except_marked(AW_window *aww);
void MP_normal_colors_in_tree(AW_window *aww);
void MP_selected_chosen(AW_window *aww);

bool MP_aborted(int gen_cnt, double avg_fit, double min_fit, double max_fit, arb_progress& progress);

char *MP_get_comment(int which, const char *str);
int MP_init_local_com_struct();

const char *MP_probe_pt_look_for_server();

#else
#error MP_externs.hxx included twice
#endif // MP_EXTERNS_HXX
