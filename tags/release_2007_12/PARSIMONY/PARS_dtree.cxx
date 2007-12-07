#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_seq_dna.hxx>
#include <awt_seq_protein.hxx>

#include <awt_csp.hxx>
#include <awt.hxx>
#include <awt_dtree.hxx>
#include <awt_sel_boxes.hxx>
#include "pars_dtree.hxx"

#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "ap_tree_nlen.hxx"
#include "pars_main.hxx"
#include "pars_debug.hxx"

extern AWT_csp *awt_csp;

char *AWT_graphic_parsimony_root_changed(void *cd, AP_tree *old, AP_tree *newroot)
{
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (old == agt->tree_root_display) agt->tree_root_display = newroot;
    if (old == agt->tree_root) agt->tree_root = newroot;
    if (old == nt->tree->tree_root) nt->tree->tree_root = newroot;
    return 0;
}


/**************************
tree_init()

	Filter & weights setup
	loads sequences at the leafs and
	initalize filters & weights for the tree
	( AP_tree_nlen expected )

**************************/
void NT_tree_init(AWT_graphic_tree *agt) {

    AP_tree *tree = agt->tree_root;
    GB_transaction dummy(gb_main);

    if (!tree) {
        return;
    }
    char *use = GBT_read_string(gb_main,AWAR_ALIGNMENT);

    long ali_len = GBT_get_alignment_len(gb_main,use);
    if (ali_len <=1) {
        aw_message("No valid alignment selected ! Try again","OK");
        exit(0);
    }


    GB_BOOL is_aa = GBT_is_alignment_protein(gb_main,use);
    //
    // filter & weights setup
    //
    if (!tree->tree_root->sequence_template) {
        AP_tree_root *tr = tree->tree_root;
        AP_sequence *sproto;
        if (is_aa) {
            sproto = (AP_sequence *)new AP_sequence_protein(tr);
        }else{
            sproto = (AP_sequence *)new AP_sequence_parsimony(tr);
        }

        tr->sequence_template = sproto;
        tr->filter = awt_get_filter(agt->aw_root, pars_global_filter);
        tr->weights = new AP_weights();

        awt_csp->go(0);
        int i;
        if (awt_csp->rates){
            for (i=0;i<ali_len;i++){
                if (awt_csp->rates[i]>0.0000001){
                    awt_csp->weights[i] *= (int)(2.0/ awt_csp->rates[i]);
                }
            }
            tr->weights->init(awt_csp->weights , tr->filter);
        }else{
            tr->weights->init(tr->filter);
        }
        tree->load_sequences_rek(use,GB_FALSE,GB_TRUE);   	// load with sequences
    }
    tree->tree_root->root_changed_cd = (void*)agt;
    tree->tree_root->root_changed = AWT_graphic_parsimony_root_changed;

    ap_main->use = use;
}

static int ap_global_abort_flag;

double funktion_quadratisch(double x,double *param_list,int param_anz) {
    AP_FLOAT ergebnis;
    double wert = (double)x;
    if (param_anz != 3) {
        AW_ERROR("funktion_quadratisch: Falsche Parameteranzahl !");
        return (AP_FLOAT)0;
    }
    ergebnis = wert * wert * param_list[0] + wert * param_list[1] + param_list[2];
    return ergebnis;
}


void PARS_kernighan_cb(AP_tree *tree) {

    GB_push_transaction(gb_main);

    global_combineCount = 0;

    AP_FLOAT pars_start, pars_prev;
    pars_prev  = pars_start = nt->tree->tree_root->costs();

    int rek_deep_max = (int)GBT_read_int(gb_main,"genetic/kh/maxdepth");

    AP_KL_FLAG funktype = (AP_KL_FLAG)GBT_read_int(gb_main,"genetic/kh/function_type");

    int param_anz;
    double param_list[3];
    double f_startx,f_maxy,f_maxx,f_max_deep;
    f_max_deep = (double)rek_deep_max;			 //		x2
    f_startx = (double)GBT_read_int(gb_main,"genetic/kh/dynamic/start");
    f_maxy = (double)GBT_read_int(gb_main,"genetic/kh/dynamic/maxy");
    f_maxx = (double)GBT_read_int(gb_main,"genetic/kh/dynamic/maxx");

    double (*funktion)(double wert,double *param_list,int param_anz);
    switch (funktype) {
        default:
        case AP_QUADRAT_START:
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[2] = f_startx;
            param_list[0] = (f_startx - f_maxy) / (f_maxx * f_maxx);
            param_list[1] = -2.0 * param_list[0] * f_maxx;
            break;
        case AP_QUADRAT_MAX:	// parameter liste fuer quadratische gleichung (y =ax^2 +bx +c)
            funktion = funktion_quadratisch;
            param_anz = 3;
            param_list[0] =  - f_maxy / (( f_max_deep -  f_maxx) * ( f_max_deep - f_maxx));
            param_list[1] =  -2.0 * param_list[0] * f_maxx;
            param_list[2] =  f_maxy  + param_list[0] * f_maxx * f_maxx;
            break;
    }


    AP_KL_FLAG searchflag=(AP_KL_FLAG)0;
    if ( GBT_read_int(gb_main,"genetic/kh/dynamic/enable")){
        searchflag = AP_DYNAMIK;
    }
    if ( GBT_read_int(gb_main,"genetic/kh/static/enable")){
        searchflag = (AP_KL_FLAG)(searchflag|AP_STATIC);
    }

    int rek_breite[8];
    rek_breite[0] = (int)GBT_read_int(gb_main,"genetic/kh/static/depth0");
    rek_breite[1] = (int)GBT_read_int(gb_main,"genetic/kh/static/depth1");
    rek_breite[2] = (int)GBT_read_int(gb_main,"genetic/kh/static/depth2");
    rek_breite[3] = (int)GBT_read_int(gb_main,"genetic/kh/static/depth3");
    rek_breite[4] = (int)GBT_read_int(gb_main,"genetic/kh/static/depth4");
    int rek_breite_anz = 5;

    int anzahl = (int)(GBT_read_float(gb_main,"genetic/kh/nodes")*tree->arb_tree_leafsum2());
    AP_tree **list;
    list = tree->getRandomNodes(anzahl);
    int i =0;


    i = 0;
    aw_openstatus("KL Optimizer");
    {
        char buffer[100];
        sprintf(buffer,"Old Parsimony: %f",pars_start);
        aw_status(buffer);
    }
    int abort_flag = AP_FALSE;

    GB_pop_transaction(gb_main);

    for (i=0;i<anzahl && ! abort_flag; i++) {
        abort_flag |= aw_status(i/(double)anzahl);
        if (abort_flag) break;

        AP_tree_nlen *tree_elem = (AP_tree_nlen *)list[i];

        if (tree_elem->gr.hidden ||
            (tree_elem->father && tree_elem->father->gr.hidden)){
            continue;	// within a folded group
        }
        {
            AP_BOOL better_tree_found = AP_FALSE;
            ap_main->push();
            display_clear(funktion,param_list,param_anz,(int)pars_start,(int)rek_deep_max);

            tree_elem->kernighan_rek(0,
                                     rek_breite,rek_breite_anz,rek_deep_max,
                                     funktion, param_list,param_anz,
                                     pars_start,  pars_start, pars_prev,
                                     searchflag,&better_tree_found);

            if (better_tree_found) {
                ap_main->clear();
                pars_start =  nt->tree->tree_root->costs();
                char buffer[100];
                sprintf(buffer,"New Parsimony: %f",pars_start);
                abort_flag |= aw_status(buffer);
            } else {
                ap_main->pop();
            }
        }
    }
    aw_closestatus();
    delete list;
    ap_global_abort_flag |= abort_flag;
    printf("Combines: %li\n",global_combineCount);
    return;
}

void PARS_optimizer_cb(AP_tree *tree) {
    AP_tree *oldrootleft = nt->tree->tree_root->leftson;
    AP_tree *oldrootright = nt->tree->tree_root->rightson;
    for (ap_global_abort_flag = 0;!ap_global_abort_flag;){
        AP_FLOAT old_pars = nt->tree->tree_root->costs();
        ((AP_tree_nlen *)tree)->nn_interchange_rek(AP_TRUE,ap_global_abort_flag,-1,AP_BL_NNI_ONLY, GB_TRUE); // only not hidden
        if (ap_global_abort_flag) break;
        PARS_kernighan_cb(tree);
        if (old_pars == nt->tree->tree_root->costs()) {
            ap_global_abort_flag = 1;
        }
    }
    if (oldrootleft->father == oldrootright) oldrootleft->set_root();
    else oldrootright->set_root();
    nt->tree->tree_root->costs();
    aw_closestatus();
}

AWT_graphic_parsimony::AWT_graphic_parsimony(AW_root *root, GBDATA *gb_maini):AWT_graphic_tree(root,gb_maini)
{;}

AWT_graphic *
PARS_generate_tree( AW_root *root )
{
    AWT_graphic_parsimony *apdt = new AWT_graphic_parsimony(root,gb_main);
    AP_tree_nlen *aptnl = new AP_tree_nlen(0);
    apdt->init((AP_tree *)aptnl);
    ap_main->tree_root = &apdt->tree_root;
    delete aptnl;
    return (AWT_graphic *)apdt;
}

AW_gc_manager
AWT_graphic_parsimony::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2)
{
    AW_init_color_group_defaults("arb_pars");

    AW_gc_manager preset_window =
        AW_manage_GC(aww,device,AWT_GC_CURSOR, AWT_GC_MAX, /*AWT_GC_CURSOR+7,*/ AW_GCM_DATA_AREA,
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

                     0 );
    return preset_window;
}

void AWT_graphic_parsimony::show(AW_device *device)
{
    long parsval = 0;
    if (nt->tree->tree_root) parsval = (long)nt->tree->tree_root->costs();
    nt->awr->awar(AWAR_PARSIMONY)->write_int( parsval);
    long best = nt->awr->awar(AWAR_BEST_PARSIMONY)->read_int();
    if (parsval < best || 0==best) {
        nt->awr->awar(AWAR_BEST_PARSIMONY)->write_int( parsval);
    }
    this->AWT_graphic_tree::show(device);
}


void AWT_graphic_parsimony::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char,
                                    AW_event_type type, AW_pos x, AW_pos y,
                                    AW_clicked_line *cl, AW_clicked_text *ct)
{
    static int bl_drag_flag;

    AWUSE(ct);
    AP_tree *at;

    bool compute_tree = false;
    bool recalc_branch_lengths = false;
    bool beautify_tree = false;

    //GBDATA *gb_main = this->tree_static->gb_main;
    switch(cmd){
        case AWT_MODE_MOVE:				// two point commands !!!!!
            if(button==AWT_M_MIDDLE){
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( !(cl && cl->exists) ){
                        break;
                    }

                    /*** check security level @@@ ***/
                    at = (AP_tree *)cl->client_data1;
                    if(at && at->father){
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                    }
                    break;

                case AW_Mouse_Drag:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        this->rot_show_line(device);
                        if (cl->exists) {
                            this->rot_cl = *cl;
                        }else{
                            rot_cl = old_rot_cl;
                        }
                        this->rot_show_line(device);
                    }
                    break;
                case AW_Mouse_Release:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        this->rot_show_line(device);
                        AP_tree *dest= 0;
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
//                         if ( dest->is_son(source)) {
//                             aw_message("This operation is only allowed with two independent subtrees");
//                             break;
//                         }
                        const char *error = 0;

                        //                         switch(cmd){
                        //                             case AWT_MODE_MOVE:

                        switch(button){
                            case AWT_M_LEFT:
                                if (dest->is_son(source)) {
                                    error = "This operation is only allowed with two independent subtrees";
                                }else{
                                    error = source->move(dest,cl->length);
                                    recalc_branch_lengths = true;
                                }
                                break;
                            case AWT_M_RIGHT:
                                error = source->move_group_info(dest);
                                break;
                            default:
                                error = "????? 45338";
                        }

                        //                             default:
                        //                                 error = "????? 45338";
                        //                         }

                        if (error) aw_message(error);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->exports.resize = 1;
                        this->tree_root->test_tree();
                        //this->tree_root->compute_tree(gb_main);
                        compute_tree = true;
                    }
                    break;
                default:
                    break;
            }
            break;

#ifdef NNI_MODES
        case AWT_MODE_NNI:
            if(type==AW_Mouse_Press){
                GB_pop_transaction(gb_main);
                switch(button){
                    case AWT_M_LEFT:
                        if (!cl->exists) break;
                        at = (AP_tree *)cl->client_data1;
                        ap_global_abort_flag = AP_FALSE;
                        ((AP_tree_nlen *)at)->nn_interchange_rek(AP_TRUE,ap_global_abort_flag,-1);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                    case AWT_M_RIGHT:
                        global_combineCount = 0;
                        ap_global_abort_flag = AP_FALSE;
                        ((AP_tree_nlen *)this->tree_root)->nn_interchange_rek(AP_TRUE,ap_global_abort_flag,-1);
                        printf("Combines: %li\n",global_combineCount);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                }
                GB_begin_transaction(gb_main);
            } /* if type */
            break;
        case AWT_MODE_KERNINGHAN:
            if(type==AW_Mouse_Press){
                GB_pop_transaction(gb_main);
                switch(button){
                    case AWT_M_LEFT:
                        if (!cl->exists) break;
                        at = (AP_tree *)cl->client_data1;
                        PARS_kernighan_cb(at);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                    case AWT_M_RIGHT:
                        PARS_kernighan_cb(this->tree_root);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                }
                GB_begin_transaction(gb_main);
            } /* if type */
            break;
        case AWT_MODE_OPTIMIZE:
            if(type==AW_Mouse_Press){
                GB_pop_transaction(gb_main);
                switch(button){
                    case AWT_M_LEFT:
                        if (!cl->exists) break;
                        at = (AP_tree *)cl->client_data1;
                        if (at) PARS_optimizer_cb(at);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                    case AWT_M_RIGHT:
                        PARS_optimizer_cb(this->tree_root);
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->tree_root->test_tree();
                        recalc_branch_lengths = true;
                        break;
                }
                GB_begin_transaction(gb_main);
            } /* if type */
            break;
#endif // NNI_MODES

        default:
            AWT_graphic_tree::command(device,cmd,button, key_modifier, key_code, key_char, type, x, y, cl, ct);
            break;
    }

    if (recalc_branch_lengths) {
        int abort_flag = AP_FALSE;
        rootEdge()->nni_rek(AP_FALSE,abort_flag,-1, GB_FALSE,AP_BL_BL_ONLY);

        beautify_tree = true; // beautify after recalc_branch_lengths
    }

    if (beautify_tree) {
        this->resort_tree(0);
        this->exports.save = 1;

        compute_tree = true; // compute_tree after beautify_tree
    }

    if (compute_tree) {
        this->tree_root->compute_tree(gb_main);
        this->exports.refresh = 1;
    }
}

