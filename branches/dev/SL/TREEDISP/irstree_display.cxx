// =============================================================== //
//                                                                 //
//   File      : irstree_display.cxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "TreeDisplay.hxx"

#include <awt_nds.hxx>
#include <aw_awars.hxx>

using namespace AW;

/* *********************** paint sub tree ************************ */

const int MAXSHOWNNODES = 5000;       // Something like max screen height/minimum font size
const int tipBoxSize    = 4;
const int nodeBoxWidth  = 5;

static struct {
    bool   ftrst_species;
    int    y;
    int    min_y;
    int    max_y;
    int    ruler_y;
    int    min_x;
    int    max_x;
    int    step_y;
    int    group_closed;
    double x_scale;

    int font_height_2;
    int is_size_device;

    void draw_top_separator(AW_device *device) {
        ftrst_species = false;
        if (!is_size_device) {
            for (int yy = min_y; yy< min_y+4; yy++) {
                device->line(AWT_GC_GROUPS, -10000, yy, 10000, yy);
            }
        }
    }

    
} irs_gl;

int AWT_graphic_tree::paint_irs_sub_tree(AP_tree *node, int x_offset) {
    int left_y;
    int left_x;
    int right_y;
    int right_x;

    /* *********************** Check clipping rectangle ************************ */
    if (!irs_gl.is_size_device) {
        if (irs_gl.y > irs_gl.max_y) {
            return irs_gl.max_y;
        }
        int height_of_subtree = irs_gl.step_y*node->gr.view_sum;
        if (irs_gl.y + height_of_subtree < irs_gl.min_y) {
            irs_gl.y += height_of_subtree;
            return irs_gl.min_y;
        }
    }

    /* *********************** i'm a leaf ************************ */
    if (node->is_leaf) {
        irs_gl.y+=irs_gl.step_y;
        if (irs_gl.ftrst_species) {
            irs_gl.draw_top_separator(disp_device);
        }
        int x = x_offset;
        int y = irs_gl.y + irs_gl.font_height_2;
        int gc = node->gr.gc;

        if (node->hasName(species_name)) {
            cursor = Position(x, irs_gl.y);

            if (irs_gl.is_size_device) {
                // hack to fix calculated cursor position:
                // - IRS tree reports different cursor positions in AW_SIZE and normal draw modes.
                // - the main reason for the difference is the number of open groups clipped away
                //   above the separator line.
                // - There is still some unhandled difference mostlikely caused by the number of
                //   open groups on the screen, but in most cases the cursor position is inside view now.
                
                double correctionPerGroup = irs_gl.step_y * 2.22222; // was determined by measurements. depends on size of marked-species-font
                double cursorCorrection   = -irs_gl.group_closed * correctionPerGroup;
                cursor.movey(cursorCorrection);
            }
        }

        const char *str = 0;
        if (!irs_gl.is_size_device) {
            AW_click_cd cd(disp_device, (AW_CL)node);
            if (node->gb_node && GB_read_flag(node->gb_node)) {
                set_line_attributes_for(node);
                filled_box(gc, Position(x, irs_gl.y), NT_BOX_WIDTH);
            }
            str = make_node_text_nds(gb_main, node->gb_node, NDS_OUTPUT_LEAFTEXT, node->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(gc, str, x, y);
        }
        return irs_gl.y;
    }

    /* *********************** i'm a grouped subtree ************************ */
    int last_y = irs_gl.y;
    const char *node_string = 0;
    if (node->gb_node) {
        if (!irs_gl.is_size_device) {
            if (!node->father) { // root node - don't try to get taxonomy
                node_string = tree_static->get_tree_name();
            }
            else {
                node_string = make_node_text_nds(gb_main, node->gb_node, NDS_OUTPUT_LEAFTEXT, node->get_gbt_tree(), tree_static->get_tree_name());
            }
        }
        else {
            node_string = "0123456789";
        }
    }
    if (node->gr.grouped) {                         // no recursion here    just a group symbol !!!
        int vsize    = node->gr.view_sum * irs_gl.step_y;
        int y_center = irs_gl.y + (vsize>>1) + irs_gl.step_y;
        if (irs_gl.y >= irs_gl.min_y) {
            if (irs_gl.ftrst_species) { // A name of a group just under the separator
                irs_gl.draw_top_separator(disp_device);
            }
            int topy = irs_gl.y+irs_gl.step_y - 2;
            int boty = irs_gl.y+irs_gl.step_y + vsize + 2;
            int rx   = x_offset + vsize + vsize;
            int gc   = AWT_GC_GROUPS;

            // draw group box (unclosed on right hand):
            AW_click_cd cd(disp_device, (AW_CL)node);
            disp_device->set_line_attributes(gc, 1.0, AW_SOLID);
            disp_device->line(gc, x_offset, topy, rx,       topy);
            disp_device->line(gc, x_offset, topy, x_offset, boty);
            disp_device->line(gc, x_offset, boty, rx,       boty);

            disp_device->set_grey_level(node->gr.gc, grey_level);
            set_line_attributes_for(node);
            disp_device->box(node->gr.gc, true, x_offset - (tipBoxSize>>1), topy - (tipBoxSize>>1), tipBoxSize, tipBoxSize, mark_filter);
            disp_device->box(node->gr.gc, true, x_offset+2,                 irs_gl.y+irs_gl.step_y, vsize,      vsize);

            irs_gl.y += vsize + 2*irs_gl.step_y;
            if (node_string) { //  a node name should be displayed
                const char *s = GBS_global_string("%s (%i)", node_string, node->gr.leave_sum);
                disp_device->text(node->gr.gc, s, x_offset + vsize + 10 + nodeBoxWidth, y_center + (irs_gl.step_y>>1));
            }
        }
        else {
            irs_gl.y += vsize;
            y_center = irs_gl.min_y;
            if (irs_gl.y > irs_gl.min_y) {
                irs_gl.y = irs_gl.min_y;
            }
        }
        return y_center;
    }

    // ungrouped groups go here

    /* *********************** i'm a labeled node ************************ */
    /*  If I have only one child + pruneLevel != MAXPRUNE -> no labeling */

    if (node_string != NULL) {          //  A node name should be displayed
        if (last_y >= irs_gl.min_y) {
            if (irs_gl.ftrst_species) { // A name of a group just under the separator
                irs_gl.draw_top_separator(disp_device);
            }
            last_y = irs_gl.y + irs_gl.step_y;
        }
        else {
            last_y = irs_gl.min_y;
            irs_gl.min_y += int(irs_gl.step_y * 1.8);
        }
        irs_gl.y+=int(irs_gl.step_y * 1.8);
        int gc = AWT_GC_GROUPS;
        AW_click_cd cd(disp_device, (AW_CL)node);
        disp_device->set_line_attributes(gc, 1.0, AW_SOLID);
        disp_device->line(gc, x_offset, last_y, x_offset+400, last_y); // opened-group-frame

        disp_device->set_grey_level(node->gr.gc, grey_level);
        set_line_attributes_for(node); 
        disp_device->box(node->gr.gc, true, x_offset- (tipBoxSize>>1), last_y- (tipBoxSize>>1), tipBoxSize, tipBoxSize, mark_filter);
        const char *s = GBS_global_string("%s (%i)", node_string, node->gr.leave_sum);
        disp_device->text(node->gr.gc, s, x_offset + 10 + nodeBoxWidth, last_y + irs_gl.step_y + 1);
    }

    /* *********************** connect two nodes == draw branches ************************ */

    left_x = (int)(x_offset + 0.9 + irs_gl.x_scale * node->leftlen);
    left_y = paint_irs_sub_tree(node->get_leftson(), left_x);

    right_x = int(x_offset + 0.9 + irs_gl.x_scale * node->rightlen);
    right_y = paint_irs_sub_tree(node->get_rightson(), right_x);

    if (node_string != NULL) irs_gl.group_closed++;
    
    /* *********************** draw structure ************************ */

    if (left_y > irs_gl.min_y) {
        if (left_y < irs_gl.max_y) { // clip y on top border
            AW_click_cd cd(disp_device, (AW_CL)node->leftson);
            if (node->leftson->remark_branch) {
                AWT_show_branch_remark(disp_device, node->leftson->remark_branch, node->leftson->is_leaf, left_x, left_y, 1, text_filter);
            }
            set_line_attributes_for(node->get_leftson()); 
            disp_device->line(node->get_leftson()->gr.gc, x_offset, left_y, left_x,  left_y);
        }
    }
    else {
        left_y = irs_gl.min_y;
    }

    int y_center = (left_y + right_y) / 2; // clip center(?) on bottom border

    if (right_y > irs_gl.min_y && right_y < irs_gl.max_y) { // visible right branch in lower part of display
        AW_click_cd cd(disp_device, (AW_CL)node->rightson);
        if (node->rightson->remark_branch) {
            AWT_show_branch_remark(disp_device, node->rightson->remark_branch, node->rightson->is_leaf, right_x, right_y, 1, text_filter);
        }
        set_line_attributes_for(node->get_rightson()); 
        disp_device->line(node->get_rightson()->gr.gc, x_offset, right_y, right_x,  right_y);
    }

    AW_click_cd cd(disp_device, (AW_CL)node);
    set_line_attributes_for(node->get_leftson());
    disp_device->line(node->get_leftson()->gr.gc, x_offset, y_center, x_offset, left_y);

    set_line_attributes_for(node->get_rightson());
    disp_device->line(node->get_rightson()->gr.gc, x_offset, y_center, x_offset, right_y);

    irs_gl.ruler_y = y_center;

    if (node_string != 0) {             //  A node name should be displayed
        irs_gl.y+=irs_gl.step_y / 2;
        int gc = AWT_GC_GROUPS;
        disp_device->set_line_attributes(gc, 1.0, AW_SOLID);
        disp_device->line(gc, x_offset-1, irs_gl.y, x_offset+400, irs_gl.y); // opened-group-frame
        disp_device->line(gc, x_offset-1, last_y, x_offset-1,  irs_gl.y); // opened-group-frame
    }
    return y_center;
}

void AWT_graphic_tree::show_irs_tree(AP_tree *at, int height) {
    disp_device->push_clip_scale();
    const AW_font_limits& limits = disp_device->get_font_limits(AWT_GC_SELECTED, 0);

    int x;
    int y;
    disp_device->rtransform(0, 0, x, y); // calculate real world coordinates of left/upper screen border
    
    int clipped_l, clipped_t;
    int clipped_r, clipped_b;
    disp_device->rtransform(disp_device->clip_rect.l, disp_device->clip_rect.t, clipped_l, clipped_t);
    disp_device->rtransform(disp_device->clip_rect.r, disp_device->clip_rect.b, clipped_r, clipped_b);

    irs_gl.font_height_2  = limits.ascent/2;
    disp_device         = disp_device;
    irs_gl.ftrst_species  = true;
    irs_gl.y              = 0;
    irs_gl.min_x          = x;
    irs_gl.max_x          = 100;
    irs_gl.min_y          = y;
    irs_gl.max_y          = clipped_b;
    irs_gl.ruler_y        = 0;
    irs_gl.step_y         = height;
    irs_gl.x_scale        = (clipped_r-clipped_l)*0.8 / at->gr.tree_depth;
    irs_gl.group_closed   = 0;
    irs_gl.is_size_device = disp_device->type() == AW_DEVICE_SIZE;

    paint_irs_sub_tree(at, 0);

    // provide some information for ruler:
    list_tree_ruler_y           = irs_gl.ruler_y;
    irs_tree_ruler_scale_factor = irs_gl.x_scale;

    disp_device->pop_clip_scale();
}
