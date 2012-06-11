// ================================================================ //
//                                                                  //
//   File      : MP_main.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <multi_probe.hxx>

#include "MP_probe.hxx"
#include "mp_proto.hxx"

#include <aw_root.hxx>
#include <aw_awar_defs.hxx>
#include <awt_canvas.hxx>

awar_vars  mp_gl_awars;
MP_Main   *mp_main   = NULL;
MP_Global *mp_global = NULL;

MP_Main::MP_Main(AW_root *awr, AWT_canvas *canvas) {
    aw_root   = awr;
    scr       = canvas;
    create_awars();
    mp_window = new MP_Window(aw_root, canvas->gb_main);
}

MP_Main::~MP_Main() {
    aw_root->awar_int(MP_AWAR_QUALITY)->remove_callback(MP_gen_quality);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->remove_callback(MP_gen_singleprobe, (AW_CL)0, (AW_CL)0);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->remove_callback(MP_modify_selected);

    delete mp_window;
}

void MP_Main::create_awars() {
    aw_root->awar_string(MP_AWAR_SEQIN);
    aw_root->awar_string(MP_AWAR_SELECTEDPROBES)->add_target_var(& mp_gl_awars.selected_probes);
    aw_root->awar_string(MP_AWAR_PROBELIST)->add_target_var(& mp_gl_awars.probelist);
    aw_root->awar_int(MP_AWAR_WEIGHTEDMISMATCHES)->add_target_var(& mp_gl_awars.weightedmismatches)->write_int(2);
    aw_root->awar_int(MP_AWAR_COMPLEMENT, 1)->add_target_var(& mp_gl_awars.complement);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->add_target_var(& mp_gl_awars.no_of_mismatches)->add_callback(MP_modify_selected);
    aw_root->awar_int(MP_AWAR_PTSERVER)->add_target_var(& mp_gl_awars.ptserver);
    aw_root->awar_string(MP_AWAR_RESULTPROBES)->add_target_var(& mp_gl_awars.result_probes);
    aw_root->awar_string(MP_AWAR_RESULTPROBESCOMMENT)->add_target_var(& mp_gl_awars.result_probes_comment);
    aw_root->awar_int(MP_AWAR_NOOFPROBES)->add_target_var(& mp_gl_awars.no_of_probes)->write_int(3);
    aw_root->awar_int(MP_AWAR_QUALITY)->add_target_var(& mp_gl_awars.probe_quality)->add_callback(MP_gen_quality)->write_int(QUALITYDEFAULT);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->add_target_var(& mp_gl_awars.singlemismatches)->add_callback(MP_gen_singleprobe, (AW_CL)0, (AW_CL)0);
    aw_root->awar_float(MP_AWAR_OUTSIDEMISMATCHES)->add_target_var(& mp_gl_awars.outside_mismatches_difference)->write_float(1.0);
    aw_root->awar_int(MP_AWAR_QUALITYBORDER1)->add_target_var(& mp_gl_awars.qualityborder_best)->write_int(5);

    aw_root->awar_int(MP_AWAR_EMPHASIS)->add_target_var(& mp_gl_awars.emphasis)->write_int(0);
    aw_root->awar_float(MP_AWAR_GREYZONE)->add_target_var(& mp_gl_awars.greyzone)->write_float(0.0);
    aw_root->awar_int(MP_AWAR_ECOLIPOS)->add_target_var(& mp_gl_awars.ecolipos)->write_int(0);

    aw_root->awar_int(MP_AWAR_AUTOADVANCE, 1);
}


AW_window *create_multiprobe_window(AW_root *root, AW_CL cl_canvas) {
    if (!mp_main) {
        mp_main = new MP_Main(root, (AWT_canvas *)cl_canvas);
    }

    AW_window *aw = mp_main->get_mp_window()->get_window();
    aw->show();

    return aw;
}

