#include <stdio.h>

#include <math.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>
#include <awt_nds.hxx>
#include "awt_tree.hxx"
#include "awt_dtree.hxx"
#include "awt_irstree.hxx"
#include <aw_awars.hxx>


/* *********************** paint sub tree ************************ */
/* *********************** paint sub tree ************************ */
/* *********************** paint sub tree ************************ */
#define IS_HIDDEN(node) (type == AWT_IRS_HIDDEN_TREE && node->gr.gc>0)

const int	MAXSHOWNNODES = 5000;		// Something like max screen height/minimum font size
const int	tipBoxSize = 4;
const int	nodeBoxWidth = 5;

static struct {
    GB_BOOL ftrst_species;
    int y;
    int min_y;
    int max_y;
    int ruler_y;
    int min_x;
    int max_x;
    int step_y;
    double x_scale;
    AW_device *device;
    int	nodes_xpos[MAXSHOWNNODES];	// needed for Query results drawing
    int	nodes_ypos[MAXSHOWNNODES];
    AP_tree	*nodes_id[MAXSHOWNNODES];
    int	nodes_ntip;			// counts the tips stored in nodes_xx
    int	nodes_nnnodes;			// counts the inner nodes (reverse counter !!!)
    int font_height_2;
    enum IRS_PRUNE_LEVEL pruneLevel;
    int is_size_device;
} irs_gl;

void draw_top_seperator(){
    int gc = AWT_GC_GROUPS;
    int y;
    irs_gl.ftrst_species = GB_FALSE;
    if (!irs_gl.is_size_device){
        for (y = irs_gl.min_y; y< irs_gl.min_y+4;y++){
            irs_gl.device->line(gc,-10000,y,10000,y,-1,(AW_CL)0,(AW_CL)0);
        }
    }
}

int AWT_graphic_tree::paint_sub_tree(AP_tree *node, int x_offset, int type){

    int left_y;
    int left_x;
    int right_y;
    int right_x;


    // if (irs_gl.y > irs_gl.max_clipped_y) irs_gl.max_clipped_y = irs_gl.max_y; // @@@ ralf
    
    /* *********************** Check clipping rectangle ************************ */
    if (!irs_gl.is_size_device){
        if (irs_gl.y > irs_gl.max_y) {
            return irs_gl.max_y;
        }
        int height_of_subtree = irs_gl.step_y*node->gr.view_sum;
        if (irs_gl.y + height_of_subtree < irs_gl.min_y) {
            irs_gl.y+= height_of_subtree;
            return irs_gl.min_y;
        }
    }



    /* *********************** i'm a leaf ************************ */
    if (node->is_leaf) {
        irs_gl.y+=irs_gl.step_y;
        if (irs_gl.ftrst_species) {
            draw_top_seperator();
        }
        int x = x_offset;
        int y = irs_gl.y + irs_gl.font_height_2;
        int gc = node->gr.gc;



        if (node->name && node->name[0] == this->species_name[0] &&
			!strcmp(node->name,this->species_name)) {
			x_cursor = x; y_cursor = irs_gl.y;
        }

        const char *str = 0;
        if (!irs_gl.is_size_device){
            if (node->gb_node && GB_read_flag(node->gb_node)){
                NT_scalebox(gc,x, irs_gl.y, NT_BOX_WIDTH);
            }
            str = make_node_text_nds(gb_main,node->gb_node,0,node->get_gbt_tree(), tree_name);
            irs_gl.device->text(gc,str, x, y,0.0,-1, (AW_CL)node,0);
        }
        return irs_gl.y;
    }

    /* *********************** i'm a grouped subtree ************************ */
    int last_y = irs_gl.y;
    const char *node_string = 0;
    if (node->gb_node){
        if (!irs_gl.is_size_device){
            node_string = make_node_text_nds(gb_main,node->gb_node,0,node->get_gbt_tree(), tree_name);
        }else{
            node_string = "0123456789";
        }
    }
    if (node->gr.grouped) {		// no recursion here	just a group symbol !!!
        int vsize = node->gr.view_sum * irs_gl.step_y;
        int y_center = irs_gl.y + (vsize>>1) + irs_gl.step_y;
        if ( irs_gl.y >= irs_gl.min_y) {
            if (irs_gl.ftrst_species) {	// A name of a group just under the seperator
                draw_top_seperator();
            }
            int topy = irs_gl.y+irs_gl.step_y - 2;
            int boty = irs_gl.y+irs_gl.step_y+ vsize + 2;
            int rx = x_offset + vsize + vsize;
            int gc = AWT_GC_GROUPS;
            irs_gl.device->line(gc,x_offset, topy, rx, topy, -1,(AW_CL)node,0);

            irs_gl.device->box(node->gr.gc,x_offset - (tipBoxSize>>1), topy - (tipBoxSize>>1),
                               tipBoxSize, tipBoxSize, -1, (AW_CL)node,0);

            irs_gl.device->line(gc,x_offset, topy, x_offset, boty, -1, (AW_CL)node,0);
            irs_gl.device->line(gc,x_offset, boty, rx,       boty, -1, (AW_CL)node,0);

            irs_gl.device->box(node->gr.gc,x_offset+2,irs_gl.y+irs_gl.step_y,vsize,vsize, -1,(AW_CL)node,0);

            irs_gl.y += vsize + 2*irs_gl.step_y ;
            if (node_string) {
                const char *s = GBS_global_string("%s (%i:%i)",node_string,node->gr.leave_sum,0);
                irs_gl.device->text(node->gr.gc,s,x_offset + vsize + 10 + nodeBoxWidth,
                                    y_center + (irs_gl.step_y>>1),0.0, //  A node name should be displayed
                                    -1, (AW_CL)node, 0);
            }
        }else{
            irs_gl.y+= vsize;
            y_center = irs_gl.min_y;
            if ( irs_gl.y > irs_gl.min_y) {
                irs_gl.y = irs_gl.min_y;
            }
        }
        return y_center;
    }

    if ( irs_gl.pruneLevel != IRS_NOPRUNE ){
        node_string = 0;
    }

    /* *********************** i'm a labeled node ************************ */
    /*	If I have only one child + pruneLevel != MAXPRUNE -> no labeling */

    if (node_string != NULL) {		//  A node name should be displayed
        if (last_y >= irs_gl.min_y) {
            if (irs_gl.ftrst_species) {	// A name of a group just under the seperator
                draw_top_seperator();
            }
            last_y = irs_gl.y + irs_gl.step_y;
        }else{
            last_y = irs_gl.min_y;
            irs_gl.min_y += int(irs_gl.step_y * 1.8);
        }
        irs_gl.y+=int(irs_gl.step_y * 1.8);
        int gc = AWT_GC_GROUPS;
        irs_gl.device->line(gc,x_offset,last_y,  x_offset+400, last_y, -1, (AW_CL)node,0);

        irs_gl.device->box(node->gr.gc,x_offset- (tipBoxSize>>1), last_y- (tipBoxSize>>1),
                           tipBoxSize,tipBoxSize,
                           -1, (AW_CL)node,0);
        const char *s = GBS_global_string("%s (%i:%i)",node_string,node->gr.leave_sum,0);
        irs_gl.device->text(node->gr.gc,s, x_offset + 10 + nodeBoxWidth, last_y + irs_gl.step_y + 1,0.0,
                            -1, (AW_CL)node,0);
    }

    /* *********************** connect two nodes == draw branches ************************ */
    int y_center;

    left_x = (int)(x_offset + 0.9 + irs_gl.x_scale * node->leftlen);
    left_y = paint_sub_tree(node->leftson, left_x, type);

    right_x = int(x_offset + 0.9 + irs_gl.x_scale * node->rightlen);
    right_y = paint_sub_tree(node->rightson, right_x, type);


    /* *********************** draw structure ************************ */

    if (left_y > irs_gl.min_y){
        if (left_y < irs_gl.max_y){ // clip y on top border
            if (node->leftson->remark_branch ) {
                AWT_show_remark_branch(disp_device, node->leftson->remark_branch, node->leftson->is_leaf, left_x, left_y, 1, text_filter, (AW_CL)node->leftson, 0);
            }
            irs_gl.device->line(node->leftson->gr.gc,x_offset,left_y,  left_x,   left_y, -1, (AW_CL)node->leftson,0); //  ***
        }
    }else{
        left_y = irs_gl.min_y;
    }

    y_center = (left_y + right_y) / 2; // clip conter on bottom border

    if (right_y > irs_gl.min_y && right_y < irs_gl.max_y) { // visible right branch in lower part of display

        if (node->rightson->remark_branch ) {
            AWT_show_remark_branch(disp_device, node->rightson->remark_branch, node->rightson->is_leaf, right_x, right_y, 1, text_filter, (AW_CL)node->rightson, 0);
        }
        irs_gl.device->line(node->rightson->gr.gc,x_offset,right_y,  right_x,   right_y, -1, (AW_CL)node->rightson,0);
    }

    irs_gl.device->line(node->leftson->gr.gc, x_offset,y_center,x_offset, left_y,  -1, (AW_CL)node,0);
    irs_gl.device->line(node->rightson->gr.gc,x_offset,y_center,x_offset, right_y, -1, (AW_CL)node,0);
    irs_gl.ruler_y = y_center;

    if (node_string != 0) {		//  A node name should be displayed
        irs_gl.y+=irs_gl.step_y /2;
        int gc = AWT_GC_GROUPS;
        irs_gl.device->line(gc,x_offset-1,irs_gl.y, x_offset+400,  irs_gl.y, -1,(AW_CL)node,0);
        irs_gl.device->line(gc,x_offset-1,last_y,   x_offset-1,  irs_gl.y, -1,(AW_CL)node,0);
    }
    if (0 &&  !irs_gl.is_size_device){
        if (irs_gl.nodes_nnnodes>irs_gl.nodes_ntip+1){
            irs_gl.nodes_nnnodes--;
            irs_gl.nodes_xpos[irs_gl.nodes_nnnodes] = x_offset;
            irs_gl.nodes_ypos[irs_gl.nodes_nnnodes] = y_center;
            irs_gl.nodes_id[irs_gl.nodes_nnnodes] = node;
        }
    }

    return y_center;
}

int AWT_graphic_tree::draw_slot(int x_offset, GB_BOOL draw_at_tips){
    int maxx = x_offset;
    int i;
    int no_compress = 0;
    if (!draw_at_tips) no_compress = 1;
    for (i=0;i<irs_gl.nodes_ntip;i++){
        AP_tree *tip = irs_gl.nodes_id[i];
        const char *str = make_node_text_nds(gb_main,tip->gb_node,no_compress,tip->get_gbt_tree(), tree_name);
        int len = irs_gl.device->get_string_size(tip->gr.gc,str,0);
        int x = 0;
        int y = irs_gl.nodes_ypos[i]+ irs_gl.font_height_2;
        if (draw_at_tips) {
            x = x_offset + irs_gl.nodes_xpos[i];
        }else{
            irs_gl.device->text(tip->gr.gc,str,x,irs_gl.nodes_ypos[i],0,-1,(AW_CL)tip,0);
        }
        if (x + len > maxx) maxx = x+len;
        irs_gl.device->text(tip->gr.gc,str, x, y,0.0,-1, (AW_CL)tip,0);
    }
    return maxx;
}


void AWT_graphic_tree::show_irs(AP_tree *at,AW_device *device, int height){
    device->push_clip_scale();
    int x;
    int y;
    const AW_font_information *font_info = device->get_font_information(AWT_GC_SELECTED,0);
    device->rtransform(0,0,x,y); // calculate real world coordinates of left/upper screen border
    int clipped_l,clipped_t;
    int clipped_r,clipped_b;
    device->rtransform(device->clip_rect.l,device->clip_rect.t,clipped_l,clipped_t);
    device->rtransform(device->clip_rect.r,device->clip_rect.b,clipped_r,clipped_b);

    irs_gl.nodes_nnnodes = MAXSHOWNNODES;
    irs_gl.nodes_ntip = 0;
    irs_gl.font_height_2 = font_info->max_letter.ascent/2;
    irs_gl.device = device;
    irs_gl.ftrst_species = GB_TRUE;
    irs_gl.y = 0;
    irs_gl.min_x = x;
    irs_gl.max_x = 100;
    irs_gl.min_y = y;
    irs_gl.max_y = clipped_b;
    irs_gl.ruler_y = 0;
    irs_gl.step_y = height;
    irs_gl.x_scale = 600.0 / at->gr.tree_depth;
    irs_gl.is_size_device = 0;
    if (irs_gl.device->type() == AW_DEVICE_SIZE){
        irs_gl.is_size_device = 1;
    }

    this->paint_sub_tree(at,0,AWT_IRS_NORMAL_TREE );

    // provide some information for ruler :
    y_pos                       = irs_gl.ruler_y;
    irs_tree_ruler_scale_factor = irs_gl.x_scale;

    if (irs_gl.is_size_device){
        irs_gl.device->invisible(0,irs_gl.min_x,irs_gl.y + (irs_gl.min_y-y) + 200,-1,0,0);
    }
    device->pop_clip_scale();
}
