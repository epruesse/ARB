/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>

#include "GEN.hxx"
#include "GEN_gene.hxx"

using namespace std;

//  --------------------------------------------------------------
//      GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_)
//  --------------------------------------------------------------
GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_) {
    gb_gene = gb_gene_;
    root    = root_;
    level   = 0;

    GBDATA *gbd = GB_find(gb_gene, "name", 0, down_level);
    name        = string(gbd ? GB_read_char_pntr(gbd) : "noGeneName");
    gbd         = GB_find(gb_gene, "pos_begin", 0, down_level);
    pos1        = gbd ? GB_read_int(gbd) : -1L;
    gbd         = GB_find(gb_gene, "pos_end", 0, down_level);
    pos2        = gbd ? GB_read_int(gbd) : -1L;
}

//  ------------------------------
//      GEN_gene::~GEN_gene()
//  ------------------------------
GEN_gene::~GEN_gene() {
}

//  -----------------------------------------------------------------------------------------------------------------------------
//      GEN_root::GEN_root(const char *species_name_, const char *gene_name_, GBDATA *gb_main_, const char *genom_alignment)
//  -----------------------------------------------------------------------------------------------------------------------------
GEN_root::GEN_root(const char *species_name_, const char *gene_name_, GBDATA *gb_main_, const char *genom_alignment, int lines) {
    gb_main      = gb_main_;
    species_name = species_name_;
    gene_name    = gene_name_;
    error_reason = "";
    length       = -1L;
    bp_per_line  = -1L;

    {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_species = GBT_find_species(gb_main, species_name.c_str());

        if (!gb_species) {
            error_reason = strdup("Please select a species.");
        }
        else {
            GBDATA *gb_gene = GEN_first_gene(gb_species);

            if (!gb_gene) {
                error_reason = GBS_global_string("Species '%s' has no gene-information", species_name.c_str());
            }
            else {
                GBDATA *gb_seq = GBT_read_sequence(gb_species, genom_alignment);

                length      = GB_read_count(gb_seq);// get sequence length
                bp_per_line = length/lines;

                while (gb_gene) {
                    gene_set.insert(GEN_gene(gb_gene, this));
                    gb_gene = GEN_next_gene(gb_gene);
                }
            }
        }
    }
}

//  ------------------------------
//      GEN_root::~GEN_root()
//  ------------------------------
GEN_root::~GEN_root() {
}

