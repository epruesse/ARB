#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include "primer_design.hxx"
#include "PRD_Design.hxx"

extern GBDATA *gb_main;

void create_primer_design_variables(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_LEFT_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_LEFT_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_RIGHT_MIN , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_RIGHT_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MIN , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_USE , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MIN , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MIN , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_EXPAND_IUPAC , 0, aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_MAX_PAIRS , 0, aw_def);
    aw_root->awar_float(AWAR_PRIMER_DESIGN_GC_FACTOR , 0, aw_def);
    aw_root->awar_float(AWAR_PRIMER_DESIGN_TEMP_FACTOR , 0, aw_def);
}

void probe_design_event(AW_window *aww) {
    AW_root    *root = aww->get_root();
    char *sequence;
    {
        GB_transaction  dummy(gb_main);
        char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA         *gb_species       = GBT_find_species(gb_main,selected_species);
        const char     *alignment        = GBT_get_default_alignment(gb_main);
        GBDATA         *gb_seq           = GBT_read_sequence(gb_species, alignment);

        sequence = GB_read_string(gb_seq);
    }

    int dist_min = root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->read_int();
    int dist_max = root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->read_int();

    if (root->awar(AWAR_PRIMER_DESIGN_DIST_USE)->read_int() == 0)
    {
        dist_min = dist_max = -1;
    }

    PrimerDesign *PD = new PrimerDesign(sequence,
                                        Range(root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MAX)->read_int()),
                                        Range(root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MAX)->read_int()),
                                        Range(root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int()),
                                        Range(dist_min, dist_max),
                                        Range(root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MAX)->read_int()),
                                        Range(root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX)->read_int()),
                                        root->awar(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST)->read_int(),
                                        (bool)root->awar(AWAR_PRIMER_DESIGN_EXPAND_IUPAC)->read_int(),
                                        root->awar(AWAR_PRIMER_DESIGN_MAX_PAIRS)->read_int(),
                                        root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->read_float(),
                                        root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->read_float()
                                        );
    free(sequence);
}

AW_window *create_primer_design_window( AW_root *root,AW_default def)  {
    AW_window *create_probe_design_window( AW_root *root,AW_default def)  {
    AWUSE(def);
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "PRIMER_DESIGN","PRIMER DESIGN", 10, 10 );

    aws->load_xfig("prd_main.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"primer_new.hlp");
    aws->create_button("HELP","HELP","H");

    aws->callback( primer_design_event);
    aws->at("design");
    aws->highlight();
    aws->create_button("GO","GO","G");

    aws->at("minleft"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_LEFT_MIN, 7);
    aws->at("maxleft"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_LEFT_MAX, 7);

    aws->at("minright"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_RIGHT_MIN, 7);
    aws->at("maxright"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_RIGHT_MAX, 7);

    aws->at("minlen"); aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MIN, 7);
    aws->at("maxlen"); aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MAX, 7);

    aws->at("usedist"); aws->create_toggle(AWAR_PRIMER_DESIGN_DIST_USE);
    aws->at("mindist"); aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MIN, 7);
    aws->at("maxdist"); aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MAX, 7);

    aws->at("mingc"); aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MIN, 7);
    aws->at("maxgc"); aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MAX, 7);

    aws->at("mintemp"); aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MIN, 7);
    aws->at("maxtemp"); aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MAX, 7);


    aws->at("allowed_match"); aws->create_input_field( AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST, 7);
    aws->at("expand_IUPAC"); aws->create_toggle( AWAR_PRIMER_DESIGN_EXPAND_IUPAC);
    aws->at("max_pairs"); aws->create_input_field( AWAR_PRIMER_DESIGN_MAX_PAIRS, 7);
    aws->at("GC_factor"); aws->create_input_field( AWAR_PRIMER_DESIGN_GC_FACTOR, 7);
    aws->at("temp_factor"); aws->create_input_field( AWAR_PRIMER_DESIGN_TEMP_FACTOR, 7);


}



