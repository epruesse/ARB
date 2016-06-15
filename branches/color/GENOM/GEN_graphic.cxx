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
#include <ad_colorset.h>

#include <aw_awar.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <climits>

using namespace std;
using namespace AW;

// ---------------------
//      GEN_graphic

GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_, GEN_graphic_cb_installer callback_installer_, int window_nr_)
    : aw_root(aw_root_),
      gb_main(gb_main_),
      callback_installer(callback_installer_),
      window_nr(window_nr_),
      gen_root(0),
      want_zoom_reset(false),
      disp_device(NULL)
{
    exports.set_standard_default_padding();

    set_display_style(GEN_DisplayStyle(aw_root->awar(AWAR_GENMAP_DISPLAY_TYPE(window_nr))->read_int()));
}

GEN_graphic::~GEN_graphic() {}

AW_gc_manager *GEN_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas *scr) {
    disp_device = device;

    AW_gc_manager *gc_manager =
        AW_manage_GC(aww,
                     scr->get_gc_base_name(),
                     device,
                     GEN_GC_FIRST_FONT, GEN_GC_MAX, AW_GCM_DATA_AREA,
                     makeGcChangedCallback(AWT_GC_changed_cb, scr),
                     "#55C0AA",
                     "Default$#5555ff",
                     "Gene$#000000",
                     "Marked$#ffff00",
                     "Cursor$#ff0000",

                     "&color_groups", // use color groups

                     NULL);

    return gc_manager;
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

void GEN_graphic::handle_command(AW_device *, AWT_graphic_event& event) {
    if (event.type() == AW_Mouse_Press) {
        switch (event.cmd()) {
            case AWT_MODE_ZOOM: {
                break;
            }
            case AWT_MODE_SELECT:
            case AWT_MODE_INFO: {
                if (event.button()==AW_BUTTON_LEFT) {
                    const AW_clicked_element *clicked = event.best_click();
                    if (clicked) {
                        GEN_gene *gene = (GEN_gene*)clicked->cd1();
                        if (gene) {
                            GB_transaction ta(gb_main);
                            aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->write_string(gene->Name().c_str());

                            if (event.cmd() == AWT_MODE_INFO) {
                                GEN_popup_gene_infowindow(aw_root, gb_main);
                            }
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

    PAINT_MAX = PAINT_SELECTED,
    PAINT_COUNT, // has to be last (do NOT remove)
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
            int color_group = AW_color_groups_active() ? GBT_get_color_group(gb_gene) : 0;
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
                float factor_x = aw_root->awar(AWAR_GENMAP_VERTICAL_FACTOR_X)->read_float();
                float factor_y = aw_root->awar(AWAR_GENMAP_VERTICAL_FACTOR_Y)->read_float();
                int   arrow_x  = int(factor_x*arrow_size);
                int   arrow_y  = int(factor_y*arrow_size);

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
                int   display_width  = aw_root->awar(AWAR_GENMAP_BOOK_BASES_PER_LINE)->read_int();
                float width_factor   = aw_root->awar(AWAR_GENMAP_BOOK_WIDTH_FACTOR)->read_float();
                int   line_height    = aw_root->awar(AWAR_GENMAP_BOOK_LINE_HEIGHT)->read_int();
                int   line_space     = aw_root->awar(AWAR_GENMAP_BOOK_LINE_SPACE)->read_int();
                int   height_of_line = line_height+line_space;
                int   xLeft          = 0;
                int   xRight         = int(display_width*width_factor+0.5);
                int   arrowMid       = line_height/2;

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
        }
    }
}

void GEN_graphic::delete_gen_root(AWT_canvas *scr, bool just_forget_callbacks) {
    callback_installer(just_forget_callbacks ? FORGET_CBS : REMOVE_CBS, scr, this);
    delete gen_root;
    gen_root = 0;
}

void GEN_graphic::reinit_gen_root(AWT_canvas *scr, bool force_reinit) {
    char *organism_name = aw_root->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->read_string();
    char *gene_name     = aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->read_string();

    if (gen_root) {
        if (force_reinit || (gen_root->OrganismName() != string(organism_name))) {
            bool just_forget_callbacks = false;
            if (gen_root->OrganismName().length() == 0) {
                want_zoom_reset = true; // no organism was displayed before
            }
            else {
                GB_transaction ta(gb_main);
                if (!GEN_find_organism(gb_main, gen_root->OrganismName().c_str())) {
                    just_forget_callbacks = true; // genome already deleted -> just clean up callback table
                    want_zoom_reset       = true; // invalid (=none) organism was displayed before
                }
            }
            delete_gen_root(scr, just_forget_callbacks);
        }
        if (gen_root && gen_root->GeneName() != string(gene_name)) {
            gen_root->set_GeneName(gene_name);
        }
    }

    if (!gen_root) {
        gen_root = new GEN_root(organism_name, gene_name, gb_main, aw_root, this);
        callback_installer(INSTALL_CBS, scr, this);
    }

    free(organism_name);
    free(gene_name);
}

void GEN_graphic::set_display_style(GEN_DisplayStyle type) {
    style = type;
    
    switch (style) {
        case GEN_DISPLAY_STYLE_RADIAL:
            exports.fit_mode = AWT_FIT_LARGER;
            break;

        case GEN_DISPLAY_STYLE_VERTICAL:
        case GEN_DISPLAY_STYLE_BOOK:
            exports.fit_mode = AWT_FIT_SMALLER;
            break;
    }

    exports.zoom_mode = AWT_ZOOM_BOTH;
    want_zoom_reset   = true;
}


