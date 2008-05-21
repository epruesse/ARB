//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.cxx                                       //
//    Purpose   : Implementation of SQ_functions.h                       //
//    Time-stamp: <Wed May/21/2008 19:10 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include "arbdb.h"
#include "arbdbt.h"
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>

#include "SQ_GroupData.h"
#include "SQ_ambiguities.h"
#include "SQ_helix.h"
#include "SQ_physical_layout.h"
#include "SQ_functions.h"

using namespace std;

static SQ_GroupDataDictionary group_dict;
static int globalcounter = -1;
static int groupcounter = -1;
static int globalcounter_notree = 0;
static int pass1_counter_notree = 0;
static int pass2_counter_notree = 0;

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

GB_ERROR SQ_reset_quality_calcstate(GBDATA * gb_main) {
    GB_push_transaction(gb_main);

    GB_ERROR error = 0;
    char *alignment_name = GBT_get_default_alignment(gb_main);

    for (GBDATA * gb_species = GBT_first_species(gb_main); gb_species && !error; gb_species
            = GBT_next_species(gb_species)) {
        GBDATA *gb_quality = GB_search(gb_species, "quality",
                GB_CREATE_CONTAINER);
        if (!gb_quality)
            error = GB_get_error();
        else {
            GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                    GB_CREATE_CONTAINER);
            if (!gb_quality_ali) {
                error = no_data_error(gb_quality, alignment_name);
            } else {
                GBDATA *gb_calcstate = GB_search(gb_quality_ali, "calcstate",
                        GB_INT);
                if (!gb_calcstate)
                    error = GB_get_error();
                else {
                    GB_write_int(gb_calcstate, CS_CLEAR); // clear calculation state
                }
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

int SQ_get_value(GBDATA * gb_main, const char *option) {
    int result = 0;
    char *alignment_name;

    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);

    /*marked_only */
    getFirst = GBT_first_marked_species;
    getNext = GBT_next_marked_species;

    for (gb_species = getFirst(gb_main); gb_species; gb_species
            = getNext(gb_species)) {
        gb_name = GB_entry(gb_species, "name");

        if (gb_name) {
            GBDATA *gb_quality = GB_entry(gb_species, "quality");
            if (gb_quality) {
                GBDATA *gb_quality_ali = GB_entry(gb_quality, alignment_name);
                if (gb_quality_ali) {
                    GBDATA *gb_result1 = GB_search(gb_quality_ali, option, GB_INT);
                    result = GB_read_int(gb_result1);
                }
            }
        }
    }
    free(alignment_name);

    GB_pop_transaction(gb_main);
    return result;
}

int SQ_get_value_no_tree(GBDATA * gb_main, const char *option) {
    int result = 0;
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);

    /*marked_only */
    getFirst = GBT_first_marked_species;
    getNext = GBT_next_marked_species;

    for (gb_species = getFirst(gb_main); gb_species; gb_species
            = getNext(gb_species)) {
        gb_name = GB_entry(gb_species, "name");
        if (gb_name) {

            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);
            if (gb_ali)
                read_sequence = GB_entry(gb_ali, "data");

            GBDATA *gb_quality = GB_search(gb_species, "quality",
                    GB_CREATE_CONTAINER);
            if (gb_quality && read_sequence) {
                GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                        GB_CREATE_CONTAINER);
                if (gb_quality_ali) {
                    GBDATA *gb_result1 = GB_search(gb_quality_ali, option,
                            GB_INT);
                    result = GB_read_int(gb_result1);
                }
            }
        }
    }
    free(alignment_name);

    GB_pop_transaction(gb_main);
    return result;
}

GB_ERROR SQ_evaluate(GBDATA * gb_main, const SQ_weights & weights) {
    char *alignment_name;

    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);

    //getFirst = GBT_first_marked_species;
    //getNext = GBT_next_marked_species;

    getFirst = GBT_first_species;
    getNext = GBT_next_species;

    for (gb_species = getFirst(gb_main); gb_species && !error; gb_species
            = getNext(gb_species)) {
        gb_name = GB_entry(gb_species, "name");

        if (!gb_name)
            error = GB_get_error();
        else {
            GBDATA *gb_quality = GB_entry(gb_species, "quality");
            if (gb_quality) {
                GBDATA *gb_quality_ali = GB_entry(gb_quality, alignment_name);

                if (!gb_quality_ali) {
                    error = GBS_global_string("No alignment entry '%s' in quality data", alignment_name);
                }
                else {
                    int    bases  = 0;
                    int    dfa    = 0;
                    int    noh    = 0;
                    int    cos    = 0;
                    int    iupv   = 0;
                    int    gcprop = 0;
                    int    value2 = 0;
                    double value  = 0;
                    double result = 0;

                    //evaluate the percentage of bases the actual sequence consists of
                    GBDATA *gb_result1 = GB_search(gb_quality_ali, "percent_of_bases", GB_INT);
                    bases = GB_read_int(gb_result1);
                    if (bases < 4)
                        result = 0;
                    else {
                        if (bases < 6)
                            result = 1;
                        else {
                            result = 2;
                        }
                    }
                    if (result != 0)
                        result = (result * weights.bases) / 2;
                    value += result;

                    //evaluate the difference in number of bases from sequence to group
                    GBDATA *gb_result2 = GB_search(gb_quality_ali,
                            "percent_base_deviation", GB_INT);
                    dfa = GB_read_int(gb_result2);
                    if (abs(dfa) < 2)
                        result = 5;
                    else {
                        if (abs(dfa) < 4)
                            result = 4;
                        else {
                            if (abs(dfa) < 6)
                                result = 3;
                            else {
                                if (abs(dfa) < 8)
                                    result = 2;
                                else {
                                    if (abs(dfa) < 10)
                                        result = 1;
                                    else {
                                        result = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (result != 0)
                        result = (result * weights.diff_from_average) / 5;
                    value += result;

                    //evaluate the number of positions where no helix can be built
                    GBDATA *gb_result3 = GB_search(gb_quality_ali,
                            "number_of_no_helix", GB_INT);
                    noh = GB_read_int(gb_result3);
                    if (noh < 20)
                        result = 5;
                    else {
                        if (noh < 50)
                            result = 4;
                        else {
                            if (noh < 125)
                                result = 3;
                            else {
                                if (noh < 250)
                                    result = 2;
                                else {
                                    if (noh < 500)
                                        result = 1;
                                    else {
                                        result = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (result != 0)
                        result = (result * weights.helix) / 5;
                    value += result;

                    //evaluate the consensus
                    GBDATA *gb_result4 = GB_search(gb_quality_ali,
                            "consensus_evaluated", GB_INT);
                    cos = GB_read_int(gb_result4);
                    result = cos;
                    if (result != 0)
                        result = (result * weights.consensus) / 12;
                    value += result;

                    //evaluate the number of iupacs in a sequence
                    GBDATA *gb_result5 = GB_search(gb_quality_ali,
                            "iupac_value", GB_INT);
                    iupv = GB_read_int(gb_result5);
                    if (iupv < 1)
                        result = 3;
                    else {
                        if (iupv < 5)
                            result = 2;
                        else {
                            if (iupv < 10)
                                result = 1;
                            else {
                                result = 0;
                            }
                        }
                    }
                    if (result != 0)
                        result = (result * weights.iupac) / 3;
                    value += result;

                    //evaluate the difference in the GC proportion from sequence to group
                    GBDATA *gb_result6 = GB_search(gb_quality_ali,
                            "percent_GC_difference", GB_INT);
                    gcprop = GB_read_int(gb_result6);
                    if (abs(gcprop) < 1)
                        result = 5;
                    else {
                        if (abs(gcprop) < 2)
                            result = 4;
                        else {
                            if (abs(gcprop) < 4)
                                result = 3;
                            else {
                                if (abs(gcprop) < 8)
                                    result = 2;
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
                    if (result != 0)
                        result = (result * weights.gc) / 5;
                    value += result;

                    /*write the final value of the evaluation */
                    value2 = sq_round(value);
                    GBDATA *gb_result7 = GB_search(gb_quality_ali,
                            "evaluation", GB_INT);
                    seq_assert(gb_result7);
                    GB_write_int(gb_result7, value2);
                }
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

char *SQ_fetch_filtered_sequence(GBDATA * read_sequence, AP_filter * filter) {
    if (!read_sequence)
        return 0;

    const char *rawSequence = GB_read_char_pntr(read_sequence);
    int filteredLength = filter->real_len;

    if (filter->filterpos_2_seqpos == 0)
        filter->calc_filter_2_seq();

    char *filteredSequence = (char *) malloc(filteredLength * sizeof(char));

    if (filteredSequence) {
        for (int i = 0; i < filteredLength; ++i) {
            filteredSequence[i] = rawSequence[filter->filterpos_2_seqpos[i]];
        }
    }

    return filteredSequence;
}

GB_ERROR SQ_pass1(SQ_GroupData * globalData, GBDATA * gb_main, GBT_TREE * node,
        AP_filter * filter) {
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);
    gb_species = node->gb_node;
    gb_name = GB_entry(gb_species, "name");

    if (!gb_name)
        error = GB_get_error();
    else {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

        if (!gb_ali) {
            error = no_data_error(gb_species, alignment_name);
        } else {
            GBDATA *gb_quality = GB_search(gb_species, "quality",
                    GB_CREATE_CONTAINER);

            if (!gb_quality) {
                error = GB_get_error();
            }

            read_sequence = GB_entry(gb_ali, "data");

            GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                    GB_CREATE_CONTAINER);
            if (!gb_quality_ali)
                error = GB_get_error();

            /*real calculations start here */
            if (read_sequence) {
                char *rawSequence = SQ_fetch_filtered_sequence(read_sequence,
                        filter);
                int sequenceLength = filter->real_len;

                /*calculate physical layout of sequence */
                {
                    SQ_physical_layout ps_chan;
                    ps_chan.SQ_calc_physical_layout(rawSequence,
                            sequenceLength, gb_quality_ali);

                    /*calculate the average number of bases in group */
                    globalData->SQ_count_sequences();
                    globalData->SQ_set_avg_bases(ps_chan.
                    SQ_get_number_of_bases());
                    globalData->SQ_set_avg_gc(ps_chan.
                    SQ_get_gc_proportion());
                }

                /*get values for ambiguities */
                {
                    SQ_ambiguities ambi_chan;
                    ambi_chan.SQ_count_ambiguities(rawSequence, sequenceLength,
                            gb_quality_ali);
                }

                /*calculate the number of strong, weak and no helixes */
                {
                    SQ_helix heli_chan(sequenceLength);
                    heli_chan.SQ_calc_helix_layout(rawSequence, gb_main,
                            alignment_name, gb_quality_ali, filter);
                }

                /*calculate consensus sequence */
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

    free(alignment_name);

    if (error)
        GB_abort_transaction(gb_main);
    else
        GB_pop_transaction(gb_main);

    return error;
}

GB_ERROR SQ_pass1_no_tree(SQ_GroupData * globalData, GBDATA * gb_main,
        AP_filter * filter) {
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);

    getFirst = GBT_first_species;
    getNext = GBT_next_species;

    /*first pass operations */
    for (gb_species = getFirst(gb_main); gb_species && !error; gb_species
            = getNext(gb_species)) {

        gb_name = GB_entry(gb_species, "name");

        if (!gb_name)
            error = GB_get_error();
        else {
            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

            if (!gb_ali) {
                error = no_data_error(gb_species, alignment_name);
            } else {
                GBDATA *gb_quality = GB_search(gb_species, "quality",
                        GB_CREATE_CONTAINER);
                if (!gb_quality) {
                    error = GB_get_error();
                }

                read_sequence = GB_entry(gb_ali, "data");

                GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                        GB_CREATE_CONTAINER);
                if (!gb_quality_ali)
                    error = GB_get_error();

                /*real calculations start here */
                if (read_sequence) {
                    char *rawSequence = SQ_fetch_filtered_sequence(
                            read_sequence, filter);
                    int sequenceLength = filter->real_len;

                    /*calculate physical layout of sequence */
                    SQ_physical_layout *ps_chan = new SQ_physical_layout();
                    ps_chan->SQ_calc_physical_layout(rawSequence,
                            sequenceLength, gb_quality_ali);

                    /*calculate the average number of bases in group */
                    globalData->SQ_count_sequences();
                    globalData->SQ_set_avg_bases(ps_chan->
                    SQ_get_number_of_bases());
                    globalData->SQ_set_avg_gc(ps_chan->
                    SQ_get_gc_proportion());
                    delete ps_chan;

                    /*get values for ambiguities */
                    SQ_ambiguities *ambi_chan = new SQ_ambiguities();
                    ambi_chan->SQ_count_ambiguities(rawSequence,
                            sequenceLength, gb_quality_ali);
                    delete ambi_chan;

                    /*claculate the number of strong, weak and no helixes */
                    SQ_helix *heli_chan = new SQ_helix(sequenceLength);
                    heli_chan->SQ_calc_helix_layout(rawSequence, gb_main,
                            alignment_name, gb_quality_ali, filter);
                    delete heli_chan;

                    /*calculate consensus sequence */
                    {
                        if (!globalData->SQ_is_initialized()) {
                            globalData->SQ_init_consensus(sequenceLength);
                        }
                        globalData->SQ_add_sequence(rawSequence);
                    }
                    pass1_counter_notree++;
                    aw_status((double) pass1_counter_notree
                            / (double) globalcounter_notree);

                    delete(rawSequence);
                }
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

GB_ERROR SQ_pass2(const SQ_GroupData * globalData, GBDATA * gb_main,
        GBT_TREE * node, AP_filter * filter) {
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);
    gb_species = node->gb_node;
    gb_name = GB_entry(gb_species, "name");

    if (!gb_name)
        error = GB_get_error();
    else {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

        if (!gb_ali) {
            error = no_data_error(gb_species, alignment_name);
        } else {
            GBDATA *gb_quality = GB_search(gb_species, "quality",
                    GB_CREATE_CONTAINER);
            if (!gb_quality)
                error = GB_get_error();

            GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                    GB_CREATE_CONTAINER);
            if (!gb_quality_ali)
                error = GB_get_error();

            read_sequence = GB_entry(gb_ali, "data");

            /*real calculations start here */
            if (read_sequence) {
                double value1 = 0;
                double value2 = 0;
                double eval = 0;
                int value3 = 0;
                int evaluation = 0;
                int bases = 0;
                int avg_bases = 0;
                double diff = 0;
                int diff_percent = 0;
                double avg_gc = 0;
                double gcp = 0;

                const char *rawSequence = SQ_fetch_filtered_sequence(
                        read_sequence, filter);

                /*
                 calculate the average number of bases in group, and the difference of
                 a single seqeunce in group from it
                 */
                GBDATA *gb_result1 = GB_search(gb_quality_ali,
                        "number_of_bases", GB_INT);
                bases = GB_read_int(gb_result1);
                avg_bases = globalData->SQ_get_avg_bases();

                if (avg_bases != 0) {
                    diff = bases - avg_bases;
                    diff = (100 * diff) / avg_bases;
                    diff_percent = sq_round(diff);
                }

                GBDATA *gb_result2 = GB_search(gb_quality_ali,
                        "percent_base_deviation", GB_INT);
                seq_assert(gb_result2);
                GB_write_int(gb_result2, diff_percent);

                /*
                 calculate the average gc proportion in group, and the difference of
                 a single seqeunce in group from it
                 */
                GBDATA *gb_result6 = GB_search(gb_quality_ali, "GC_proportion",
                        GB_FLOAT);
                gcp = GB_read_float(gb_result6);
                avg_gc = globalData->SQ_get_avg_gc();

                if (avg_gc != 0) {
                    diff = gcp - avg_gc;
                    diff = (100 * diff) / avg_gc;
                    diff_percent = sq_round(diff);
                }

                GBDATA *gb_result7 = GB_search(gb_quality_ali,
                        "percent_GC_difference", GB_INT);
                seq_assert(gb_result7);
                GB_write_int(gb_result7, diff_percent);

                /*
                 get groupnames of visited groups
                 search for name in group dictionary
                 evaluate sequence with group consensus
                 */
                GBDATA *gb_con_con = GB_search(gb_quality_ali,
                        "consensus_conformity", GB_CREATE_CONTAINER);
                if (!gb_con_con)
                    error = GB_get_error();
                GBDATA *gb_con_dev = GB_search(gb_quality_ali,
                        "consensus_deviation", GB_CREATE_CONTAINER);
                if (!gb_con_dev)
                    error = GB_get_error();

                GBT_TREE *backup = node; // needed?
                int whilecounter = 0;
                while (backup->father) {
                    if (backup->name) {
                        SQ_GroupDataDictionary::iterator GDI =
                                group_dict.find(backup->name);
                        if (GDI != group_dict.end()) {
                            SQ_GroupDataPtr GD_ptr = GDI->second;

                            consensus_result cr =
                                    GD_ptr->SQ_calc_consensus(rawSequence);
                            value1 = cr.conformity;
                            value2 = cr.deviation;
                            value3 = GD_ptr->SQ_get_nr_sequences();

                            GBDATA *gb_con_node = GB_create_container(
                                    gb_con_con, "node");
                            if (!gb_con_node)
                                error = GB_get_error();

                            GBDATA *gb_dev_node = GB_create_container(
                                    gb_con_dev, "node");
                            if (!gb_dev_node)
                                error = GB_get_error();

                            GBDATA *gb_node_entry = GB_search(gb_con_node,
                                    "name", GB_STRING);
                            seq_assert(gb_node_entry);
                            GB_write_string(gb_node_entry, backup->name);

                            gb_node_entry = GB_search(gb_con_node, "value",
                                    GB_FLOAT);seq_assert(gb_node_entry);
                            GB_write_float(gb_node_entry, value1);

                            gb_node_entry = GB_search(gb_con_node,
                                    "num_species", GB_INT);seq_assert(gb_node_entry);
                            GB_write_int(gb_node_entry, value3);

                            gb_node_entry = GB_search(gb_dev_node, "name",
                                    GB_STRING);seq_assert(gb_node_entry);
                            GB_write_string(gb_node_entry, backup->name);

                            gb_node_entry = GB_search(gb_dev_node, "value",
                                    GB_FLOAT);seq_assert(gb_node_entry);
                            GB_write_float(gb_node_entry, value2);

                            gb_node_entry = GB_search(gb_dev_node,
                                    "num_species", GB_INT);seq_assert(gb_node_entry);
                            GB_write_int(gb_node_entry, value3);

                            //if you parse the upper two values in the evaluate() function cut the following out
                            //for time reasons i do the evaluation here, as i still have the upper two values
                            //-------------cut this-----------------
                            if (value1 > 0.95)
                                eval += 5;
                            else {
                                if (value1 > 0.8)
                                    eval += 4;
                                else {
                                    if (value1 > 0.6)
                                        eval += 3;
                                    else {
                                        if (value1 > 0.4)
                                            eval += 2;
                                        else {
                                            if (value1 > 0.25)
                                                eval += 1;
                                            else {
                                                eval += 0;
                                            }
                                        }
                                    }
                                }
                            }
                            if (value2 > 0.6)
                                eval += 0;
                            else {
                                if (value2 > 0.4)
                                    eval += 1;
                                else {
                                    if (value2 > 0.2)
                                        eval += 2;
                                    else {
                                        if (value2 > 0.1)
                                            eval += 3;
                                        else {
                                            if (value2 > 0.05)
                                                eval += 4;
                                            else {
                                                if (value2 > 0.025)
                                                    eval += 5;
                                                else {
                                                    if (value2 > 0.01)
                                                        eval += 6;
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
                            //---------to this and scroll down--------
                        }
                    }
                    backup = backup->father;
                }

                //--------also cut this------
                if (eval != 0) {
                    eval = eval / whilecounter;
                    evaluation = sq_round(eval);
                }
                GBDATA *gb_result5 = GB_search(gb_quality_ali,
                        "consensus_evaluated", GB_INT);
                seq_assert(gb_result5);
                GB_write_int(gb_result5, evaluation);
                //--------end cut this-------

                delete(rawSequence);
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

GB_ERROR SQ_pass2_no_tree(const SQ_GroupData * globalData, GBDATA * gb_main,
        AP_filter * filter) {
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);
    getFirst = GBT_first_species;
    getNext = GBT_next_species;

    /*second pass operations */
    for (gb_species = getFirst(gb_main); gb_species && !error; gb_species
            = getNext(gb_species)) {
        gb_name = GB_entry(gb_species, "name");

        if (!gb_name)
            error = GB_get_error();
        else {
            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);
            if (!gb_ali) {
                error = no_data_error(gb_species, alignment_name);
            } else {
                GBDATA *gb_quality = GB_search(gb_species, "quality",
                        GB_CREATE_CONTAINER);
                if (!gb_quality)
                    error = GB_get_error();

                GBDATA *gb_quality_ali = GB_search(gb_quality, alignment_name,
                        GB_CREATE_CONTAINER);
                if (!gb_quality_ali)
                    error = GB_get_error();

                read_sequence = GB_entry(gb_ali, "data");

                /*real calculations start here */
                if (read_sequence) {
                    double value1 = 0;
                    double value2 = 0;
                    double eval = 0;
                    int value3 = 0;
                    int evaluation = 0;
                    int bases = 0;
                    int avg_bases = 0;
                    double diff = 0;
                    int diff_percent = 0;
                    double avg_gc = 0;
                    double gcp = 0;

                    const char *rawSequence = SQ_fetch_filtered_sequence(
                            read_sequence, filter);

                    /*
                     calculate the average number of bases in group, and the difference of
                     a single seqeunce in group from it
                     */
                    GBDATA *gb_result1 = GB_search(gb_quality_ali,
                            "number_of_bases", GB_INT);
                    bases = GB_read_int(gb_result1);
                    avg_bases = globalData->SQ_get_avg_bases();

                    if (avg_bases != 0) {
                        diff = bases - avg_bases;
                        diff = (100 * diff) / avg_bases;
                        diff_percent = sq_round(diff);
                    }

                    GBDATA *gb_result2 = GB_search(gb_quality_ali,
                            "percent_base_deviation", GB_INT);
                    seq_assert(gb_result2);
                    GB_write_int(gb_result2, diff_percent);

                    /*
                     calculate the average gc proportion in group, and the difference of
                     a single seqeunce in group from it
                     */
                    GBDATA *gb_result6 = GB_search(gb_quality_ali,
                            "GC_proportion", GB_FLOAT);
                    gcp = GB_read_float(gb_result6);
                    avg_gc = globalData->SQ_get_avg_gc();

                    if (avg_gc != 0) {
                        diff = gcp - avg_gc;
                        diff = (100 * diff) / avg_gc;
                        diff_percent = sq_round(diff);
                    }

                    GBDATA *gb_result7 = GB_search(gb_quality_ali,
                            "percent_GC_difference", GB_INT);
                    seq_assert(gb_result7);
                    GB_write_int(gb_result7, diff_percent);

                    /*
                     get groupnames of visited groups
                     search for name in group dictionary
                     evaluate sequence with group consensus
                     */
                    GBDATA *gb_con_con = GB_search(gb_quality_ali,
                            "consensus_conformity", GB_CREATE_CONTAINER);
                    if (!gb_con_con)
                        error = GB_get_error();
                    GBDATA *gb_con_dev = GB_search(gb_quality_ali,
                            "consensus_deviation", GB_CREATE_CONTAINER);
                    if (!gb_con_dev)
                        error = GB_get_error();

                    consensus_result cr =
                            globalData->SQ_calc_consensus(rawSequence);
                    value1 = cr.conformity;
                    value2 = cr.deviation;
                    value3 = globalData->SQ_get_nr_sequences();

                    GBDATA *gb_con_node = GB_create_container(gb_con_con,
                            "node");
                    if (!gb_con_node)
                        error = GB_get_error();

                    GBDATA *gb_dev_node = GB_create_container(gb_con_dev,
                            "node");
                    if (!gb_dev_node)
                        error = GB_get_error();

                    GBDATA *gb_node_entry = GB_search(gb_con_node, "name",
                            GB_STRING);
                    seq_assert(gb_node_entry);
                    GB_write_string(gb_node_entry, "one global consensus");

                    gb_node_entry = GB_search(gb_con_node, "value", GB_FLOAT);seq_assert(gb_node_entry);
                    GB_write_float(gb_node_entry, value1);

                    gb_node_entry = GB_search(gb_con_node, "num_species",
                            GB_INT);seq_assert(gb_node_entry);
                    GB_write_int(gb_node_entry, value3);

                    gb_node_entry = GB_search(gb_dev_node, "name", GB_STRING);seq_assert(gb_node_entry);
                    GB_write_string(gb_node_entry, "one global consensus");

                    gb_node_entry = GB_search(gb_dev_node, "value", GB_FLOAT);seq_assert(gb_node_entry);
                    GB_write_float(gb_node_entry, value2);

                    gb_node_entry = GB_search(gb_dev_node, "num_species",
                            GB_INT);seq_assert(gb_node_entry);
                    GB_write_int(gb_node_entry, value3);

                    //if you parse the upper two values in the evaluate() function cut the following out
                    //for time reasons i do the evaluation here, as i still have the upper two values
                    //-------------cut this-----------------
                    if (value1 > 0.95)
                        eval += 5;
                    else {
                        if (value1 > 0.8)
                            eval += 4;
                        else {
                            if (value1 > 0.6)
                                eval += 3;
                            else {
                                if (value1 > 0.4)
                                    eval += 2;
                                else {
                                    if (value1 > 0.25)
                                        eval += 1;
                                    else {
                                        eval += 0;
                                    }
                                }
                            }
                        }
                    }
                    if (value2 > 0.6)
                        eval += 0;
                    else {
                        if (value2 > 0.4)
                            eval += 1;
                        else {
                            if (value2 > 0.2)
                                eval += 2;
                            else {
                                if (value2 > 0.1)
                                    eval += 3;
                                else {
                                    if (value2 > 0.05)
                                        eval += 4;
                                    else {
                                        if (value2 > 0.025)
                                            eval += 5;
                                        else {
                                            if (value2 > 0.01)
                                                eval += 6;
                                            else {
                                                eval += 7;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (eval != 0) {
                        evaluation = sq_round(eval);
                    }
                    GBDATA *gb_result5 = GB_search(gb_quality_ali,
                            "consensus_evaluated", GB_INT);
                    seq_assert(gb_result5);
                    GB_write_int(gb_result5, evaluation);
                    //--------end cut this-------
                    pass2_counter_notree++;
                    aw_status((double) pass2_counter_notree
                            / (double) globalcounter_notree);

                    delete(rawSequence);
                }
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

// counts number of named groups in subtree
int SQ_count_nr_of_groups(GBT_TREE * node) {
    if (node->is_leaf)
        return 0;

    return (node->name != 0) + SQ_count_nr_of_groups(node->leftson)
            + SQ_count_nr_of_groups(node->rightson);
}

// counts number of species
GB_ERROR SQ_count_nr_of_species(GBDATA * gb_main) {
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);
    getFirst = GBT_first_species;
    getNext = GBT_next_species;

    for (gb_species = getFirst(gb_main); gb_species && !error; gb_species
            = getNext(gb_species)) {

        gb_name = GB_entry(gb_species, "name");

        if (!gb_name)
            error = GB_get_error();
        else {
            GBDATA *gb_ali = GB_entry(gb_species, alignment_name);

            if (!gb_ali) {
                error = no_data_error(gb_species, alignment_name);
            } else {
                GBDATA *gb_quality = GB_search(gb_species, "quality",
                        GB_CREATE_CONTAINER);
                if (!gb_quality)
                    error = GB_get_error();
                read_sequence = GB_entry(gb_ali, "data");

                if (read_sequence) {
                    globalcounter_notree++;
                }
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

void SQ_reset_counters(GBT_TREE * root) {
    globalcounter = 0;
    groupcounter = SQ_count_nr_of_groups(root);
}

void create_multi_level_consensus(GBT_TREE * node, SQ_GroupData * data) {
    SQ_GroupData *newData = data->clone(); //save actual consensus
    *newData = *data;
    group_dict[node->name] = newData; //and link it with an name
}

void SQ_calc_and_apply_group_data(GBT_TREE * node, GBDATA * gb_main,
        SQ_GroupData * data, AP_filter * filter) {
    if (node->is_leaf) {
        if (node->gb_node) {
            SQ_pass1(data, gb_main, node, filter);seq_assert(data->getSize()> 0);
        }
    } else {
        SQ_GroupData *leftData= NULL;
        SQ_GroupData *rightData= NULL;
        GBT_TREE *node1 = node->leftson;
        GBT_TREE *node2 = node->rightson;
        bool parentIsEmpty = false;

        if (node->name) {
            if (data->getSize() == 0) {
                parentIsEmpty = true;
                SQ_calc_and_apply_group_data(node1, gb_main, data, filter); // process left branch with empty data
                seq_assert(data->getSize()> 0);
            } else {
                leftData = data->clone(); // create new empty SQ_GroupData
                SQ_calc_and_apply_group_data(node1, gb_main, leftData, filter); // process left branch
                seq_assert(leftData->getSize()> 0);
            }

            rightData = data->clone(); // create new empty SQ_GroupData
            SQ_calc_and_apply_group_data(node2, gb_main, rightData, filter); // process right branch
            seq_assert(rightData->getSize()> 0);

            if (!parentIsEmpty) {
                data->SQ_add(*leftData);
                delete leftData;
            }

            data->SQ_add(*rightData);
            delete rightData;

            create_multi_level_consensus(node, data);
            globalcounter++;
            aw_status((double) globalcounter / (double) groupcounter);
        } else {
            SQ_calc_and_apply_group_data(node1, gb_main, data, filter); // enter left branch
            seq_assert(data->getSize()> 0);

            SQ_calc_and_apply_group_data(node2, gb_main, data, filter); // enter right branch
            seq_assert(data->getSize()> 0);
        }
    }
}

void SQ_calc_and_apply_group_data2(GBT_TREE * node, GBDATA * gb_main,
        const SQ_GroupData * data, AP_filter * filter) {
    if (node->is_leaf) {
        if (node->gb_node) {
            SQ_pass2(data, gb_main, node, filter);
        }
    } else {
        GBT_TREE *node1 = node->leftson;
        GBT_TREE *node2 = node->rightson;

        if (node1) {
            SQ_calc_and_apply_group_data2(node1, gb_main, data, filter);
        }
        if (node2) {
            SQ_calc_and_apply_group_data2(node2, gb_main, data, filter);
        }
        if (node->name) { //  group identified
            globalcounter++;
            aw_status((double) globalcounter / (double) groupcounter);
        }
    }
}

//marks species that are below threshold "evaluation"
GB_ERROR SQ_mark_species(GBDATA * gb_main, int condition, bool marked_only) {
    char *alignment_name;
    int result = 0;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);seq_assert(alignment_name);

    GBDATA *(*getFirst)(GBDATA *) = 0;
    GBDATA *(*getNext)(GBDATA *) = 0;

    if (marked_only) {
        getFirst = GBT_first_marked_species;
        getNext = GBT_next_marked_species;
    } else {
        getFirst = GBT_first_species;
        getNext = GBT_next_species;
    }

    for (gb_species = getFirst(gb_main); gb_species; gb_species
            = getNext(gb_species)) {
        GBDATA *gb_ali = GB_entry(gb_species, alignment_name);
        bool marked = false;
        if (gb_ali) {
            GBDATA *gb_quality = GB_search(gb_species, "quality",
                    GB_CREATE_CONTAINER);
            if (gb_quality) {
                read_sequence = GB_entry(gb_ali, "data");
                if (read_sequence) {
                    GBDATA *gb_quality_ali = GB_search(gb_quality,
                            alignment_name, GB_CREATE_CONTAINER);
                    if (gb_quality_ali) {
                        GBDATA *gb_result1 = GB_search(gb_quality_ali,
                                "evaluation", GB_INT);
                        result = GB_read_int(gb_result1);

                        if (result < condition) {
                            marked = true;
                        }
                        pass1_counter_notree++;
                        aw_status((double) pass1_counter_notree
                                / (double) globalcounter_notree);
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
    } else {
        retval = SQ_check_tree_structure(node->leftson);
        if (retval == NONE)
            retval = SQ_check_tree_structure(node->rightson);
    }

    return retval;
}
