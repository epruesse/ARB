#ifndef MPEXTERNS
#define MPEXTERNS

//#include <mpdefs.h>
//#include <aw_root.hxx>
//#include <aw_window.hxx>
//#include <awt.hxx>
class AW_window;
class AW_root;

void		MP_take_manual_sequence		(AW_window *aww);
void		MP_clear_manual_sequence	(AW_window *aww);
void 		MP_leftright			(AW_window *aww);
void 		MP_rightleft			(AW_window *aww);
void		MP_all_right			(AW_window *aww);
void		MP_del_all_sel_probes		(AW_window *aww);
void		MP_del_all_probes		(AW_window *aww);
void		MP_del_sel_probes		(AW_window *aww);
void		MP_del_probes			(AW_window *aww);
void		MP_show_probes_in_tree		(AW_window *aww);
void		MP_result_window		(AW_window *aww);
void		MP_del_all_result		(AW_window *aww);
void		MP_del_sel_result		(AW_window *aww);
void		MP_stop_comp			(AW_window *aww);
void		MP_result_chosen		(AW_window *aww);
void 		MP_close_main			(AW_window *aww);
void        MP_group_all_except_marked(AW_window *aww);
void 		MP_normal_colors_in_tree	(AW_window *aww);
void		MP_selected_chosen		(AW_window *aww);

char 		*MP_get_comment			(int which, char *str);		//faengt bei 1 an
char		*MP_remove_comment		(char *);
char 		*MP_get_probes			(char *str);
int 		MP_init_local_com_struct();
char		*MP_probe_pt_look_for_server();
// int 		MP_probe_design_send_data(T_PT_PDC  pdc);

#endif
