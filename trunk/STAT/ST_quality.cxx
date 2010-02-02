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

#include <aw_awars.hxx>
#include <BI_helix.hxx>
#include <AP_filter.hxx>

#include <ColumnStat.hxx>

#include <cctype>

// --------------------------
//      LikelihoodRanges

LikelihoodRanges::LikelihoodRanges(size_t no_of_ranges) {
    ranges     = no_of_ranges;
    likelihood = new SummarizedLikelihoods[ranges];
}

SummarizedLikelihoods LikelihoodRanges::summarize_all_ranges() {
    SummarizedLikelihoods allRange;
    for (size_t range = 0; range<ranges; ++range) {
        allRange.add(likelihood[range]);
    }
    return allRange;
}

char *LikelihoodRanges::generate_string() {
    SummarizedLikelihoods  allRange = summarize_all_ranges();
    char                  *report   = new char[ranges+1];

    if (allRange.get_count()) {
        double all_mean_likelihood  = allRange.get_mean_likelihood();
        double all_mean_likelihood2 = allRange.get_mean_likelihood2();
        double mean_sigma           = sqrt(all_mean_likelihood2 - all_mean_likelihood*all_mean_likelihood);

        for (size_t range = 0; range<ranges; ++range) {
            size_t count = likelihood[range].get_count();
            if (count>1) {
                double variance = mean_sigma / sqrt(count);
                double diff     = likelihood[range].get_mean_likelihood() - all_mean_likelihood;
                
                double val  = 0.7 * diff / variance;
                int    ival = int (val + .5) + 5;

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

#warning stat_cons unused - implement

static void st_ml_add_sequence_part_to_stat(ST_ML *st_ml, ColumnStat */* awt_csp */,
                                            const char *species_name, int seq_len, int bucket_size,
                                            GB_HASH *species_to_info_hash, int start, int end)
{
    AP_tree *node = STAT_find_node_by_name(st_ml, species_name);
    if (node) {
        ST_sequence_ml *sml = st_ml->get_ml_vectors(0, node, start, end);
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
                double max = 0;
                double v;
                int    b;

                for (b = ST_A; b < ST_MAX_BASE; b++) {
                    v = vec->b[b];
                    if (v > max) max = v;
                }
                AWT_dna_base base = awt_dna_table.char_to_enum(source_sequence[pos]);
                if (base != ST_UNKNOWN && base != ST_GAP) { // don't count gaps
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

static GB_ERROR st_ml_add_quality_string_to_species(GBDATA *gb_main,
                                                    const char *alignment_name, const char *species_name, size_t seq_len,
                                                    size_t bucket_size, GB_HASH *species_to_info_hash, st_report_enum report,
                                                    const char *dest_field)
{
    GB_ERROR error = 0;
    
    GBDATA *gb_species     = GBT_find_species(gb_main, species_name);
    if (!gb_species) error = GBS_global_string("Unknown species '%s'", species_name);
    else {
        ColumnQualityInfo *info = (ColumnQualityInfo *) GBS_read_hash(species_to_info_hash, species_name);
        if (!info) error        = GBS_global_string("Statistic missing for species '%s'", species_name);
        else {
            GBDATA *gb_dest     = GB_search(gb_species, dest_field, GB_STRING);
            if (!gb_dest) error = GB_await_error();
            else {
                char *user_str = info->stat_user.generate_string();
                {
                    char *half_str = info->stat_half.generate_string();
                    char *five_str = info->stat_five.generate_string();

                    GBS_strstruct *buffer = GBS_stropen(info->overall_range_count()+10);

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
                        char *rp = new char[seq_len + 1];
                        rp[seq_len] = 0;
                        for (size_t i = 0; i < seq_len; i++) rp[i] = user_str[i / bucket_size];
                        error = GB_write_string(gb_report, rp);
                        if (report == ST_QUALITY_REPORT_TEMP) GB_set_temporary(gb_report);
                        delete [] rp;
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

GB_ERROR st_ml_check_sequence_quality(GBDATA *gb_main, const char *tree_name,
                                      const char *alignment_name, ColumnStat *colstat, int bucket_size,
                                      int marked_only, st_report_enum report, const char *dest_field)
{
    int      seq_len = GBT_get_alignment_len(gb_main, alignment_name); // @@@ filter -> use filter len
    ST_ML    st_ml(gb_main);
    GB_ERROR error   = st_ml.init_st_ml(tree_name, alignment_name, 0, marked_only, colstat, true);

    if (!error) {
        GB_HASH *species_to_info_hash = GBS_create_dynaval_hash(GBT_get_species_count(gb_main), GB_IGNORE_CASE, destroy_ColumnQualityInfo);
        GB_CSTR *snames               = GBT_get_names_of_species_in_tree(st_ml.get_gbt_tree());

        aw_openstatus("Sequence Quality Check");
        aw_status(0.0);

        size_t parts         = (seq_len-1)/ST_MAX_SEQ_PART+1;
        size_t species_count = 0;
        while (snames[species_count]) ++species_count;

        aw_status_counter progress(parts*species_count);

        for (int pos = 0; pos < seq_len && !progress.aborted_by_user(); pos += ST_MAX_SEQ_PART) {
            int end                = pos + ST_MAX_SEQ_PART - 1;
            if (end > seq_len) end = seq_len;

            for (GB_CSTR *pspecies_name = snames; *pspecies_name; pspecies_name++) {
                st_ml_add_sequence_part_to_stat(&st_ml, colstat, *pspecies_name, seq_len, bucket_size, species_to_info_hash, pos, end);
                progress.inc();
            }
        }

        if (!error && !progress.aborted_by_user()) {
            aw_status("Generating Result String");
            progress.restart(species_count);
            for (GB_CSTR *pspecies_name = snames; *pspecies_name && !error && !progress.aborted_by_user(); pspecies_name++) {
                error = st_ml_add_quality_string_to_species(gb_main, alignment_name, *pspecies_name, seq_len, bucket_size, species_to_info_hash, report, dest_field);
                progress.inc();
            }
        }

        if (!error && progress.aborted_by_user()) error = "Aborted by user";

        aw_closestatus();
        free(snames);
        GBS_free_hash(species_to_info_hash);
    }

    return error;
}
