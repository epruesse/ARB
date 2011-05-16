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

    
} IRS;

int AWT_graphic_tree::paint_irs_sub_tree(AP_tree *node, int x_offset) {
    int left_y;
    int left_x;
    int right_y;
    int right_x;

    /* *********************** Check clipping rectangle ************************ */
    if (!IRS.is_size_device) {
        if (IRS.y > IRS.max_y) {
            return IRS.max_y;
        }
        int height_of_subtree = IRS.step_y*node->gr.view_sum;
        if (IRS.y + height_of_subtree < IRS.min_y) {
            IRS.y += height_of_subtree;
            return IRS.min_y;
        }
    }

    /* *********************** i'm a leaf ************************ */
    if (node->is_leaf) {
        IRS.y+=IRS.step_y;
        if (IRS.ftrst_species) {
            IRS.draw_top_separator(disp_device);
        }
        int x = x_offset;
        int y = IRS.y + IRS.font_height_2;
        int gc = node->gr.gc;

        if (node->hasName(species_name)) {
            cursor = Position(x, IRS.y);

            if (IRS.is_size_device) {
                // hack to fix calculated cursor position:
                // - IRS tree reports different cursor positions in AW_SIZE and normal draw modes.
                // - the main reason for the difference is the number of open groups clipped away
                //   above the separator line.
                // - There is still some unhandled difference mostlikely caused by the number of
                //   open groups on the screen, but in most cases the cursor position is inside view now.
                
                double correctionPerGroup = IRS.step_y * 2.22222; // was determined by measurements. depends on size of marked-species-font
                double cursorCorrection   = -IRS.group_closed * correctionPerGroup;
                cursor.movey(cursorCorrection);
            }
        }

        const char *str = 0;
        if (!IRS.is_size_device) {
            AW_click_cd cd(disp_device, (AW_CL)node);
            if (node->gb_node && GB_read_flag(node->gb_node)) {
                set_line_attributes_for(node);
                filled_box(gc, Position(x, IRS.y), NT_BOX_WIDTH);
            }
            str = make_node_text_nds(gb_main, node->gb_node, NDS_OUTPUT_LEAFTEXT, node->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(gc, str, x, y);
        }
        return IRS.y;
    }

    /* *********************** i'm a grouped subtree ************************ */
    int last_y = IRS.y;
    const char *node_string = 0;
    if (node->gb_node) {
        if (!IRS.is_size_device) {
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
        int vsize    = node->gr.view_sum * IRS.step_y;
        int y_center = IRS.y + (vsize>>1) + IRS.step_y;
        if (IRS.y >= IRS.min_y) {
            if (IRS.ftrst_species) { // A name of a group just under the separator
                IRS.draw_top_separator(disp_device);
            }
            int topy = IRS.y+IRS.step_y - 2;
            int boty = IRS.y+IRS.step_y + vsize + 2;
            int rx   = x_offset + vsize + vsize;
            int gc   = AWT_GC_GROUPS;

            // draw group box (unclosed on right hand):
            AW_click_cd cd(disp_device, (AW_CL)node);
            disp_device->set_line_attributes(gc, 1, AW_SOLID);
            disp_device->line(gc, x_offset, topy, rx,       topy);
            disp_device->line(gc, x_offset, topy, x_offset, boty);
            disp_device->line(gc, x_offset, boty, rx,       boty);

            disp_device->set_grey_level(node->gr.gc, grey_level);
            set_line_attributes_for(node);
            disp_device->box(node->gr.gc, true, x_offset - (tipBoxSize>>1), topy - (tipBoxSize>>1), tipBoxSize, tipBoxSize, mark_filter);
            disp_device->box(node->gr.gc, true, x_offset+2,                 IRS.y+IRS.step_y, vsize,      vsize);

            IRS.y += vsize + 2*IRS.step_y;
            if (node_string) { //  a node name should be displayed
                const char *s = GBS_global_string("%s (%i)", node_string, node->gr.leave_sum);
                disp_device->text(node->gr.gc, s, x_offset + vsize + 10 + nodeBoxWidth, y_center + (IRS.step_y>>1));
            }
        }
        else {
            IRS.y += vsize;
            y_center = IRS.min_y;
            if (IRS.y > IRS.min_y) {
                IRS.y = IRS.min_y;
            }
        }
        return y_center;
    }

    // ungrouped groups go here

    /* *********************** i'm a labeled node ************************ */
    /*  If I have only one child + pruneLevel != MAXPRUNE -> no labeling */

    if (node_string != NULL) {          //  A node name should be displayed
        if (last_y >= IRS.min_y) {
            if (IRS.ftrst_species) { // A name of a group just under the separator
                IRS.draw_top_separator(disp_device);
            }
            last_y = IRS.y + IRS.step_y;
        }
        else {
            last_y = IRS.min_y;
            IRS.min_y += int(IRS.step_y * 1.8);
        }
        IRS.y+=int(IRS.step_y * 1.8);
        int gc = AWT_GC_GROUPS;
        AW_click_cd cd(disp_device, (AW_CL)node);
        disp_device->set_line_attributes(gc, 1, AW_SOLID);
        disp_device->line(gc, x_offset, last_y, x_offset+400, last_y); // opened-group-frame

        disp_device->set_grey_level(node->gr.gc, grey_level);
        set_line_attributes_for(node); 
        disp_device->box(node->gr.gc, true, x_offset- (tipBoxSize>>1), last_y- (tipBoxSize>>1), tipBoxSize, tipBoxSize, mark_filter);
        const char *s = GBS_global_string("%s (%i)", node_string, node->gr.leave_sum);
        disp_device->text(node->gr.gc, s, x_offset + 10 + nodeBoxWidth, last_y + IRS.step_y + 1);
    }

    /* *********************** connect two nodes == draw branches ************************ */

    left_x = (int)(x_offset + 0.9 + IRS.x_scale * node->leftlen);
    left_y = paint_irs_sub_tree(node->get_leftson(), left_x);

    right_x = int(x_offset + 0.9 + IRS.x_scale * node->rightlen);
    right_y = paint_irs_sub_tree(node->get_rightson(), right_x);

    if (node_string != NULL) IRS.group_closed++;
    
    /* *********************** draw structure ************************ */

    if (left_y > IRS.min_y) {
        if (left_y < IRS.max_y) { // clip y on top border
            AW_click_cd cd(disp_device, (AW_CL)node->leftson);
            if (node->leftson->remark_branch) {
                AWT_show_branch_remark(disp_device, node->leftson->remark_branch, node->leftson->is_leaf, left_x, left_y, 1, text_filter);
            }
            set_line_attributes_for(node->get_leftson()); 
            disp_device->line(node->get_leftson()->gr.gc, x_offset, left_y, left_x,  left_y);
        }
    }
    else {
        left_y = IRS.min_y;
    }

    int y_center = (left_y + right_y) / 2; // clip center(?) on bottom border

    if (right_y > IRS.min_y && right_y < IRS.max_y) { // visible right branch in lower part of display
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

    IRS.ruler_y = y_center;

    if (node_string != 0) {             //  A node name should be displayed
        IRS.y+=IRS.step_y / 2;
        int gc = AWT_GC_GROUPS;
        disp_device->set_line_attributes(gc, 1, AW_SOLID);
        disp_device->line(gc, x_offset-1, IRS.y, x_offset+400, IRS.y); // opened-group-frame
        disp_device->line(gc, x_offset-1, last_y, x_offset-1,  IRS.y); // opened-group-frame
    }
    return y_center;
}

void AWT_graphic_tree::show_irs_tree(AP_tree *at, int height) {
    disp_device->push_clip_scale();
    const AW_font_limits& limits = disp_device->get_font_limits(AWT_GC_SELECTED, 0);

    Position  corner = disp_device->rtransform(Origin); // real world coordinates of left/upper screen corner
    Rectangle rclip  = disp_device->get_rtransformed_cliprect();

    IRS.font_height_2  = limits.ascent/2;
    IRS.ftrst_species  = true;
    IRS.y              = 0;
    IRS.min_x          = corner.xpos();
    IRS.max_x          = 100;
    IRS.min_y          = corner.ypos();
    IRS.max_y          = rclip.bottom();
    IRS.ruler_y        = 0;
    IRS.step_y         = height;
    IRS.x_scale        = rclip.width()*0.8 / at->gr.tree_depth;
    IRS.group_closed   = 0;
    IRS.is_size_device = disp_device->type() == AW_DEVICE_SIZE;

    paint_irs_sub_tree(at, 0);

    // provide some information for ruler:
    list_tree_ruler_y           = IRS.ruler_y;
    irs_tree_ruler_scale_factor = IRS.x_scale;

    disp_device->invisible(AWT_GC_CURSOR, IRS.min_x, 0);
    disp_device->invisible(AWT_GC_CURSOR, IRS.max_x, IRS.y+IRS.step_y);

    disp_device->pop_clip_scale();
}
