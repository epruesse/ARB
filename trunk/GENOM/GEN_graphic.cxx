/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"

using namespace std;

//  -------------------
//      GEN_graphic
//  -------------------

GEN_graphic *GEN_GRAPHIC = 0;

//  ------------------------------------------------------------------------------------------------------------------------------
//      GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_, void (*callback_installer_)(bool install, AWT_canvas*))
//  ------------------------------------------------------------------------------------------------------------------------------
GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_, void (*callback_installer_)(bool install, AWT_canvas*)) {
    change_flag        = 0;
    callback_installer = callback_installer_;

    exports.dont_fit_x    = 0;
    exports.dont_fit_y    = 0;
    exports.left_offset   = 10;
    exports.right_offset  = 30;
    exports.top_offset    = 5;
    exports.bottom_offset = 5;
    exports.dont_scroll   = 0;

    aw_root = aw_root_;
    gb_main = gb_main_;

    rot_ct.exists = AW_FALSE;
    rot_cl.exists = AW_FALSE;

    gen_root = 0;
    set_display_style(GEN_DisplayStyle(aw_root->awar(AWAR_GENMAP_DISPLAY_TYPE)->read_int()));
}
//  ------------------------------------
//      GEN_graphic::~GEN_graphic()
//  ------------------------------------
GEN_graphic::~GEN_graphic() {
}
//  ---------------------------------------------------------------------------------------------------------------
//      AW_gc_manager GEN_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas *ntw, AW_CL cd2)
//  ---------------------------------------------------------------------------------------------------------------
AW_gc_manager GEN_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas *ntw, AW_CL cd2) {
    AW_gc_manager preset_window =
        AW_manage_GC(aww,
                     device,
                     GEN_GC_FIRST_FONT,
                     GEN_GC_MAX,
                     AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb,
                     (AW_CL)ntw,
                     cd2,
                     "#55C0AA",
                     "Default$#5555ff",
                     "Gene$#ffffaa",
                     "Marked$#5cb1ff",
                     "Cursor$#ff0000",
                     0 );

    return preset_window;

}

//  --------------------------------------------------
//      void GEN_graphic::show(AW_device *device)
//  --------------------------------------------------
void GEN_graphic::show(AW_device *device) {
    if (gen_root) {
#if defined(DEBUG) && 0
        fprintf(stderr, "GEN_graphic::show\n");
#endif // DEBUG
        gen_root->paint(device);
    }
    else {
        device->line(GEN_GC_DEFAULT, -100,-100,100,100);
        device->line(GEN_GC_DEFAULT, -100,100,100,-100);
    }
}

//  ----------------------------------------------
//      inline int update_max(int u1, int u2)
//  ----------------------------------------------
inline int update_max(int u1, int u2) {
    if (u1 == 1 || u2 == 1) return 1;
    if (u1 == -1 || u2 == -1) return -1;
    return 0;
}

//  -------------------------------------------------------
//      int GEN_graphic::check_update(GBDATA *gbdummy)
//  -------------------------------------------------------
int GEN_graphic::check_update(GBDATA *gbdummy) {
    int iwantupdate = AWT_graphic::check_update(gbdummy);

    iwantupdate               = update_max(iwantupdate, change_flag);
    if (gen_root) iwantupdate = update_max(iwantupdate, gen_root->check_update(gbdummy));

    change_flag = 0;
    return iwantupdate;
}

//  ----------------------------------------------------------------------------------------------------------------
//      void GEN_graphic::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct)
//  ----------------------------------------------------------------------------------------------------------------
void GEN_graphic::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) {
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);
}

//  ----------------------------------------------------------------------------------------------------------
//      void GEN_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type,
//  ----------------------------------------------------------------------------------------------------------
void GEN_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type,
                          AW_pos screen_x, AW_pos screen_y, AW_clicked_line *cl, AW_clicked_text *ct) {
    AW_pos world_x;
    AW_pos world_y;
    device->rtransform(screen_x, screen_y, world_x, world_y);

    if (type == AW_Mouse_Press) {
        switch (cmd) {
            case AWT_MODE_ZOOM: {
                break;
            }
            case AWT_MODE_MOD: {
                if(button==AWT_M_LEFT) {
                    GEN_gene *gene = 0;
                    if (ct) gene   = (GEN_gene*)ct->client_data1;
                    if (cl) gene   = (GEN_gene*)cl->client_data1;

                    if (gene) {
                        GB_transaction dummy(gb_main);
                        aw_root->awar(AWAR_GENE_NAME)->write_string(gene->Name().c_str());
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

//  -------------------------------------------------------------------------------------------------------------
//      inline void getDrawGcs(GEN_iterator& gene, const string& curr_gene_name, int& draw_gc, int& text_gc)
//  -------------------------------------------------------------------------------------------------------------
inline void getDrawGcs(GEN_iterator& gene, const string& curr_gene_name, int& draw_gc, int& text_gc) {
    if (curr_gene_name == gene->Name()) { // current gene
        draw_gc = text_gc = GEN_GC_CURSOR;
    }
    else if (GB_read_flag((GBDATA*)gene->GbGene())) { // marked genes
        draw_gc = text_gc = GEN_GC_MARKED;
    }
    else {
        draw_gc = GEN_GC_GENE;
        text_gc = GEN_GC_DEFAULT;
    }
}

//  --------------------------------------------------------
//      int GEN_root::check_update(GBDATA */*gbdummy*/)
//  --------------------------------------------------------
int GEN_root::check_update(GBDATA */*gbdummy*/) {
    int iwantupdate = change_flag;
    change_flag = 0;
    return iwantupdate;
}
//  ------------------------------------------------
//      void GEN_root::paint(AW_device *device)
//  ------------------------------------------------
void GEN_root::paint(AW_device *device) {
    if (error_reason.length()) {
        device->text(GEN_GC_DEFAULT, error_reason.c_str(), 0, 0, 0.0, -1, 0, 0, 0);
        return;
    }

    AW_root *aw_root    = GEN_GRAPHIC->aw_root;
    int      arrow_size = aw_root->awar(AWAR_GENMAP_ARROW_SIZE)->read_int();

    switch (GEN_GRAPHIC->get_display_style()) {
        case GEN_DISPLAY_STYLE_RADIAL: {
            double w0      = 2.0*M_PI/double(length);
            double inside  = aw_root->awar(AWAR_GENMAP_RADIAL_INSIDE)->read_float()*1000;
            double outside = aw_root->awar(AWAR_GENMAP_RADIAL_OUTSIDE)->read_float();

            GEN_iterator curr = gene_set.begin();
            while (curr != gene_set.end()) {
                int    draw_gc, text_gc;
                getDrawGcs(curr, gene_name, draw_gc, text_gc);
                double w      = w0*curr->StartPos();
                double sinw   = sin(w);
                double cosw   = cos(w);
                int    len    = curr->Length();

                int xi = int(cosw*inside+0.5);
                int yi = int(sinw*inside+0.5);
                int xo = xi+int(cosw*outside*len+0.5);
                int yo = yi+int(sinw*outside*len+0.5);

                device->text(text_gc, curr->NodeInfo().c_str(), xo+20, yo, 0.0, -1, (AW_CL)(&*curr), 0, 0);
                device->line(draw_gc, xi, yi, xo, yo, -1, (AW_CL)(&*curr), 0);

                int sa = int(sinw*arrow_size+0.5);
                int ca = int(cosw*arrow_size+0.5);

                if (curr->Complement()) {
                    int xa = xi-sa+ca;
                    int ya = yi+ca+sa;
                    device->line(draw_gc, xi, yi, xa, ya, -1, (AW_CL)(&*curr), 0);
                }
                else {
                    int xa = xo+sa-ca;
                    int ya = yo-ca-sa;
                    device->line(draw_gc, xo, yo, xa, ya, -1, (AW_CL)(&*curr), 0);
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
            while (curr != gene_set.end()) {
                int draw_gc, text_gc;
                getDrawGcs(curr, gene_name, draw_gc, text_gc);
                int y         = int(curr->StartPos()*factor_y+0.5);
                int x2        = int(curr->Length()*factor_x+0.5);

                device->text(text_gc, curr->NodeInfo().c_str(), x2+20, y, 0.0, -1, (AW_CL)(&*curr), 0, 0);
                device->line(draw_gc, 0, y, x2, y, -1, (AW_CL)(&*curr), 0);

                if (curr->Complement()) {
                    device->line(draw_gc, 0, y, arrow_x, y-arrow_y, -1, (AW_CL)(&*curr), 0);
                }
                else {
                    device->line(draw_gc, x2, y, x2-arrow_x, y-arrow_y, -1, (AW_CL)(&*curr), 0);
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
            while (curr != gene_set.end()) {
                int draw_gc, text_gc;
                getDrawGcs(curr, gene_name, draw_gc, text_gc);

                int line1 = curr->StartPos()/display_width;
                int line2 = curr->EndPos()  /display_width;
                int x1    = int((curr->StartPos()-line1*display_width)*width_factor+0.5);
                int x2    = int((curr->EndPos()  -line2*display_width)*width_factor+0.5);
                int y1    = line1*height_of_line;
                int y1o   = y1-line_height;

                if (line1 == line2) { // whole gene in one book-line
                    device->line(draw_gc, x1, y1,  x1, y1o, -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, x2, y1,  x2, y1o, -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, x1, y1,  x2, y1,  -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, x1, y1o, x2, y1o, -1, (AW_CL)(&*curr), 0);
                    device->text(text_gc, curr->NodeInfo().c_str(), x1+2, y1-2, 0.0, -1, (AW_CL)(&*curr), 0, 0);

                    if (curr->Complement()) {
                        device->line(draw_gc, x2, y1o, x2-arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
                        device->line(draw_gc, x2, y1,  x2-arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
                    }
                    else {
                        device->line(draw_gc, x1, y1o,  x1+arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
                        device->line(draw_gc, x1, y1,   x1+arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
                    }
                }
                else {
                    int y2  = line2*height_of_line;
                    int y2o = y2-line_height;

                    // upper line (don't draw right border)
                    device->line(draw_gc, x1, y1,  x1,     y1o, -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, x1, y1,  xRight, y1,  -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, x1, y1o, xRight, y1o, -1, (AW_CL)(&*curr), 0);
                    device->text(text_gc, curr->NodeInfo().c_str(), x1+2, y1-2, 0.0, -1, (AW_CL)(&*curr), 0, 0);

                    // lower line (don't draw left border)
                    device->line(draw_gc, x2,    y2,  x2, y2o, -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, xLeft, y2,  x2, y2,  -1, (AW_CL)(&*curr), 0);
                    device->line(draw_gc, xLeft, y2o, x2, y2o, -1, (AW_CL)(&*curr), 0);
                    device->text(text_gc, curr->NodeInfo().c_str(), xLeft+2, y2-2, 0.0, -1, (AW_CL)(&*curr), 0, 0);

                    if (curr->Complement()) {
                        device->line(draw_gc, x2, y2o, x2-arrowMid, y2o+arrowMid, -1, (AW_CL)(&*curr), 0);
                        device->line(draw_gc, x2, y2,  x2-arrowMid, y2o+arrowMid, -1, (AW_CL)(&*curr), 0);
                    }
                    else {
                        device->line(draw_gc, x1, y1o, x1+arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
                        device->line(draw_gc, x1, y1,  x1+arrowMid, y1o+arrowMid, -1, (AW_CL)(&*curr), 0);
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

//  -----------------------------------------------------------
//      void GEN_graphic::delete_gen_root(AWT_canvas *ntw)
//  -----------------------------------------------------------
void GEN_graphic::delete_gen_root(AWT_canvas *ntw) {
    callback_installer(false, ntw);
    delete gen_root;
    gen_root = 0;
}
//  -----------------------------------------------------------
//      void GEN_graphic::reinit_gen_root(AWT_canvas *ntw)
//  -----------------------------------------------------------
void GEN_graphic::reinit_gen_root(AWT_canvas *ntw) {
    char *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    char *gene_name    = aw_root->awar(AWAR_GENE_NAME)->read_string();

    if (gen_root) {
        if (gen_root->SpeciesName() != string(species_name)) {
            delete_gen_root(ntw);
        }
        if (gen_root && gen_root->GeneName() != string(gene_name)) {
            gen_root->set_GeneName(gene_name);
        }
    }

    if (!gen_root) {
        gen_root = new GEN_root(species_name, gene_name, gb_main, "ali_genom");
        callback_installer(true, ntw);
    }

    free(species_name);
    free(gene_name);
}

//  -------------------------------------------------------------------
//      void GEN_graphic::set_display_style(GEN_DisplayStyle type)
//  -------------------------------------------------------------------
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

    change_flag = -1;
}

