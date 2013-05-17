// =========================================================== //
//                                                             //
//   File      : ED4_colStat.cxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#include "ed4_colStat.hxx"
#include "ed4_class.hxx"
#include "ed4_extern.hxx"

#include <st_window.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>

inline void set_col_stat_activated_and_refresh(bool activated) {
    ED4_ROOT->column_stat_activated = activated;
    ED4_ROOT->request_refresh_for_sequence_terminals();
}

static void col_stat_activated(AW_window *) {
    ED4_ROOT->column_stat_initialized  = true;
    set_col_stat_activated_and_refresh(true);
}

void ED4_activate_col_stat(AW_window *aww, AW_CL, AW_CL) {
    if (!ED4_ROOT->column_stat_initialized) {
        AW_window *aww_st = STAT_create_main_window(ED4_ROOT->aw_root, ED4_ROOT->st_ml, (AW_CB0)col_stat_activated, (AW_window *)aww);
        aww_st->show();
    }
    else { // re-activate
        set_col_stat_activated_and_refresh(true);
    }
}
void ED4_disable_col_stat(AW_window *, AW_CL, AW_CL) {
    if (ED4_ROOT->column_stat_initialized && ED4_ROOT->column_stat_activated) {
        set_col_stat_activated_and_refresh(false);
    }
}


static void show_detailed_column_stats_activated(AW_window *aww) {
    ED4_ROOT->column_stat_initialized = true;
    ED4_toggle_detailed_column_stats(aww, 0, 0);
}

double ED4_columnStat_terminal::threshold = -1;
int ED4_columnStat_terminal::threshold_is_set() {
    return threshold>=0 && threshold<=100;
}
void ED4_columnStat_terminal::set_threshold(double aThreshold) {
    threshold = aThreshold;
    e4_assert(threshold_is_set());
}

void ED4_set_col_stat_threshold(AW_window *, AW_CL, AW_CL) {
    double default_threshold = 90.0;
    if (ED4_columnStat_terminal::threshold_is_set()) {
        default_threshold = ED4_columnStat_terminal::get_threshold();
    }
    char default_input[40];
    sprintf(default_input, "%6.2f", default_threshold);

    char *input = aw_input("Please insert threshold value for marking:", default_input);
    if (input) {
        double input_threshold = atof(input);

        if (input_threshold<0 || input_threshold>100) {
            aw_message("Illegal threshold value (allowed: 0..100)");
        }
        else {
            ED4_columnStat_terminal::set_threshold(input_threshold);
            ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_COL_STAT);
        }
        free(input);
    }
}

void ED4_toggle_detailed_column_stats(AW_window *aww, AW_CL, AW_CL) {
    while (!ED4_columnStat_terminal::threshold_is_set()) {
        ED4_set_col_stat_threshold(aww, 0, 0);
    }

    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();

    GB_ERROR error = NULL;
    if (!cursor->owner_of_cursor) {
        error = "First you have to place your cursor";
    }
    else if (!cursor->in_species_seq_terminal()) {
        error = "Display of column-statistic-details is only possible for species!";
    }
    else {
        ED4_sequence_terminal *seq_term = cursor->owner_of_cursor->to_sequence_terminal();
        if (!seq_term->st_ml_node && !(seq_term->st_ml_node = STAT_find_node_by_name(ED4_ROOT->st_ml, seq_term->species_name))) {
            if (ED4_ROOT->column_stat_initialized) {
                error = "Cannot display column statistics for this species (internal error?)";
            }
            else {
                AW_window *aww_st = STAT_create_main_window(ED4_ROOT->aw_root, ED4_ROOT->st_ml, (AW_CB0)show_detailed_column_stats_activated, (AW_window *)aww);
                aww_st->show();
            }
        }
        else {
            ED4_multi_sequence_manager *multi_seq_man    = seq_term->get_parent(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
            ED4_base                   *existing_colstat = multi_seq_man->search_spec_child_rek(ED4_L_COL_STAT);

            if (existing_colstat) {
                ED4_manager *colstat_seq_man = existing_colstat->get_parent(ED4_L_SEQUENCE)->to_manager();
                colstat_seq_man->Delete();
            }
            else { // add
                char buffer[35];
                int count = 1;
                sprintf(buffer, "Sequence_Manager.%ld.%d", ED4_counter, count++);

                ED4_sequence_manager *new_seq_man = new ED4_sequence_manager(buffer, 0, 0, 0, 0, multi_seq_man);
                new_seq_man->set_property(ED4_P_MOVABLE);
                multi_seq_man->children->append_member(new_seq_man);

                int    pixel_length     = max_seq_terminal_length;
                AW_pos font_height      = ED4_ROOT->font_group.get_height(ED4_G_SEQUENCES);
                AW_pos columnStatHeight = ceil((COLUMN_STAT_ROWS+0.5 /* reserve a bit more space */)*COLUMN_STAT_ROW_HEIGHT(font_height));

                ED4_columnStat_terminal    *ref_colStat_terminal      = ED4_ROOT->ref_terminals.get_ref_column_stat();
                ED4_sequence_info_terminal *ref_colStat_info_terminal = ED4_ROOT->ref_terminals.get_ref_column_stat_info();

                ref_colStat_terminal->extension.size[HEIGHT] = columnStatHeight;
                ref_colStat_terminal->extension.size[WIDTH]  = pixel_length;

                ED4_sequence_info_terminal *new_colStat_info_term = new ED4_sequence_info_terminal("CStat", 0, 0, SEQUENCEINFOSIZE, columnStatHeight, new_seq_man);
                new_colStat_info_term->set_property((ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE));
                new_colStat_info_term->set_links(ref_colStat_info_terminal, ref_colStat_terminal);
                new_seq_man->children->append_member(new_colStat_info_term);

                sprintf(buffer, "Column_Statistic_Terminal.%ld.%d", ED4_counter, count++);
                ED4_columnStat_terminal *new_colStat_term = new ED4_columnStat_terminal(buffer, SEQUENCEINFOSIZE, 0, 0, columnStatHeight, new_seq_man);
                new_colStat_term->set_links(ref_colStat_terminal, ref_colStat_terminal);
                new_seq_man->children->append_member(new_colStat_term);

                ED4_counter++;

                new_seq_man->resize_requested_by_child();
            }
        }
    }

    if (error) aw_message(error);
}


