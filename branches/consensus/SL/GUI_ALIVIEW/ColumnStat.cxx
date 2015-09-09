// =============================================================== //
//                                                                 //
//   File      : ColumnStat.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ColumnStat.hxx"
#include "awt_sel_boxes.hxx"
#include "ga_local.h"
#include <AP_filter.hxx>
#include <BI_helix.hxx>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>

#include <arbdbt.h>
#include <arb_strbuf.h>

#define AWAR_COLSTAT_NAME         "/name=/name"
#define AWAR_COLSTAT_ALIGNMENT    "/name=/alignment"
#define AWAR_COLSTAT_SMOOTH       "/name=/smooth"
#define AWAR_COLSTAT_ENABLE_HELIX "/name=/enable_helix"

void ColumnStat::refresh_sai_selection_list() {
    GB_transaction ta(gb_main);

    freeset(alignment_name, awr->awar(awar_alignment)->read_string());
    freeset(type_path, GBS_string_eval(alignment_name, "*=*1/_TYPE", 0));

    if (saisel) saisel->refresh();
}

static void refresh_columnstat_selection(AW_root *, AW_CL cl_column_stat) {
    ColumnStat *column_stat = (ColumnStat *)cl_column_stat;
    column_stat->refresh_sai_selection_list();
}

ColumnStat::ColumnStat(GBDATA *gb_maini, AW_root *awri, const char *awar_template, AW_awar *awar_used_alignment) {
    /* awar_template ==     ".../name"
     *  -> generated        ".../alignment"
     *                      ".../smooth"
     *                      ".../enable_helix"
     */

    memset((char *)this, 0, sizeof(ColumnStat));

    gb_main = gb_maini;
    awr     = awri;

    awar_name         = GBS_string_eval(awar_template, AWAR_COLSTAT_NAME, 0);
    awar_alignment    = GBS_string_eval(awar_template, AWAR_COLSTAT_ALIGNMENT, 0);
    awar_smooth       = GBS_string_eval(awar_template, AWAR_COLSTAT_SMOOTH, 0);
    awar_enable_helix = GBS_string_eval(awar_template, AWAR_COLSTAT_ENABLE_HELIX, 0);

    ga_assert(strcmp(awar_name, awar_alignment) != 0); // awar_template must end with (or contain) "/name"

    awr->awar_string(awar_name, "");
    awr->awar_string(awar_alignment)->map(awar_used_alignment);
    awr->awar_int(awar_smooth);
    awr->awar_int(awar_enable_helix, 1);

    awr->awar(awar_alignment)->add_callback(refresh_columnstat_selection, (AW_CL)this);
    refresh_sai_selection_list();                       // same as refresh_columnstat_selection(this)
}

void ColumnStat::forget() {
    delete [] weights;  weights  = NULL;
    delete [] rates;    rates    = NULL;
    delete [] ttratio;  ttratio  = NULL;
    delete [] is_helix; is_helix = NULL;
    delete [] mut_sum;  mut_sum  = NULL;
    delete [] freq_sum; freq_sum = NULL;
    delete desc;        desc     = NULL;

    for (int i=0; i<256; i++) {
        delete [] frequency[i];
        frequency[i] = NULL;
    }
    seq_len = 0; // mark invalid
}

ColumnStat::~ColumnStat() {
    forget();
    delete awar_name;
    delete awar_alignment;
    delete awar_smooth;
    delete awar_enable_helix;
}

GB_ERROR ColumnStat::calculate(AP_filter *filter) {
    forget(); // delete previously calculated stats

    GB_transaction ta(gb_main);
    GB_ERROR       error            = filter ? filter->is_invalid() : NULL;
    size_t         alignment_length = 0;

    if (!error) {
        long alignment_length_l = GBT_get_alignment_len(gb_main, alignment_name);
        if (alignment_length_l <= 1) {
            error = GBS_global_string("Unknown size for alignment '%s'", alignment_name);
        }
        else {
            alignment_length = alignment_length_l;
            if (filter && filter->get_length() != alignment_length) {
                error = GBS_global_string("Incompatible filter (filter=%zu bp, alignment=%zu bp)",
                                          filter->get_length(), alignment_length);
            }
        }
    }
    seq_len = 0;

    if (!error) {
        char   *sai_name = awr->awar(awar_name)->read_string();
        GBDATA *gb_sai   = GBT_find_SAI(gb_main, sai_name);

        if (!gb_sai) {
            if (strcmp(sai_name, "") != 0) { // no error if SAI name is empty
                error = GBS_global_string("Column statistic: no such SAI: '%s'", sai_name);
            }
        }

        if (gb_sai) {
            GBDATA *gb_ali   = 0;
            GBDATA *gb_freqs = 0;
            if (!error) {
                gb_ali = GB_entry(gb_sai, alignment_name);
                if (!gb_ali) error = GBS_global_string("SAI '%s' does not exist in alignment '%s'", sai_name, alignment_name);
            }
            if (!error) {
                gb_freqs = GB_entry(gb_ali, "FREQUENCIES");
                if (!gb_freqs) error = GBS_global_string("Column statistic '%s' is invalid (has no FREQUENCIES)", sai_name);
            }

            if (!error) {
                seq_len = filter ? filter->get_filtered_length() : alignment_length;

                delete [] weights; weights  = new GB_UINT4[seq_len];
                delete [] rates;   rates    = new float[seq_len];
                delete [] ttratio; ttratio  = new float[seq_len];
                delete [] is_helix; is_helix = new bool[seq_len];
                delete [] mut_sum; mut_sum  = new GB_UINT4[seq_len];
                delete [] freq_sum; freq_sum = new GB_UINT4[seq_len];

                delete desc; desc = 0;

                size_t i;
                size_t j;
                for (i=0; i<256; i++) { delete frequency[i]; frequency[i]=0; }

                long use_helix = awr->awar(awar_enable_helix)->read_int();

                for (i=0; i<seq_len; i++) {
                    is_helix[i] = false;
                    weights[i]  = 1;
                }

                if (!error && use_helix) {            // calculate weights and helix filter
                    BI_helix helix;
                    error = helix.init(this->gb_main, alignment_name);
                    if (error) {
                        aw_message(error);
                        error = 0;
                        goto no_helix;
                    }
                    error = 0;
                    for (j=i=0; i<alignment_length; i++) {
                        if (!filter || filter->use_position(i)) {
                            if (helix.pairtype(i) == HELIX_PAIR) {
                                is_helix[j] = true;
                                weights[j]  = 1;
                            }
                            else {
                                is_helix[j] = false;
                                weights[j]  = 2;
                            }
                            j++;
                        }
                    }
                }
              no_helix :

                for (i=0; i<seq_len; i++) freq_sum[i] = 0;

                GBDATA *gb_freq;
                GB_UINT4 *freqi[256];
                for (i=0; i<256; i++) freqi[i] = 0;
                int wf;                 // ********* read the frequency statistic
                for (gb_freq = GB_child(gb_freqs); gb_freq; gb_freq = GB_nextChild(gb_freq)) {
                    char *key = GB_read_key(gb_freq);
                    if (key[0] == 'N' && key[1] && !key[2]) {
                        wf = key[1];
                        freqi[wf] = GB_read_ints(gb_freq);
                    }
                    free(key);
                }

                GB_UINT4 *minmut      = 0;
                GB_UINT4 *transver    = 0;
                GBDATA   *gb_minmut   = GB_entry(gb_freqs, "TRANSITIONS");
                GBDATA   *gb_transver = GB_entry(gb_freqs, "TRANSVERSIONS");

                if (!gb_minmut)   error = GBS_global_string("Column statistic '%s' is invalid (has no FREQUENCIES/TRANSITIONS)", sai_name);
                if (!gb_transver) error = GBS_global_string("Column statistic '%s' is invalid (has no FREQUENCIES/TRANSVERSIONS)", sai_name);

                if (gb_minmut) {
                    minmut = GB_read_ints(gb_minmut);
                    if (!minmut) error = GBS_global_string("Column statistic '%s' is invalid (error reading FREQUENCIES/TRANSITIONS: %s)", sai_name, GB_await_error());
                }
                if (gb_transver) {
                    transver = GB_read_ints(gb_transver);
                    if (!transver) error = GBS_global_string("Column statistic '%s' is invalid (error reading FREQUENCIES/TRANSVERSIONS: %s)", sai_name, GB_await_error());
                }

                if (!error) {
                    unsigned long max_freq_sum = 0;
                    for (wf = 0; wf<256; wf++) {    // ********* calculate sum of mutations
                        if (!freqi[wf]) continue;   // ********* calculate nbases for each col.
                        for (j=i=0; i<alignment_length; i++) {
                            if (filter && !filter->use_position(i)) continue;
                            freq_sum[j] += freqi[wf][i];
                            mut_sum[j] = minmut[i];
                            j++;
                        }
                    }
                    for (i=0; i<seq_len; i++)
                        if (freq_sum[i] > max_freq_sum)
                            max_freq_sum = freq_sum[i];
                    unsigned long min_freq_sum = DIST_MIN_SEQ(max_freq_sum);

                    for (wf = 0; wf<256; wf++) {
                        if (!freqi[wf]) continue;
                        frequency[wf] = new float[seq_len];
                        for (j=i=0; i<alignment_length; i++) {
                            if (filter && !filter->use_position(i)) continue;
                            if (freq_sum[j] > min_freq_sum) {
                                frequency[wf][j] = freqi[wf][i]/(float)freq_sum[j];
                            }
                            else {
                                frequency[wf][j] = 0;
                                weights[j] = 0;
                            }
                            j++;
                        }
                    }

                    for (j=i=0; i<alignment_length; i++) { // ******* calculate rates
                        if (filter && !filter->use_position(i)) continue;
                        if (!weights[j]) {
                            rates[j] = 1.0;
                            ttratio[j] = 0.5;
                            j++;
                            continue;
                        }
                        rates[j] = (mut_sum[j] / (float)freq_sum[j]);
                        if (transver[i] > 0) {
                            ttratio[j] = (minmut[i] - transver[i]) / (float)transver[i];
                        }
                        else {
                            ttratio[j] = 2.0;
                        }
                        if (ttratio[j] < 0.05) ttratio[j] = 0.05;
                        if (ttratio[j] > 5.0) ttratio[j] = 5.0;
                        j++;
                    }

                    // ****** normalize rates

                    double sum_rates = 0;
                    for (i=0; i<seq_len; i++) sum_rates += rates[i];
                    sum_rates /= seq_len;
                    for (i=0; i<seq_len; i++) rates[i] /= sum_rates;
                }

                free(transver);
                free(minmut);
                for (i=0; i<256; i++) free(freqi[i]);
            }
        }
        free(sai_name);
    }

    return error;
}

void ColumnStat::weight_by_inverseRates() const {
    for (size_t i = 0; i<seq_len; ++i) {
        if (rates[i]>0.0000001) {
            weights[i] *= (int)(2.0 / rates[i]);
        }
    }
}



void ColumnStat::print() { 
    if (seq_len) {
        double sum_rates[2] = { 0.0, 0.0 };
        double sum_tt[2] = { 0.0, 0.0 };
        long   count[2] = { 0, 0 };

        for (unsigned j=0; j<seq_len; j++) {
            if (weights[j]) {
                fputc(".#"[is_helix[j]], stdout);
                printf("%u:    minmut %5i  freqs %5i   rates  %5f  tt %5f  ",
                       j, mut_sum[j], freq_sum[j], rates[j], ttratio[j]);
                for (int wf = 0; wf<256; wf++) {
                    if (frequency[wf]) printf("%c:%5f ", wf, frequency[wf][j]);
                }
                sum_rates[is_helix[j]] += rates[j];
                sum_tt[is_helix[j]] += ttratio[j];
                count[is_helix[j]]++;
                printf("w: %i\n", weights[j]);
            }
        }
        printf("Helical Rates %5f   Non Hel. Rates  %5f\n",
               sum_rates[1]/count[1], sum_rates[0]/count[0]);
        printf("Helical TT %5f  Non Hel. TT %5f\n",
               sum_tt[1]/count[1], sum_tt[0]/count[0]);
    }
}

static char *filter_columnstat_SAIs(GBDATA *gb_extended, AW_CL cl_column_stat) {
    // return NULL for non-columnstat SAIs
    ColumnStat *column_stat = (ColumnStat*)cl_column_stat;

    char *result = NULL;
    if (column_stat->has_valid_alignment()) {
        GBDATA *gb_type = GB_search(gb_extended, column_stat->get_type_path(), GB_FIND);

        if (gb_type) {
            const char *type = GB_read_char_pntr(gb_type);

            if (GBS_string_matches(type, "PV?:*", GB_MIND_CASE)) {
                GBS_strstruct *strstruct = GBS_stropen(100);

                GBS_strcat(strstruct, GBT_read_name(gb_extended));
                GBS_strcat(strstruct, ":      <");
                GBS_strcat(strstruct, type);
                GBS_strcat(strstruct, ">");

                result = GBS_strclose(strstruct);
            }
        }
    }
    return result;
}

void ColumnStat::create_sai_selection_list(AW_window *aww) {
    GB_transaction ta(gb_main);
    saisel = awt_create_SAI_selection_list(gb_main, aww, awar_name, true, filter_columnstat_SAIs, (AW_CL)this);
}

void COLSTAT_create_selection_list(AW_window *aws, ColumnStat *column_stat) {
    column_stat->create_sai_selection_list(aws);
}

AW_window *COLSTAT_create_selection_window(AW_root *aw_root, ColumnStat *column_stat) {
    GB_transaction ta(column_stat->get_gb_main());
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SELECT_CSP", "Select Column Statistic");
    aws->load_xfig("awt/col_statistic.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("awt_csp.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("box");
    COLSTAT_create_selection_list(aws, column_stat);

    aws->at("smooth");
    aws->create_toggle_field(column_stat->get_awar_smooth());
    aws->insert_toggle("Calculate each column (nearly) independently", "D", 0);
    aws->insert_toggle("Smooth parameter estimates a little", "M", 1);
    aws->insert_toggle("Smooth parameter estimates across columns", "S", 2);
    aws->update_toggle_field();

    aws->at("helix");
    aws->label("Use Helix Information (SAI 'HELIX')");
    aws->create_toggle(column_stat->get_awar_enable_helix());
    return (AW_window *)aws;
}
