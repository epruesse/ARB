// =============================================================== //
//                                                                 //
//   File      : primer_design.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in February 2001                    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "primer_design.hxx"
#include "PRD_Design.hxx"
#include "PRD_SequenceIterator.hxx"

#include <GEN.hxx>

#include <awt_sel_boxes.hxx>
#include <awt_config_manager.hxx>

#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_select.hxx>

#include <arbdbt.h>
#include <adGene.h>

#include <string>
#include <cmath>

using std::string;

static AW_window_simple  *pdrw       = 0;
static AW_selection_list *resultList = 0;

static double get_estimated_memory(AW_root *root) {
    int    bases  = root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->read_int() + root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->read_int();
    int    length = root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int();
    double mem    = bases*length*0.9*(sizeof(Node)+16);
    return mem;
}

static void primer_design_event_update_memory(AW_window *aww) {
    AW_root *root = aww->get_root();
    double   mem  = get_estimated_memory(root);

    if (mem > 1073741824) {
        mem = mem / 1073741824;
        root->awar(AWAR_PRIMER_DESIGN_APROX_MEM)->write_string(GBS_global_string("%.1f TB", mem));
    }
    else if (mem > 1048576) {
        mem = mem / 1048576;
        root->awar(AWAR_PRIMER_DESIGN_APROX_MEM)->write_string(GBS_global_string("%.1f MB", mem));
    }
    else if (mem > 1024) {
        mem = mem / 1024;
        root->awar(AWAR_PRIMER_DESIGN_APROX_MEM)->write_string(GBS_global_string("%.1f KB", mem));
    }
    else {
        root->awar(AWAR_PRIMER_DESIGN_APROX_MEM)->write_string(GBS_global_string("%.0f bytes", mem));
    }
}

void create_primer_design_variables(AW_root *aw_root, AW_default aw_def, AW_default global)
{
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LEFT_POS,                  0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LEFT_LENGTH,             100, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_RIGHT_POS,              1000, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_RIGHT_LENGTH,            100, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MIN,               10, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MAX,               20, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MIN,               1050, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MAX,               1200, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MIN,              10, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MAX,              50, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN,          30, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX,          80, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST,      0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_EXPAND_IUPAC,              1, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_MAX_PAIRS,                25, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GC_FACTOR,                50, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMP_FACTOR,              50, aw_def);

    aw_root->awar_string(AWAR_PRIMER_DESIGN_APROX_MEM,             "", aw_def);

    aw_root->awar_string(AWAR_PRIMER_TARGET_STRING,                0, global);
}

static void create_primer_design_result_window(AW_window *aww) {
    if (!pdrw) {
        pdrw = new AW_window_simple;
        pdrw->init(aww->get_root(), "PRD_RESULT", "Primer Design RESULT");
        pdrw->load_xfig("pd_reslt.fig");

        pdrw->button_length(6);
        pdrw->auto_space(10, 10);

        pdrw->at("help");
        pdrw->callback(makeHelpCallback("primerdesignresult.hlp"));
        pdrw->create_button("HELP", "HELP", "H");

        pdrw->at("result");
        resultList = pdrw->create_selection_list(AWAR_PRIMER_TARGET_STRING, 40, 5, true);

        const StorableSelectionList *storable_primer_list = new StorableSelectionList(TypedSelectionList("prim", resultList, "primers", "primer")); // never freed

        pdrw->at("buttons");

        pdrw->callback((AW_CB0)AW_POPDOWN);
        pdrw->create_button("CLOSE", "CLOSE", "C");

        pdrw->callback(makeWindowCallback(awt_clear_selection_list_cb, resultList));
        pdrw->create_button("CLEAR", "CLEAR", "R");
        
        pdrw->callback(makeCreateWindowCallback(create_load_box_for_selection_lists, storable_primer_list));
        pdrw->create_button("LOAD", "LOAD", "L");

        pdrw->callback(makeCreateWindowCallback(create_save_box_for_selection_lists, storable_primer_list));
        pdrw->create_button("SAVE", "SAVE", "S");

        pdrw->callback(makeWindowCallback(create_print_box_for_selection_lists, &storable_primer_list->get_typedsellist()));
        pdrw->create_button("PRINT", "PRINT", "P");
    }

    pdrw->show();
}

static void primer_design_event_go(AW_window *aww, AW_CL cl_gb_main) {
    AW_root  *root     = aww->get_root();
    GB_ERROR  error    = 0;
    char     *sequence = 0;
    long int  length   = 0;

    if ((get_estimated_memory(root)/1024.0) > GB_get_usable_memory()) {
        if (aw_question(NULL, "ARB may crash due to memory problems.", "Continue, Abort") == 1) {
            return;
        }
    }

    {
        GBDATA         *gb_main          = (GBDATA*)cl_gb_main;
        GB_transaction  ta(gb_main);
        char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA         *gb_species       = GBT_find_species(gb_main, selected_species);

        if (!gb_species) {
            error = "you have to select a species!";
        }
        else {
            const char *alignment = GBT_get_default_alignment(gb_main);
            GBDATA     *gb_seq    = GBT_read_sequence(gb_species, alignment);

            if (!gb_seq) {
                error = GBS_global_string("Selected species has no sequence data in alignment '%s'", alignment);
            }
            else {
                sequence = GB_read_string(gb_seq);
                length   = GB_read_count(gb_seq);
            }
        }
    }

    if (!error) {
        arb_progress progress("Searching PCR primer pairs");
        PrimerDesign *PD =
            new PrimerDesign(sequence, length,
                             Range(root->awar(AWAR_PRIMER_DESIGN_LEFT_POS)->read_int(),  root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_RIGHT_POS)->read_int(), root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MAX)->read_int()),
                             Range(root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX)->read_int()),
                             root->awar(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST)->read_int(),
                             (bool)root->awar(AWAR_PRIMER_DESIGN_EXPAND_IUPAC)->read_int(),
                             root->awar(AWAR_PRIMER_DESIGN_MAX_PAIRS)->read_int(),
                             (float)root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->read_int()/100,
                             (float)root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->read_int()/100
                             );

        try {
            PD->run();
        }

        catch (string& s) {
            error = GBS_static_string(s.c_str());
        }
        catch (...) {
            error = "Unknown error (maybe out of memory ? )";
        }
        if (!error) error = PD->get_error();

        if (!error) {
            if (!resultList) create_primer_design_result_window(aww);
            prd_assert(resultList);

            // create result-list:
            resultList->clear();
            int max_primer_length   = PD->get_max_primer_length();
            int max_position_length = int(std::log10(double (PD->get_max_primer_pos())))+1;
            int max_length_length   = int(std::log10(double(PD->get_max_primer_length())))+1;

            if (max_position_length < 3) max_position_length = 3;
            if (max_length_length   < 3) max_length_length   = 3;

            resultList->insert(GBS_global_string(" Rating | %-*s %-*s %-*s G/C Tmp | %-*s %-*s %-*s G/C Tmp",
                                                 max_primer_length,   "Left primer",
                                                 max_position_length, "Pos",
                                                 max_length_length,   "Len",
                                                 max_primer_length,   "Right primer",
                                                 max_position_length, "Pos",
                                                 max_length_length,   "Len"),
                               "");

            int r;

            for (r = 0; ; ++r) {
                const char *primers = 0;
                const char *result  = PD->get_result(r, primers, max_primer_length, max_position_length, max_length_length);
                if (!result) break;
                resultList->insert(result, primers);
            }

            resultList->insert_default(r ? "**** End of list" : "**** There are no results", "");
            resultList->update();

            pdrw->show();
        }
    }
    if (sequence) free(sequence);
    if (error) aw_message(error);
}


static void primer_design_event_check_temp_factor(AW_window *aww) {
    AW_root *root = aww->get_root();

    int temp = root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->read_int();
    if (temp > 100) temp = 100;
    if (temp <   0) temp =     0;
    root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->write_int(temp);
    root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->write_int(100-temp);
}

static void primer_design_event_check_gc_factor(AW_window *aww) {
    AW_root *root = aww->get_root();

    int gc = root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->read_int();
    if (gc > 100) gc = 100;
    if (gc <   0) gc =     0;
    root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->write_int(gc);
    root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->write_int(100-gc);
}

static void primer_design_event_check_primer_length(AW_window *aww, AW_CL cl_max_changed) {
    AW_root *root        = aww->get_root();
    int      max_changed = int(cl_max_changed);
    int      min_length  = root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int();
    int      max_length  = root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int();

    if (max_length<1) max_length = 1;
    if (min_length<1) min_length = 1;

    if (min_length >= max_length) {
        if (max_changed) min_length = max_length-1;
        else max_length             = min_length+1;
    }

    if (min_length<1) min_length = 1;

    root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->write_int(min_length);
    root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->write_int(max_length);

    primer_design_event_update_memory(aww);
}

static void primer_design_event_init(AW_window *aww, AW_CL cl_gb_main, AW_CL cl_from_gene) {
    bool            from_gene        = bool(cl_from_gene);
    AW_root        *root             = aww->get_root();
    GB_ERROR        error            = 0;
    GBDATA         *gb_main          = (GBDATA*)cl_gb_main;
    GB_transaction  ta(gb_main);
    char           *selected_species = 0;
    char           *selected_gene    = 0;
    GBDATA         *gb_species       = 0;
    GBDATA         *gb_gene          = 0;
    GEN_position   *genPos           = 0;
    GBDATA         *gb_seq           = 0;
    bool            is_genom_db      = GEN_is_genome_db(gb_main, -1);

    if (is_genom_db && from_gene) {
        selected_species = root->awar(AWAR_ORGANISM_NAME)->read_string();
        selected_gene    = root->awar(AWAR_GENE_NAME)->read_string();

        root->awar(AWAR_SPECIES_NAME)->write_string(selected_species);
    }
    else {
        selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
    }

    gb_species = GBT_find_species(gb_main, selected_species);

    if (!gb_species) {
        error = "You have to select a species!";
    }
    else {
        const char *alignment = GBT_get_default_alignment(gb_main);
        gb_seq                = GBT_read_sequence(gb_species, alignment);
        if (!gb_seq) {
            error = GB_export_errorf("Species '%s' has no data in alignment '%s'", selected_species, alignment);
        }
    }

    if (!error && from_gene) {
        prd_assert(is_genom_db); // otherwise button should not exist

        gb_gene = GEN_find_gene(gb_species, selected_gene);
        if (!gb_gene) {
            error = GB_export_errorf("Species '%s' has no gene named '%s'", selected_species, selected_gene);
        }
        else {
            genPos = GEN_read_position(gb_gene);
            if (!genPos) {
                error = GB_await_error();
            }
            else if (!genPos->joinable) {
                error = GBS_global_string("Gene '%s' is not joinable", selected_gene);
            }
        }
    }

    if (!error && gb_seq) {
        SequenceIterator *i          = 0;
        PRD_Sequence_Pos  length     = 0;
        PRD_Sequence_Pos  add_offset = 0;           // offset to add to positions (used for genes)
        char             *sequence   = 0;

        if (gb_gene) {
            size_t gene_length;
            sequence   = GBT_read_gene_sequence_and_length(gb_gene, false, 0, &gene_length);
            if (!sequence) {
                error = GB_await_error();
            }
            else {
                length     = gene_length;
                add_offset = genPos->start_pos[0];
#if defined(WARN_TODO)
#warning does this work with splitted genes ?
#warning warn about uncertainties ?
#endif
            }
        }
        else {
            sequence = GB_read_string(gb_seq);
            length   = strlen(sequence);
        }

        if (!error) {
            long int dist_min;
            long int dist_max;

            // find reasonable parameters
            // left pos (first base from start)

            long int left_len = root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->read_int();
            if (left_len == 0 || left_len<0) left_len = 100;

            i = new SequenceIterator(sequence, 0, SequenceIterator::IGNORE, left_len, SequenceIterator::FORWARD);

            i->nextBase();                                        // find first base from start
            PRD_Sequence_Pos left_min = i->pos;                   // store pos. of first base
            while (i->nextBase() != SequenceIterator::EOS) ;      // step to 'left_len'th base from start
            PRD_Sequence_Pos left_max = i->pos;                   // store pos. of 'left_len'th base

            root->awar(AWAR_PRIMER_DESIGN_LEFT_POS)->write_int(left_min+add_offset);
            root->awar(AWAR_PRIMER_DESIGN_LEFT_LENGTH)->write_int(left_len);

            long int right_len = root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->read_int();
            if (right_len == 0 || right_len<0) right_len = 100;

            // right pos ('right_len'th base from end)
            i->restart(length, 0, right_len, SequenceIterator::BACKWARD);

            i->nextBase();                                   // find last base
            PRD_Sequence_Pos right_max = i->pos;             // store pos. of last base
            while (i->nextBase() != SequenceIterator::EOS) ; // step to 'right_len'th base from end
            PRD_Sequence_Pos right_min = i->pos;             // store pos of 'right_len'th base from end

            root->awar(AWAR_PRIMER_DESIGN_RIGHT_POS)->write_int(right_min+add_offset);
            root->awar(AWAR_PRIMER_DESIGN_RIGHT_LENGTH)->write_int(right_len);

            // primer distance
            if (right_min >= left_max) {                                    // non-overlapping ranges
                i->restart(left_max, right_min, SequenceIterator::IGNORE, SequenceIterator::FORWARD);
                long int bases_between  = 0;                                // count bases from right_min to left_max
                while (i->nextBase()   != SequenceIterator::EOS) ++bases_between;
                dist_min = bases_between;                                   // take bases between as min distance
            }
            else {                                                        // overlapping ranges
                dist_min = right_min - left_min + 1;
            }
            dist_max = right_max - left_min;
            root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->write_int(dist_min);
            root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->write_int(dist_max);

            // update mem-info
            primer_design_event_update_memory(aww);

#if defined(DUMP_PRIMER)
            printf ("primer_design_event_init : left_min   %7li\n", left_min);
            printf ("primer_design_event_init : left_max   %7li\n", left_max);
            printf ("primer_design_event_init : right_min  %7li\n", right_min);
            printf ("primer_design_event_init : right_max  %7li\n", right_max);
            printf ("primer_design_event_init : dist_min   %7li\n", dist_min);
            printf ("primer_design_event_init : dist_max   %7li\n\n", dist_max);
#endif
        }
        delete i;
        free(sequence);
    }

    if (error) {
        aw_message(error);
    }

    GEN_free_position(genPos);
    free(selected_gene);
    free(selected_species);
}

static AWT_config_mapping_def primer_design_config_mapping[] = {
    { AWAR_PRIMER_DESIGN_LEFT_POS,               "lpos" },
    { AWAR_PRIMER_DESIGN_LEFT_LENGTH,            "llen" },
    { AWAR_PRIMER_DESIGN_RIGHT_POS,              "rpos" },
    { AWAR_PRIMER_DESIGN_RIGHT_LENGTH,           "rlen" },
    { AWAR_PRIMER_DESIGN_LENGTH_MIN,             "lenmin" },
    { AWAR_PRIMER_DESIGN_LENGTH_MAX,             "lenmax" },
    { AWAR_PRIMER_DESIGN_DIST_MIN,               "distmin" },
    { AWAR_PRIMER_DESIGN_DIST_MAX,               "distmax" },
    { AWAR_PRIMER_DESIGN_GCRATIO_MIN,            "gcmin" },
    { AWAR_PRIMER_DESIGN_GCRATIO_MAX,            "gcmax" },
    { AWAR_PRIMER_DESIGN_TEMPERATURE_MIN,        "tempmin" },
    { AWAR_PRIMER_DESIGN_TEMPERATURE_MAX,        "tempmax" },
    { AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST, "minmatchdist" },
    { AWAR_PRIMER_DESIGN_EXPAND_IUPAC,           "iupac" },
    { AWAR_PRIMER_DESIGN_MAX_PAIRS,              "maxpairs" },
    { AWAR_PRIMER_DESIGN_GC_FACTOR,              "gcfactor" },
    { AWAR_PRIMER_DESIGN_TEMP_FACTOR,            "temp_factor" },
    { 0, 0 }
};

static char *primer_design_store_config(AW_window *aww, AW_CL,  AW_CL) {
    AWT_config_definition cdef(aww->get_root(), primer_design_config_mapping);
    return cdef.read();
}

static void primer_design_restore_config(AW_window *aww, const char *stored_string, AW_CL,  AW_CL) {
    AWT_config_definition cdef(aww->get_root(), primer_design_config_mapping);
    cdef.write(stored_string);
    primer_design_event_update_memory(aww);
}

AW_window *create_primer_design_window(AW_root *root, GBDATA *gb_main) {
    bool is_genome_db;
    {
        GB_transaction  ta(gb_main);
        char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA         *gb_species       = GBT_find_species(gb_main, selected_species);

        if (!gb_species) {
            aw_message("You have to select a species!");
        }

        is_genome_db = GEN_is_genome_db(gb_main, -1);
    }

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "PRIMER_DESIGN", "PRIMER DESIGN");

    aws->load_xfig("prd_main.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("primer_new.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("init1");
    aws->callback(primer_design_event_init, (AW_CL)gb_main, 0);
    aws->create_button("INIT_FROM_SPECIES", "Species", "I");
    
    if (is_genome_db) {
        aws->at("init2");
        aws->callback(primer_design_event_init, (AW_CL)gb_main, 1);
        aws->create_button("INIT_FROM_GENE", "Gene", "I");
    }

    aws->at("design");
    aws->callback(primer_design_event_go, (AW_CL)gb_main);
    aws->create_button("GO", "GO", "G");
    aws->highlight();

    aws->at("minleft");
    aws->create_input_field(AWAR_PRIMER_DESIGN_LEFT_POS, 7);
    
    aws->at("maxleft");
    aws->callback(primer_design_event_update_memory);
    aws->create_input_field(AWAR_PRIMER_DESIGN_LEFT_LENGTH, 9);

    aws->at("minright");
    aws->create_input_field(AWAR_PRIMER_DESIGN_RIGHT_POS, 7);
    
    aws->at("maxright");
    aws->callback(primer_design_event_update_memory);
    aws->create_input_field(AWAR_PRIMER_DESIGN_RIGHT_LENGTH, 9);

    aws->at("minlen");
    aws->callback(primer_design_event_check_primer_length, 0);
    aws->create_input_field(AWAR_PRIMER_DESIGN_LENGTH_MIN, 7);
    
    aws->at("maxlen");
    aws->callback(primer_design_event_check_primer_length, 1);
    aws->create_input_field(AWAR_PRIMER_DESIGN_LENGTH_MAX, 7);

    aws->at("mindist");       aws->create_input_field(AWAR_PRIMER_DESIGN_DIST_MIN,               7);
    aws->at("maxdist");       aws->create_input_field(AWAR_PRIMER_DESIGN_DIST_MAX,               7);
    aws->at("mingc");         aws->create_input_field(AWAR_PRIMER_DESIGN_GCRATIO_MIN,            7);
    aws->at("maxgc");         aws->create_input_field(AWAR_PRIMER_DESIGN_GCRATIO_MAX,            7);
    aws->at("mintemp");       aws->create_input_field(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN,        7);
    aws->at("maxtemp");       aws->create_input_field(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX,        7);
    aws->at("allowed_match"); aws->create_input_field(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST, 7);
    aws->at("max_pairs");     aws->create_input_field(AWAR_PRIMER_DESIGN_MAX_PAIRS,              7);
    
    aws->at("expand_IUPAC");
    aws->create_toggle(AWAR_PRIMER_DESIGN_EXPAND_IUPAC);

    aws->at("GC_factor");
    aws->callback(primer_design_event_check_gc_factor);
    aws->create_input_field(AWAR_PRIMER_DESIGN_GC_FACTOR, 7);
    
    aws->at("temp_factor");
    aws->callback(primer_design_event_check_temp_factor);
    aws->create_input_field(AWAR_PRIMER_DESIGN_TEMP_FACTOR, 7);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "pcr_primer_design", primer_design_store_config, primer_design_restore_config, 0, 0);

    aws->at("aprox_mem");
    aws->create_input_field(AWAR_PRIMER_DESIGN_APROX_MEM, 11);

    return aws;
}
