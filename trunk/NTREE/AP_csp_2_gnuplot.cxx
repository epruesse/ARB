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
#include <awt_csp.hxx>
#include "ap_csp_2_gnuplot.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

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

    nt_assert(name_prefix);
    nt_assert(name_postfix);

    *dirPtr          = dir;
    *name_prefixPtr  = name_prefix;
    *name_postfixPtr = name_postfix;

    return 0;
}
// -------------------------------------------------------------------------------------------
//      static char * get_overlay_files(AW_root *awr, const char *fname, GB_ERROR& error)
// -------------------------------------------------------------------------------------------
static char * get_overlay_files(AW_root *awr, const char *fname, GB_ERROR& error) {
    nt_assert(!error);

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

enum PlotType {
    PT_GC_RATIO,
    PT_GA_RATIO,
    PT_RATE,
    PT_TT_RATIO,
    PT_MOST_FREQUENT_BASE,
    PT_SECOND_FREQUENT_BASE,
    PT_THIRD_FREQUENT_BASE,
    PT_LEAST_FREQUENT_BASE,
    PT_BASE_A,
    PT_BASE_C,
    PT_BASE_G,
    PT_BASE_TU,
    PT_HELIX,

    PT_PLOT_TYPES,
    PT_UNKNOWN
};

static const char *plotTypeName[PT_PLOT_TYPES] = {
    "gc_gnu",
    "ga_gnu",
    "rate_gnu",
    "tt_gnu",
    "f1_gnu",
    "f2_gnu",
    "f3_gnu",
    "f4_gnu",
    "a_gnu",
    "c_gnu",
    "g_gnu",
    "tu_gnu",
    "helix_gnu"
};

static const char *plotTypeDescription[PT_PLOT_TYPES] = {
    "G+C ratio",
    "G+A ratio",
    "Rate",
    "TT Ratio",
    "Most frequent base",
    "2nd frequent base",
    "3rd frequent base",
    "Least frequent base",
    "A ratio",
    "C ratio",
    "G ratio",
    "T/U ratio",
    "Helix"
};

static PlotType string2PlotType(const char *type) {
    for (int pt = 0; pt<PT_PLOT_TYPES; ++pt) {
        if (strcmp(type, plotTypeName[pt]) == 0) {
            return PlotType(pt);
        }
    }

    return PT_UNKNOWN;
}

static const char *makeTitle(const char *fname) {
    const char *rslash = strrchr(fname, '/');
    if (rslash) rslash++;
    else        rslash = fname;

    char *name = strdup(rslash);
    char *rdot = strrchr(name, '.');

    PlotType pt  = PT_UNKNOWN;
    if (rdot) pt = string2PlotType(rdot+1);

    static char *title  = 0;
    free(title); title = 0;

    if (pt == PT_UNKNOWN) {
        title = GBS_global_string_copy("%s (unknown type)", name);
    }
    else {
        rdot[0] = 0;
        title = GBS_global_string_copy("%s (%s)", name, plotTypeDescription[pt]);
    }

    free(name);

    return title;
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
        char *filterstring     = aww->get_root()->awar(AP_AWAR_FILTER_FILTER)->read_string();
        char *alignment_name   = aww->get_root()->awar(AP_AWAR_FILTER_ALIGNMENT)->read_string();
        long  alignment_length = GBT_get_alignment_len(gb_main,alignment_name);
        error                  = filter.init(filterstring,"0",alignment_length);
        free(alignment_name);
        free(filterstring);

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

            nt_assert(out || error);

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

                PlotType plot_type = string2PlotType(type);
                nt_assert(plot_type != PT_UNKNOWN); // 'type' is not a PlotType

                for (j=0;j<csp->seq_len; j++) {
                    if (!csp->weights[j]) continue;
                    fprintf(out,"%i ",j);

                    //                     if (plot_type == PT_ALL_FREQUENCIES) {
                    //                         fprintf(out," %f %f %f %f\n",
                    //                                 csp->frequency['A'][j], csp->frequency['C'][j],
                    //                                 csp->frequency['G'][j], csp->frequency['U'][j]);
                    //                     }
                    //                     else {

                    double amount =
                        csp->frequency[(unsigned char)'A'][j] + csp->frequency[(unsigned char)'C'][j] +
                        csp->frequency[(unsigned char)'G'][j] + csp->frequency[(unsigned char)'U'][j] ;

                    switch (plot_type) {
                        case PT_GC_RATIO:
                            val = ( csp->frequency[(unsigned char)'G'][j] + csp->frequency[(unsigned char)'C'][j] ) / amount;
                            break;
                        case PT_GA_RATIO:
                            val = ( csp->frequency[(unsigned char)'G'][j] + csp->frequency[(unsigned char)'A'][j] ) / amount;
                            break;

                        case PT_BASE_A: val  = csp->frequency[(unsigned char)'A'][j] / amount; break;
                        case PT_BASE_C: val  = csp->frequency[(unsigned char)'C'][j] / amount; break;
                        case PT_BASE_G: val  = csp->frequency[(unsigned char)'G'][j] / amount; break;
                        case PT_BASE_TU: val = csp->frequency[(unsigned char)'U'][j] / amount; break;

                        case PT_RATE:                   val = csp->rates[j]; break;
                        case PT_TT_RATIO:               val = csp->ttratio[j]; break;
                        case PT_HELIX:                  val = csp->is_helix[j]; break;

                        case PT_MOST_FREQUENT_BASE:     val = f[3][j]; break;
                        case PT_SECOND_FREQUENT_BASE:   val = f[2][j]; break;
                        case PT_THIRD_FREQUENT_BASE:    val = f[1][j]; break;
                        case PT_LEAST_FREQUENT_BASE:    val = f[0][j]; break;
                        default:  {
                            nt_assert(0); // unknown calculation requested..
                            val = 0;
                        }
                    }
                    smoothed = val/smooth + smoothed *(smooth-1)/(smooth);
                    fprintf(out,"%f\n",smoothed);
                    //                     }
                }

                free(type);
                fclose(out);
                aww->get_root()->awar(AP_AWAR_CSP_DIRECTORY)->touch(); // reload file selection box

                if (mode == 1) { // run gnuplot ?
                    char command_file[100];
#if defined(ASSERTION_USED)
                    int  printed =
#endif // ASSERTION_USED
                        sprintf(command_file,"/tmp/arb_gnuplot_commands_%s_%i",GB_getenv("USER"), getpid());
                    nt_assert(printed<100);

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
                                    fprintf(out, "%s \"%s\" %s title \"%s\"\n", plot_command[int(plotted)], name, smooth, makeTitle(name));
                                    plotted = true;
                                }
                            }
                            free(found_files);
                        }

                        fprintf(out, "%s \"%s\" %s title \"%s\"\n", plot_command[int(plotted)], fname, smooth, makeTitle(fname));
                        fprintf(out, "pause -1 \"Press RETURN to close gnuplot\"\n");
                        fclose(out);

                        if (mode == 1) {
                            printf("command_file='%s'\n", command_file);
                            char *script = GB_strdup(GBS_global_string("gnuplot %s && rm -f %s", command_file, command_file));
                            GB_xcmd(script, true, true);
                            free(script);
                        }
                        else {
                            nt_assert(mode == 2);
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
    aws->init( root, "EXPORT_CSP_TO_GNUPLOT", "Export Column statistic to GnuPlot");
    aws->load_xfig("cpro/csp_2_gnuplot.fig");

    root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", gb_main);

    root->awar_int(AP_AWAR_CSP_SMOOTH);
    root->awar_int(AP_AWAR_CSP_GNUPLOT_OVERLAY_POSTFIX);
    root->awar_int(AP_AWAR_CSP_GNUPLOT_OVERLAY_PREFIX);
    root->awar_string(AP_AWAR_CSP_SMOOTH_GNUPLOT);

    root->awar_string(AP_AWAR_CSP_NAME);
    root->awar_string(AP_AWAR_CSP_ALIGNMENT);
    root->awar(AP_AWAR_CSP_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT); // csp of the correct al.

    root->awar_string(AP_AWAR_FILTER_NAME);
    root->awar_string(AP_AWAR_FILTER_FILTER);
    root->awar_string(AP_AWAR_FILTER_ALIGNMENT);
    root->awar(AP_AWAR_FILTER_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);  // csp of the correct al.

    aw_create_selection_box_awars(root, AP_AWAR_CSP, "", ".gc_gnu", "noname.gc_gnu");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"csp_2_gnuplot.hlp");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box(aws,AP_AWAR_CSP);

    aws->at("csp");
    create_selection_list_on_csp(aws,csp);

    aws->at("what");
    AW_selection_list* selid = aws->create_selection_list(AP_AWAR_CSP_SUFFIX);
    for (int pt = 0; pt<PT_PLOT_TYPES; ++pt) {
        aws->insert_selection(selid, plotTypeDescription[pt], plotTypeName[pt]);
    }
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
