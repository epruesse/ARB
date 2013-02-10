// =============================================================== //
//                                                                 //
//   File      : PARS_dtree.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pars_dtree.hxx"
#include "pars_main.hxx"
#include "pars_debug.hxx"
#include "ap_tree_nlen.hxx"

#include <AP_seq_dna.hxx>
#include <AP_seq_protein.hxx>
#include <AP_filter.hxx>

#include <ColumnStat.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <gui_aliview.hxx>

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>

static void AWT_graphic_parsimony_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;

    if (old == agt->tree_root_display) agt->tree_root_display = newroot;
}

static AliView *pars_generate_aliview(WeightedFilter *pars_weighted_filter) {
    GBDATA *gb_main = pars_weighted_filter->get_gb_main();
    char *ali_name;
    {
        GB_transaction ta(gb_main);
        ali_name = GBT_read_string(gb_main, AWAR_ALIGNMENT);
    }
    AliView *aliview = pars_weighted_filter->create_aliview(ali_name);
    if (!aliview) aw_popup_exit(GB_await_error());
    free(ali_name);
    return aliview;
}

void PARS_tree_init(AWT_graphic_tree *agt) {
    ap_assert(agt->get_root_node());
    ap_assert(agt == ap_main->get_tree_root());

    GB_transaction dummy(GLOBAL_gb_main);

    const char *use     = ap_main->get_aliname();
    long        ali_len = GBT_get_alignment_len(GLOBAL_gb_main, use);
    if (ali_len <= 1) {
        aw_popup_exit("No valid alignment selected! Try again");
    }

    agt->tree_static->set_root_changed_callback(AWT_graphic_parsimony_root_changed, agt);
}

static double funktion_quadratisch(double wert, double *param_list, int param_anz) {
    if (param_anz != 3) {
        ap_assert(0); // wrong number of parameters
        return 0;
    }
    return wert * wert * param_list[0] + wert * param_list[1] + param_list[2];
}


static void PARS_kernighan_cb(AP_tree *tree) {
    GB_push_transaction(GLOBAL_gb_main);

    long prevCombineCount = AP_sequence::combine_count();

    AP_FLOAT pars_start, pars_prev;
    pars_prev  = pars_start = GLOBAL_PARS->get_root_node()->costs();

    int rek_deep_max = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/maxdepth");

    AP_KL_FLAG funktype = (AP_KL_FLAG)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/function_type");

    int param_anz;
    double param_list[3];
    double f_startx, f_maxy, f_maxx, f_max_deep;
    f_max_deep = (double)rek_deep_max;                   //             x2
    f_startx = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/start");
    f_maxy = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/maxy");
    f_maxx = (double)*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/maxx");

    double (*funktion)(double wert, double *param_list, int param_anz);
    switch (funktype) {
        default:
        case AP_QUADRAT_START:
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[2] = f_startx;
            param_list[0] = (f_startx - f_maxy) / (f_maxx * f_maxx);
            param_list[1] = -2.0 * param_list[0] * f_maxx;
            break;
        case AP_QUADRAT_MAX:    // parameter liste fuer quadratische gleichung (y =ax^2 +bx +c)
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[0] =  - f_maxy / ((f_max_deep -  f_maxx) * (f_max_deep - f_maxx));
            param_list[1] =  -2.0 * param_list[0] * f_maxx;
            param_list[2] =  f_maxy  + param_list[0] * f_maxx * f_maxx;
            break;
    }


    AP_KL_FLAG searchflag=(AP_KL_FLAG)0;
    if (*GBT_read_int(GLOBAL_gb_main, "genetic/kh/dynamic/enable")) {
        searchflag = AP_DYNAMIK;
    }
    if (*GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/enable")) {
        searchflag = (AP_KL_FLAG)(searchflag|AP_STATIC);
    }

    int rek_breite[8];
    rek_breite[0] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth0");
    rek_breite[1] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth1");
    rek_breite[2] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth2");
    rek_breite[3] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth3");
    rek_breite[4] = *GBT_read_int(GLOBAL_gb_main, "genetic/kh/static/depth4");
    int rek_breite_anz = 5;

    int       anzahl = (int)(*GBT_read_float(GLOBAL_gb_main, "genetic/kh/nodes")*tree->arb_tree_leafsum2());
    AP_tree **list   = tree->getRandomNodes(anzahl);
    
    arb_progress progress(anzahl);

    progress.subtitle(GBS_global_string("Old Parsimony: %.1f", pars_start));

    GB_pop_transaction(GLOBAL_gb_main);

    for (int i=0; i<anzahl && !progress.aborted(); i++) {
        AP_tree_nlen *tree_elem = (AP_tree_nlen *)list[i];

        bool in_folded_group = tree_elem->gr.hidden ||
            (tree_elem->father && tree_elem->get_father()->gr.hidden);

        if (!in_folded_group) {
            bool better_tree_found = false;
            ap_main->push();
            display_clear(funktion, param_list, param_anz, (int)pars_start, (int)rek_deep_max);

            tree_elem->kernighan_rek(0,
                                     rek_breite, rek_breite_anz, rek_deep_max,
                                     funktion, param_list, param_anz,
                                     pars_start,  pars_start, pars_prev,
                                     searchflag, &better_tree_found);

            if (better_tree_found) {
                ap_main->clear();
                pars_start =  GLOBAL_PARS->get_root_node()->costs();
                progress.subtitle(GBS_global_string("New parsimony: %.1f (gain: %.1f)", pars_start, pars_prev-pars_start));
            }
            else {
                ap_main->pop();
            }
        }
        progress.inc();
    }
    delete list;
    printf("Combines: %li\n", AP_sequence::combine_count()-prevCombineCount);
}

void PARS_optimizer_cb(AP_tree *tree, arb_progress& progress) {
    AP_tree        *oldrootleft  = GLOBAL_PARS->get_root_node()->get_leftson();
    AP_tree        *oldrootright = GLOBAL_PARS->get_root_node()->get_rightson();
    const AP_FLOAT  org_pars     = GLOBAL_PARS->get_root_node()->costs();
    AP_FLOAT        prev_pars    = org_pars;

    progress.subtitle(GBS_global_string("Old parsimony: %.1f", org_pars));

    while (!progress.aborted()) {
        AP_FLOAT nni_pars = ((AP_tree_nlen *)tree)->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);

        if (nni_pars == prev_pars) { // NNI did not reduce costs -> kern-lin
            PARS_kernighan_cb(tree);
            AP_FLOAT ker_pars = GLOBAL_PARS->get_root_node()->costs();
            if (ker_pars == prev_pars) break; // kern-lin did not improve tree -> done
            prev_pars = ker_pars;
        }
        else {
            prev_pars = nni_pars;
        }
        progress.subtitle(GBS_global_string("New parsimony: %.1f (gain: %.1f)", prev_pars, org_pars-prev_pars));
    }

    if (oldrootleft->father == oldrootright) oldrootleft->set_root();
    else oldrootright->set_root();

    GLOBAL_PARS->get_root_node()->costs();
}

AWT_graphic_parsimony::AWT_graphic_parsimony(AW_root *root, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_)
    : AWT_graphic_tree(root, gb_main_, map_viewer_cb_)
{}

AWT_graphic_tree *PARS_generate_tree(AW_root *root, WeightedFilter *pars_weighted_filter) {
    AliView     *aliview   = pars_generate_aliview(pars_weighted_filter);
    AP_sequence *seq_templ = 0;

    GBDATA *gb_main = aliview->get_gb_main();
    {
        GB_transaction ta(gb_main);
        bool           is_aa = GBT_is_alignment_protein(gb_main, aliview->get_aliname());

        if (is_aa) seq_templ = new AP_sequence_protein(aliview);
        else seq_templ       = new AP_sequence_parsimony(aliview);
    }

    AWT_graphic_parsimony *apdt = new AWT_graphic_parsimony(root, aliview->get_gb_main(), PARS_map_viewer);

    apdt->init(AP_tree_nlen(0), aliview, seq_templ, true, false);

    ap_main->set_tree_root(apdt);
    return apdt;
}

AW_gc_manager AWT_graphic_parsimony::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2) {
    AW_init_color_group_defaults("arb_pars");

    AW_gc_manager preset_window =
        AW_manage_GC(aww, device, AWT_GC_CURSOR, AWT_GC_MAX, /* AWT_GC_CURSOR+7, */ AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb, (AW_CL)ntw, cd2,
                     true,      // uses color groups
                     "#AAAA55",

                     // Important note :
                     // Many gc indices are shared between ABR_NTREE and ARB_PARSIMONY
                     // e.g. the tree drawing routines use same gc's for drawing both trees
                     // (check AWT_dtree.cxx AWT_graphic_tree::init_devices)

                     "Cursor$#FFFFFF",
                     "Branch remarks$#DBE994",
                     "+-Bootstrap$#DBE994",    "-B.(limited)$white",
                     "--unused$#ff0000",
                     "Marked$#FFFF00",
                     "Some marked$#eeee88",
                     "Not marked$black",
                     "Zombies etc.$#cc5924",

                     "--unused", "--unused", // these reserve the numbers which are used for probe colors in ARB_NTREE
                     "--unused", "--unused", // (this is necessary because ARB_PARS and ARB_NTREE use the same tree painting routines)
                     "--unused", "--unused",
                     "--unused", "--unused",

                     NULL);
    return preset_window;
}

void AWT_graphic_parsimony::show(AW_device *device)
{
    long          parsval   = 0;
    AP_tree_nlen *root_node = GLOBAL_PARS->get_root_node();
    if (root_node) parsval  = root_node->costs();

    GLOBAL_PARS->awr->awar(AWAR_PARSIMONY)->write_int(parsval);
    long best = GLOBAL_PARS->awr->awar(AWAR_BEST_PARSIMONY)->read_int();
    if (parsval < best || 0==best) {
        GLOBAL_PARS->awr->awar(AWAR_BEST_PARSIMONY)->write_int(parsval);
    }
    this->AWT_graphic_tree::show(device);
}


void AWT_graphic_parsimony::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char,
                                    AW_event_type type, AW_pos x, AW_pos y,
                                    AW_clicked_line *cl, AW_clicked_text *ct)
{
    static int bl_drag_flag;

    AP_tree *at;

    bool compute_tree = false;
    bool recalc_branch_lengths = false;
    bool beautify_tree = false;

    switch (cmd) {
        case AWT_MODE_MOVE:                             // two point commands !!!!!
            if (button==AW_BUTTON_MIDDLE) {
                break;
            }
            switch (type) {
                case AW_Mouse_Press:
                    if (!(cl && cl->exists)) {
                        break;
                    }

                    // @@@ check security level
                    at = (AP_tree *)cl->client_data1;
                    if (at && at->father) {
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                    }
                    break;

                case AW_Mouse_Drag:
                    if (bl_drag_flag && this->rot_at && this->rot_at->father) {
                        this->rot_show_line(device);
                        if (cl->exists) {
                            this->rot_cl = *cl;
                        }
                        else {
                            rot_cl = old_rot_cl;
                        }
                        this->rot_show_line(device);
                    }
                    break;
                case AW_Mouse_Release:
                    if (bl_drag_flag && this->rot_at && this->rot_at->father) {
                        this->rot_show_line(device);
                        AP_tree *dest = 0;
                        if (cl->exists) dest = (AP_tree *)cl->client_data1;
                        AP_tree *source = rot_at;
                        if (!(source && dest)) {
                            aw_message("Please Drag Line to a valid branch");
                            break;
                        }
                        if (source == dest) {
                            aw_message("Please drag mouse from source to destination");
                            break;
                        }
                        const char *error = 0;

                        switch (button) {
                            case AW_BUTTON_LEFT:
                                error = source->cantMoveTo(dest);
                                if (!error) {
                                    source->moveTo(dest, cl->nearest_rel_pos);
                                    recalc_branch_lengths = true;
                                }
                                break;
                            case AW_BUTTON_RIGHT:
                                error = source->move_group_info(dest);
                                break;
                        }

                        if (error) aw_message(error);
                        this->exports.refresh = 1;
                        this->exports.save    = 1;
                        this->exports.resize  = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        compute_tree = true;
                    }
                    break;
                default:
                    break;
            }
            break;

#ifdef NNI_MODES
        case AWT_MODE_NNI:
            if (type==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (button) {
                    case AW_BUTTON_LEFT: {
                        if (cl->exists) {
                            arb_progress progress("NNI optimize subtree");

                            at                = (AP_tree *)cl->client_data1;
                            AP_tree_nlen *atn = DOWNCAST(AP_tree_nlen*, at);
                            atn->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);

                            exports.refresh = 1;
                            exports.save    = 1;

                            ASSERT_VALID_TREE(get_root_node());
                            recalc_branch_lengths = true;
                        }
                        break;
                    }
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("NNI optimize tree");
                        long         prevCombineCount = AP_sequence::combine_count();
                        
                        AP_tree_nlen *atn = DOWNCAST(AP_tree_nlen*, get_root_node());
                        atn->nn_interchange_rek(-1, AP_BL_NNI_ONLY, false);
                        printf("Combines: %li\n", AP_sequence::combine_count()-prevCombineCount);

                        exports.refresh = 1;
                        exports.save    = 1;

                        ASSERT_VALID_TREE(get_root_node());
                        recalc_branch_lengths = true;
                        break;
                    }
                }
                GB_begin_transaction(gb_main);
            }
            break;
        case AWT_MODE_KERNINGHAN:
            if (type==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            arb_progress progress("Kernighan-Lin optimize subtree");
                            at = (AP_tree *)cl->client_data1;
                            PARS_kernighan_cb(at);
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                            recalc_branch_lengths = true;
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Kernighan-Lin optimize tree");
                        PARS_kernighan_cb(get_root_node());
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        recalc_branch_lengths = true;
                        break;
                    }
                }
                GB_begin_transaction(gb_main);
            }
            break;
        case AWT_MODE_OPTIMIZE:
            if (type==AW_Mouse_Press) {
                GB_pop_transaction(gb_main);
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            arb_progress progress("Optimizing subtree");

                            at = (AP_tree *)cl->client_data1;
                            
                            if (at) PARS_optimizer_cb(at, progress);
                            this->exports.refresh = 1;
                            this->exports.save    = 1;
                            ASSERT_VALID_TREE(get_root_node());
                            recalc_branch_lengths = true;
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Optimizing tree");
                        
                        PARS_optimizer_cb(get_root_node(), progress);
                        this->exports.refresh = 1;
                        this->exports.save    = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        recalc_branch_lengths = true;
                        break;
                    }
                }
                GB_begin_transaction(gb_main);
            }
            break;
#endif // NNI_MODES

        default:
            AWT_graphic_tree::command(device, cmd, button, key_modifier, key_code, key_char, type, x, y, cl, ct);
            break;
    }

    if (recalc_branch_lengths) {
        arb_progress progress("Recalculating branch lengths");
        rootEdge()->calc_branchlengths();

        beautify_tree = true; // beautify after recalc_branch_lengths
    }

    if (beautify_tree) {
        this->resort_tree(0);
        this->exports.save = 1;

        compute_tree = true; // compute_tree after beautify_tree
    }

    if (compute_tree) {
        get_root_node()->compute_tree(gb_main);
        exports.refresh = 1;
    }
}
