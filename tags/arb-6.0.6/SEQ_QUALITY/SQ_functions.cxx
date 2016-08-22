//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.cxx                                       //
//    Purpose   : Implementation of SQ_functions.h                       //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include "SQ_ambiguities.h"
#include "SQ_helix.h"
#include "SQ_physical_layout.h"
#include "SQ_functions.h"

#include <aw_preset.hxx>
#include <arb_progress.h>
#include <arbdbt.h>

using namespace std;

typedef GBDATA *(*species_iterator)(GBDATA *);

static SQ_GroupDataDictionary group_dict;

enum {
    CS_CLEAR, CS_PASS1
};

void SQ_clear_group_dictionary() {
    SQ_GroupDataDictionary tmp;
    swap(tmp, group_dict);
}

static GB_ERROR no_data_error(GBDATA * gb_species, const char *ali_name) {
    GBDATA *gb_name = GB_entry(gb_species, "name");
    const char *name = "<unknown>";
    if (gb_name)
        name = GB_read_char_pntr(gb_name);
    return GBS_global_string("Species '%s' has no data in alignment '%s'",
            name, ali_name);
}

static int sq_round(double value) {
    int x;

    value += 0.5;
    x = (int) floor(value);
    return x;
}

GB_ERROR SQ_remove_quality_entries(GBDATA *gb_main) {
    GB_push_transaction(gb_main);
    GB_ERROR error = NULL;

    for (GBDATA *gb_species = GBT_first_species(gb_main); gb_species && !error; gb_species
    = GBT_next_species(gb_species)) {
        GBDATA *gb_quality = GB_search(gb_species, "quality", GB_FIND);
        if (gb_quality)
            error = GB_delete(gb_quality);
    }
    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR SQ_evaluate(GBDATA * gb_main, const SQ_weights & weights, bool marked_only) {
    GB_push_transaction(gb_main);
    char *alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    species_iterator getFirst = marked_only ? GBT_first_marked_species : GBT_first_species;
    species_iterator getNext  = marked_only ? GBT_next_marked_species  : GBT_next_species;

    GB_ERROR error = 0;
    for (GBDATA *gb_species = getFirst(gb_main); gb_species && !error; gb_species = getNext(gb_species)) {
        GBDATA *gb_name = GB_entry(gb_species, "name");
        if (!gb_name) {
            error = GB_get_error();
        }
        else {
            GBDATA *gb_quality = GB_entry(gb_species, "quality");
            if (gb_quality) {
                GBDATA *gb_quality_ali = GB_entry(gb_quality, alignment_name);

                if (!gb_quality_ali) {
                    error = GBS_global_string("No alignment entry '%s' in quality data", alignment_name);
                }
                else {

                    // evaluate the percentage of bases the actual sequence consists of
                    GBDATA *gb_result1 = GB_search(gb_quality_ali, "percent_of_bases", GB_INT);
                    int     bases      = GB_read_int(gb_result1);
                    
                    double result = bases<4 ? 0 : (bases<6 ? 1 : 2);

                    if (result != 0) result = (result * weights.bases) / 2;

                    double value = 0;
                    value += result;

                    // evaluate the difference in number of bases from sequence to group
                    GBDATA *gb_result2 = GB_search(gb_quality_ali, "percent_base_deviation", GB_INT);
                    int     dfa        = GB_read_int(gb_result2);
                    if (abs(dfa) < 2) {
                        result = 5;
                    }
                    else {
                        if (abs(dfa) < 4) {
                            result = 4;
                        }
                        else {
                            if (abs(dfa) < 6) {
                                result = 3;
                            }
                            else {
                                if (abs(dfa) < 8) {
                                    result = 2;
                                }
                                else {
                                    if (abs(dfa) < 10) {
                                        result = 1;
                                    }
                                    else {
                                        result = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (result != 0) result = (result * weights.diff_from_average) / 5;
                    value += result;

                    // evaluate the number of positions where no helix can be built
                    GBDATA *gb_result3 = GB_search(gb_quality_ali, "number_of_no_helix", GB_INT);
                    int     noh        = GB_read_int(gb_result3);
                    if (noh < 20) {
                        result = 5;
                    }
                    else {
                        if (noh < 50) {
                            result = 4;
                        }
                        else {
                            if (noh < 125) {
                                result = 3;
                            }
                            else {
                                if (noh < 250) {
                                    result = 2;
                                }
                                else {
                                    if (noh < 500) {
                                        result = 1;
                                    }
                                    else {
                                        result = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (result != 0) result = (result * weights.helix) / 5;
                    value += result;

                    // evaluate the consensus
                    GBDATA *gb_result4 = GB_search(gb_quality_ali, "consensus_evaluated", GB_INT);
                    int     cos        = GB_read_int(gb_result4);

                    result                   = cos;
                    if (result != 0) result  = (result * weights.consensus) / 12;
                    value                   += result;

                    // evaluate the number of iupacs in a sequence
                    GBDATA *gb_result5 = GB_search(gb_quality_ali, "iupac_value", GB_INT);
                    int     iupv       = GB_read_int(gb_result5);
                    
                    if (iupv < 1) {
                        result = 3;
                    }
                    else {
                        if (iupv < 5) {
                            result = 2;
                        }
                        else {
                            if (iupv < 10) {
                                result = 1;
                            }
                            else {
                                result = 0;
                            }
                        }
                    }
                    if (result != 0) result = (result * weights.iupac) / 3;
                    value += result;

                    // evaluate the difference in the GC proportion from sequence to group
                    GBDATA *gb_result6 = GB_search(gb_quality_ali, "percent_GC_difference", GB_INT);
                    int     gcprop     = GB_read_int(gb_result6);
                    
                    if (abs(gcprop) < 1) {
                        result = 5;
                    }
                    else {
                        if (abs(gcprop) < 2) {
                            result = 4;
                        }
                        else {
                            if (abs(gcprop) < 4) {
                                result = 3;
                            }
                            else {
                                if (abs(gcprop) < 8) {
                                    result = 2;
                                }
                                else {
                                    if (abs(gcprop) < 16)
                                        result = 1;
                                    else {
                                        result = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (result != 0) result = (result * weights.gc) / 5;
                    value += result;

                    // write the final value of the evaluation
                    int     value2     = sq_round(value);
                    GBDATA *gb_result7 = GB_search(gb_quality_ali, "evaluation", GB_INT);
                    seq_assert(gb_result7);
                    GB_write_int(gb_result7, value2);
                }
            }
        }
    }
    free(alignment_name);

    error = GB_end_transaction(gb_main, error);
    return error;
}

static char *SQ_fetch_filtered_sequence(GBDATA * read_sequence, AP_filter * filter) {
    char *filteredSequence = 0;
    if (read_sequence) {
        const char   *rawSequence        = GB_read_char_pntr(read_sequence);
        int           filteredLength     = filter->get_filtered_length();
        const size_t *filterpos_2_seqpos = filter->get_filterpos_2_seqpos();

        filteredSequence = (char*)malloc(filteredLength * sizeof(char));
        if (filteredSequence) {
            for (int i = 0; i < filteredLength; ++i) {
                filteredSequence[i] = rawSequence[filterpos_2_seqpos[i]];
            }
        }
    }
    return filteredSequence;
}

static GB_ERROR SQ_pass1(SQ_GroupData * globalData, GBDATA * gb_main, GBT_TREE * node, AP_filter * filter) {
    GB_push_transaction(gb_main);

    GB_ERROR  error          = 0;
    char     *alignment_name = GBT_get_default_alignment(gb_main); seq_assert(alignment_name);
    GBDATA   *gb_species     = node->gb_node;
    GBDATA   *gb_name        = GB_entry(gb_species, "name");

    if (!gb_name) error = GB_get_error();
    else {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

        if (!gb_ali) {
            error = no_data_error(gb_species, alignment_name);
        }
        else {
            GBDATA *gb_quality = GB_search(gb_species, "quality", GB_CREATE_CONTAINER);

            if (!gb_quality) {
                error = GB_get_error();
            }

            GBDATA *read_sequence  = GB_entry(gb_ali, "data");
            GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name, GB_CREATE_CONTAINER);
            
            if (!gb_quality_ali) error = GB_get_error();

            // real calculations start here
            if (read_sequence) {
                char *rawSequence = SQ_fetch_filtered_sequence(read_sequence, filter);
                int sequenceLength = filter->get_filtered_length();

                // calculate physical layout of sequence
                {
                    SQ_physical_layout ps_chan;
                    ps_chan.SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality_ali);

                    // calculate the average number of bases in group
                    globalData->SQ_count_sequences();
                    globalData->SQ_set_avg_bases(ps_chan.
                    SQ_get_number_of_bases());
                    globalData->SQ_set_avg_gc(ps_chan.
                    SQ_get_gc_proportion());
                }

                // get values for ambiguities
                {
                    SQ_ambiguities ambi_chan;
                    ambi_chan.SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality_ali);
                }

                // calculate the number of strong, weak and no helixes
                {
                    SQ_helix heli_chan(sequenceLength);
                    heli_chan.SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality_ali, filter);
                }

                // calculate consensus sequence
                {
                    if (!globalData->SQ_is_initialized()) {
                        globalData->SQ_init_consensus(sequenceLength);
                    }
                    globalData->SQ_add_sequence(rawSequence);
                }

                free(rawSequence);
            }
        }
    }

    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

GB_ERROR SQ_pass1_no_tree(SQ_GroupData * globalData, GBDATA * gb_main, AP_filter * filter, arb_progress& progress) {
    GBDATA *read_sequence = 0;

    GB_push_transaction(gb_main);

    char     *alignment_name = GBT_get_default_alignment(gb_main);
    GB_ERROR  error          = 0;

    seq_assert(alignment_name);

    // first pass operations
    for (GBDATA *gb_species = GBT_first_species(gb_main); gb_species && !error; gb_species = GBT_next_species(gb_species)) {
        GBDATA *gb_name = GB_entry(gb_species, "name");

        if (!gb_name) {
            error = GB_get_error();
        }
        else {
            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

            if (!gb_ali) {
                error = no_data_error(gb_species, alignment_name);
            }
            else {
                GBDATA *gb_quality = GB_search(gb_species, "quality", GB_CREATE_CONTAINER);
                if (!gb_quality) {
                    error = GB_get_error();
                }

                read_sequence = GB_entry(gb_ali, "data");

                GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name, GB_CREATE_CONTAINER);
                if (!gb_quality_ali)
                    error = GB_get_error();

                // real calculations start here
                if (read_sequence) {
                    char *rawSequence = SQ_fetch_filtered_sequence(read_sequence, filter);
                    int sequenceLength = filter->get_filtered_length();

                    // calculate physical layout of sequence
                    SQ_physical_layout *ps_chan = new SQ_physical_layout;
                    ps_chan->SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality_ali);

                    // calculate the average number of bases in group
                    globalData->SQ_count_sequences();
                    globalData->SQ_set_avg_bases(ps_chan->SQ_get_number_of_bases());
                    globalData->SQ_set_avg_gc(ps_chan->SQ_get_gc_proportion());
                    delete ps_chan;

                    // get values for ambiguities
                    SQ_ambiguities *ambi_chan = new SQ_ambiguities;
                    ambi_chan->SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality_ali);
                    delete ambi_chan;

                    // calculate the number of strong, weak and no helixes
                    SQ_helix *heli_chan = new SQ_helix(sequenceLength);
                    heli_chan->SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality_ali, filter);
                    delete heli_chan;

                    // calculate consensus sequence
                    {
                        if (!globalData->SQ_is_initialized()) {
                            globalData->SQ_init_consensus(sequenceLength);
                        }
                        globalData->SQ_add_sequence(rawSequence);
                    }
                    delete(rawSequence);
                }
            }
        }
        progress.inc_and_check_user_abort(error);
    }

    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

static GB_ERROR SQ_pass2(const SQ_GroupData * globalData, GBDATA * gb_main, GBT_TREE * node, AP_filter * filter) {
    GB_push_transaction(gb_main);

    char *alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    GBDATA   *gb_species = node->gb_node;
    GBDATA   *gb_name    = GB_entry(gb_species, "name");
    GB_ERROR  error      = 0;

    if (!gb_name) error = GB_get_error();
    else {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

        if (!gb_ali) {
            error = no_data_error(gb_species, alignment_name);
        }
        else {
            GBDATA *gb_quality = GB_search(gb_species, "quality", GB_CREATE_CONTAINER);
            if (!gb_quality) error = GB_get_error();

            GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name, GB_CREATE_CONTAINER);
            if (!gb_quality_ali) error = GB_get_error();

            GBDATA *read_sequence = GB_entry(gb_ali, "data");

            // real calculations start here
            if (read_sequence) {
                char *rawSequence = SQ_fetch_filtered_sequence(read_sequence, filter);

                /*
                  calculate the average number of bases in group, and the difference of
                  a single sequence in group from it
                */
                {
                    GBDATA *gb_result1   = GB_search(gb_quality_ali, "number_of_bases", GB_INT);
                    int     bases        = GB_read_int(gb_result1);
                    int     avg_bases    = globalData->SQ_get_avg_bases();
                    int     diff_percent = 0;

                    if (avg_bases != 0) {
                        double diff  = bases - avg_bases;
                        diff         = (100 * diff) / avg_bases;
                        diff_percent = sq_round(diff);
                    }

                    GBDATA *gb_result2 = GB_search(gb_quality_ali, "percent_base_deviation", GB_INT);
                    seq_assert(gb_result2);
                    GB_write_int(gb_result2, diff_percent);
                }

                /*
                  calculate the average gc proportion in group, and the difference of
                  a single sequence in group from it
                */
                {
                    GBDATA *gb_result6   = GB_search(gb_quality_ali, "GC_proportion", GB_FLOAT);
                    double  gcp          = GB_read_float(gb_result6);
                    double  avg_gc       = globalData->SQ_get_avg_gc();
                    int     diff_percent = 0;

                    if (avg_gc != 0) {
                        double diff  = gcp - avg_gc;
                        diff         = (100 * diff) / avg_gc;
                        diff_percent = sq_round(diff);
                    }

                    GBDATA *gb_result7 = GB_search(gb_quality_ali, "percent_GC_difference", GB_INT);
                    seq_assert(gb_result7);
                    GB_write_int(gb_result7, diff_percent);
                }

                /*
                  get groupnames of visited groups
                  search for name in group dictionary
                  evaluate sequence with group consensus
                */
                GBDATA *gb_con     = GB_search(gb_quality_ali, "consensus_conformity", GB_CREATE_CONTAINER);
                if (!gb_con) error = GB_get_error();

                GBDATA *gb_dev     = GB_search(gb_quality_ali, "consensus_deviation", GB_CREATE_CONTAINER);
                if (!gb_dev) error = GB_get_error();

                GBT_TREE *backup       = node; // needed?
                int       whilecounter = 0;
                double    eval         = 0;

                while (backup->father) {
                    if (backup->name) {
                        SQ_GroupDataDictionary::iterator GDI = group_dict.find(backup->name);
                        if (GDI != group_dict.end()) {
                            SQ_GroupDataPtr GD_ptr = GDI->second;

                            consensus_result cr = GD_ptr->SQ_calc_consensus(rawSequence);

                            double value1 = cr.conformity;
                            double value2 = cr.deviation;
                            int    value3 = GD_ptr->SQ_get_nr_sequences();

                            GBDATA *gb_node_entry = GB_search(gb_con, "name", GB_STRING);
                            seq_assert(gb_node_entry);
                            GB_write_string(gb_node_entry, backup->name);

                            gb_node_entry = GB_search(gb_con, "value", GB_FLOAT); seq_assert(gb_node_entry);
                            GB_write_float(gb_node_entry, value1);

                            gb_node_entry = GB_search(gb_con, "num_species", GB_INT); seq_assert(gb_node_entry);
                            GB_write_int(gb_node_entry, value3);

                            gb_node_entry = GB_search(gb_dev, "name", GB_STRING); seq_assert(gb_node_entry);
                            GB_write_string(gb_node_entry, backup->name);

                            gb_node_entry = GB_search(gb_dev, "value", GB_FLOAT); seq_assert(gb_node_entry);
                            GB_write_float(gb_node_entry, value2);

                            gb_node_entry = GB_search(gb_dev, "num_species", GB_INT); seq_assert(gb_node_entry);
                            GB_write_int(gb_node_entry, value3);

                            // if you parse the upper two values in the evaluate() function cut the following out
                            // for time reasons i do the evaluation here, as i still have the upper two values
                            // -------------cut this-----------------
                            if (value1 > 0.95) {
                                eval += 5;
                            }
                            else {
                                if (value1 > 0.8) {
                                    eval += 4;
                                }
                                else {
                                    if (value1 > 0.6) {
                                        eval += 3;
                                    }
                                    else {
                                        if (value1 > 0.4) {
                                            eval += 2;
                                        }
                                        else {
                                            if (value1 > 0.25) {
                                                eval += 1;
                                            }
                                            else {
                                                eval += 0;
                                            }
                                        }
                                    }
                                }
                            }
                            if (value2 > 0.6) {
                                eval += 0;
                            }
                            else {
                                if (value2 > 0.4) {
                                    eval += 1;
                                }
                                else {
                                    if (value2 > 0.2) {
                                        eval += 2;
                                    }
                                    else {
                                        if (value2 > 0.1) {
                                            eval += 3;
                                        }
                                        else {
                                            if (value2 > 0.05) {
                                                eval += 4;
                                            }
                                            else {
                                                if (value2 > 0.025) {
                                                    eval += 5;
                                                }
                                                else {
                                                    if (value2 > 0.01) {
                                                        eval += 6;
                                                    }
                                                    else {
                                                        eval += 7;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            whilecounter++;
                            // ---------to this and scroll down--------
                        }
                    }
                    backup = backup->father;
                }

                // --------also cut this------
                int evaluation = 0;
                if (eval != 0) {
                    eval = eval / whilecounter;
                    evaluation = sq_round(eval);
                }
                GBDATA *gb_result5 = GB_search(gb_quality_ali, "consensus_evaluated", GB_INT);
                seq_assert(gb_result5);
                GB_write_int(gb_result5, evaluation);
                // --------end cut this-------

                free(rawSequence);
            }
        }
    }

    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

GB_ERROR SQ_pass2_no_tree(const SQ_GroupData * globalData, GBDATA * gb_main, AP_filter * filter, arb_progress& progress) {
    GBDATA *read_sequence = 0;

    GB_push_transaction(gb_main);

    char *alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    // second pass operations
    GB_ERROR error = 0;
    for (GBDATA *gb_species = GBT_first_species(gb_main); gb_species && !error; gb_species = GBT_next_species(gb_species)) {
        GBDATA *gb_name = GB_entry(gb_species, "name");

        if (!gb_name) {
            error = GB_get_error();
        }
        else {
            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);
            if (!gb_ali) {
                error = no_data_error(gb_species, alignment_name);
            }
            else {
                GBDATA *gb_quality = GB_search(gb_species, "quality", GB_CREATE_CONTAINER);
                if (!gb_quality) error = GB_get_error();

                GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name, GB_CREATE_CONTAINER);
                if (!gb_quality_ali) error = GB_get_error();

                read_sequence = GB_entry(gb_ali, "data");

                // real calculations start here
                if (read_sequence) {
                    const char *rawSequence = SQ_fetch_filtered_sequence(read_sequence, filter);

                    /*
                      calculate the average number of bases in group, and the difference of
                      a single sequence in group from it
                    */
                    {
                        GBDATA *gb_result1   = GB_search(gb_quality_ali, "number_of_bases", GB_INT);
                        int     bases        = GB_read_int(gb_result1);
                        int     avg_bases    = globalData->SQ_get_avg_bases();
                        int     diff_percent = 0;

                        if (avg_bases != 0) {
                            double diff  = bases - avg_bases;
                            diff         = (100 * diff) / avg_bases;
                            diff_percent = sq_round(diff);
                        }

                        GBDATA *gb_result2 = GB_search(gb_quality_ali, "percent_base_deviation", GB_INT);
                        seq_assert(gb_result2);
                        GB_write_int(gb_result2, diff_percent);
                    }

                    /*
                     calculate the average gc proportion in group, and the difference of
                     a single sequence in group from it
                    */
                    {
                        GBDATA *gb_result6   = GB_search(gb_quality_ali, "GC_proportion", GB_FLOAT);
                        double  gcp          = GB_read_float(gb_result6);
                        double  avg_gc       = globalData->SQ_get_avg_gc();
                        int     diff_percent = 0;

                        if (avg_gc != 0) {
                            double diff  = gcp - avg_gc;
                            diff         = (100 * diff) / avg_gc;
                            diff_percent = sq_round(diff);
                        }

                        GBDATA *gb_result7 = GB_search(gb_quality_ali, "percent_GC_difference", GB_INT);
                        seq_assert(gb_result7);
                        GB_write_int(gb_result7, diff_percent);
                    }
                    /*
                      get groupnames of visited groups
                      search for name in group dictionary
                      evaluate sequence with group consensus
                    */
                    GBDATA *gb_con     = GB_search(gb_quality_ali, "consensus_conformity", GB_CREATE_CONTAINER);
                    if (!gb_con) error = GB_get_error();

                    GBDATA *gb_dev     = GB_search(gb_quality_ali, "consensus_deviation", GB_CREATE_CONTAINER);
                    if (!gb_dev) error = GB_get_error();

                    consensus_result cr = globalData->SQ_calc_consensus(rawSequence);

                    double value1 = cr.conformity;
                    double value2 = cr.deviation;
                    int    value3 = globalData->SQ_get_nr_sequences();

                    GBDATA *gb_node_entry = GB_search(gb_con, "name", GB_STRING);
                    seq_assert(gb_node_entry);
                    GB_write_string(gb_node_entry, "one global consensus");

                    gb_node_entry = GB_search(gb_con, "value", GB_FLOAT); seq_assert(gb_node_entry);
                    GB_write_float(gb_node_entry, value1);

                    gb_node_entry = GB_search(gb_con, "num_species", GB_INT); seq_assert(gb_node_entry);
                    GB_write_int(gb_node_entry, value3);

                    gb_node_entry = GB_search(gb_dev, "name", GB_STRING); seq_assert(gb_node_entry);
                    GB_write_string(gb_node_entry, "one global consensus");

                    gb_node_entry = GB_search(gb_dev, "value", GB_FLOAT); seq_assert(gb_node_entry);
                    GB_write_float(gb_node_entry, value2);

                    gb_node_entry = GB_search(gb_dev, "num_species", GB_INT); seq_assert(gb_node_entry);
                    GB_write_int(gb_node_entry, value3);

                    double eval = 0;
                    
                    // if you parse the upper two values in the evaluate() function cut the following out
                    // for time reasons i do the evaluation here, as i still have the upper two values
                    // -------------cut this-----------------
                    if (value1 > 0.95) {
                        eval += 5;
                    }
                    else {
                        if (value1 > 0.8) {
                            eval += 4;
                        }
                        else {
                            if (value1 > 0.6) {
                                eval += 3;
                            }
                            else {
                                if (value1 > 0.4) {
                                    eval += 2;
                                }
                                else {
                                    if (value1 > 0.25) {
                                        eval += 1;
                                    }
                                    else {
                                        eval += 0;
                                    }
                                }
                            }
                        }
                    }
                    if (value2 > 0.6) {
                        eval += 0;
                    }
                    else {
                        if (value2 > 0.4) {
                            eval += 1;
                        }
                        else {
                            if (value2 > 0.2) {
                                eval += 2;
                            }
                            else {
                                if (value2 > 0.1) {
                                    eval += 3;
                                }
                                else {
                                    if (value2 > 0.05) {
                                        eval += 4;
                                    }
                                    else {
                                        if (value2 > 0.025) {
                                            eval += 5;
                                        }
                                        else {
                                            if (value2 > 0.01) {
                                                eval += 6;
                                            }
                                            else {
                                                eval += 7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    {
                        int evaluation            = 0;
                        if (eval != 0) evaluation = sq_round(eval);

                        GBDATA *gb_result5 = GB_search(gb_quality_ali, "consensus_evaluated", GB_INT);
                        seq_assert(gb_result5);
                        GB_write_int(gb_result5, evaluation);
                    }
                    // --------end cut this-------
                    delete(rawSequence);
                }
            }
        }
        progress.inc_and_check_user_abort(error);
    }
    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

int SQ_count_nodes(GBT_TREE *node) {
    // calculate number of nodes in tree
    return GBT_count_leafs(node)*2-1;
}

static void create_multi_level_consensus(GBT_TREE * node, SQ_GroupData * data) {
    SQ_GroupData *newData = data->clone(); // save actual consensus
    *newData = *data;
    group_dict[node->name] = newData; // and link it with an name
}

void SQ_calc_and_apply_group_data(GBT_TREE * node, GBDATA * gb_main, SQ_GroupData * data, AP_filter * filter, arb_progress& progress) {
    if (node->is_leaf) {
        if (node->gb_node) {
            SQ_pass1(data, gb_main, node, filter);
            seq_assert(data->getSize()> 0);
        }
    }
    else {
        GBT_TREE *node1 = node->leftson;
        GBT_TREE *node2 = node->rightson;

        if (node->name) {
            SQ_GroupData *leftData      = NULL;
            bool          parentIsEmpty = false;

            if (data->getSize() == 0) {
                parentIsEmpty = true;
                SQ_calc_and_apply_group_data(node1, gb_main, data, filter, progress); // process left branch with empty data
                seq_assert(data->getSize()> 0);
            }
            else {
                leftData = data->clone(); // create new empty SQ_GroupData
                SQ_calc_and_apply_group_data(node1, gb_main, leftData, filter, progress); // process left branch
                seq_assert(leftData->getSize()> 0);
            }

            SQ_GroupData *rightData = data->clone(); // create new empty SQ_GroupData
            SQ_calc_and_apply_group_data(node2, gb_main, rightData, filter, progress); // process right branch
            seq_assert(rightData->getSize()> 0);

            if (!parentIsEmpty) {
                data->SQ_add(*leftData);
                delete leftData;
            }

            data->SQ_add(*rightData);
            delete rightData;

            create_multi_level_consensus(node, data);
        }
        else {
            SQ_calc_and_apply_group_data(node1, gb_main, data, filter, progress); // enter left branch
            seq_assert(data->getSize()> 0);

            SQ_calc_and_apply_group_data(node2, gb_main, data, filter, progress); // enter right branch
            seq_assert(data->getSize()> 0);
        }
    }
    progress.inc();
}

void SQ_calc_and_apply_group_data2(GBT_TREE * node, GBDATA * gb_main, const SQ_GroupData * data, AP_filter * filter, arb_progress& progress) {
    if (node->is_leaf) {
        if (node->gb_node) {
            SQ_pass2(data, gb_main, node, filter);
        }
    }
    else {
        GBT_TREE *node1 = node->leftson;
        GBT_TREE *node2 = node->rightson;

        if (node1) SQ_calc_and_apply_group_data2(node1, gb_main, data, filter, progress);
        if (node2) SQ_calc_and_apply_group_data2(node2, gb_main, data, filter, progress);
    }
    progress.inc();
}

// marks species that are below threshold "evaluation"
GB_ERROR SQ_mark_species(GBDATA * gb_main, int condition, bool marked_only) {
    char *alignment_name;
    int result = 0;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    alignment_name = GBT_get_default_alignment(gb_main); seq_assert(alignment_name);

    species_iterator  getFirst      = 0;
    species_iterator  getNext       = 0;

    if (marked_only) {
        getFirst = GBT_first_marked_species;
        getNext = GBT_next_marked_species;
    }
    else {
        getFirst = GBT_first_species;
        getNext = GBT_next_species;
    }

    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species)) {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);
        bool marked = false;
        if (gb_ali) {
            GBDATA *gb_quality = GB_search(gb_species, "quality", GB_CREATE_CONTAINER);
            if (gb_quality) {
                read_sequence = GB_entry(gb_ali, "data");
                if (read_sequence) {
                    GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name, GB_CREATE_CONTAINER);
                    if (gb_quality_ali) {
                        GBDATA *gb_result1 = GB_search(gb_quality_ali, "evaluation", GB_INT);
                        result = GB_read_int(gb_result1);

                        if (result < condition) marked = true;
                    }
                }
            }
        }

        if (GB_read_flag(gb_species) != marked) {
            GB_write_flag(gb_species, marked);
        }
    }
    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

SQ_TREE_ERROR SQ_check_tree_structure(GBT_TREE * node) {
    SQ_TREE_ERROR retval = NONE;

    if (!node)
        return MISSING_NODE;

    if (node->is_leaf) {
        if (!node->gb_node)
            retval = ZOMBIE;
    }
    else {
        retval = SQ_check_tree_structure(node->leftson);
        if (retval == NONE)
            retval = SQ_check_tree_structure(node->rightson);
    }

    return retval;
}
