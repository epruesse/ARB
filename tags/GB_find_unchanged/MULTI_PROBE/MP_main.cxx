#include <aw_root.hxx>
#include <aw_window.hxx>
#include <math.h>

#include "MultiProbe.hxx"
#include "mp_proto.hxx"

awar_vars  mp_gl_awars;
MP_Main   *mp_main             = NULL;
char       MP_probe_tab[256];   //zum checken, ob ein eingegebener Sondenstring ok ist
int        remembered_mismatches;
int        anz_elem_marked     = 0;
int        anz_elem_unmarked   = 0;
int        outside_mismatches  = 0;
BOOL       pt_server_different = FALSE;

double MAXMARKEDFACTOR = 1.0;
double MINUNMARKEDFACTOR = 1.0;
double SUMMARKEDFACTOR = 1.0;
double SUMUNMARKEDFACTOR = 1.0;


MP_Main::MP_Main(AW_root *awr,AWT_canvas *ntwt)
{
    aw_initstatus();
    aw_root = awr;
    ntw     = ntwt;
    stc     = NULL;
    create_awars();
    mp_window   = new MP_Window(aw_root);
    p_eval  = NULL;
}

MP_Main::~MP_Main()
{
    aw_root->awar_int(MP_AWAR_QUALITY)->remove_callback(MP_gen_quality,(AW_CL)0,(AW_CL)0);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->remove_callback(MP_gen_singleprobe,(AW_CL)0,(AW_CL)0);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->remove_callback(MP_modify_selected,(AW_CL)0,(AW_CL)0);

    delete p_eval;
    delete stc;
    delete mp_window;

    delete glob_old_seq;
    glob_old_seq = NULL;
    new_pt_server = TRUE;
}

void MP_Main::destroy_probe_eval()
{
    delete p_eval;
    p_eval = NULL;
}

ProbeValuation *MP_Main::new_probe_eval(char **field, int size, int *array, int *single_mismatch)
{
    p_eval = new ProbeValuation(field, size, array, single_mismatch);
    p_eval->set_act_gen(new Generation(p_eval->get_max_init_for_gen(), 1)); //erste Generation = Ausgangspopulation
    return p_eval;
}

void MP_Main::create_awars()
{
    aw_root->awar_string(MP_AWAR_SEQUENZEINGABE)->add_target_var(& mp_gl_awars.manual_sequence);
    aw_root->awar_string(MP_AWAR_SELECTEDPROBES)->add_target_var(& mp_gl_awars.selected_probes);
    aw_root->awar_string(MP_AWAR_PROBELIST)->add_target_var(& mp_gl_awars.probelist);
    aw_root->awar_int(MP_AWAR_WEIGHTEDMISMATCHES)->add_target_var(& mp_gl_awars.weightedmismatches)->write_int(2);
    aw_root->awar_int(MP_AWAR_COMPLEMENT,1)->add_target_var(& mp_gl_awars.complement);
    aw_root->awar_int(MP_AWAR_MISMATCHES)->add_target_var(& mp_gl_awars.no_of_mismatches)->add_callback(MP_modify_selected,(AW_CL)0,(AW_CL)0);
    remembered_mismatches = 0;      //derselbe initiale Wert wie mp_gl_awars.no_of_mismatches
    aw_root->awar_int(MP_AWAR_PTSERVER)->add_target_var( & mp_gl_awars.ptserver);
    aw_root->awar_string(MP_AWAR_RESULTPROBES)->add_target_var( & mp_gl_awars.result_probes );
    aw_root->awar_string(MP_AWAR_RESULTPROBESCOMMENT)->add_target_var( & mp_gl_awars.result_probes_comment );
    aw_root->awar_int(MP_AWAR_NOOFPROBES)->add_target_var( & mp_gl_awars.no_of_probes )->write_int(3);
    aw_root->awar_int(MP_AWAR_QUALITY)->add_target_var( & mp_gl_awars.probe_quality )->add_callback(MP_gen_quality,(AW_CL)0,(AW_CL)0)->write_int(QUALITYDEFAULT);
    aw_root->awar_int(MP_AWAR_SINGLEMISMATCHES)->add_target_var( & mp_gl_awars.singlemismatches )->add_callback(MP_gen_singleprobe,(AW_CL)0,(AW_CL)0);
    aw_root->awar_float(MP_AWAR_OUTSIDEMISMATCHES)->add_target_var( & mp_gl_awars.outside_mismatches_difference )->write_float(1.0);
    aw_root->awar_int(MP_AWAR_QUALITYBORDER1)->add_target_var( & mp_gl_awars.qualityborder_best )->write_int(5);

    aw_root->awar_int(MP_AWAR_EMPHASIS)->add_target_var( & mp_gl_awars.emphasis )->write_int(0);
    aw_root->awar_float(MP_AWAR_GREYZONE)->add_target_var( & mp_gl_awars.greyzone )->write_float(0.0);
    aw_root->awar_int(MP_AWAR_ECOLIPOS)->add_target_var( & mp_gl_awars.ecolipos )->write_int(0);

    aw_root->awar_int(MP_AWAR_AUTOADVANCE,1);
}


void create_tables()
{
    int i;

    //probe_tab
    for (i=0; i<256; i++)
        MP_probe_tab[i] = FALSE;

    const unsigned char *true_chars = (const unsigned char *)"atgucnATGUCN";
    for (i = 0; true_chars[i]; ++i) {
        MP_probe_tab[true_chars[i]] = TRUE;
    }
}

AW_window *MP_main(AW_root *root, AW_default def)
{
    if (mp_main)
    {
        mp_main->get_mp_window()->get_window()->show();
        return (AW_window *)mp_main->get_mp_window()->get_window();
    }

    create_tables();

    mp_main = new MP_Main(root, (AWT_canvas *)def);

    mp_main->get_mp_window()->get_window()->show();

    //    alex_main();  //normal erst spaeter aufrufen !!!

    return (AW_window *)mp_main->get_mp_window()->get_window();
}



