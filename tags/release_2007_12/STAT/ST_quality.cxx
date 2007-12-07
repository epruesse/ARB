#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_awars.hxx>
#include <BI_helix.hxx>

#include <aw_root.hxx>
#include <awt_tree.hxx>
#include <awt_csp.hxx>
#include "st_ml.hxx"
#include "st_window.hxx"
#include "st_quality.hxx"

st_cq_stat::st_cq_stat(int isize)
{
    size = isize;
    likelihoods = new double[size];
    square_liks = new double[size];
    n_elems = new int[size];
    int i;
    for (i = 0; i < size; i++) {
        n_elems[i] = 0;
        square_liks[i] = 0;
        likelihoods[i] = 0;
    }
}

st_cq_stat::~st_cq_stat()
{
    delete[]square_liks;
    delete[]likelihoods;
    delete[]n_elems;
}

void st_cq_stat::add(int pos, double lik)
{
    assert(pos >= 0 && pos < size);
    likelihoods[pos] += lik;
    square_liks[pos] += lik * lik;
    n_elems[pos]++;
}

char *st_cq_stat::generate_string()
{
    int i;
    double sum_lik = 0;
    double square_sum_lik = 0;
    int sum_elems = 0;
    char *res = new char[size + 1];
    for (i = 0; i < size; i++) {
        sum_lik += likelihoods[i];
        square_sum_lik += square_liks[i];
        sum_elems += n_elems[i];
        res[i] = '.';
    }
    res[size] = 0;
    if (sum_elems == 0) {
        return res;
    }
    double mean_lik = sum_lik / sum_elems;
    double mean_sigma =
        sqrt(square_sum_lik / sum_elems - mean_lik * mean_lik);
    for (i = 0; i < size; i++) {
        if (n_elems[i] <= 1)
            continue;

        double variance = mean_sigma / sqrt(n_elems[i]);
        double diff = likelihoods[i] / n_elems[i] - mean_lik;
        double val = .7 * diff / variance;
        int ival = int (val + .5) + 5;
        if (ival > 9)
            ival = 9;
        if (ival < 0)
            ival = 0;
        res[i] = '0' + ival;
        if (res[i] == '5')
            res[i] = '-';
    }
    return res;
}

st_cq_info::st_cq_info(int seq_len, int bucket_size):ss2(2), ss5(5), ssu(seq_len / bucket_size + 1),
sscon(2)
{
    ;
}

st_cq_info::~st_cq_info()
{
    ;
}

void st_ml_add_sequence_part_to_stat(ST_ML * st_ml,
                                     AWT_csp * /*awt_csp */ ,
                                     const char *species_name, int seq_len,
                                     int bucket_size,
                                     GB_HASH * species_to_info_hash,
                                     int start, int end)
{

    AP_tree *node =
        st_ml_convert_species_name_to_node(st_ml, species_name);
    if (!node)
        return;
    ST_sequence_ml *sml = st_ml->get_ml_vectors(0, node, start, end);
    if (!sml)
        return;                 // no statistic available
    st_cq_info *info;
    if (start > 0) {
        info =
            (st_cq_info *) GBS_read_hash(species_to_info_hash,
                                         species_name);
    } else {
        info = new st_cq_info(seq_len, bucket_size);
        GBS_write_hash(species_to_info_hash, species_name, long (info));
    }
    int pos;
    char *source_sequence = 0;
    int source_sequence_len = 0;

    if (sml->gb_data) {
        source_sequence_len = GB_read_string_count(sml->gb_data);
        source_sequence = GB_read_char_pntr(sml->gb_data);
    }
    if (end > source_sequence_len) {
        end = source_sequence_len;
    }

    ST_base_vector *vec = sml->tmp_out + start;
    for (pos = start; pos < end; vec++, pos++) {
        double max = 0;
        register double v;
        register int b;
        for (b = ST_A; b < ST_MAX_BASE; b++) {
            v = vec->b[b];
            if (v > max)
                max = v;
        }
        AWT_dna_base base =
            awt_dna_table.char_to_enum(source_sequence[pos]);
        if (base != ST_UNKNOWN && base != ST_GAP) {     // dont count gaps
            double val = max / (0.0001 + vec->b[base]);
            double log_val = log(val);
            info->ss2.add(pos * 2 / seq_len, log_val);
            info->ss5.add(pos * 5 / seq_len, log_val);
            info->ssu.add(pos * info->ssu.size / seq_len, log_val);
        }
    }
}

void st_ml_add_quality_string_to_species(GBDATA * gb_main,
                                         const char *alignment_name,
                                         const char *species_name,
                                         int seq_len, int bucket_size,
                                         GB_HASH * species_to_info_hash,
                                         st_report_enum report,
                                         const char *dest_field)
{
    GBDATA *gb_species = GBT_find_species(gb_main, species_name);
    if (!gb_species)
        return;                 // invalid species
    st_cq_info *info =
        (st_cq_info *) GBS_read_hash(species_to_info_hash, species_name);
    if (!info)
        return;
    GBDATA *gb_dest = GB_search(gb_species, dest_field, GB_STRING);
    GB_ERROR error = 0;
    if (!gb_dest) {
        error = GB_get_error();
    } else {
        char buffer[256];
        char *s2 = info->ss2.generate_string();
        char *s5 = info->ss5.generate_string();
        char *su = info->ssu.generate_string();
        snprintf(buffer, 256, "a%s b%s c%s", s2, s5, su);
        delete[]s2;
        delete[]s5;
        error = GB_write_string(gb_dest, buffer);

        if (!error && report) {
            GBDATA *gb_report =
                GBT_add_data(gb_species, alignment_name, "quality",
                             GB_STRING);
            if (!gb_report) {
                error = GB_get_error();
            } else {
                char *rp = new char[seq_len + 1];
                rp[seq_len] = 0;
                int i;
                for (i = 0; i < seq_len; i++) {
                    rp[i] = su[i / bucket_size];
                }
                error = GB_write_string(gb_report, rp);
                if (report == ST_QUALITY_REPORT_TEMP) {
                    GB_set_temporary(gb_report);
                }
                delete rp;
            }
        }
        delete[]su;
    }
    if (error) {
        aw_message(error);
    }
    delete info;
    GBS_write_hash(species_to_info_hash, species_name, 0);

}

GB_ERROR st_ml_check_sequence_quality(GBDATA * gb_main,
                                      const char *tree_name,
                                      const char *alignment_name,
                                      AWT_csp * awt_csp,
                                      int bucket_size, int marked_only,
                                      st_report_enum report,
                                      const char *filter_string,
                                      const char *dest_field)
{
    AP_filter filter;
    int seq_len = GBT_get_alignment_len(gb_main, alignment_name);
    filter.init(filter_string, "0 ", seq_len);
    ST_ML st_ml(gb_main);
    GB_ERROR error = st_ml.init(tree_name, alignment_name,
                                0, marked_only,
                                filter_string, awt_csp);
    if (error) {
        return error;
    }

    GB_HASH *species_to_info_hash =
        GBS_create_hash(GBT_get_species_count(gb_main), 1);
    GB_CSTR *snames =
        GBT_get_species_names_of_tree((GBT_TREE *) st_ml.tree_root->tree);

    int pos;
    aw_openstatus("Sequence Quality Check");
    for (pos = 0; pos < seq_len; pos += ST_MAX_SEQ_PART) {
        int end = pos + ST_MAX_SEQ_PART - 1;
        if (end > seq_len)
            end = seq_len;
        if (aw_status(pos / double (seq_len))) {
            return "aborted";
        }
        const char **pspecies_name;
        for (pspecies_name = snames; *pspecies_name; pspecies_name++) {
            st_ml_add_sequence_part_to_stat(&st_ml, awt_csp,
                                            *pspecies_name,
                                            seq_len, bucket_size,
                                            species_to_info_hash, pos,
                                            end);
        }
    }
    aw_status("Generating Result String");
    const char **pspecies_name;
    for (pspecies_name = snames; *pspecies_name; pspecies_name++) {
        st_ml_add_quality_string_to_species(gb_main, alignment_name,
                                            *pspecies_name,
                                            seq_len, bucket_size,
                                            species_to_info_hash,
                                            report, dest_field);
    }
    aw_closestatus();
    free(snames);
    GBS_free_hash(species_to_info_hash);
    return NULL;
}
