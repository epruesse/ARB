// =============================================================== //
//                                                                 //
//   File      : GEN_graphic.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"

#include <arbdbt.h>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt_attributes.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <climits>

using namespace std;
using namespace AW;

//  -------------------
//      GEN_graphic
//  -------------------

GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_, GEN_graphic_cb_installer callback_installer_, int window_nr_)
    : aw_root(aw_root_)
    , gb_main(gb_main_)
    , callback_installer(callback_installer_)
    , window_nr(window_nr_)
    , gen_root(0)
    , want_zoom_reset(false)
{
    exports.dont_fit_x  = 0;
    exports.dont_fit_y  = 0;
    exports.dont_scroll = 0;
    exports.set_standard_default_padding();

    rot_ct.exists = false;
    rot_cl.exists = false;

    set_display_style(GEN_DisplayStyle(aw_root->awar(AWAR_GENMAP_DISPLAY_TYPE(window_nr))->read_int()));
}

GEN_graphic::~GEN_graphic() {
}

AW_gc_manager GEN_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas *ntw, AW_CL cd2) {
    disp_device                 = device;
    AW_gc_manager preset_window = AW_manage_GC(aww, device,
                                               GEN_GC_FIRST_FONT, GEN_GC_MAX, AW_GCM_DATA_AREA,
                                               (AW_CB)AWT_resize_cb, (AW_CL)ntw, cd2,
                                               true, // define color groups
                                               "#55C0AA",
                                               "Default$#5555ff",
                                               "Gene$#000000",
                                               "Marked$#ffff00",
                                               "Cursor$#ff0000",
                                               NULL);
    return preset_window;
}

void GEN_graphic::show(AW_device *device) {
    if (gen_root) {
#if defined(DEBUG) && 0
        fprintf(stderr, "GEN_graphic::show\n");
#endif // DEBUG

        gen_root->paint(device);
    }
    else {
        device->line(GEN_GC_DEFAULT, -100, -100, 100, 100);
        device->line(GEN_GC_DEFAULT, -100, 100, 100, -100);
    }
}

int GEN_graphic::check_update(GBDATA *) {
    // if check_update returns >0 -> zoom_reset is done
    int do_zoom_reset = want_zoom_reset;
    want_zoom_reset   = false;
    return do_zoom_reset;
}

void GEN_graphic::info(AW_device */*device*/, AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line */*cl*/, AW_clicked_text */*ct*/) {
    aw_message("INFO MESSAGE");
}

void GEN_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod /* key_modifier */, AW_key_code /* key_code */, char /* key_char */, AW_event_type type,
                          AW_pos screen_x, AW_pos screen_y, AW_clicked_line *cl, AW_clicked_text *ct) {
    AW_pos world_x;
    AW_pos world_y;
    device->rtransform(screen_x, screen_y, world_x, world_y);

    if (type == AW_Mouse_Press) {
        switch (cmd) {
            case AWT_MODE_ZOOM: {
                break;
            }
            case AWT_MODE_SELECT:
            case AWT_MODE_EDIT: {
                if (button==AWT_M_LEFT) {
                    GEN_gene *gene = 0;
                    if (ct) gene   = (GEN_gene*)ct->client_data1;
                    if (cl) gene   = (GEN_gene*)cl->client_data1;

                    if (gene) {
                        GB_transaction dummy(gb_main);
                        aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->write_string(gene->Name().c_str());

                        if (cmd == AWT_MODE_EDIT) {
                            GEN_create_gene_window(aw_root, (AW_CL)gb_main)->activate();
                        }
                    }
                }
                break;
            }
            default: {
                gen_assert(0);
                break;
            }
        }
    }
}

inline int GEN_root::smart_text(AW_device *device, int gc, const char *str, AW_pos x, AW_pos y) {
    int slen = strlen(str);
    int res  = device->text(gc, str, x, y, 0.0, AW_ALL_DEVICES_UNSCALED, slen);
    if (gc == GEN_GC_CURSOR) {
        int                   xsize = device->get_string_size(gc, str, slen);
        const AW_font_limits& lim   = device->get_font_limits(gc, 0);

        Position  stext = device->transform(Position(x, y));
        Rectangle srect(stext.xpos(), stext.ypos()-lim.ascent, stext.xpos()+xsize, stext.ypos()+lim.descent);
        Rectangle wrect = device->rtransform(srect);
        increase_selected_range(wrect);
    }
    return res;
}

inline int GEN_root::smart_line(AW_device *device, int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1) {
    int res = device->line(gc, x0, y0, x1, y1);
    if (gc == GEN_GC_CURSOR) increase_selected_range(Rectangle(x0, y0, x1, y1));
    return res;
}

enum PaintWhat {
    PAINT_MIN,
    PAINT_NORMAL = PAINT_MIN,
    PAINT_COLORED,
    PAINT_MARKED,
    PAINT_SELECTED,
    PAINT_MAX    = PAINT_SELECTED,
};

inline bool getDrawGcs(GEN_iterator& gene, PaintWhat what, const string& curr_gene_name, int& draw_gc, int& text_gc) {
    bool draw = false;
    if (curr_gene_name == gene->Name()) { // current gene
        draw_gc = text_gc = GEN_GC_CURSOR;
        draw    = (what == PAINT_SELECTED);
    }
    else {
        GBDATA *gb_gene = (GBDATA*)gene->GbGene();

        if (GB_read_flag(gb_gene)) { // marked genes
            draw_gc = text_gc = GEN_GC_MARKED;
            draw    = (what == PAINT_MARKED);
        }
        else {
            int color_group = AWT_gene_get_dominant_color(gb_gene);

            if (color_group) {
                draw_gc = text_gc = GEN_GC_FIRST_COLOR_GROUP+color_group-1;
                draw    = (what == PAINT_COLORED);
            }
            else {
                draw_gc = GEN_GC_GENE;
                text_gc = GEN_GC_DEFAULT; // see show_all_nds in GEN_root::paint if you change this!!!
                draw    = (what == PAINT_NORMAL);
            }
        }
    }
    return draw;
}

void GEN_root::paint(AW_device *device) {
    if (error_reason.length()) {
        device->text(GEN_GC_DEFAULT, error_reason.c_str(), 0, 0);
        return;
    }

    clear_selected_range();

    AW_root *aw_root      = gen_graphic->get_aw_root();
    int      arrow_size   = aw_root->awar(AWAR_GENMAP_ARROW_SIZE)->read_int();
    int      show_all_nds = aw_root->awar(AWAR_GENMAP_SHOW_ALL_NDS)->read_int();

    for (PaintWhat paint_what = PAINT_MIN; paint_what <= PAINT_MAX; paint_what = PaintWhat((int(paint_what)+1))) {
        switch (gen_graphic->get_display_style()) {
            case GEN_DISPLAY_STYLE_RADIAL: {
                double w0      = 2.0*M_PI/double(length);
                double mp2     = M_PI/2;
                double inside  = aw_root->awar(AWAR_GENMAP_RADIAL_INSIDE)->read_float()*1000;
                double outside = aw_root->awar(AWAR_GENMAP_RADIAL_OUTSIDE)->read_float();

                GEN_iterator curr = gene_set.begin();
                GEN_iterator end  = gene_set.end();

                while (curr != end) {
                    int draw_gc, text_gc;
                    if (getDrawGcs(curr, paint_what, gene_name, draw_gc, text_gc)) {
                        double w    = w0*curr->StartPos()-mp2;
                        double sinw = sin(w);
                        double cosw = cos(w);
                        int    len  = curr->Length();

                        int xi = int(cosw*inside+0.5);
                        int yi = int(sinw*inside+0.5);
                        int xo = xi+int(cosw*outside*len+0.5);
                        int yo = yi+int(sinw*outside*len+0.5);

                        AW_click_cd cd(device, (AW_CL)&*curr);
                        if (show_all_nds || text_gc != GEN_GC_DEFAULT) {
                            smart_text(device, text_gc, curr->NodeInfo().c_str(), xo+20, yo);
                        }
                        smart_line(device, draw_gc, xi, yi, xo, yo);

                        int sa = int(sinw*arrow_size+0.5);
                        int ca = int(cosw*arrow_size+0.5);

                        if (curr->Complement()) {
                            int xa = xi-sa+ca;
                            int ya = yi+ca+sa;
                            smart_line(device, draw_gc, xi, yi, xa, ya);
                        }
                        else {
                            int xa = xo+sa-ca;
                            int ya = yo-ca-sa;
                            smart_line(device, draw_gc, xo, yo, xa, ya);
                        }
                    }
                    ++curr;
                }
                break;
            }
            case GEN_DISPLAY_STYLE_VERTICAL: {
                double factor_x = aw_root->awar(AWAR_GENMAP_VERTICAL_FACTOR_X)->read_float();
                double factor_y = aw_root->awar(AWAR_GENMAP_VERTICAL_FACTOR_Y)->read_float();
                int    arrow_x  = int(factor_x*arrow_size);
                int    arrow_y  = int(factor_y*arrow_size);

                GEN_iterator curr = gene_set.begin();
                GEN_iterator end  = gene_set.end();

                while (curr != end) {
                    int draw_gc, text_gc;
                    if (getDrawGcs(curr, paint_what, gene_name, draw_gc, text_gc)) {
                        int y         = int(curr->StartPos()*factor_y+0.5);
                        int x2        = int(curr->Length()*factor_x+0.5);

                        AW_click_cd cd(device, (AW_CL)&*curr);
                        if (show_all_nds || text_gc != GEN_GC_DEFAULT) {
                            smart_text(device, text_gc, curr->NodeInfo().c_str(), x2+20, y);
                        }
                        smart_line(device, draw_gc, 0, y, x2, y);
                        if (curr->Complement()) {
                            smart_line(device, draw_gc, 0, y, arrow_x, y-arrow_y);
                        }
                        else {
                            smart_line(device, draw_gc, x2, y, x2-arrow_x, y-arrow_y);
                        }
                    }
                    ++curr;
                }
                break;
            }
            case GEN_DISPLAY_STYLE_BOOK: {
                int    display_width  = aw_root->awar(AWAR_GENMAP_BOOK_BASES_PER_LINE)->read_int();
                double width_factor   = aw_root->awar(AWAR_GENMAP_BOOK_WIDTH_FACTOR)->read_float();
                int    line_height    = aw_root->awar(AWAR_GENMAP_BOOK_LINE_HEIGHT)->read_int();
                int    line_space     = aw_root->awar(AWAR_GENMAP_BOOK_LINE_SPACE)->read_int();
                int    height_of_line = line_height+line_space;
                int    xLeft          = 0;
                int    xRight         = int(display_width*width_factor+0.5);
                int    arrowMid       = line_height/2;

                GEN_iterator curr = gene_set.begin();
                GEN_iterator end  = gene_set.end();

                while (curr != end) {
                    int draw_gc, text_gc;
                    if (getDrawGcs(curr, paint_what, gene_name, draw_gc, text_gc)) {
                        int line1 = curr->StartPos()/display_width;
                        int line2 = curr->EndPos()  / display_width;
                        int x1    = int((curr->StartPos()-line1*display_width)*width_factor+0.5);
                        int x2    = int((curr->EndPos()  -line2*display_width)*width_factor+0.5);
                        int y1    = line1*height_of_line;
                        int y1o   = y1-line_height;

                        AW_click_cd cd(device, (AW_CL)&*curr);
                        if (line1 == line2) { // whole gene in one book-line
                            smart_line(device, draw_gc, x1, y1,  x1, y1o);
                            smart_line(device, draw_gc, x2, y1,  x2, y1o);
                            smart_line(device, draw_gc, x1, y1,  x2, y1);
                            smart_line(device, draw_gc, x1, y1o, x2, y1o);
                            if (show_all_nds || text_gc != GEN_GC_DEFAULT) smart_text(device, text_gc, curr->NodeInfo().c_str(), x1+2, y1-2);

                            if (curr->Complement()) {
                                smart_line(device, draw_gc, x2, y1o, x2-arrowMid, y1o+arrowMid);
                                smart_line(device, draw_gc, x2, y1,  x2-arrowMid, y1o+arrowMid);
                            }
                            else {
                                smart_line(device, draw_gc, x1, y1o,  x1+arrowMid, y1o+arrowMid);
                                smart_line(device, draw_gc, x1, y1,   x1+arrowMid, y1o+arrowMid);
                            }
                        }
                        else {
                            int y2  = line2*height_of_line;
                            int y2o = y2-line_height;

                            // upper line (don't draw right border)
                            smart_line(device, draw_gc, x1, y1,  x1,     y1o);
                            smart_line(device, draw_gc, x1, y1,  xRight, y1);
                            smart_line(device, draw_gc, x1, y1o, xRight, y1o);
                            if (show_all_nds || text_gc != GEN_GC_DEFAULT) smart_text(device, text_gc, curr->NodeInfo().c_str(), x1+2, y1-2);

                            // lower line (don't draw left border)
                            smart_line(device, draw_gc, x2,    y2,  x2, y2o);
                            smart_line(device, draw_gc, xLeft, y2,  x2, y2);
                            smart_line(device, draw_gc, xLeft, y2o, x2, y2o);
                            if (show_all_nds || text_gc != GEN_GC_DEFAULT) smart_text(device, text_gc, curr->NodeInfo().c_str(), xLeft+2, y2-2);

                            if (curr->Complement()) {
                                smart_line(device, draw_gc, x2, y2o, x2-arrowMid, y2o+arrowMid);
                                smart_line(device, draw_gc, x2, y2,  x2-arrowMid, y2o+arrowMid);
                            }
                            else {
                                smart_line(device, draw_gc, x1, y1o, x1+arrowMid, y1o+arrowMid);
                                smart_line(device, draw_gc, x1, y1,  x1+arrowMid, y1o+arrowMid);
                            }
                        }
                    }
                    ++curr;
                }
                break;
            }
            default: {
                gen_assert(0);
                break;
            }
        }
    }
}

void GEN_graphic::delete_gen_root(AWT_canvas *ntw) {
    callback_installer(false, ntw, this);
    delete gen_root;
    gen_root = 0;
}

void GEN_graphic::reinit_gen_root(AWT_canvas *ntw, bool force_reinit) {
    char *organism_name = aw_root->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->read_string();
    char *gene_name     = aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->read_string();

    if (gen_root) {
        if (force_reinit || (gen_root->OrganismName() != string(organism_name))) {
            if (gen_root->OrganismName().length() == 0) {
                want_zoom_reset = true; // no organism was displayed before
            }
            delete_gen_root(ntw);
        }
        if (gen_root && gen_root->GeneName() != string(gene_name)) {
            gen_root->set_GeneName(gene_name);
        }
    }

    if (!gen_root) {
        gen_root = new GEN_root(organism_name, gene_name, gb_main, aw_root, this);
        callback_installer(true, ntw, this);
    }

    free(organism_name);
    free(gene_name);
}

void GEN_graphic::set_display_style(GEN_DisplayStyle type) {
    style = type;

    switch (style) {
        case GEN_DISPLAY_STYLE_RADIAL: {
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 0;
            exports.dont_fit_larger = 0;
            break;
        }
        case GEN_DISPLAY_STYLE_VERTICAL: {
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            break;
        }
        case GEN_DISPLAY_STYLE_BOOK: {
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    want_zoom_reset = true;
}

