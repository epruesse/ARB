// =============================================================== //
//                                                                 //
//   File      : ColumnStat_2_gnuplot.cxx                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <ColumnStat.hxx>
#include <AP_filter.hxx>
#include <awt_filter.hxx>
#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_select.hxx>
#include <arbdbt.h>
#include <arb_file.h>

#include <unistd.h>

#define AWAR_CS2GP "tmp/ntree/colstat_2_gnuplot"

#define AWAR_CS2GP_NAME      AWAR_CS2GP "/name"
#define AWAR_CS2GP_SUFFIX    AWAR_CS2GP "/filter"
#define AWAR_CS2GP_DIRECTORY AWAR_CS2GP "/directory"
#define AWAR_CS2GP_FILENAME  AWAR_CS2GP "/file_name"

#define AWAR_CS2GP_SMOOTH_VALUES           AWAR_CS2GP "/smooth_values"
#define AWAR_CS2GP_SMOOTH_GNUPLOT          AWAR_CS2GP "/smooth_gnuplot"
#define AWAR_CS2GP_GNUPLOT_OVERLAY_PREFIX  AWAR_CS2GP "/gnuplot_overlay_prefix"
#define AWAR_CS2GP_GNUPLOT_OVERLAY_POSTFIX AWAR_CS2GP "/gnuplot_overlay_postfix"
#define AWAR_CS2GP_FILTER_NAME             AWAR_CS2GP "/ap_filter/name"

static GB_ERROR split_stat_filename(const char *fname, char **dirPtr, char **name_prefixPtr, char **name_postfixPtr) {
    // 'fname' is sth like 'directory/prefix.sth_gnu'
    // 'dirPtr' is set to a malloc-copy of 'directory'
    // 'name_prefixPtr' is set to a malloc-copy of 'prefix' (defaults to '*')
    // 'name_postfixPtr' is set to a malloc-copy of 'sth_gnu' (defaults to '*_gnu')

    *dirPtr          = 0;
    *name_prefixPtr  = 0;
    *name_postfixPtr = 0;

    const char *lslash = strrchr(fname, '/');
    if (!lslash) return GB_export_errorf("'%s' has to contain a '/'", fname);

    char *dir         = strdup(fname);
    dir[lslash-fname] = 0; // cut off at last '/'

    char *name_prefix  = strdup(lslash+1);
    char *name_postfix = 0;
    char *ldot         = strrchr(name_prefix, '.');

    if (ldot) {
        ldot[0]      = 0;
        name_postfix = strdup(ldot+1);
    }
    if (!ldot || name_prefix[0] == 0) freedup(name_prefix, "*"); // no dot or empty name_prefix
    if (!name_postfix || name_postfix[0] == 0) freedup(name_postfix, "*_gnu");

    nt_assert(name_prefix);
    nt_assert(name_postfix);

    *dirPtr          = dir;
    *name_prefixPtr  = name_prefix;
    *name_postfixPtr = name_postfix;

    return 0;
}

static char * get_overlay_files(AW_root *awr, const char *fname, GB_ERROR& error) {
    nt_assert(!error);

    bool overlay_prefix  = awr->awar(AWAR_CS2GP_GNUPLOT_OVERLAY_PREFIX)->read_int();
    bool overlay_postfix = awr->awar(AWAR_CS2GP_GNUPLOT_OVERLAY_POSTFIX)->read_int();

    char *dir, *name_prefix, *name_postfix;
    error = split_stat_filename(fname, &dir, &name_prefix, &name_postfix);

    char *found_files = 0;
    if (!error) {
        char *found_prefix_files  = 0;
        char *found_postfix_files = 0;

        if (overlay_prefix || overlay_postfix) {
            char *mask = GBS_global_string_copy("%s.*_gnu", name_prefix);
            if (overlay_prefix) {
#if defined(WARN_TODO)
#warning change error handling for GB_find_all_files() - globally!
#endif
                found_prefix_files             = GB_find_all_files(dir, mask, false);
                if (!found_prefix_files) error = GB_get_error();
            }
            free(mask);

            if (!error) {
                mask = GBS_global_string_copy("*.%s", name_postfix);
                if (overlay_postfix) {
                    found_postfix_files             = GB_find_all_files(dir, mask, false);
                    if (!found_postfix_files) error = GB_get_error();
                }
                free(mask);
            }
        }

        if (!error) {
            if (found_prefix_files) {
                if (found_postfix_files) {
                    found_files = GBS_global_string_copy("%s*%s", found_prefix_files, found_postfix_files);
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

    static char *title = 0;
    freenull(title);

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

// -------------------
//      SortedFreq

class SortedFreq : virtual Noncopyable {
    float *freq[4];

public:
    SortedFreq(const ColumnStat *column_stat);
    ~SortedFreq();

    float get(PlotType plot_type, size_t pos) const {
        float f;
        switch (plot_type) {
            case PT_MOST_FREQUENT_BASE:   f = freq[0][pos]; break;
            case PT_SECOND_FREQUENT_BASE: f = freq[1][pos]; break;
            case PT_THIRD_FREQUENT_BASE:  f = freq[2][pos]; break;
            case PT_LEAST_FREQUENT_BASE:  f = freq[3][pos]; break;
            default: nt_assert(0); f = 0; break;
        }
        return f;
    }
};

SortedFreq::SortedFreq(const ColumnStat *column_stat) {
    size_t len = column_stat->get_length();
    for (int i = 0; i<4; ++i) { // 4 best frequencies
        freq[i] = new float[len];
        for (size_t p = 0; p<len; ++p) freq[i][p] = 0.0; // clear
    }

    for (unsigned int c = 0; c<256; ++c) { // all character stats
        const float *cfreq = column_stat->get_frequencies((unsigned char)c);
        if (cfreq) {
            for (size_t p = 0; p<len; ++p) {            // all positions
                if (freq[3][p] < cfreq[p]) {
                    freq[3][p] = cfreq[p];          // found higher freq

                    for (int i = 3; i > 0; --i) { // bubble upwards to sort
                        if (freq[i-1][p] >= freq[i][p]) break; // sorted!

                        float f      = freq[i][p];
                        freq[i][p]   = freq[i-1][p];
                        freq[i-1][p] = f;
                    }
                }
            }
        }
    }

#if defined(DEBUG)
    for (size_t p = 0; p<len; ++p) {                // all positions
        nt_assert(freq[0][p] >= freq[1][p]);
        nt_assert(freq[1][p] >= freq[2][p]);
        nt_assert(freq[2][p] >= freq[3][p]);
    }
#endif // DEBUG
}
SortedFreq::~SortedFreq() {
    for (int i = 0; i<4; ++i) delete [] freq[i];
}

class Smoother : virtual Noncopyable {
    double *value;
    size_t *index;
    size_t  size;
    size_t  halfsize;
    size_t  next;
    size_t  added;
    double  sum;

    size_t wrap(size_t idx) { return idx >= size ? idx-size : idx; }

public:
    Smoother(size_t smooth_range)
        : size(smooth_range),
          halfsize(size/2), 
          next(0),
          added(0),
          sum(0.0)
    {
        nt_assert(size>0);
        
        value = new double[size];
        index = new size_t[size];

        for (size_t i = 0; i<size; ++i) value[i] = 0.0;
    }
    ~Smoother() {
        delete [] value;
        delete [] index;
    }

    void print(FILE *out, size_t i, double v) {
        sum = sum+v-value[next];

        index[next] = i;
        value[next] = v;

        next = wrap(next+1);

        if (added<size) ++added;
        if (added == size) {
            size_t printNext = wrap(next+halfsize);
            fprintf(out, "%zu %f\n", index[printNext], sum/size);
        }
    }
};

enum PlotMode {
    PLOT,          // write file
    PLOT_AND_VIEW, // write file and run gnuplot
    PLOT_CLEANUP,  // delete all files with same prefix
};

struct PlotParam {
    ColumnStat       *colstat;
    adfiltercbstruct *filterdef;

    PlotParam(ColumnStat *colstat_, adfiltercbstruct *filterdef_)
        : colstat(colstat_),
          filterdef(filterdef_)
    {}
};

static void colstat_2_gnuplot_cb(AW_window *aww, PlotParam *param, PlotMode mode) {
    GB_transaction  ta(GLOBAL.gb_main);
    GB_ERROR        error       = NULL;
    ColumnStat     *column_stat = param->colstat;
    AW_root        *awr         = aww->get_root();

    if (mode == PLOT || mode == PLOT_AND_VIEW) {
        char *filterstring     = awr->awar(param->filterdef->def_filter)->read_string();
        char *alignment_name   = awr->awar(param->filterdef->def_alignment)->read_string();
        long  alignment_length = GBT_get_alignment_len(GLOBAL.gb_main, alignment_name);

        if (alignment_length<0) {
            GB_clear_error();
            error = "Please select a valid alignment";
        }
        else {
            AP_filter filter(filterstring, "0", alignment_length);

            error = column_stat->calculate(&filter);

            if (!error && !column_stat->get_length()) error = "Please select column statistic";
        }

        free(alignment_name);
        free(filterstring);
    }

    if (!error) {
        char *fname = awr->awar(AWAR_CS2GP_FILENAME)->read_string();

        if (!strchr(fname, '/')) freeset(fname, GBS_global_string_copy("./%s", fname));
        if (strlen(fname) < 1) error = "Please enter file name";

        if (mode == PLOT_CLEANUP) { // delete overlay files
            if (!error) {
                char *found_files = get_overlay_files(awr, fname, error);

                if (found_files) {
                    for (char *name = strtok(found_files, "*"); name; name = strtok(0, "*")) {
                        printf("Deleting gnuplot file '%s'\n", name);
                        if (unlink(name) != 0) printf("Can't delete '%s'\n", name);
                    }
                    free(found_files);
                    awr->awar(AWAR_CS2GP_DIRECTORY)->touch(); // reload file selection box
                }
            }
        }
        else {
            FILE *out = 0;
            if (!error) {
                out = fopen(fname, "w");
                if (!out) error = GB_export_errorf("Cannot write to file '%s'", fname);
            }

            nt_assert(out || error);

            if (!error) {
                char   *type       = awr->awar(AWAR_CS2GP_SUFFIX)->read_string();
                long    smoothSize = awr->awar(AWAR_CS2GP_SMOOTH_VALUES)->read_int();  // 1 = discrete values
                size_t  columns    = column_stat->get_length();

                enum {
                    STAT_AMOUNT,
                    STAT_SIMPLE_FLOAT,
                    STAT_SIMPLE_BOOL,
                    STAT_SORT,
                    STAT_UNKNOWN
                } stat_type = STAT_UNKNOWN;
                union {
                    struct {
                        const float *A;
                        const float *C;
                        const float *G;
                        const float *TU;
                    } amount;                       // STAT_AMOUNT
                    const float *floatVals;         // STAT_SIMPLE_FLOAT
                    const bool  *boolVals;          // STAT_SIMPLE_BOOL
                    SortedFreq  *sorted;            // STAT_SORT
                } data;

                data.amount.C  = NULL; // silence warnings
                data.amount.G  = NULL;
                data.amount.TU = NULL;

                PlotType plot_type = string2PlotType(type);
                switch (plot_type) {
                    case PT_GC_RATIO:
                    case PT_GA_RATIO:
                    case PT_BASE_A:
                    case PT_BASE_C:
                    case PT_BASE_G:
                    case PT_BASE_TU: {
                        stat_type = STAT_AMOUNT;

                        data.amount.A  = column_stat->get_frequencies('A');
                        data.amount.C  = column_stat->get_frequencies('C');
                        data.amount.G  = column_stat->get_frequencies('G');
                        data.amount.TU = column_stat->get_frequencies('U');
                        break;
                    }
                    case PT_RATE:
                        stat_type = STAT_SIMPLE_FLOAT;
                        data.floatVals = column_stat->get_rates();
                        break;

                    case PT_TT_RATIO:
                        stat_type = STAT_SIMPLE_FLOAT;
                        data.floatVals = column_stat->get_ttratio();
                        break;

                    case PT_HELIX: {
                        stat_type = STAT_SIMPLE_BOOL;
                        data.boolVals  = column_stat->get_is_helix();
                        break;
                    }
                    case PT_MOST_FREQUENT_BASE:
                    case PT_SECOND_FREQUENT_BASE:
                    case PT_THIRD_FREQUENT_BASE:
                    case PT_LEAST_FREQUENT_BASE: {
                        stat_type   = STAT_SORT;
                        data.sorted = new SortedFreq(column_stat);
                        break;
                    }
                    case PT_PLOT_TYPES:
                    case PT_UNKNOWN:
                        error = "Please select what to plot";
                        break;
                }

                const GB_UINT4 *weights = column_stat->get_weights();

                if (!error) {
                    Smoother smoother(smoothSize);

                    for (size_t j=0; j<columns; ++j) {
                        if (!weights[j]) continue;

                        double val = 0;
                        switch (stat_type) {
                            case STAT_AMOUNT: {
                                float A  = data.amount.A[j];
                                float C  = data.amount.C[j];
                                float G  = data.amount.G[j];
                                float TU = data.amount.TU[j];

                                float amount = A+C+G+TU;

                                switch (plot_type) {
                                    case PT_GC_RATIO: val = (G+C)/amount; break;
                                    case PT_GA_RATIO: val = (G+A)/amount; break;
                                    case PT_BASE_A:   val = A/amount; break;
                                    case PT_BASE_C:   val = C/amount; break;
                                    case PT_BASE_G:   val = G/amount; break;
                                    case PT_BASE_TU:  val = TU/amount; break;

                                    default: nt_assert(0); break;
                                }
                                break;
                            }
                            case STAT_SIMPLE_FLOAT: val = data.floatVals[j]; break;
                            case STAT_SIMPLE_BOOL:  val = data.boolVals[j]; break;
                            case STAT_SORT:         val = data.sorted->get(plot_type, j); break;

                            case STAT_UNKNOWN: nt_assert(0); break;
                        }

                        smoother.print(out, j, val);
                    }
                }

                if (stat_type == STAT_SORT) delete data.sorted;
                free(type);
            }

            if (out) {
                fclose(out);
                out = NULL;
            }

            if (!error) {
                awr->awar(AWAR_CS2GP_DIRECTORY)->touch(); // reload file selection box

                if (mode == PLOT_AND_VIEW) { // run gnuplot?
                    char *command_file;
                    char *command_name = GB_unique_filename("arb", "gnuplot");

                    out             = GB_fopen_tempfile(command_name, "wt", &command_file);
                    if (!out) error = GB_await_error();
                    else {
                        char *smooth      = awr->awar(AWAR_CS2GP_SMOOTH_GNUPLOT)->read_string();
                        char *found_files = get_overlay_files(awr, fname, error);

                        fprintf(out, "set samples 1000\n");

                        bool plotted = false; // set to true after first 'plot' command (other plots use 'replot')
                        const char *plot_command[] = { "plot", "replot" };

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
                        fprintf(out, "pause mouse any \"Any key or button will terminate gnuplot\"\n");
                        fclose(out);
                        out = 0;

                        char *script = GBS_global_string_copy("gnuplot %s && rm -f %s", command_file, command_file);
                        GB_xcmd(script, true, true);
                        free(script);
                        free(smooth);
                    }
                    free(command_file);
                    free(command_name);
                }
            }
        }
        free(fname);
    }

    if (error) aw_message(error);
}

static void colstat_ali_changed_cb(AW_root *root, AW_awar *awar_ali) {
    if (GLOBAL.gb_main) {
        long smooth_max = GBT_get_alignment_len(GLOBAL.gb_main, awar_ali->read_char_pntr());
        if (smooth_max<0) { // ali not found
            GB_clear_error();
            smooth_max = INT_MAX;
        }
        root->awar(AWAR_CS2GP_SMOOTH_VALUES)->set_minmax(1, smooth_max);
    }
}

AW_window *NT_create_colstat_2_gnuplot_window(AW_root *root) {
    GB_transaction ta(GLOBAL.gb_main);

    AW_awar *awar_default_alignment = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", GLOBAL.gb_main);

    ColumnStat       *column_stat = new ColumnStat(GLOBAL.gb_main, root, AWAR_CS2GP_NAME, awar_default_alignment);
    AW_window_simple *aws         = new AW_window_simple;

    aws->init(root, "EXPORT_CSP_TO_GNUPLOT", "Export Column statistic to GnuPlot");
    aws->load_xfig("cpro/csp_2_gnuplot.fig");

    root->awar_int(AWAR_CS2GP_SMOOTH_VALUES, 1);
    root->awar_int(AWAR_CS2GP_GNUPLOT_OVERLAY_POSTFIX);
    root->awar_int(AWAR_CS2GP_GNUPLOT_OVERLAY_PREFIX);
    root->awar_string(AWAR_CS2GP_SMOOTH_GNUPLOT);

    AW_create_fileselection_awars(root, AWAR_CS2GP, "", ".gc_gnu", "noname.gc_gnu");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("csp_2_gnuplot.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_standard_fileselection(aws, AWAR_CS2GP);

    aws->at("csp");
    COLSTAT_create_selection_list(aws, column_stat);

    aws->at("what");
    AW_selection_list *plotTypeList = aws->create_selection_list(AWAR_CS2GP_SUFFIX, true);
    for (int pt = 0; pt<PT_PLOT_TYPES; ++pt) {
        plotTypeList->insert(plotTypeDescription[pt], plotTypeName[pt]);
    }
    plotTypeList->insert_default("<select one>", "");
    plotTypeList->update();

    awt_create_filter_awars(root, AW_ROOT_DEFAULT, AWAR_CS2GP_FILTER_NAME, AWAR_DEFAULT_ALIGNMENT);
    adfiltercbstruct *filter = awt_create_select_filter(root, GLOBAL.gb_main, AWAR_CS2GP_FILTER_NAME);
    {
        AW_awar *awar_ali = root->awar(filter->def_alignment);
        awar_ali->add_callback(makeRootCallback(colstat_ali_changed_cb, awar_ali));
        awar_ali->touch();
    }

    aws->at("ap_filter");
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, filter));
    aws->create_button("SELECT_FILTER", AWAR_CS2GP_FILTER_NAME);

    aws->at("smooth");
    aws->create_input_field(AWAR_CS2GP_SMOOTH_VALUES, 8);

    aws->at("smooth2");
    aws->create_toggle_field(AWAR_CS2GP_SMOOTH_GNUPLOT, 1);
    aws->insert_default_toggle("None", "N", "");
    aws->insert_toggle("Unique", "U", "smooth unique");
    aws->insert_toggle("CSpline", "S", "smooth cspline");
    aws->insert_toggle("Bezier", "B", "smooth bezier");
    aws->update_toggle_field();

    aws->auto_space(10, 10);
    aws->button_length(13);

    PlotParam *param = new PlotParam(column_stat, filter); // bound to CB, dont free

    aws->at("save");
    aws->callback(makeWindowCallback(colstat_2_gnuplot_cb, param, PLOT));
    aws->create_button("SAVE", "Save");

    aws->highlight();
    aws->callback(makeWindowCallback(colstat_2_gnuplot_cb, param, PLOT_AND_VIEW));
    aws->create_button("SAVE_AND_VIEW", "Save & View");

    aws->at("overlay1");
    aws->label("Overlay statistics with same prefix?");
    aws->create_toggle(AWAR_CS2GP_GNUPLOT_OVERLAY_PREFIX);

    aws->at("overlay2");
    aws->label("Overlay statistics with same postfix?");
    aws->create_toggle(AWAR_CS2GP_GNUPLOT_OVERLAY_POSTFIX);

    aws->at("del_overlays");
    aws->callback(makeWindowCallback(colstat_2_gnuplot_cb, param, PLOT_CLEANUP));
    aws->create_autosize_button("DEL_OVERLAYS", "Delete currently overlayed files", "D", 2);

    return aws;
}


