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
#include <AP_TreeColors.hxx>

#include <nds.h>

using namespace AW;

// *********************** paint sub tree ************************

const int TIP_BOX_SIZE = 3;

struct IRS_data {
    bool   draw_separator;
    AW_pos y;
    AW_pos min_y;    // ypos of folding line
    AW_pos max_y;
    AW_pos step_y;
    AW_pos halfstep_y;
    AW_pos onePixel;
    AW_pos fold_x1, fold_x2;

    int    group_closed;
    double x_scale;

    Vector adjust_text;
    AW_pos tree_depth;

    AW_pos gap; // between group frame and (box or text)
    AW_pos openGroupExtra; // extra y-size of unfolded groups

    AW_bitset sep_filter;
    
    bool is_size_device;

    void draw_top_separator_once(AW_device *device) {
        if (draw_separator) {
            if (!is_size_device) {
                device->set_line_attributes(AWT_GC_IRS_GROUP_BOX, 4, AW_SOLID);
                device->line(AWT_GC_IRS_GROUP_BOX, fold_x1, min_y, fold_x2, min_y, sep_filter);
            }
            draw_separator = false;
        }
    }

    
};
static IRS_data IRS;

AW_pos AWT_graphic_tree::paint_irs_sub_tree(AP_tree *node, AW_pos x_offset) {
    if (!IRS.is_size_device) {
        // check clipping rectangle
        if (IRS.y > IRS.max_y) {
            return IRS.max_y;
        }
        AW_pos height_of_subtree = IRS.step_y*node->gr.view_sum;
        if (IRS.y + height_of_subtree < IRS.min_y) {
            IRS.y += height_of_subtree;
            return IRS.min_y;
        }
    }

    if (node->is_leaf) {
        IRS.y+=IRS.step_y;
        IRS.draw_top_separator_once(disp_device);

        Position leaf(x_offset, IRS.y);

        if (node->hasName(species_name)) {
            cursor = leaf;
            if (IRS.is_size_device) {
                // hack to fix calculated cursor position:
                // - IRS tree reports different cursor positions in AW_SIZE and normal draw modes.
                // - the main reason for the difference is the number of open groups clipped away
                //   above the separator line.
                // - There is still some unhandled difference mostlikely caused by the number of
                //   open groups on the screen, but in most cases the cursor position is inside view now.
                
                double correctionPerGroup = IRS.openGroupExtra; // depends on size of marked-species-font
                double cursorCorrection   = -IRS.group_closed * correctionPerGroup;
                cursor.movey(cursorCorrection);
            }
        }

        AW_click_cd cd(disp_device, (AW_CL)node, CL_NODE);
        
        int gc = node->gr.gc;
        if (node->gb_node && GB_read_flag(node->gb_node)) {
            set_line_attributes_for(node);
            filled_box(gc, leaf, NT_BOX_WIDTH);
        }

        Position    textpos  = leaf+IRS.adjust_text;
        const char *specinfo = make_node_text_nds(gb_main, node->gb_node, NDS_OUTPUT_LEAFTEXT, node, tree_static->get_tree_name());
        disp_device->text(gc, specinfo, textpos);

        return IRS.y;
    }

    AW_pos frame_width = NAN;
    AW_pos group_y1    = NAN;

    if (node->is_named_group()) {
        frame_width = node->gr.max_tree_depth * IRS.x_scale;

        if (node->gr.grouped) { // folded group
            AW_pos y_center;

            AW_pos frame_height = node->gr.view_sum * IRS.step_y;
            AW_pos frame_y1     = IRS.y+IRS.halfstep_y+IRS.gap;
            AW_pos frame_y2     = frame_y1 + frame_height;

            if (frame_y2 >= IRS.min_y) {
                if (frame_y1 < IRS.min_y) { // shift folded groups into the folding area (disappears when completely inside)
                    frame_y1  = IRS.min_y;
                    IRS.min_y += IRS.halfstep_y+IRS.gap;
                }

                AW_pos    visible_frame_height = frame_y2-frame_y1;
                Rectangle frame(Position(x_offset, frame_y1), Vector(frame_width, visible_frame_height));

                // draw group frame (unclosed on right hand):
                AW_click_cd cd(disp_device, (AW_CL)node, CL_NODE);

                {
                    const int gc = AWT_GC_IRS_GROUP_BOX;
                    disp_device->set_line_attributes(gc, 1, AW_SOLID);
                    disp_device->line(gc, frame.upper_edge());
                    disp_device->line(gc, frame.left_edge());
                    disp_device->line(gc, frame.lower_edge());
                }

                const int gc = node->gr.gc;
                set_line_attributes_for(node);
                filled_box(gc, frame.upper_left_corner(), TIP_BOX_SIZE);

                Vector    frame2box(IRS.gap, IRS.gap);
                Rectangle gbox(frame.upper_left_corner()+frame2box, Vector(frame.width()*.5, frame.height()-2*IRS.gap));

                disp_device->set_grey_level(gc, group_greylevel);
                disp_device->box(gc, AW::FillStyle::SHADED_WITH_BORDER, gbox);

                Position box_rcenter = gbox.right_edge().centroid();

                const GroupInfo& info = get_group_info(node, group_info_pos == GIP_SEPARATED ? GI_SEPARATED : GI_COMBINED, group_info_pos == GIP_OVERLAYED);
                if (info.name) { //  a node name should be displayed
                    disp_device->text(gc, info.name, box_rcenter+IRS.adjust_text, 0.0, AW_ALL_DEVICES_UNSCALED, info.name_len);
                }
                if (info.count) {
                    Position box_lcenter = gbox.left_edge().centroid();
                    disp_device->text(gc, info.count, box_lcenter+IRS.adjust_text, 0.0, AW_ALL_DEVICES_UNSCALED, info.count_len);
                }

                IRS.draw_top_separator_once(disp_device);

                IRS.y    += frame_height + 2*IRS.gap;
                y_center  = box_rcenter.ypos();
            }
            else {
                IRS.y    += frame_height + 2*IRS.gap;
                y_center  = IRS.min_y;

                if (IRS.y > IRS.min_y) {
                    IRS.y = IRS.min_y;
                }
            }
            return y_center;
        }

        // -----------------------------------
        //      otherwise: unfolded group

        group_y1 = IRS.y;
        if (group_y1 >= IRS.min_y) {
            IRS.draw_top_separator_once(disp_device);
            group_y1 = IRS.y + IRS.halfstep_y+IRS.gap;
        }
        else {
            group_y1   = IRS.min_y;
            IRS.min_y += IRS.openGroupExtra;
        }
        IRS.y += IRS.openGroupExtra;

        const int   gc = AWT_GC_IRS_GROUP_BOX;
        AW_click_cd cd(disp_device, (AW_CL)node, CL_NODE);
        disp_device->set_line_attributes(gc, 1, AW_DOTTED);
        disp_device->line(gc, x_offset-IRS.onePixel, group_y1, x_offset+frame_width, group_y1); // opened-group-frame

        const GroupInfo& info = get_group_info(node, GI_COMBINED);

        td_assert(info.name); // if fails -> maybe skip whole headerline
        disp_device->text(node->gr.gc,
                          info.name,
                          x_offset-IRS.onePixel + IRS.gap,
                          group_y1 + 2*IRS.adjust_text.y() + IRS.gap,
                          0.0,
                          AW_ALL_DEVICES_UNSCALED,
                          info.name_len);
    }

    // draw subtrees
    AW_pos left_x = x_offset + IRS.x_scale * node->leftlen;
    AW_pos left_y = paint_irs_sub_tree(node->get_leftson(), left_x);

    AW_pos right_x = x_offset + IRS.x_scale * node->rightlen;
    AW_pos right_y = paint_irs_sub_tree(node->get_rightson(), right_x);

    if (node->is_named_group()) IRS.group_closed++;

    // draw structure
    if (left_y > IRS.min_y) {
        if (left_y < IRS.max_y) { // clip y on top border
            AW_click_cd cd(disp_device, (AW_CL)node->leftson, CL_NODE);
            Position    left(left_x, left_y);
            if (node->leftson->get_remark()) {
                TREE_show_branch_remark(disp_device, node->leftson->get_remark(), node->leftson->is_leaf, left, 1, remark_text_filter, bootstrap_min);
            }
            set_line_attributes_for(node->get_leftson());
            draw_branch_line(node->get_leftson()->gr.gc, Position(x_offset, left_y), left, line_filter);
        }
    }
    else {
        left_y = IRS.min_y;
    }

    AW_pos y_center = (left_y + right_y)*0.5;

    if (right_y > IRS.min_y && right_y < IRS.max_y) { // visible right branch in lower part of display
        AW_click_cd cd(disp_device, (AW_CL)node->rightson, CL_NODE);
        Position    right(right_x, right_y);
        if (node->rightson->get_remark()) {
            TREE_show_branch_remark(disp_device, node->rightson->get_remark(), node->rightson->is_leaf, right, 1, remark_text_filter, bootstrap_min);
        }
        set_line_attributes_for(node->get_rightson());
        draw_branch_line(node->get_rightson()->gr.gc, Position(x_offset, right_y), right, line_filter);
    }

    AW_click_cd cd(disp_device, (AW_CL)node, CL_NODE);
    set_line_attributes_for(node->get_leftson());
    disp_device->line(node->get_leftson()->gr.gc, x_offset, y_center, x_offset, left_y);

    set_line_attributes_for(node->get_rightson());
    disp_device->line(node->get_rightson()->gr.gc, x_offset, y_center, x_offset, right_y);

    if (node->is_named_group()) { // close unfolded group brackets and draw tipbox
        IRS.y  += IRS.halfstep_y+IRS.gap;

        {
            const int gc = AWT_GC_IRS_GROUP_BOX;
            disp_device->set_line_attributes(gc, 1, AW_DOTTED);
            disp_device->line(gc, x_offset-IRS.onePixel, IRS.y, x_offset+frame_width, IRS.y); // opened-group-frame
            disp_device->line(gc, x_offset-IRS.onePixel, group_y1, x_offset-IRS.onePixel,  IRS.y); // opened-group-frame
        }
        
        const int gc = node->gr.gc;
        set_line_attributes_for(node);
        filled_box(gc, Position(x_offset-IRS.onePixel, group_y1), TIP_BOX_SIZE);
    }
    return y_center;
}

void AWT_graphic_tree::show_irs_tree(AP_tree *at, double height) {

    IRS.draw_separator = true;
    IRS.y              = 0;
    IRS.step_y         = height;
    IRS.halfstep_y     = IRS.step_y*0.5;
    IRS.x_scale        = 200.0;      // @@@ should not have any effect, since display gets x-scaled. But if it's to low (e.g. 1.0) scaling on zoom-reset does not work

    const AW_font_limits& limits = disp_device->get_font_limits(AWT_GC_ALL_MARKED, 0);

    IRS.adjust_text    = disp_device->rtransform(Vector(NT_BOX_WIDTH, limits.ascent*0.5));
    IRS.onePixel       = disp_device->rtransform_size(1.0);
    IRS.gap            = 3*IRS.onePixel;
    IRS.group_closed   = 0;
    IRS.tree_depth     = at->gr.max_tree_depth;
    IRS.openGroupExtra = IRS.step_y+IRS.gap;
    IRS.sep_filter     = AW_SCREEN|AW_PRINTER_CLIP;

    IRS.is_size_device = disp_device->type() == AW_DEVICE_SIZE;

    Position  corner = disp_device->rtransform(Origin);   // real world coordinates of left/upper screen corner
    Rectangle rclip  = disp_device->get_rtransformed_cliprect();

    // the following values currently contain nonsense for size device @@@
    IRS.min_y   = corner.ypos();
    IRS.max_y   = rclip.bottom();
    IRS.fold_x1 = rclip.left();
    IRS.fold_x2 = rclip.right();

    list_tree_ruler_y           = paint_irs_sub_tree(at, 0);
    irs_tree_ruler_scale_factor = IRS.x_scale;
    
    disp_device->invisible(corner); // @@@ remove when size-dev works
}
