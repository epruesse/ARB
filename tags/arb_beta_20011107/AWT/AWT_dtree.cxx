#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <iostream.h>

#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_nds.hxx>
#include "awt_tree.hxx"
#include "awt_dtree.hxx"
#include <aw_preset.hxx>
#include <aw_awars.hxx>



/***************************
      class AP_tree
****************************/

AW_gc_manager AWT_graphic_tree::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2)
{
    AW_gc_manager preset_window =
        AW_manage_GC(aww,device,AWT_GC_CURSOR, AWT_GC_MAX, AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb, (AW_CL)ntw, cd2,
                     true, // define color groups
                     "#3be",
                     "CURSOR$white",
                     "BRANCHES$green",
                     "-GROUP_BRACKETS$#000",
                     "Marked$#ffe560",
                     "Some marked$#bb8833",
                     "Not marked$#622300",
                     "Zombies etc.$#977a0e",

                     "+-No probe$black",    "-Probes 1+2$yellow",
                     "+-Probe 1$red",       "-Probes 1+3$magenta",
                     "+-Probe 2$green",     "-Probes 2+3$cyan",
                     "+-Probe 3$blue",      "-All probes$white",

                     0 );

    return preset_window;
}

void AWT_graphic_tree::mark_tree(AP_tree *at, int mark)
{
    if(!at) return;
    GB_transaction dummy(this->tree_static->gb_species_data);
    if (at->is_leaf) {
        if(at->gb_node){
            GB_write_flag(at->gb_node, mark);
        }
    }
    mark_tree(at->leftson, mark);
    mark_tree(at->rightson, mark);
}

AP_tree *AWT_graphic_tree::search(AP_tree *root, const char *name)
{
    if(!root) return 0;
    if (root->is_leaf) {
        if(root->name){
            if (!strcmp(name,root->name)) {
                return root;
            }
        }
    }else{
        AP_tree *result = this->search(root->leftson,name);
        if (result) return result;
        result = this->search(root->rightson,name);
        if (result) return result;
    }
    return 0;
}

void AWT_graphic_tree::jump(AP_tree *at, const char *name)
{
    if (this->tree_sort == AP_NDS_TREE) return;

    at = this->search(at,name);
    if(!at) return;
    if (this->tree_sort == AP_LIST_TREE) {
        this->tree_root_display = this->tree_root;
    }else{
        while (at->father &&
               at->gr.view_sum<15 &&
               0 == at->gr.grouped) {
            at = at->father;
        }
        this->tree_root_display = at;
    }
}

int AWT_graphic_tree::group_tree(AP_tree *at, int mode)	// run on father !!!
{
    /* mode	0	all
       1	all but marked
       2	all but groups with subgroups
       4	non
    */

    int flag;
    int ret_val;
    GBDATA *gn;
    if(!at) return 1;
    GB_transaction dummy(this->tree_static->gb_species_data);

    if (at->is_leaf) {
        ret_val=0;
        if ( mode == 4){
            ret_val=1;
        }else{
            if ( (mode & 1) &&
                 at->gb_node &&
                 GB_read_flag(at->gb_node) ){
                ret_val=1;
            }
        }
        return ret_val;
    }

    flag = this->group_tree(at->leftson, mode);
    flag += this->group_tree(at->rightson, mode);
    at->gr.grouped=0;
    if (!flag) {
        if (at->gb_node) {
            gn = GB_find(at->gb_node,
                         "group_name",NULL,down_level);
            if (gn) {
                if (strlen(GB_read_char_pntr(gn))>0){
                    at->gr.grouped=1;
                    if (mode & 2) flag = 1;
                }
            }
        }
    }
    if (!at->father) this->tree_root->compute_tree(this->tree_static->gb_species_data);

    return flag;
}



int AWT_graphic_tree::resort_tree( int mode, struct AP_tree *at ) // run on father !!!
{
    /* mode

       0	to top
       1	to bottom
       2	center (to top)
       3	center (to bottom; don't use this - it's used internal)

       post-condition: leafname contains alphabetically "smallest" name of subtree

    */
    static const char *leafname;

    if (!at) {
        GB_transaction dummy(this->gb_main);
        at = this->tree_root;
        if(!at) return 0;
        at->arb_tree_set_leafsum_viewsum();

        this->resort_tree(mode,at);
        at->compute_tree(this->gb_main);
        return 0;
    }

    if (at->is_leaf) {
        leafname = at->name;
        return 1;
    }
    int leftsize = at->leftson->gr.leave_sum;
    int rightsize = at->rightson->gr.leave_sum;

    if ( (mode &1) == 0 ) { // to top
        if (rightsize >leftsize ) {
            at->swap_sons();
        }
    }else{
        if (rightsize < leftsize ) {
            at->swap_sons();
        }
    }

    int lmode = mode;
    int rmode = mode;
    if ( (mode & 2) == 2){
        lmode = 2;
        rmode = 3;
    }

    resort_tree(lmode,at->leftson);
    gb_assert(leafname);
    const char *leftleafname = leafname;

    resort_tree(rmode,at->rightson);
    gb_assert(leafname);
    const char *rightleafname = leafname;

    gb_assert(leftleafname && rightleafname);

    if (leftleafname && rightleafname) {
        int name_cmp = strcmp(leftleafname,rightleafname);
        if (name_cmp<0) { // leftleafname < rightleafname
            leafname = leftleafname;
        } else { // (name_cmp>=0) aka: rightleafname <= leftleafname
            leafname = rightleafname;
            if (rightsize==leftsize && name_cmp>0) { // if sizes of subtrees are equal and rightleafname<leftleafname -> swap branches
                at->swap_sons();
            }
        }
    }else{
        if (leftleafname) leafname = leftleafname;
        else leafname = rightleafname;
    }

    return 0;
}


void AWT_graphic_tree::rot_show_line(AW_device *device)
{
    double             sx, sy;
    double             x, y;
    sx = (this->old_rot_cl.x0+this->old_rot_cl.x1)*.5;
    sy = (this->old_rot_cl.y0+this->old_rot_cl.y1)*.5;
    x = this->rot_cl.x0 * (1.0-this->rot_cl.length) + this->rot_cl.x1 * this->rot_cl.length;
    y = this->rot_cl.y0 * (1.0-this->rot_cl.length) + this->rot_cl.y1 * this->rot_cl.length;
    int gc = this->drag_gc;
    device->line(gc, sx, sy, x, y, AWT_F_ALL, 0, 0);
}

void AWT_graphic_tree::rot_show_triangle( AW_device *device)
{
    double          w;
    double          len;
    double             sx, sy;
    double             x1, y1, x2, y2;
    AP_tree *at;

    at = this->rot_at;

    if (!at || !at->father)
        return;

    sx = this->old_rot_cl.x0;
    sy = this->old_rot_cl.y0;

    if (at == at->father->leftson)
        len = at->father->leftlen;
    else
        len = at->father->rightlen;

    w = this->rot_orientation;	x1 = this->old_rot_cl.x0 + cos(w) * len;
    y1 = this->old_rot_cl.y0 + sin(w) * len;

    int gc = this->drag_gc;

    device->line(gc, sx, sy, x1, y1, AWT_F_ALL, 0, 0);


    if (!at->is_leaf) {
        sx = x1; sy = y1;
        len = at->gr.tree_depth;
        w = this->rot_orientation - this->rot_spread*.5*.5 + at->gr.right_angle;
        x1 = sx + cos(w) * len;
        y1 = sy + sin(w) * len;
        w = this->rot_orientation + this->rot_spread*.5*.5 + at->gr.left_angle;
        x2 = sx + cos(w) * len;
        y2 = sy + sin(w) * len;

        device->line(gc, sx, sy, x1, y1, AWT_F_ALL, 0, 0);
        device->line(gc, sx, sy, x2, y2, AWT_F_ALL, 0, 0);
        device->line(gc, x2, y2, x1, y1, AWT_F_ALL, 0, 0);

    }


}
void AWT_show_circle(AW_device *device, const char *bootstrap, double zoom_factor, double len, double x, double y, int filter, AW_CL cd1,AW_CL cd2){
    double radius = .01 * atoi(bootstrap);     // Groesse der Bootstrap Kreise
    if (radius < .1) radius = .1;
    radius = 1.0 / sqrt(radius);
    radius -= 1.0;
    radius *= 2;
    radius *= len * zoom_factor;
    if (radius < 0) return;
    //if (radius > 0.04) radius = 0.04;
    if (radius > 0.06) radius = 0.06;
    device->circle(AWT_GC_BRANCH_REMARK, x,y,radius,radius,filter,cd1 , (AW_CL) cd2 );
}

double comp_rot_orientation(AW_clicked_line *cl)
{
    return atan2(cl->y1 - cl->y0, cl->x1 - cl->x0);
}

double comp_rot_spread(AP_tree *at, AWT_graphic_tree *ntw)
{
    double zw = 0;
    AP_tree *bt;

    if(!at) return 0;

    zw = 1.0;

    for(bt=at; bt->father && (bt!=ntw->tree_root_display); bt=bt->father){
        zw *= bt->gr.spread;
    }

    zw *= bt->gr.spread;
    zw *= 	(double)at->gr.view_sum /
        (double)bt->gr.view_sum;
    if(ntw->tree_sort == AP_RADIAL_TREE){
        zw *= 2*M_PI;
    }else{
        zw *= 0.5*M_PI;
    }

    return zw;
}

void reset_spread(AP_tree *at)
{
    if(!at) return;
    at->gr.spread =  1.0;
    reset_spread(at->leftson);
    reset_spread(at->rightson);
}

void reset_rotation(AP_tree *at)
{
    if(!at) return;
    at->gr.left_angle = 0.0;
    at->gr.right_angle = 0.0;
    reset_rotation(at->leftson);
    reset_rotation(at->rightson);
}

void reset_line_width(AP_tree *at)
{
    if(!at) return;
    at->gr.left_linewidth =  0;
    at->gr.right_linewidth =  0;
    reset_line_width(at->leftson);
    reset_line_width(at->rightson);
}

char *AWT_graphic_tree_root_changed(void *cd, AP_tree *old, AP_tree *newroot)
{
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->tree_root_display == old ||
        agt->tree_root_display->is_son(old)) agt->tree_root_display = newroot;
    if (agt->tree_root == old ||
        agt->tree_root->is_son(old)) agt->tree_root = newroot;
    return 0;
}
char *AWT_graphic_tree_node_deleted(void *cd, AP_tree *old)
{
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->tree_root_display == old) agt->tree_root_display = agt->tree_root;
    if (old == agt->tree_root) {
        agt->tree_root = 0;
        agt->tree_root_display = 0;
    }
    return 0;
}
void AWT_graphic_tree::toggle_group(AP_tree * at)
{
    if (at->gb_node) {
        GBDATA         *gb_name;
        gb_name = GB_search(at->gb_node, "group_name", GB_FIND);
        char *gname;
        if (gb_name) {
            gname = GB_read_string(gb_name);

            const char *msg = GBS_global_string("Do You really want to delete group '%s'",gname);
            delete gname;
            switch (aw_message(msg,"Hide Only,Destroy,Cancel")){
                case 0:
                    at->gr.grouped = 0;
                    return;
                case 1:
                    at->gr.grouped = 0;
                    at->name = 0;
                    GB_delete(at->gb_node);
                    at->gb_node = 0;
                    return;
                case 2:
                    return;
            }
        }
    }
    create_group(at);
    at->gr.grouped = 1;
}

void AWT_graphic_tree::create_group(AP_tree * at)
{
    if (!at->gb_node) {
        at->gb_node = GB_create_container(this->tree_static->gb_tree, "node");
        GBDATA         *gb_id;
        gb_id = GB_search(at->gb_node, "id", GB_INT);
        GB_write_int(gb_id, 0);
        this->exports.save = 1;
    }
    if (!at->name) {
        at->name = aw_input("Enter Name of Group",0);
        if (!at->name) return;
        GBDATA         *gb_name;
        gb_name = GB_search(at->gb_node, "group_name", GB_STRING);
        GB_write_string(gb_name, "noname");
    }
    return;
}

void AWT_graphic_tree::command(AW_device *device, AWT_COMMAND_MODE cmd, int button,
                               AW_event_type type, AW_pos x, AW_pos y,
                               AW_clicked_line *cl, AW_clicked_text *ct)
{
    static int rot_drag_flag, bl_drag_flag;

    AWUSE(ct);
    AP_tree *at;
    // clicked on a species list !!!!
    if ( ct->exists && ct->client_data2 && !strcmp((char *) ct->client_data2, "species")){
        // @@@ FIXME: einige Modes sollten auch in species list funktionieren!
        GBDATA *gbd = (GBDATA *)ct->client_data1;
        if (type == AW_Mouse_Press) {
            AD_map_viewer(gbd);
        }
        return;
    }

    if (!this->tree_static) return;		// no tree -> no commands

    if ( (rot_ct.exists && (rot_ct.distance == 0) && (!rot_ct.client_data1) &&
          rot_ct.client_data2 && !strcmp((char *) rot_ct.client_data2, "ruler") ) ||
         (ct && ct->exists && (!ct->client_data1) &&
          ct->client_data2 && !strcmp((char *) ct->client_data2, "ruler"))) {
        const char *tree_awar;
        char awar[256];
        float h;
        switch(cmd){
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_MOVE:	// Move Ruler text
                switch(type){
                    case AW_Mouse_Press:
                        rot_ct = *ct;
                        rot_ct.x0 = x;
                        rot_ct.y0 = y;
                        break;
                    case AW_Mouse_Drag:
                        tree_awar = show_ruler(device, this->drag_gc);
                        sprintf(awar,"ruler/text_x");
                        h = (x - rot_ct.x0)/device->get_scale() +
                            GBT_read_float2(this->tree_static->gb_tree, awar, 0.0);
                        GBT_write_float(this->tree_static->gb_tree, awar, h);
                        sprintf(awar,"ruler/text_y");
                        h = (y - rot_ct.y0)/device->get_scale() +
                            GBT_read_float2(this->tree_static->gb_tree, awar, 0.0);
                        GBT_write_float(this->tree_static->gb_tree, awar, h);
                        rot_ct.x0 = x;
                        rot_ct.y0 = y;
                        show_ruler(device, this->drag_gc);
                        break;
                    case AW_Mouse_Release:
                        rot_ct.exists = AW_FALSE;
                        this->exports.resize = 1;
                        break;
                    default: break;
                }
                return;
            default:	break;
        } // switch cmd
    }	// if


    if ( (rot_cl.exists && (!rot_cl.client_data1) &&
          rot_cl.client_data2 && !strcmp((char *) rot_cl.client_data2, "ruler") ) ||
         (cl && cl->exists && (!cl->client_data1) &&
          cl->client_data2 && !strcmp((char *) cl->client_data2, "ruler"))) {
        const char *tree_awar;
        char awar[256];
        float h;
        switch(cmd){
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_MOVE:	// Move Ruler
                switch(type){
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        rot_cl.y0 = y;
                        break;
                    case AW_Mouse_Drag:
                        tree_awar = show_ruler(device, this->drag_gc);
                        sprintf(awar,"ruler/%s/ruler_x",tree_awar);
                        h = (x - rot_cl.x0)/device->get_scale() +
                            GBT_read_float2(this->tree_static->gb_tree, awar, 0.0);
                        GBT_write_float(this->tree_static->gb_tree, awar, h);
                        sprintf(awar,"ruler/%s/ruler_y",tree_awar);
                        h = (y - rot_cl.y0)/device->get_scale() +
                            GBT_read_float2(this->tree_static->gb_tree, awar, 0.0);
                        GBT_write_float(this->tree_static->gb_tree, awar, h);

                        rot_cl.x0 = x;
                        rot_cl.y0 = y;
                        show_ruler(device, this->drag_gc);
                        break;
                    case AW_Mouse_Release:
                        rot_cl.exists = AW_FALSE;
                        this->exports.resize = 1;
                        break;
                    default:
                        break;
                }
                break;


            case AWT_MODE_LENGTH:
                switch(type){	// resize Ruler
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        break;
                    case AW_Mouse_Drag:
                        tree_awar = show_ruler(device, this->drag_gc);
                        sprintf(awar,"ruler/size");

                        h = (x - rot_cl.x0)/device->get_scale() +
                            GBT_read_float2(this->tree_static->gb_tree, awar, 0.0);
                        if (h<0.01) h = 0.01;
                        GBT_write_float(this->tree_static->gb_tree, awar, h);
                        rot_cl.x0 = x;
                        show_ruler(device, this->drag_gc);
                        break;
                    case AW_Mouse_Release:
                        rot_cl.exists = AW_FALSE;
                        this->exports.refresh = 1;
                        break;
                    default:
                        break;
                }
                break;
            case AWT_MODE_LINE:
                if (type == AW_Mouse_Press) {
                    long i;
                    sprintf(awar,"ruler/ruler_width");
                    i = GBT_read_int2(this->tree_static->gb_tree, awar , 0);
                    switch(button){
                        case AWT_M_LEFT:
                            i --;
                            if (i<0) i= 0;
                            else{
                                GBT_write_int(this->tree_static->gb_tree, awar,i);
                                this->exports.refresh = 1;
                            }
                            break;
                        case AWT_M_RIGHT:
                            i++;
                            GBT_write_int(this->tree_static->gb_tree, awar,i);
                            show_ruler(device, AWT_GC_CURSOR);
                            break;
                        default: break;
                    }
                }
                break;
            default:	break;
        }
        return;
    }

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

                        GB_ERROR error;
                        switch(button){
                            case AWT_M_LEFT:
                                if (  dest->is_son(source)) {
                                    error = "This operation is only allowed with two independent subtrees";
                                }else{
                                    error = source->move(dest,cl->length);
                                }
                                break;
                            case AWT_M_RIGHT:
                                error = source->move_group_info(dest);
                                break;
                            default:
                                error = "????? 45338";
                        }

                        if (error){
                            aw_message(error);
                        }else{
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                            this->exports.resize = 1;
                            this->tree_root->test_tree();
                            this->tree_root->compute_tree(gb_main);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;


        case AWT_MODE_LENGTH:
            if(button!=AWT_M_LEFT){
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( cl && cl->exists ){
                        at = (AP_tree *)cl->client_data1;
                        if (!at) break;
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                        device->transform(cl->x0, cl->y0, rot_cl.x0, rot_cl.y0);
                        device->transform(cl->x1, cl->y1, rot_cl.x1, rot_cl.y1);

                        this->rot_orientation = comp_rot_orientation(&rot_cl);
                        this->rot_spread = comp_rot_spread(at, this);
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Drag:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        double len, ex, ey;

                        rot_show_triangle(device);

                        if (rot_at == rot_at->father->leftson){
                            len = rot_at->father->leftlen;
                        }else{
                            len = rot_at->father->rightlen;
                        }

                        ex = x-rot_cl.x0;
                        ey = y-rot_cl.y0;

                        len = ex * cos(this->rot_orientation) +
                            ey * sin(this->rot_orientation);

                        if (len<0.0){
                            len = 0.0;
                        }

                        len = len/device->get_scale();

                        if (rot_at == rot_at->father->leftson) {
                            rot_at->father->leftlen = len;
                        }else{
                            rot_at->father->rightlen = len;
                        }

                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Release:
                    this->exports.refresh = 1;
                    this->exports.save = 1;
                    rot_drag_flag = 0;
                    this->tree_root->compute_tree(gb_main);
                    break;
                default:
                    break;
            }
            break;

        case AWT_MODE_ROT:
            if(button!=AWT_M_LEFT){
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( cl && cl->exists ){
                        at = (AP_tree *)cl->client_data1;
                        if (!at) break;
                        rot_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                        device->transform(cl->x0, cl->y0, rot_cl.x0, rot_cl.y0);
                        device->transform(cl->x1, cl->y1, rot_cl.x1, rot_cl.y1);

                        this->rot_orientation = comp_rot_orientation(&rot_cl);
                        this->rot_spread = comp_rot_spread(at, this);
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Drag:
                    if( rot_drag_flag && this->rot_at && this->rot_at->father){
                        double new_a;

                        rot_show_triangle(device);
                        new_a = atan2(y - rot_cl.y0, x - rot_cl.x0);
                        new_a -= this->rot_orientation;
                        this->rot_orientation += new_a;
                        if (this->rot_at == this->rot_at->father->leftson) {
                            this->rot_at->father->gr.left_angle += new_a;
                            if (!this->rot_at->father->father) {
                                this->rot_at->father->gr.right_angle += new_a;
                            }
                        }else{
                            this->rot_at->father->gr.right_angle += new_a;
                            if (!this->rot_at->father->father) {
                                this->rot_at->father->gr.left_angle += new_a;
                            }
                        }
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Release:
                    this->exports.refresh = 1;
                    this->exports.save = 1;
                    rot_drag_flag = 0;
                    break;
                default:	break;
            }

            break;

        case AWT_MODE_LZOOM:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            this->tree_root_display = at;
                            this->exports.zoom_reset = 1;
                            this->exports.refresh = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        if(this->tree_root_display->father){
                            this->tree_root_display =
                                this->tree_root_display->father;
                            this->exports.refresh = 1;
                            this->exports.zoom_reset = 1;
                        }
                        break;
                }
                this->tree_root->compute_tree(gb_main);
            } /* if type */
            break;

        case AWT_MODE_RESET:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        /** reset rotation **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            reset_rotation(at);
                            this->exports.save = 1;
                            this->exports.refresh = 1;
                        }
                        break;
                    case AWT_M_MIDDLE:
                        /** reset spread **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            reset_spread(at);
                            this->exports.save = 1;
                            this->exports.refresh = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        /** reset linewidth **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            reset_line_width(at);
                            this->exports.save = 1;
                            this->exports.refresh = 1;
                        }
                        break;
                }
                this->tree_root->compute_tree(gb_main);
            } /* if press */
            break;

        case AWT_MODE_LINE:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            if(at->father){
                                if(at->father->leftson == at){
                                    at->father->gr.left_linewidth-=1;
                                    if(at->father->gr.left_linewidth<0){
                                        at->father->gr.left_linewidth=0;
                                    }
                                }else{
                                    at->father->gr.right_linewidth-=1;
                                    if(at->father->gr.right_linewidth<0){
                                        at->father->gr.right_linewidth=0;
                                    }
                                }
                                this->exports.save = 1;
                                this->exports.refresh = 1;
                            }
                        }
                        break;
                    case AWT_M_RIGHT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            if(at->father){
                                if(at->father->leftson == at){
                                    at->father->gr.left_linewidth+=1;
                                }else{
                                    at->father->gr.right_linewidth+=1;
                                }
                                this->exports.save = 1;
                                this->exports.refresh = 1;
                            }
                        }
                        break;
                } /* switch button */
            } /* if type */
            break;

        case AWT_MODE_SPREAD:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread -= PH_CLICK_SPREAD;
                            if(at->gr.spread<0) at->gr.spread = 0;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread += PH_CLICK_SPREAD;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                }
            } /* if type */
            break;

    act_like_group:
        case AWT_MODE_GROUP:
            if(type==AW_Mouse_Press){
                if(cl->exists){
                    at = (AP_tree *)cl->client_data1;
                    if (!at) break;
                }else break;

                switch(button){
                    case AWT_M_LEFT:
                        // Do not group any subtree
                        if ( (!at->gr.grouped) && (!at->name)) break;
                        if (this->tree_static->gb_tree) {
                            this->create_group(at);
                        }
                        if (at->name) {
                            at->gr.grouped ^= 1;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                            this->exports.resize = 1;
                            this->tree_root->compute_tree(gb_main);
                        }
                        break;
                    case AWT_M_RIGHT:
                        if (this->tree_static->gb_tree) {
                            this->toggle_group(at);
                        }
                        this->exports.refresh = 1;
                        this->exports.save = 1;
                        this->exports.resize = 1;
                        this->tree_root->compute_tree(gb_main);

                        break;
                    default:
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_SETROOT:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->set_root();
                            this->exports.save = 1;
                            this->exports.zoom_reset = 1;
                            this->tree_root->compute_tree(gb_main);
                        }
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_SWAP:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->swap_sons();

                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_MARK:

            if(type==AW_Mouse_Press && (cl->exists || ct->exists) ){
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                }else{
                    at = (AP_tree *)ct->client_data1;
                }
                if (!at) break;
                GB_transaction dummy(this->tree_static->gb_species_data);
                switch(button){
                    case AWT_M_LEFT:
                        this->mark_tree(at, 1);
                        break;
                    case AWT_M_RIGHT:

                        this->mark_tree(at, 0);
                        break;
                }
                this->exports.refresh                                        = 1;
                this->tree_static->update_timers(); // do not reload the tree
                this->tree_root->calc_color();
            }                   /* if type */
            break;
        case AWT_MODE_NONE:
        case AWT_MODE_SELECT:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) && button != AWT_M_MIDDLE){
                GB_transaction dummy(this->tree_static->gb_species_data);
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                }else{
                    at = (AP_tree *)ct->client_data1;
                }
                if (!at) break;

                //this->exports.refresh = 1;		// No refresh needed !! AD_map_viewer will do the refresh
                AD_map_viewer(at->gb_node, ADMVT_SELECT);
                goto act_like_group; // now do the same like in AWT_MODE_GROUP
            }
            break;

        case AWT_MODE_MOD:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) ){
                GB_transaction dummy(this->tree_static->gb_species_data);
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                }else{
                    at = (AP_tree *)ct->client_data1;
                }
                if (!at) break;
                //                 if (button == AWT_M_RIGHT) {
                //                     if (this->tree_static->gb_tree) {
                //                         this->create_group(at);
                //                     }
                //                     this->exports.refresh = 1;
                //                 }
                //this->exports.refresh = 1;		// No refresh needed !! AD_map_viewer will do the refresh
                AD_map_viewer(at->gb_node,ADMVT_INFO);
            }
            break;
        case AWT_MODE_WWW:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) ){
                GB_transaction dummy(this->tree_static->gb_species_data);
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                }else{
                    at = (AP_tree *)ct->client_data1;
                }
                if (!at) break;
//                 if (button == AWT_M_RIGHT) {
//                     if (this->tree_static->gb_tree) {
//                         this->create_group(at);
//                     }
//                     this->exports.refresh = 1;
//                 }
                //this->exports.refresh = 1;		// No refresh needed !! AD_map_viewer will do the refresh
                AD_map_viewer(at->gb_node,ADMVT_WWW);
            }
            break;
        default:
            break;
    }
}

void AWT_graphic_tree::set_tree_type(AP_tree_sort type)
{
    tree_sort = type;
    switch(type) {
        case AP_RADIAL_TREE:
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 0;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 150;
            exports.right_offset    = 150;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;

        case AP_NDS_TREE:
            exports.dont_fit_x      = 1;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 0;
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;

        case AP_IRS_TREE:
            exports.dont_fit_x      = 1;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 0;
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 1;
            break;

        case AP_LIST_TREE:
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 0;
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;
    }
}

AWT_graphic_tree::AWT_graphic_tree(AW_root *aw_rooti, GBDATA *gb_maini):AWT_graphic()
{
    line_filter = AW_SCREEN|AW_CLICK|AW_CLICK_DRAG|AW_SIZE|AW_PRINTER;
    vert_line_filter = AW_SCREEN|AW_PRINTER;
    text_filter = AW_SCREEN|AW_CLICK|AW_PRINTER;
    mark_filter = AW_SCREEN|AW_PRINTER_EXT;
    ruler_filter = AW_SCREEN|AW_CLICK|AW_PRINTER|AW_SIZE;
    root_filter = AW_SCREEN|AW_CLICK|AW_PRINTER_EXT;
    set_tree_type(AP_LIST_TREE);
    tree_root_display = 0;
    tree_root = 0;
    y_pos = 0;
    tree_proto = 0;
    tree_static = 0;
    baselinewidth = 0;
    species_name = 0;
    this->aw_root = aw_rooti;
    this->gb_main = gb_maini;
    rot_ct.exists = AW_FALSE;
    rot_cl.exists = AW_FALSE;

}

AWT_graphic_tree::~AWT_graphic_tree(void)
{
    delete this->tree_proto;
    delete this->tree_root;
    delete this->tree_static;
}

void AWT_graphic_tree::init(AP_tree * tree_prot)
{
    this->tree_proto = tree_prot->dup();
}

void AWT_graphic_tree::unload(void)
{
    rot_at = 0;

    delete this->tree_root;
    delete this->tree_static;

    this->tree_root = 0;
    this->tree_static = 0;
    this->tree_root_display = 0;
}

GB_ERROR AWT_graphic_tree::load(GBDATA *dummy, const char *name,AW_CL link_to_database, AW_CL insert_delete_cbs)
{
    AWUSE(dummy);
    static GB_ERROR olderror = 0;
    AP_tree_root *tr = 0;
    AP_tree *apt = this->tree_proto->dup();
    tr = new AP_tree_root(gb_main,apt,name);
    GB_ERROR error = apt->load(tr,(int)link_to_database,(GB_BOOL)insert_delete_cbs, GB_TRUE); // show status

    this->unload();

    if (error) {
        delete apt;
        delete tr;
        if (error == olderror) return 0;
        olderror = error;
        return error;
    }
    olderror = 0;
    this->tree_root = apt;
    this->tree_static = tr;
    this->tree_root_display = this->tree_root;
    this->tree_root->compute_tree(gb_main);
    tr->root_changed_cd = (void*)this;
    tr->root_changed = AWT_graphic_tree_root_changed;
    tr->node_deleted_cd = (void*)this;
    tr->node_deleted = AWT_graphic_tree_node_deleted;
    return 0;
}

GB_ERROR AWT_graphic_tree::save(GBDATA *dummy, const char *name,AW_CL cd1,AW_CL cd2)
{
    AWUSE(dummy);
    AWUSE(cd1);AWUSE(name);AWUSE(cd2);
    if (!this->tree_root) return 0;
    return this->tree_root->save(0);
}

int AWT_graphic_tree::check_update(GBDATA *gbdummy)
{
    AWUSE(gbdummy);
    AP_UPDATE_FLAGS flags;
    char *name;
    GB_ERROR error;
    if (!this->tree_static) return 0;
    GB_transaction dummy(gb_main);

    flags = this->tree_root->check_update();
    switch (flags){
        case AP_UPDATE_OK:
        case AP_UPDATE_ERROR:
            return flags;
        case AP_UPDATE_RELOADED:
            name = strdup(this->tree_static->tree_name);
            error = this->load(gb_main,name,1,0);
            if (error) aw_message(error);
            delete name;
            this->exports.resize = 1;
            break;
        case AP_UPDATE_RELINKED:
            error = this->tree_root->relink();
            if (error) aw_message(error);
            else	this->tree_root->compute_tree(gb_main);
            break;
    }
    return (int)flags;
}

void AWT_graphic_tree::update(GBDATA *gbdummy){
    AWUSE(gbdummy);
    if (!this->tree_static) return;
    GB_transaction dummy(gb_main);
    this->tree_root->update();
}

void AWT_graphic_tree::NT_scalebox(int gc, double u, double v, int width)
{
    double diam = (double)width/disp_device->get_scale();
    disp_device->set_fill(gc, this->grey_level);
    disp_device->box(gc,-diam+u,-diam+v, diam+diam,diam+diam,mark_filter,0,0);
}

void AWT_graphic_tree::NT_emptybox(int gc, double u, double v, int width)
{
    double diam = (double)width/disp_device->get_scale();
    disp_device->line(gc,u-diam,v-diam, u-diam,v+diam,mark_filter,0,0);
    disp_device->line(gc,u-diam,v-diam, u+diam,v-diam,mark_filter,0,0);
    disp_device->line(gc,u+diam,v+diam, u-diam,v+diam,mark_filter,0,0);
    disp_device->line(gc,u+diam,v+diam, u+diam,v-diam,mark_filter,0,0);
}

void AWT_graphic_tree::NT_rotbox(int gc, double u, double v, int width)
{
    double diam = (double)width/disp_device->get_scale();
    disp_device->line(gc,u,v-diam, u-diam,v,mark_filter,0,0);
    disp_device->line(gc,u,v-diam, u+diam,v,mark_filter,0,0);
    disp_device->line(gc,u+diam,v, u,v+diam,mark_filter,0,0);
    disp_device->line(gc,u-diam,v, u,v+diam,mark_filter,0,0);
}


double AWT_graphic_tree::show_list_tree_rek(AP_tree *at, double x_father, double x_son)
{
    double          ny0, ny1, nx0, nx1, ry, l_min, l_max,offset;
    AWUSE(x_father);
    ny0 = y_pos;


    if (disp_device->type() != AW_DEVICE_SIZE){	// tree below cliprect bottom can be cut
        AW_pos xs = 0;
        AW_pos ys = y_pos - scale *2.0;
        AW_pos X,Y;
        disp_device->transform(xs,ys,X,Y);
        if ( Y > disp_device->clip_rect.b){
            y_pos += scale;
            return ny0;
        }
        ys = y_pos + scale * (at->gr.view_sum+2);
        disp_device->transform(xs,ys,X,Y);

        if ( Y  < disp_device->clip_rect.t){
            y_pos += scale*at->gr.view_sum;
            return ny0;
        }
    }

    if (at->is_leaf) {
        /* draw mark box */
        if (at->gb_node && GB_read_flag(at->gb_node)){
            NT_scalebox(at->gr.gc,x_son, ny0, (int)NT_BOX_WIDTH);
        }
        if (at->name && at->name[0] == this->species_name[0] &&
            !strcmp(at->name,this->species_name)) {
            x_cursor = x_son; y_cursor = ny0;
        }
        if (at->name && (disp_device->filter & text_filter) ){
            // text darstellen
            const char *data = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree());

            offset = scale*0.4;
            disp_device->text(at->gr.gc,data ,
                              (AW_pos) x_son+offset,(AW_pos) ny0+offset,
                              (AW_pos) 0 , text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }


        y_pos += scale;
        return ny0;
    }
    if (at->gr.grouped) {
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.tree_depth;
        ny1 = y_pos += scale*(at->gr.view_sum-1);
        nx0 = x_son + l_max;
        nx1 = x_son + l_min;

        int linewidth = 0;
        if (at->father) {
            if (at->father->leftson == at) {
                linewidth = at->father->gr.left_linewidth;
            }else{
                linewidth = at->father->gr.right_linewidth;
            }
        }

        disp_device->set_line_attributes(at->gr.gc,linewidth+baselinewidth,AW_SOLID);

        AW_pos q[8];
        q[0] = x_son;	q[1] = ny0;
        q[2] = x_son;	q[3] = ny1;
        q[4] = nx1;	q[5] = ny1;
        q[6] = nx0;	q[7] = ny0;

        disp_device->set_fill(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 4, &q[0], line_filter, (AW_CL)at,0);


        if (at->gb_node && (disp_device->filter & text_filter)){
            const char *data = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree());

            offset = (ny1-ny0) *0.5;
            disp_device->text(at->gr.gc,data ,
                              (AW_pos) nx0,(AW_pos) ny0+offset,
                              (AW_pos) 0 , text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }
        disp_device->text(at->gr.gc,(char *)GBS_global_string(" %i",at->gr.leave_sum),
                          x_son,(ny0+ny1)*.5,0,text_filter,(AW_CL)at,0);

        y_pos += scale;


        return (ny0+ny1)*.5;
    }
    nx0 =  (x_son +  at->leftlen) ;
    nx1 =  (x_son +  at->rightlen) ;
    ny0 = show_list_tree_rek(at->leftson,x_son, nx0);
    ry = (double) y_pos-.5*scale;
    ny1 = (double) show_list_tree_rek(at->rightson,x_son, nx1);

    if (at->name) {
        NT_rotbox(at->gr.gc,x_son, ry, (int)NT_BOX_WIDTH<<1);
    }

    if (at->leftson->remark_branch ) {
        disp_device->text(AWT_GC_BRANCH_REMARK, at->leftson->remark_branch ,
                          (AW_pos) nx0,(AW_pos) ny0-scale*.1,
                          (AW_pos) 1 , text_filter,
                          (AW_CL) at , (AW_CL) 0 );
        if (show_circle){
            AWT_show_circle(disp_device,at->leftson->remark_branch, circle_zoom_factor,at->leftlen,nx0, ny0, text_filter, (AW_CL) at->leftson, (AW_CL) 0);
        }
    }

    if (at->rightson->remark_branch ) {
        disp_device->text(AWT_GC_BRANCH_REMARK,at->rightson->remark_branch ,
                          (AW_pos) nx1,(AW_pos) ny1-scale*.1,
                          (AW_pos) 1 , text_filter,
                          (AW_CL) at , (AW_CL) 0 );
        if (show_circle){
            AWT_show_circle(disp_device,at->rightson->remark_branch,circle_zoom_factor, at->rightlen,nx1, ny1, text_filter, (AW_CL) at->rightson, (AW_CL) 0);
        }
    }


    int lw = at->gr.left_linewidth+baselinewidth;
    disp_device->set_line_attributes(at->gr.gc,lw,AW_SOLID);
    disp_device->line(at->leftson->gr.gc, x_son, ny0, nx0, ny0,
                      line_filter,
                      (AW_CL)at->leftson,0);

    int rw = at->gr.right_linewidth+baselinewidth;
    disp_device->set_line_attributes(at->gr.gc,rw,AW_SOLID);
    disp_device->line(at->rightson->gr.gc,x_son, ny1, nx1, ny1,
                      line_filter,
                      (AW_CL)at->rightson,0);

    if (lw == rw) {
        disp_device->line(at->gr.gc, x_son, ny0, x_son, ny1,
                          vert_line_filter,
                          (AW_CL)at,0);
    }else{
        disp_device->set_line_attributes(at->gr.gc,lw,AW_SOLID);
        disp_device->line(at->gr.gc, x_son, ny0, x_son, ry,
                          vert_line_filter,
                          (AW_CL)at,0);

        disp_device->set_line_attributes(at->gr.gc,rw,AW_SOLID);
        disp_device->line(at->gr.gc, x_son, ry, x_son, ny1,
                          vert_line_filter,
                          (AW_CL)at,0);
    }
    return ry;
}


void AWT_graphic_tree::scale_text_koordinaten(AW_device *device, int gc,double& x,double& y,double orientation,int flag)
{
    AW_font_information *fontinfo = device->get_font_information(gc,'A');
    double  text_height = fontinfo->max_letter_height/ disp_device->get_scale() ;
    double dist = fontinfo->max_letter_height/ disp_device->get_scale();
    if (flag==1) {
        dist += 1;
    } else {
        x += cos(orientation) * dist;
        y += sin(orientation) * dist + 0.3*text_height;
    }
    return;
}


//  ********* shell and radial tree
void AWT_graphic_tree::show_tree_rek(AP_tree * at, double x_center,
                                     double y_center, double tree_spread,double tree_orientation,
                                     double x_root, double y_root, int linewidth)
{
    double l,r,w,z,l_min,l_max;

    disp_device->set_line_attributes(at->gr.gc,linewidth+baselinewidth,AW_SOLID);
    disp_device->line(at->gr.gc, x_root, y_root, x_center, y_center,
                      line_filter,
                      (AW_CL)at,0);

    // draw mark box
    if (at->gb_node && GB_read_flag(at->gb_node)) {
        NT_scalebox(at->gr.gc, x_center, y_center, (int)NT_BOX_WIDTH);
    }

    if( at->is_leaf){
        if (at->name && (disp_device->filter & text_filter) ){
            if (at->name[0] == this->species_name[0] &&
                !strcmp(at->name,this->species_name)) {
                x_cursor = x_center; y_cursor = y_center;
            }
            scale_text_koordinaten(disp_device,at->gr.gc, x_center,y_center,tree_orientation,0);
            const char *data = 	make_node_text_nds(this->gb_main,at->gb_node,0,at->get_gbt_tree());
            // PJ vectorfont - example for calling
            //                      disp_device->zoomtext(at->gr.gc,data,
            //                              (AW_pos)x_center,(AW_pos) y_center,
            //                              0.005, .5-.5*cos(tree_orientation), 0,
            //                              text_filter, (AW_CL) at, (AW_CL) 0);

            disp_device->text(at->gr.gc,data,
                              (AW_pos)x_center,(AW_pos) y_center,
                              (AW_pos) .5 - .5 * cos(tree_orientation),
                              text_filter,	(AW_CL) at , (AW_CL) 0 );
        }
        return;
    }

    if( at->gr.grouped){
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.tree_depth;

        r = l = 0.5;
        AW_pos q[6];
        q[0] = x_center;		q[1] = y_center;
        w = tree_orientation + r*0.5*tree_spread+ at->gr.right_angle;
        q[2] = x_center+l_min*cos(w);	q[3] = y_center+l_min*sin(w);
        w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
        q[4] = x_center+l_max*cos(w);	q[5] = y_center+l_max*sin(w);

        disp_device->set_fill(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 3, &q[0], line_filter, (AW_CL)at,0);

        if (at->gb_node && (disp_device->filter & text_filter) ){
            w = tree_orientation + at->gr.right_angle;
            l_max = (l_max+l_min)*.5;
            x_center= x_center+l_max*cos(w);
            y_center= y_center+l_max*sin(w);
            scale_text_koordinaten(disp_device,at->gr.gc,x_center,y_center,w,0);

            /* insert text (e.g. name of group) */
            const char *data = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree());
            //PJ vectorfont - possibly groups should be rendered bigger than species
            //                        disp_device->zoomtext(at->gr.gc ,data,
            //                                (AW_pos)x_center,(AW_pos) y_center, 0.01,
            //                                (AW_pos).5 - .5 * cos(tree_orientation),0 ,
            //                                text_filter,
            //                                (AW_CL) at , (AW_CL) 0 );
            disp_device->text(at->gr.gc,data,
                              (AW_pos)x_center,(AW_pos) y_center,
                              (AW_pos).5 - .5 * cos(tree_orientation),
                              text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }
        return;
    }
    l = (double) at->leftson->gr.view_sum / (double)at->gr.view_sum;
    r = 1.0 - (double)l;

    if (at->leftson->gr.gc > at->rightson->gr.gc) {
        // bring selected gc to front

        /*** left branch ***/
        w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
        z = at->leftlen;
        show_tree_rek(at->leftson,
                      x_center+ z * cos(w),
                      y_center+ z * sin(w),
                      (at->leftson->is_leaf) ? 1.0 :
                      tree_spread * l * at->leftson->gr.spread,
                      w,
                      x_center, y_center, at->gr.left_linewidth );

        /*** right branch ***/
        w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
        z = at->rightlen;
        show_tree_rek(at->rightson,
                      x_center+ z * cos(w),
                      y_center+ z * sin(w),
                      (at->rightson->is_leaf) ? 1.0 :
                      tree_spread * r * at->rightson->gr.spread,
                      w,
                      x_center, y_center, at->gr.right_linewidth);
    }else{
        /*** right branch ***/
        w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
        z = at->rightlen;
        show_tree_rek(at->rightson,
                      x_center+ z * cos(w),
                      y_center+ z * sin(w),
                      (at->rightson->is_leaf) ? 1.0 :
                      tree_spread * r * at->rightson->gr.spread,
                      w,
                      x_center, y_center, at->gr.right_linewidth);

        /*** left branch ***/
        w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
        z = at->leftlen;
        show_tree_rek(at->leftson,
                      x_center+ z * cos(w),
                      y_center+ z * sin(w),
                      (at->leftson->is_leaf) ? 1.0 :
                      tree_spread * l * at->leftson->gr.spread,
                      w,
                      x_center, y_center, at->gr.left_linewidth );
    }
    if (show_circle){
        /*** left branch ***/
        /*** left branch ***/
        if (at->leftson->remark_branch){
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen * .5;
            AWT_show_circle(disp_device,at->leftson->remark_branch,circle_zoom_factor, at->leftlen,x_center+ z * cos(w),y_center+ z * sin(w),text_filter, (AW_CL)at,0);
        }
        if (at->rightson->remark_branch){
            /*** right branch ***/
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen * .5;
            AWT_show_circle(disp_device,at->rightson->remark_branch,circle_zoom_factor, at->rightlen,x_center+ z * cos(w),y_center+ z * sin(w),text_filter, (AW_CL)at,0);
        }
    }
}

const char *AWT_graphic_tree::show_ruler(AW_device *device, int gc) {
    const char *tree_awar = 0;

    char awar[256];
    if (!this->tree_static->gb_tree)	return 0;	// no ruler !!!!
    GB_transaction dummy(this->tree_static->gb_tree);

    sprintf(awar,"ruler/size");
    float ruler_size = GBT_read_float2(	this->tree_static->gb_tree, awar, 0.1);
    float ruler_x = 0.0;
    float ruler_y = 0.0;
    float ruler_text_x = 0.0;
    float ruler_text_y = 0.0;
    float ruler_add_y = 0.0;
    float ruler_add_x = 0.0;

    switch (tree_sort) {
        case AP_LIST_TREE:
            tree_awar = "LIST";
            break;
        case AP_RADIAL_TREE:
            tree_awar = "RADIAL";
            break;
        case AP_NDS_TREE:
            tree_awar = "NDS";
            break;
        case AP_IRS_TREE:
            tree_awar = "IRS";
            break;
    }

    sprintf(awar,"ruler/%s/ruler_y",tree_awar);
    if (!GB_search(this->tree_static->gb_tree,awar,GB_FIND)){
        if (device->type() == AW_DEVICE_SIZE) {
            AW_world world;
            device ->get_size_information(&world);
            ruler_y = world.b * 1.3;
        }
    }

    switch (tree_sort) {
        case AP_LIST_TREE:
            ruler_y = 0;
            ruler_add_y = this->list_tree_ruler_y;
            ruler_add_x = ruler_size * 0.5;
            break;
        default:
            break;
    }


    ruler_y = ruler_add_y +
        GBT_read_float2(	this->tree_static->gb_tree, awar, ruler_y);

    sprintf(awar,"ruler/%s/ruler_x",tree_awar);
    ruler_x = ruler_add_x +
        GBT_read_float2(	this->tree_static->gb_tree, awar, ruler_x);

    sprintf(awar,"ruler/text_x");
    ruler_text_x = GBT_read_float2(	this->tree_static->gb_tree, awar, ruler_text_x);

    sprintf(awar,"ruler/text_y");
    ruler_text_y = GBT_read_float2(	this->tree_static->gb_tree, awar, ruler_text_y);

    sprintf(awar,"ruler/ruler_width");
    double ruler_width = (double)GBT_read_int2(this->tree_static->gb_tree, awar, 0);

    device->set_line_attributes(gc, ruler_width+baselinewidth, AW_SOLID);

    device->line(gc, 		ruler_x - ruler_size*.5 ,ruler_y,
                 ruler_x + ruler_size*.5 , ruler_y,
                 this->ruler_filter,
                 0, (AW_CL)"ruler");
    char ruler_text[20];
    sprintf(ruler_text,"%4.2f",ruler_size);

    device->text(gc,ruler_text, 	ruler_x + ruler_text_x,
                 ruler_y + ruler_text_y, 0.5,
                 this->ruler_filter & ~AW_SIZE,
                 0, (AW_CL)"ruler");

    return tree_awar;
}


void AWT_graphic_tree::show_nds_list_rek(GBDATA * dummy)
{
    AWUSE(dummy);
    GBDATA         *gb_species;
    GBDATA         *gb_name;
    AW_pos          y_position = scale;
    AW_pos		offset;
    long		max_strlen = 0;

    disp_device->text(AWT_GC_CURSOR,"NDS List of all species:",
                      (AW_pos) scale * 2, (AW_pos) 0,
                      (AW_pos) 0, text_filter,
                      (AW_CL) 0, (AW_CL) 0);

    for (gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species)) {

        y_position += scale;
        gb_name = GB_find(gb_species, "name", 0, down_level);
        if (gb_name && !strcmp(GB_read_char_pntr(gb_name), this->species_name)) {
            x_cursor = scale;
            y_cursor = y_position + NT_SELECTED_WIDTH;
        }

        {
            AW_pos xs = 0;
            AW_pos X,Y;
            max_strlen = 1000;
            disp_device->transform(xs,y_position + scale * 2,X,Y);
            if ( Y  < disp_device->clip_rect.t) continue;
            disp_device->transform(xs,y_position - scale * 2,X,Y);
            if ( Y > disp_device->clip_rect.b) continue;
        }

        if (disp_device->type() != AW_DEVICE_SIZE){ // tree below cliprect bottom can be cut
            const char *data     = make_node_text_nds(gb_main, gb_species, 1, 0);
            long        slen     = strlen(data);
            offset               = scale * 0.5;

            disp_device->text(GB_read_flag(gb_species) ? AWT_GC_SELECTED : AWT_GC_NSELECTED,
                              data,
                              (AW_pos) scale * 2, (AW_pos) y_position + offset,
                              (AW_pos) 0, text_filter,
                              (AW_CL) gb_species, (AW_CL) "species", slen);
            if (slen> max_strlen) max_strlen = slen;
        }
    }
    disp_device->invisible(AWT_GC_CURSOR,0,0,-1,0,0);
    disp_device->invisible(AWT_GC_CURSOR,max_strlen*scale,y_position,-1,0,0);


}


void AWT_graphic_tree::show(AW_device *device)	{
    int nsort = tree_sort;
    if (this->tree_static && this->tree_static->gb_tree) {
        this->check_update(this->gb_main);
    }
    if (!this->tree_root_display){
        nsort = AP_NDS_TREE;
    }

    this->disp_device = device;

    AW_font_information *fontinfo = disp_device->get_font_information(AWT_GC_SELECTED, 0);
    scale = fontinfo->max_letter_height/ device->get_scale();
    make_node_text_init(this->gb_main);
    scale *= this->aw_root->awar(AWAR_DTREE_VERICAL_DIST)->read_float();
    this-> grey_level = this->aw_root->awar(AWAR_DTREE_GREY_LEVEL)->read_int()*.01;

    this->baselinewidth = (int)this->aw_root->awar(AWAR_DTREE_BASELINEWIDTH)->read_int();
    this->show_circle = (int)this->aw_root->awar(AWAR_DTREE_SHOW_CIRCLE)->read_int();
    this->circle_zoom_factor = this->aw_root->awar(AWAR_DTREE_CIRCLE_ZOOM)->read_float();
    delete (this->species_name);
    this->species_name = this->aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    x_cursor = y_cursor = 0.0;
    switch (nsort) {
        case AP_LIST_TREE:
            if (!this->tree_root_display)	return;
            y_pos = 0.05;
            show_list_tree_rek(this->tree_root_display, 0, 0);
            list_tree_ruler_y = y_pos + 2.0 * scale;
            break;
        case AP_RADIAL_TREE:
            if (!this->tree_root_display)	return;
            NT_emptybox(this->tree_root_display->gr.gc,
                        0, 0, (int)NT_ROOT_WIDTH);
            show_tree_rek(this->tree_root_display, 0,0,2*M_PI,
                          0.0,0, 0, this->tree_root_display->gr.left_linewidth);
            break;
        case AP_IRS_TREE:
            show_irs(this->tree_root_display,disp_device,fontinfo->max_letter_height);
            //show_nds_list_rek(this->gb_main);
            break;
        case AP_NDS_TREE:
            show_nds_list_rek(this->gb_main);
            break;
    }
    if (x_cursor != 0.0 || y_cursor != 0.0) {
        NT_emptybox(AWT_GC_CURSOR,
                    x_cursor, y_cursor, (int)NT_SELECTED_WIDTH);
    }
    if (nsort != AP_NDS_TREE && nsort != AP_IRS_TREE) {
        show_ruler(device, AWT_GC_CURSOR);
    }
}

void AWT_graphic_tree::info(AW_device *device, AW_pos x, AW_pos y,
                            AW_clicked_line *cl, AW_clicked_text *ct)
{
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);

}

AWT_graphic *NT_generate_tree( AW_root *root,GBDATA *gb_main )
{
    AWT_graphic_tree *apdt = new AWT_graphic_tree(root,gb_main);
    AP_tree tree_proto(0);
    apdt->init(&tree_proto);		// no tree_root !!! load will do this
    return (AWT_graphic *)apdt;
}

void awt_create_dtree_awars(AW_root *aw_root,AW_default def)
{
    aw_root->awar_int(AWAR_DTREE_BASELINEWIDTH,0,def)	->set_minmax(0,10);
    aw_root->awar_float(AWAR_DTREE_VERICAL_DIST,1.0,def)	->set_minmax(0.1,3);
    aw_root->awar_int(AWAR_DTREE_AUTO_JUMP,1,def);
    aw_root->awar_int(AWAR_DTREE_SHOW_CIRCLE,0,def);
    aw_root->awar_float(AWAR_DTREE_CIRCLE_ZOOM,1.0,def)	->set_minmax(0.1,20);
    aw_root->awar_int(AWAR_DTREE_GREY_LEVEL,20,def)		->set_minmax(0,100);
}
