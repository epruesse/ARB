#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include <awt_csp.hxx>
#include "ap_csp_2_gnuplot.hxx"

extern GBDATA *gb_main;


// ------------------------------------------------------------------------------------------------------------------------------
//      static GB_ERROR split_stat_filename(const char *fname, char **dirPtr, char **name_prefixPtr, char **name_postfixPtr)
// ------------------------------------------------------------------------------------------------------------------------------
static GB_ERROR split_stat_filename(const char *fname, char **dirPtr, char **name_prefixPtr, char **name_postfixPtr) {
    // 'fname' is sth like 'directory/prefix.sth_gnu'
    // 'dirPtr' is set to a malloc-copy of 'directory'
    // 'name_prefixPtr' is set to a malloc-copy of 'prefix' (defaults to '*')
    // 'name_postfixPtr' is set to a malloc-copy of 'sth_gnu' (defaults to '*_gnu')

    *dirPtr          = 0;
    *name_prefixPtr  = 0;
    *name_postfixPtr = 0;

    const char *lslash = strrchr(fname, '/');
    if (!lslash) return GB_export_error("'%s' has to contain a '/'", fname);

    char *dir         = strdup(fname);
    dir[lslash-fname] = 0; // cut off at last '/'

    char *name_prefix  = GB_strdup(lslash+1);
    char *name_postfix = 0;
    char *ldot         = strrchr(name_prefix, '.');
    if (ldot) {
        ldot[0]      = 0;
        name_postfix = GB_strdup(ldot+1);
    }
    if (!ldot || name_prefix[0] == 0) { // no dot or empty name_prefix
        free(name_prefix);
        name_prefix = GB_strdup("*");
    }
    if (!name_postfix || name_postfix[0] == 0) {
        free(name_postfix);
        name_postfix = GB_strdup("*_gnu");
    }

    assert(name_prefix);
    assert(name_postfix);

    *dirPtr          = dir;
    *name_prefixPtr  = name_prefix;
    *name_postfixPtr = name_postfix;

    return 0;
}
// -------------------------------------------------------------------------------------------
//      static char * get_overlay_files(AW_root *awr, const char *fname, GB_ERROR& error)
// -------------------------------------------------------------------------------------------
static char * get_overlay_files(AW_root *awr, const char *fname, GB_ERROR& error) {
    assert(!error);

    bool overlay_prefix  = awr->awar(AP_AWAR_CSP_GNUPLOT_OVERLAY_PREFIX)->read_int();
    bool overlay_postfix = awr->awar(AP_AWAR_CSP_GNUPLOT_OVERLAY_POSTFIX)->read_int();

    char *dir, *name_prefix, *name_postfix;
    error = split_stat_filename(fname, &dir, &name_prefix, &name_postfix);

    char *found_files = 0;
    if (!error) {
        char *mask               = GB_strdup(GBS_global_string("%s.*_gnu", name_prefix));
        char *found_prefix_files = overlay_prefix ? GB_find_all_files(dir, mask, GB_FALSE) : 0;
        free(mask);

        mask                      = GB_strdup(GBS_global_string("*.%s", name_postfix));
        char *found_postfix_files = overlay_postfix ? GB_find_all_files(dir, mask, GB_FALSE) : 0;
        free(mask);

        if (found_prefix_files) {
            if (found_postfix_files) {
                found_files = GB_strdup(GBS_global_string("%s*%s", found_prefix_files, found_postfix_files));
            }
            else { // only found_prefix_files
                found_files        = found_prefix_files;
                found_prefix_files = 0;
            }
        }
        else {
            found_files         = found_postfix_files;
            found_postfix_files = 0;
        }

        free(found_postfix_files);
        free(found_prefix_files);
    }

    free(name_postfix);
    free(name_prefix);
    free(dir);

    return found_files;
}

// ------------------------------------------------------------------------------
//      void AP_csp_2_gnuplot_cb(AW_window *aww, AW_CL cspcd, AW_CL cl_mode)
// ------------------------------------------------------------------------------
void AP_csp_2_gnuplot_cb(AW_window *aww, AW_CL cspcd, AW_CL cl_mode) {
    // cl_mode = 0 -> write file
    // cl_mode = 1 -> write file and run gnuplot
    // cl_mode = 2 -> delete all files with same prefix

	GB_transaction  dummy(gb_main);
	AWT_csp        *csp   = (AWT_csp *)cspcd;
	GB_ERROR        error = 0;
	AP_filter       filter;
    int             mode  = int(cl_mode);

    if (mode != 2) {
        char *filterstring = aww->get_root()->awar(AP_AWAR_FILTER_FILTER)->read_string();
        char *alignment_name =  aww->get_root()->awar(AP_AWAR_FILTER_ALIGNMENT)->read_string();
        long alignment_length = GBT_get_alignment_len(gb_main,alignment_name);
        error = filter.init(filterstring,"0",alignment_length);
        delete alignment_name;
        delete filterstring;

        if (!error) error = csp->go(&filter);

        if (!error && !csp->seq_len) error = "Please select column statistic";
    }

    if (!error) {
        char *fname = aww->get_root()->awar(AP_AWAR_CSP_FILENAME)->read_string();
        if (!strchr(fname, '/')) {
            char *neu = GB_strdup(GBS_global_string("./%s", fname));
            free(fname);
            fname     = neu;
        }

        if (strlen(fname) < 1) {
            error = "Please enter file name";
        }

        if (mode == 2) {        // delete overlay files
            if (!error) {
                char *found_files = get_overlay_files(aww->get_root(), fname, error);

                if (found_files) {
                    for (char *name = strtok(found_files, "*"); name; name = strtok(0, "*")) {
                        printf("Deleting gnuplot file '%s'\n", name);
                        if (unlink(name) != 0) printf("Can't delete '%s'\n", name);
                    }
                    free(found_files);
                    aww->get_root()->awar(AP_AWAR_CSP_DIRECTORY)->touch(); // reload file selection box
                }
            }
        }
        else {
            FILE *out = 0;
            if (!error) {
                out = fopen(fname,"w");
                if (!out) error = GB_export_error("Cannot write to file '%s'",fname);
            }

            assert(out || error);

            if (!error) {
                char         *type     = aww->get_root()->awar(AP_AWAR_CSP_SUFFIX)->read_string();
                long          smooth   = aww->get_root()->awar(AP_AWAR_CSP_SMOOTH)->read_int()+1;
                double        val;
                double        smoothed = 0;
                unsigned int  j;
                float        *f[4];

                if (type[0] == 'f') { // sort frequencies
                    int wf;
                    int c = 0;

                    for (wf = 0; wf <256 && c <4; wf++) {
                        if (csp->frequency[wf]) f[c++] = csp->frequency[wf];
                    }
                    int k,l;
                    for (k=3;k>0;k--) {
                        for (l=0;l<k;l++) {
                            for (j=0;j<csp->seq_len; j++) {
                                if (f[l][j] > f[l+1][j]) {
                                    float h;
                                    h = f[l][j];
                                    f[l][j] = f[l+1][j];
                                    f[l+1][j] = h;
                                }
                            }
                        }
                    }

                }

                for (j=0;j<csp->seq_len; j++) {
                    if (!csp->weights[j]) continue;
                    fprintf(out,"%i ",j);
                    if (!strcmp(type,"all")){
                        fprintf(out," %f %f %f %f\n",
                                csp->frequency['A'][j], csp->frequency['C'][j],
                                csp->frequency['G'][j], csp->frequency['U'][j]);
                        continue;
                    }
                    if (!strcmp(type,"gc_gnu")) {
                        val = ( csp->frequency['G'][j] + csp->frequency['C'][j] ) /
                            (double)( csp->frequency['A'][j] + csp->frequency['C'][j] +
                                      csp->frequency['G'][j] + csp->frequency['U'][j] );
                    }else if (!strcmp(type,"ga_gnu")) {
                        val = ( csp->frequency['G'][j] + csp->frequency['A'][j] ) /
                            (double)( csp->frequency['A'][j] + csp->frequency['C'][j] +
                                      csp->frequency['G'][j] + csp->frequency['U'][j] );
                    }else if (!strcmp(type,"rate_gnu")) {
                        val = csp->rates[j];
                    }else if (!strcmp(type,"tt_gnu")) {
                        val = csp->ttratio[j];
                    }else if (!strcmp(type,"f1_gnu")) {
                        val = f[3][j];
                    }else if (!strcmp(type,"f2_gnu")) {
                        val = f[2][j];
                    }else if (!strcmp(type,"f3_gnu")) {
                        val = f[1][j];
                    }else if (!strcmp(type,"f4_gnu")) {
                        val = f[0][j];
                    }else if (!strcmp(type,"helix_gnu")) {
                        val = csp->is_helix[j];
                    }else{
                        val = 0;
                    }
                    smoothed = val/smooth + smoothed *(smooth-1)/(smooth);
                    fprintf(out,"%f\n",smoothed);
                }
                delete type;
                fclose(out);
                aww->get_root()->awar(AP_AWAR_CSP_DIRECTORY)->touch(); // reload file selection box

                if (mode == 1) { // run gnuplot ?
                    char command_file[100];
#if defined(DEBUG)
                    int  printed = sprintf(command_file,"/tmp/arb_gnuplot_commands_%s_%i",GB_getenv("USER"), getpid());
                    assert(printed<100);
#endif // DEBUG

                    char *smooth  = aww->get_root()->awar(AP_AWAR_CSP_SMOOTH_GNUPLOT)->read_string();
                    FILE *out = fopen(command_file, "wt");
                    if (!out) {
                        error = GB_export_error("Can't create gnuplot command file '%s'", command_file);
                    }
                    else {
                        bool plotted = false; // set to true after first 'plot' command (other plots use 'replot')
                        const char *plot_command[] = { "plot", "replot" };
                        char *found_files = get_overlay_files(aww->get_root(), fname, error);

                        fprintf(out, "set samples 1000\n");

                        if (found_files) {
                            for (char *name = strtok(found_files, "*"); name; name = strtok(0, "*")) {
                                if (strcmp(name, fname) != 0) { // latest data file is done below
                                    fprintf(out, "%s \"%s\" %s\n", plot_command[int(plotted)], name, smooth);
                                    plotted = true;
                                }
                            }
                            free(found_files);
                        }

                        fprintf(out, "%s \"%s\" %s\n", plot_command[int(plotted)], fname, smooth);
                        fprintf(out, "pause -1 \"Press RETURN to close gnuplot\"\n");
                        fclose(out);

                        if (mode == 1) {
                            char *script = GB_strdup(GBS_global_string("gnuplot %s;rm -f %s", command_file, command_file));
                            GB_xcmd(script, true, true);

                            free(script);
                        }
                        else {
                            assert(mode == 2);
                            unlink(command_file);
                        }
                    }
                    free(smooth);
                }
            }
        }
        free(fname);
    }

 	if (error) aw_message(error);
}

AW_window *AP_open_csp_2_gnuplot_window( AW_root *root ){

	GB_transaction dummy(gb_main);
	AWT_csp *csp = new AWT_csp(gb_main,root,AP_AWAR_CSP_NAME);
	AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "EXPORT_CSP_TO_GNUPLOT", "Export Column statistic to GnuPlot", 10,10);
  	aws->load_xfig("cpro/csp_2_gnuplot.fig");

	root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", gb_main);

	root->awar_int(AP_AWAR_CSP_SMOOTH);
	root->awar_int(AP_AWAR_CSP_GNUPLOT_OVERLAY_POSTFIX);
	root->awar_int(AP_AWAR_CSP_GNUPLOT_OVERLAY_PREFIX);
	root->awar_string(AP_AWAR_CSP_SMOOTH_GNUPLOT);

	root->awar_string(AP_AWAR_CSP_NAME);
	root->awar_string(AP_AWAR_CSP_ALIGNMENT);
	root->awar(AP_AWAR_CSP_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);	// csp of the correct al.

	root->awar_string(AP_AWAR_FILTER_NAME);
	root->awar_string(AP_AWAR_FILTER_FILTER);
	root->awar_string(AP_AWAR_FILTER_ALIGNMENT);
	root->awar(AP_AWAR_FILTER_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);	// csp of the correct al.

	root->awar_string(AP_AWAR_CSP_SUFFIX, "gc_gnu", AW_ROOT_DEFAULT);
	root->awar_string(AP_AWAR_CSP_DIRECTORY);
	root->awar_string(AP_AWAR_CSP_FILENAME, "noname.gc_gnu", AW_ROOT_DEFAULT);

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"csp_2_gnuplot.hlp");
	aws->create_button("HELP","HELP","H");

	awt_create_selection_box(aws,AP_AWAR_CSP);

	aws->at("csp");
	create_selection_list_on_csp(aws,csp);

	aws->at("what");
	AW_selection_list* selid = aws->create_selection_list(AP_AWAR_CSP_SUFFIX);
    aws->insert_selection(selid, "G+C Ratio",	"gc_gnu");
    aws->insert_selection(selid, "G+A Ratio",	"ga_gnu");
    aws->insert_selection(selid, "Rate",		"rate_gnu");
    aws->insert_selection(selid, "TT Ratio",	"tt_gnu");
    aws->insert_selection(selid, "Most Frequent Base","f1_gnu");
    aws->insert_selection(selid, "Second Frequent Base","f2_gnu");
    aws->insert_selection(selid, "Third Frequent Base","f3_gnu");
    aws->insert_selection(selid, "Least Frequent Base","f4_gnu");
    aws->insert_selection(selid, "All Frequencies","all");
    aws->insert_selection(selid, "Helix",		"helix_gnu");
	aws->update_selection_list(selid);

	AW_CL filter = awt_create_select_filter(root,gb_main,AP_AWAR_FILTER_NAME);
	aws->at("ap_filter");
	aws->callback(AW_POPUP,(AW_CL)awt_create_select_filter_win,filter);
	aws->create_button("SELECT_FILTER", AP_AWAR_FILTER_NAME);

	aws->at("smooth");
	aws->create_option_menu(AP_AWAR_CSP_SMOOTH);
    aws->insert_option("Dont Smooth","D",0);
    aws->insert_option("Smooth 1","1",1);
    aws->insert_option("Smooth 2","2",2);
    aws->insert_option("Smooth 3","3",3);
    aws->insert_option("Smooth 5","5",5);
    aws->insert_option("Smooth 10","0",10);
	aws->update_option_menu();

	aws->at("smooth2");
    aws->create_toggle_field(AP_AWAR_CSP_SMOOTH_GNUPLOT, 1);
    aws->insert_default_toggle("None","N", "");
    aws->insert_toggle("Unique","U", "smooth unique");
    aws->insert_toggle("CSpline","S", "smooth cspline");
    aws->insert_toggle("Bezier","B", "smooth bezier");
    aws->update_toggle_field();

    aws->auto_space(10, 10);
    aws->button_length(13);

	aws->at("save");
	aws->callback(AP_csp_2_gnuplot_cb,(AW_CL)csp, (AW_CL)0);
	aws->create_button("SAVE","Save");

	aws->highlight();
	aws->callback(AP_csp_2_gnuplot_cb,(AW_CL)csp, (AW_CL)1);
	aws->create_button("SAVE_AND_VIEW","Save & View");

    aws->at("overlay1");
    aws->label("Overlay statistics with same prefix?");
    aws->create_toggle(AP_AWAR_CSP_GNUPLOT_OVERLAY_PREFIX);

    aws->at("overlay2");
    aws->label("Overlay statistics with same postfix?");
    aws->create_toggle(AP_AWAR_CSP_GNUPLOT_OVERLAY_POSTFIX);

    aws->at("del_overlays");
    aws->callback(AP_csp_2_gnuplot_cb, (AW_CL)csp, (AW_CL)2);
    aws->create_autosize_button("DEL_OVERLAYS", "Delete currently overlayed files", "D", 2);

	return (AW_window *)aws;
}
