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
}
//  ------------------------------------
//      GEN_graphic::~GEN_graphic()
//  ------------------------------------
GEN_graphic::~GEN_graphic() {
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

        if (!gen_root) gen_root = new GEN_root(species_name, gene_name, gb_main, "ali_genom");
    }

    free(species_name);
    free(gene_name);
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

