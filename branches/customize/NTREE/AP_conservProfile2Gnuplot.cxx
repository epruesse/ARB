// =============================================================== //
//                                                                 //
//   File      : AP_conservProfile2Gnuplot.cxx                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "AP_conservProfile2Gnuplot.h"

#include <aw_window.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>

static void AP_conservProfile2Gnuplot_callback(AW_window *aww) {
    GB_ERROR  error   = 0;
    char     *command_file;
    char     *cmdName = GB_unique_filename("arb", "gnuplot");
    FILE     *cmdFile = GB_fopen_tempfile(cmdName, "wt", &command_file);

    if (!cmdFile) error = GB_await_error();
    else {
        char *fname   = aww->get_root()->awar(AP_AWAR_CONSPRO_FILENAME)->read_string();
        char *smooth  = aww->get_root()->awar(AP_AWAR_CONSPRO_SMOOTH_GNUPLOT)->read_string();
        char *legend  = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_LEGEND)->read_string();
        int   dispPos = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS)->read_int();
        int   minX    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MIN_X)->read_int();
        int   maxX    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MAX_X)->read_int();
        int   minY    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MIN_Y)->read_int();
        int   maxY    = aww->get_root()->awar(AP_AWAR_CONSPRO_GNUPLOT_MAX_Y)->read_int();

        if (minX>0 || maxX>0)   fprintf(cmdFile, "set xrange [%i:%i]\n", minX, maxX);
        if (minY>0 || maxY>0)   fprintf(cmdFile, "set yrange [%i:%i]\n", minY, maxY);

        fprintf(cmdFile, "plot \"%s\" %s title \"%s\"\n", fname, smooth, legend);

        if (dispPos) fprintf(cmdFile, "replot \"%s\" title \"Base Positions\"\n", fname);

        fprintf(cmdFile, "pause -1 \"Press RETURN to close gnuplot\"\n");

        free(legend);
        free(smooth);
        free(fname);

        fclose(cmdFile);
    }

    if (!error) {
        char *script = GBS_global_string_copy("gnuplot %s && rm -f %s", command_file, command_file);
        GB_xcmd(script, true, true);          // execute GNUPLOT using command_file
        free(script);
    }
    free(command_file);
    free(cmdName);

    if (error) aw_message(error);
}


static AW_window *AP_createConservationProfileWindow(AW_root *root) { // @@@ unused! test and add to menu if useful 

    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "DISP_CONSERV_PROFILE_GNUPLOT", "Conservation Profile Using Base Frequency Filter");
    aws->load_xfig("conservProfile2Gnuplot.fig");

    root->awar_string(AP_AWAR_CONSPRO_SMOOTH_GNUPLOT);
    root->awar_string(AP_AWAR_BASE_FREQ_FILTER_NAME);

    AW_create_fileselection_awars(root, AP_AWAR_CONSPRO, "", ".gnu", "noname.gnu");

    root->awar_string(AP_AWAR_CONSPRO_GNUPLOT_LEGEND);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MIN_X);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MAX_X);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MIN_Y);
    root->awar_int(AP_AWAR_CONSPRO_GNUPLOT_MAX_Y);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("conservProfile2Gnuplot.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_standard_fileselection(aws, AP_AWAR_CONSPRO);

    aws->at("baseFreqFlt");
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
    aws->insert_default_toggle("None", "N", "");
    aws->insert_toggle("Unique", "U", "smooth unique");
    aws->insert_toggle("Bezier", "B", "smooth bezier");
    aws->update_toggle_field();

    aws->at("dispPos");
    aws->create_toggle(AP_AWAR_CONSPRO_GNUPLOT_DISP_POS);

    aws->at("dispProfile");
    aws->callback(AP_conservProfile2Gnuplot_callback);
    aws->create_button("DISPLAY_PROFILE", "SAVE & DISPLAY CONSERVATION PROFILE");

    return aws;
}

