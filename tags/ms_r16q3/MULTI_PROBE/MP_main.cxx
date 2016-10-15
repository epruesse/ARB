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
#include "MultiProbe.hxx"
#include "mp_proto.hxx"

#include <aw_root.hxx>
#include <aw_awar_defs.hxx>
#include <awt_canvas.hxx>

awar_vars  mp_gl_awars;
MP_Main   *mp_main             = NULL;
char       MP_probe_tab[256];                       // zum checken, ob ein eingegebener Sondenstring ok ist
int        remembered_mismatches;
int        anz_elem_marked     = 0;
int        anz_elem_unmarked   = 0;
bool       pt_server_different = false;

MP_Main::MP_Main(AW_root *awr, AWT_canvas *canvas) {
    aw_root   = awr;
    scr       = canvas;
    stc       = NULL;
    create_awars();
    mp_window = new MP_Window(aw_root, canvas->gb_main);
    p_eval    = NULL;
}

MP_Main::~MP_Main()
{
    aw_root->awar_int(MP_AWAR_QUALITY)->remove_callback(MP_gen_quality);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->remove_callback(MP_gen_singleprobe);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->remove_callback(MP_modify_selected);

    delete p_eval;
    delete stc;
    delete mp_window;

    new_pt_server = true;
}

void MP_Main::destroy_probe_eval()
{
    delete p_eval;
    p_eval = NULL;
}

ProbeValuation *MP_Main::new_probe_eval(char **field, int size, int *array, int *single_mismatch)
{
    p_eval = new ProbeValuation(field, size, array, single_mismatch);
    p_eval->set_act_gen(new Generation(p_eval->get_max_init_for_gen(), 1)); // erste Generation = Ausgangspopulation
    return p_eval;
}

void MP_Main::create_awars()
{
    aw_root->awar_string(MP_AWAR_SEQIN);
    aw_root->awar_string(MP_AWAR_SELECTEDPROBES)->add_target_var(& mp_gl_awars.selected_probes);
    aw_root->awar_string(MP_AWAR_PROBELIST)->add_target_var(& mp_gl_awars.probelist);
    aw_root->awar_int(MP_AWAR_WEIGHTEDMISMATCHES)->add_target_var(& mp_gl_awars.weightedmismatches)->write_int(2);
    aw_root->awar_int(MP_AWAR_COMPLEMENT, 1)->add_target_var(& mp_gl_awars.complement);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->add_target_var(& mp_gl_awars.no_of_mismatches)->add_callback(MP_modify_selected);
    remembered_mismatches = 0;      // derselbe initiale Wert wie mp_gl_awars.no_of_mismatches
    aw_root->awar_int(MP_AWAR_PTSERVER)->add_target_var(& mp_gl_awars.ptserver);
    aw_root->awar_string(MP_AWAR_RESULTPROBES)->add_target_var(& mp_gl_awars.result_probes);
    aw_root->awar_string(MP_AWAR_RESULTPROBESCOMMENT)->add_target_var(& mp_gl_awars.result_probes_comment);
    aw_root->awar_int(MP_AWAR_NOOFPROBES)->add_target_var(& mp_gl_awars.no_of_probes)->write_int(3);
    aw_root->awar_int(MP_AWAR_QUALITY)->add_target_var(& mp_gl_awars.probe_quality)->add_callback(MP_gen_quality)->write_int(QUALITYDEFAULT);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->add_target_var(& mp_gl_awars.singlemismatches)->add_callback(MP_gen_singleprobe);
    aw_root->awar_float(MP_AWAR_OUTSIDEMISMATCHES)->add_target_var(& mp_gl_awars.outside_mismatches_difference)->write_float(1.0);
    aw_root->awar_int(MP_AWAR_QUALITYBORDER1)->add_target_var(& mp_gl_awars.qualityborder_best)->write_int(5);

    aw_root->awar_int(MP_AWAR_EMPHASIS)->add_target_var(& mp_gl_awars.emphasis)->write_int(0);
    aw_root->awar_float(MP_AWAR_GREYZONE)->add_target_var(& mp_gl_awars.greyzone)->write_float(0.0);
    aw_root->awar_int(MP_AWAR_ECOLIPOS)->add_target_var(& mp_gl_awars.ecolipos)->write_int(0);

    aw_root->awar_int(MP_AWAR_AUTOADVANCE, 1);
}


static void create_tables()
{
    int i;

    // probe_tab
    for (i=0; i<256; i++)
        MP_probe_tab[i] = false;

    const unsigned char *true_chars = (const unsigned char *)"atgucnATGUCN";
    for (i = 0; true_chars[i]; ++i) {
        MP_probe_tab[true_chars[i]] = true;
    }
}

AW_window *create_multiprobe_window(AW_root *root, TREE_canvas *canvas) {
    if (!mp_main) {
        create_tables();
        mp_main = new MP_Main(root, canvas);
    }

    AW_window *aw = mp_main->get_mp_window()->get_window();
    aw->show();

    return aw;
}

