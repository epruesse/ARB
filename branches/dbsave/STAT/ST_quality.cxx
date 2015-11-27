// =============================================================== //
//                                                                 //
//   File      : ST_quality.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "st_quality.hxx"
#include "st_ml.hxx"
#include "MostLikelySeq.hxx"

#include <ColumnStat.hxx>
#include <BI_helix.hxx>
#include <AP_filter.hxx>
#include <arb_progress.h>
#include <arbdbt.h>
#include <arb_strbuf.h>

#include <cctype>
#include <cmath>

// --------------------------
//      LikelihoodRanges

LikelihoodRanges::LikelihoodRanges(size_t no_of_ranges) {
    ranges     = no_of_ranges;
    likelihood = new VarianceSampler[ranges];
}

VarianceSampler LikelihoodRanges::summarize_all_ranges() {
    VarianceSampler allRange;
    for (size_t range = 0; range<ranges; ++range) {
        allRange.add(likelihood[range]);
    }
    return allRange;
}

char *LikelihoodRanges::generate_string(Sampler& t_values) {
    VarianceSampler  allRange = summarize_all_ranges();
    char            *report   = new char[ranges+1];

    if (allRange.get_count()>1) {
        double all_median        = allRange.get_median();
        double all_variance      = allRange.get_variance(all_median);
        double all_std_deviation = sqrt(all_variance);

        for (size_t range = 0; range<ranges; ++range) {
            size_t count = likelihood[range].get_count();
            if (count>1) {
                // perform students-t-test
            
                double standard_error = all_std_deviation / sqrt(count); // standard error of mean
                double median_diff    = likelihood[range].get_median() - all_median;
                double t_value        = median_diff / standard_error; // predictand of t_value is 0.0

                t_values.add(fabs(t_value)); // sample t_values

                double val = 0.7 * t_value;

                // val outside [-0.5 .. +0.5] -> "t-test fails"
                //
                // 0.7 * t_value = approx. t_value / 1.428;
                // 
                // 1.428 scheint ein fest-kodiertes quantil zu sein
                // (bzw. dessen Haelfte, da ja nur der Range [-0.5 .. +0.5] getestet wird)
                // Das eigentliche Quantil ist also ca. 2.856
                //
                // Eigentlich haengt das Quantil von der Stichprobengroesse (hier "count",
                // d.h. "Anzahl der Basen im aktuellen Range") ab.
                // (vgl. http://de.wikipedia.org/wiki/T-Verteilung#Ausgew.C3.A4hlte_Quantile_der_t-Verteilung )

                int ival = int (val + .5) + 5;

                if (ival > 9) ival = 9;
                if (ival < 0) ival = 0;

                report[range] = (ival == 5) ? '-' : '0'+ival;
            }
            else {
                report[range] = '.';
            }
        }
    }
    else {
        for (size_t range = 0; range<ranges; ++range) {
            report[range] = '.';
        }
    }

    report[ranges] = 0;

    return report;
}

// --------------------------
//      ColumnQualityInfo

ColumnQualityInfo::ColumnQualityInfo(int seq_len, int bucket_size)
    : stat_half(2)
    , stat_five(5)
    , stat_user(seq_len / bucket_size + 1)
    , stat_cons(2)
{}

#if defined(WARN_TODO)
#warning stat_cons unused - implement
#endif

static void st_ml_add_sequence_part_to_stat(ST_ML *st_ml, ColumnStat */* awt_csp */,
                                            const char *species_name, int seq_len, int bucket_size,
                                            GB_HASH *species_to_info_hash, int start, int end)
{
    AP_tree *node = STAT_find_node_by_name(st_ml, species_name);
    if (node) {
        MostLikelySeq *sml = st_ml->get_ml_vectors(0, node, start, end);
        if (sml) {
            ColumnQualityInfo *info;
            if (start > 0) {
                info = (ColumnQualityInfo *) GBS_read_hash(species_to_info_hash, species_name);
            }
            else {
                info = new ColumnQualityInfo(seq_len, bucket_size);
                GBS_write_hash(species_to_info_hash, species_name, long (info));
            }

            int         pos;
            const char *source_sequence     = 0;
            int         source_sequence_len = 0;

            GBDATA *gb_data = sml->get_bound_species_data();
            if (gb_data) {
                source_sequence_len = GB_read_string_count(gb_data);
                source_sequence     = GB_read_char_pntr(gb_data);
            }
            if (end > source_sequence_len) end = source_sequence_len;

            ST_base_vector *vec = sml->tmp_out + start;
            for (pos = start; pos < end; vec++, pos++) {
                DNA_Base base = dna_table.char_to_enum(source_sequence[pos]);
                if (base != ST_UNKNOWN && base != ST_GAP) { // don't count gaps
                    ST_FLOAT max = vec->max_frequency();

                    double val         = max / (0.0001 + vec->b[base]); 
                    double log_val     = log(val);
                    double seq_rel_pos = double(pos)/seq_len;

                    info->stat_half.add_relative(seq_rel_pos, log_val);
                    info->stat_five.add_relative(seq_rel_pos, log_val);
                    info->stat_user.add_relative(seq_rel_pos, log_val);
                }
            }
        }
    }
}

static GB_ERROR st_ml_add_quality_string_to_species(GBDATA *gb_main, const AP_filter *filter, 
                                                    const char *alignment_name, const char *species_name, 
                                                    size_t bucket_size, GB_HASH *species_to_info_hash, st_report_enum report,
                                                    const char *dest_field)
{
    GB_ERROR  error        = 0;
    GBDATA   *gb_species   = GBT_find_species(gb_main, species_name);
    if (!gb_species) error = GBS_global_string("Unknown species '%s'", species_name);
    else {
        ColumnQualityInfo *info = (ColumnQualityInfo *) GBS_read_hash(species_to_info_hash, species_name);
        if (!info) error        = GBS_global_string("Statistic missing for species '%s'", species_name);
        else {
            GBDATA *gb_dest     = GB_search(gb_species, dest_field, GB_STRING);
            if (!gb_dest) error = GB_await_error();
            else {
                Sampler  t_values; // sample t-values here
                char    *user_str = info->stat_user.generate_string(t_values);
                {
                    char *half_str = info->stat_half.generate_string(t_values);
                    char *five_str = info->stat_five.generate_string(t_values);

                    GBS_strstruct *buffer = GBS_stropen(2*9 + 3*2 + (2+5+info->overall_range_count()));

#if 1
                    GBS_strcat(buffer, GBS_global_string("%8.3f ", t_values.get_median()));
                    GBS_strcat(buffer, GBS_global_string("%8.3f ", t_values.get_sum()));
#else
#warning t-value summary disabled for test-purposes
#endif

                    GBS_chrcat(buffer, 'a'); GBS_strcat(buffer, half_str); GBS_chrcat(buffer, ' ');
                    GBS_chrcat(buffer, 'b'); GBS_strcat(buffer, five_str); GBS_chrcat(buffer, ' ');
                    GBS_chrcat(buffer, 'c'); GBS_strcat(buffer, user_str);

                    error = GB_write_string(gb_dest, GBS_mempntr(buffer));
                    GBS_strforget(buffer);

                    delete [] half_str;
                    delete [] five_str;
                }

                if (!error && report) {
                    // name "quality" handled specially by ../EDIT4/EDB_root_bact.cxx@chimera_check_quality_string
                    GBDATA *gb_report = GBT_add_data(gb_species, alignment_name, "quality", GB_STRING);

                    if (!gb_report) error = GB_await_error();
                    else {
                        char *report_str;
                        {
                            size_t  filtered_len = filter->get_filtered_length();
                            report_str   = new char[filtered_len + 1];

                            for (size_t i = 0; i < filtered_len; i++) {
                                report_str[i] = user_str[i / bucket_size];
                            }
                            report_str[filtered_len] = 0;
                        }

                        {
                            char *blownUp_report = filter->blowup_string(report_str, ' ');
                            error                = GB_write_string(gb_report, blownUp_report);
                            free(blownUp_report);
                        }
                        if (report == ST_QUALITY_REPORT_TEMP) error = GB_set_temporary(gb_report);

                        delete [] report_str;
                    }
                }
                delete [] user_str;
            }
        }
    }

    return error;
}

static void destroy_ColumnQualityInfo(long cl_info) {
    ColumnQualityInfo *info = (ColumnQualityInfo*)cl_info;
    delete info;
}

GB_ERROR st_ml_check_sequence_quality(GBDATA     *gb_main, const char *tree_name,
                                      const char *alignment_name, ColumnStat *colstat, const WeightedFilter *weighted_filter, int bucket_size,
                                      int         marked_only, st_report_enum report, const char *dest_field)
{
#if defined(WARN_TODO)
#warning parameter 'alignment_name' can be retrieved from WeightedFilter (as soon as automatic mapping works for filters)
#endif
    ST_ML    st_ml(gb_main);
    GB_ERROR error = st_ml.calc_st_ml(tree_name, alignment_name, 0, marked_only, colstat, weighted_filter);

    if (!error) {
        GB_HASH *species2info = GBS_create_dynaval_hash(GBT_get_species_count(gb_main), GB_IGNORE_CASE, destroy_ColumnQualityInfo);
        size_t   species_count;

        GB_CSTR *snames  = GBT_get_names_of_species_in_tree(st_ml.get_gbt_tree(), &species_count);
        int      seq_len = st_ml.get_filtered_length();
        size_t   parts   = (seq_len-1)/ST_MAX_SEQ_PART+1;

        {
            arb_progress add_progress("Calculating stat", parts*species_count);

            for (int pos = 0; pos < seq_len && !error; pos += ST_MAX_SEQ_PART) {
                int end                = pos + ST_MAX_SEQ_PART - 1;
                if (end > seq_len) end = seq_len;

                for (GB_CSTR *pspecies_name = snames; *pspecies_name && !error; pspecies_name++) {
                    st_ml_add_sequence_part_to_stat(&st_ml, colstat, *pspecies_name, seq_len, bucket_size, species2info, pos, end);
                    add_progress.inc_and_check_user_abort(error);
                }
            }
        }

        if (!error) {
            arb_progress     res_progress("Calculating reports", species_count);
            const AP_filter *filter = st_ml.get_filter();

            for (GB_CSTR *pspecies_name = snames; *pspecies_name && !error; pspecies_name++) {
                error = st_ml_add_quality_string_to_species(gb_main, filter, alignment_name, *pspecies_name, bucket_size, species2info, report, dest_field);
                res_progress.inc_and_check_user_abort(error);
            }
        }

        free(snames);
        GBS_free_hash(species2info);
    }

    return error;
}
