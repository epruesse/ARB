#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include <awt_canvas.hxx>
#include <awt_csp.hxx>
#include "ap_conservProfile2Gnuplot.hxx"
//#include "nt_cb.hxx"

extern GBDATA *gb_main;

void AP_conservProfile2Gnuplot_callback(AW_window *aww) {

    GB_ERROR error = 0;
    char    *fname = aww->get_root()->awar(AP_AWAR_CONSPRO_FILENAME)->read_string();

//     if (!strchr(fname, '/')) {
//         char *neu = GB_strdup(GBS_global_string("./%s", fname));
//         free(fname);
//         fname     = neu;
//     }

//     if (strlen(fname) < 1) {
//         error = "Please enter file name";
//     }

//     FILE *out = 0;
//     if (!error) {
//         out = fopen(fname,"w");
//         if (!out) error = GB_export_error("Cannot write to file '%s'",fname);
//     }

//     nt_assert(out || error);

//     fclose(out);
//     aww->get_root()->awar(AP_AWAR_CONSPRO_DIRECTORY)->touch(); // reload file selection box

    char *smooth  = aww->get_root()->awar(AP_AWAR_CONSPRO_SMOOTH_GNUPLOT)->read_string();
    char *legend  = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_LEGEND)->read_string();
    int   dispPos = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS)->read_int();
    int   minX    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MIN_X)->read_int();
    int   maxX    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MAX_X)->read_int();
    int   minY    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MIN_Y)->read_int();
    int   maxY    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MAX_Y)->read_int();

    char command_file[100];
    sprintf(command_file,"/tmp/arb_consProf_gnuplot_commands_%s_%i",GB_getenv("USER"), getpid());

    FILE *cmdFile = fopen(command_file, "w");
    if (!cmdFile){
        error = GB_export_error("Can't create GNUPLOT COMMAND FILE '%s'", command_file);
    }

    if(!error){
        if (minX>0 || maxX>0)   fprintf(cmdFile, "set xrange [%i:%i]\n",minX,maxX);
        if (minY>0 || maxY>0)   fprintf(cmdFile, "set yrange [%i:%i]\n",minY,maxY);

        fprintf(cmdFile, "plot \"%s\" %s title \"%s\"\n", fname, smooth, legend);

        if(dispPos)  fprintf(cmdFile, "replot \"%s\" title \"Base Positions\"\n", fname);

        fprintf(cmdFile, "pause -1 \"Press RETURN to close gnuplot\"\n");
    }
    fclose(cmdFile);

    printf("command_file='%s'\n", command_file);
    char *script = GB_strdup(GBS_global_string("gnuplot %s && rm -f %s", command_file, command_file));
    GB_xcmd(script, true, true);          // execute GNUPLOT using command_file

    free(script);
    free(smooth);
    free(fname);

    if (error)   aw_message(error);
}


AW_window *AP_openConservationPorfileWindow( AW_root *root ){

    AW_window_simple *aws = new AW_window_simple;

    aws->init( root, "DISP_CONSERV_PROFILE_GNUPLOT", "Conservation Profile Using Base Frequency Filter");
    aws->load_xfig("conservProfile2Gnuplot.fig");

    root->awar_string(AP_AWAR_CONSPRO_SMOOTH_GNUPLOT);
    root->awar_string(AP_AWAR_BASE_FREQ_FILTER_NAME);

    aw_create_selection_box_awars(root, AP_AWAR_CONSPRO, "", ".gnu", "noname.gnu");

    root->awar_string(AP_AWAR_CONSPRO_GNUPLOT_LEGEND);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MIN_X);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MAX_X);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MIN_Y);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MAX_Y);

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"coservProfile2Gnuplot.hlp");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box(aws,AP_AWAR_CONSPRO);

    aws->at("baseFreqFlt");// aws->callback((AW_CB0)AW_POPDOWN);
    //    aws->callback(createTipsAndTricks_window);
    //    aws->callback(AW_POPUP,(AW_CL)NT_system_cb,(AW_CL)"arb_phylo &");
    aws->create_button("SELECT_FILTER", AP_AWAR_BASE_FREQ_FILTER_NAME);

    aws->at("minX");
    aws->create_input_field(AP_AWAR_CONSPRO_GNUPLOT_MIN_X);

    aws->at("maxX");
    aws->create_input_field(AP_AWAR_CONSPRO_GNUPLOT_MAX_X);

    aws->at("minY");
    aws->create_input_field(AP_AWAR_CONSPRO_GNUPLOT_MIN_Y);

    aws->at("maxY");
    aws->create_input_field(AP_AWAR_CONSPRO_GNUPLOT_MAX_Y);

    aws->at("legend");
    aws->create_input_field(AP_AWAR_CONSPRO_GNUPLOT_LEGEND);

    aws->at("smooth");
    aws->create_toggle_field(AP_AWAR_CONSPRO_SMOOTH_GNUPLOT, 1);
    aws->insert_default_toggle("None","N", "");
    aws->insert_toggle("Unique","U", "smooth unique");
    //    aws->insert_toggle("CSpline","S", "smooth cspline");
    aws->insert_toggle("Bezier","B", "smooth bezier");
    aws->update_toggle_field();

    aws->at("dispPos");
    aws->create_toggle(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS);

    aws->at("dispProfile");
    aws->callback(AP_conservProfile2Gnuplot_callback);
    aws->create_button("DISPLAY_PROFILE","SAVE & DISPLAY CONSERVATION PROFILE");

    return (AW_window *)aws;
}

