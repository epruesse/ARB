/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "GEN.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"

using namespace std;

//  -------------------
//      GEN_graphic
//  -------------------

GEN_graphic *GEN_GRAPHIC = 0;

//  ----------------------------------------------------------------------
//      GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_)
//  ----------------------------------------------------------------------
GEN_graphic::GEN_graphic(AW_root *aw_root_, GBDATA *gb_main_) {
    change_flag = 0;

    exports.dont_fit_x = 0;
    exports.dont_fit_y = 0;
    exports.left_offset = 20;
    exports.right_offset = 20;
    exports.top_offset = 20;
    exports.bottom_offset = 20;
    exports.dont_scroll = 0;

    aw_root = aw_root_;
    gb_main = gb_main_;

    rot_ct.exists = AW_FALSE;
    rot_cl.exists = AW_FALSE;

    gen_root = 0;
    reinit_gen_root();
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
        gen_root->paint(device);
    }
    else {
        device->line(GEN_GC_DEFAULT, -100,-100,100,100);
        device->line(GEN_GC_DEFAULT, -100,100,100,-100);
    }
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

//  -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      void GEN_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct)
//  -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GEN_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) {
}

#define GEN_LEVEL_HEIGHT_DRAW    60
#define GEN_LEVEL_HEIGHT_SPACING 5
#define GEN_LEVEL_HEIGHT         (GEN_LEVEL_HEIGHT_DRAW+GEN_LEVEL_HEIGHT_SPACING)
#define GEN_ESTIMATED_MAXLEVEL   1
#define GEN_WANTED_LINES         (70/GEN_ESTIMATED_MAXLEVEL)
#define GEN_DISPLAY_WIDTH        (GEN_WANTED_LINES*GEN_LEVEL_HEIGHT*(GEN_ESTIMATED_MAXLEVEL+1)/2)
#define GEN_TEXT_X_OFFSET        2
#define GEN_TEXT_Y_OFFSET        -2

//  ------------------------------------------------
//      void GEN_root::paint(AW_device *device)
//  ------------------------------------------------
void GEN_root::paint(AW_device *device) {
    int x = 0;
    int y = 0;

    if (error_reason.length()) {
        device->text(GEN_GC_DEFAULT, error_reason.c_str(), x, y, 0.0, -1, 0, 0, 0);
        return;
    }

    long         this_line_first_pos = 0;
    GEN_iterator curr                = gene_set.begin();

    int max_level      = GEN_ESTIMATED_MAXLEVEL;
    int height_of_line = max_level * GEN_LEVEL_HEIGHT;

    y += height_of_line;

    while (curr != gene_set.end()) { // for each line
        long         next_line_first_pos = this_line_first_pos+bp_per_line;
        GEN_iterator first_on_next_line  = gene_set.end();

        for (; curr != gene_set.end() && curr->EndPos()<this_line_first_pos; ++curr) ;
        // curr points to first gene with EndPos >= this_line_first_pos
        for (; curr != gene_set.end() && curr->StartPos()<next_line_first_pos; ++curr)
        {
            if (curr->EndPos()                   >= next_line_first_pos &&
                first_on_next_line               == gene_set.end()) {
                first_on_next_line                = curr;
            }

            int  this_y2    = y-curr->Level()*GEN_LEVEL_HEIGHT;
            int  this_y1    = this_y2-GEN_LEVEL_HEIGHT_DRAW+1;
            bool draw_left  = false;
            bool draw_right = false;
            int  this_x1    = x;
            int  this_x2    = x+GEN_DISPLAY_WIDTH-1;

            if (curr->StartPos() >= this_line_first_pos) {
                draw_left         = true;
                this_x1           = (GEN_DISPLAY_WIDTH * (curr->StartPos()-this_line_first_pos)) / bp_per_line;
            }

            if (curr->EndPos() < next_line_first_pos) {
                draw_right = true;
                this_x2    = (GEN_DISPLAY_WIDTH * (curr->EndPos()-this_line_first_pos)) / bp_per_line;
            }

            int  line_gc    = GEN_GC_GENE;
            int  text_gc    = GEN_GC_DEFAULT;
            bool is_marked  = GB_read_flag((GBDATA*)curr->GbGene());
            bool is_current = gene_name == curr->Name();

            if (is_marked) line_gc = text_gc = GEN_GC_MARKED;
            if (is_current) line_gc = text_gc = GEN_GC_CURSOR;

            if (draw_left) device->line(line_gc, this_x1, this_y1, this_x1, this_y2, -1, 0, 0);
            if (draw_right) device->line(line_gc, this_x2, this_y1, this_x2, this_y2, -1, 0, 0);
            device->line(line_gc, this_x1, this_y1, this_x2, this_y1, -1, 0, 0);
            device->line(line_gc, this_x1, this_y2, this_x2, this_y2, -1, 0, 0);

            device->text(text_gc, curr->Name().c_str(), this_x1+GEN_TEXT_X_OFFSET, this_y2+GEN_TEXT_Y_OFFSET, 0.0, -1, (AW_CL)curr->GbGene(), 0, 0);
        }

        if (first_on_next_line != gene_set.end()) curr = first_on_next_line;
        y                   += height_of_line;
        this_line_first_pos  = next_line_first_pos;
    }
}

//  --------------------------------------------
//      void GEN_graphic::reinit_gen_root()
//  --------------------------------------------
void GEN_graphic::reinit_gen_root() {
    char *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    char *gene_name    = aw_root->awar(AWAR_GENE_NAME)->read_string();

    if (gen_root) {
        if (gen_root->SpeciesName() != string(species_name)) {
            delete gen_root;
            gen_root = 0;
        }
        if (gen_root && gen_root->GeneName() != string(gene_name)) {
            gen_root->set_GeneName(gene_name);
        }
    }

    if (!gen_root) {
        gen_root = new GEN_root(species_name, gene_name, gb_main, "ali_genom", GEN_WANTED_LINES);
    }

    free(species_name);
    free(gene_name);
}

