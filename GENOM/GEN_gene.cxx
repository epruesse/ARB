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
#include "GEN_local.hxx"

// Standard fields of a gb_gene entry:
// -----------------------------------
// name          = short name of gene (unique in one species)
// pos_begin     = start-position of gene
// pos_end       = end-position of gene
// pos_uncertain = contains 2 chars (1. for start-pos, 2. for end-pos); = means 'pos is exact'; < means 'pos may be lower'; > means 'pos may be higher'; missing -> like ==
// complement    = 1 -> encoding from right to left
//
// fields for splitted genes:
// --------------------------
// pos_joined               = xxx -> gene consists of xxx parts (may not exist if xxx == 1)
// pos_beginxxx, pos_endxxx = start-/end-positions for parts 2...n
// pos_uncertainxxx         = like above for parts 2...n
//
// fields used for display:
// ------------------------
// display_hidden           = 1 -> do not display this gene

using namespace std;

//  ----------------------------------------------------------
//      GEN_gene::init(GBDATA *gb_gene_, GEN_root *root_)
//  ----------------------------------------------------------
void GEN_gene::init(GBDATA *gb_gene_, GEN_root *root_) {
    gb_gene = gb_gene_;
    root    = root_;

    GBDATA *gbd = GB_find(gb_gene, "name", 0, down_level);
    name        = string(gbd ? GB_read_char_pntr(gbd) : "<noGeneName>");

    gbd        = GB_find(gb_gene, "complement", 0, down_level);
    complement = gbd ? GB_read_byte(gbd) == 1 : false;

}
//  ------------------------------------------------
//      void GEN_gene::load_positions(int part)
//  ------------------------------------------------
void GEN_gene::load_positions(int part) {
    GBDATA *gbd;

    if (part == 1) {
        gbd  = GB_find(gb_gene, "pos_begin", 0, down_level);
        pos1 = gbd ? GB_read_int(gbd) : -1L;

        gbd  = GB_find(gb_gene, "pos_end", 0, down_level);
        pos2 = gbd ? GB_read_int(gbd) : -1L;
    }
    else {
        gen_assert(part>1);

        const char *real_field_name = GBS_global_string("pos_begin%i", part);
        gbd                         = GB_find(gb_gene, real_field_name, 0, down_level);
        pos1                        = gbd ? GB_read_int(gbd) : -1L;

        real_field_name = GBS_global_string("pos_end%i", part);
        gbd             = GB_find(gb_gene, real_field_name, 0, down_level);
        pos2            = gbd ? GB_read_int(gbd) : -1L;
    }
}
//  --------------------------------------------------------------
//      GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_)
//  --------------------------------------------------------------
GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_) {
    init(gb_gene_, root_);
    load_positions(1);

    nodeInfo = name;
}

//  --------------------------------------------------------------------------------------------
//      GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_, int partNumber, int maxParts)
//  --------------------------------------------------------------------------------------------
//  partNumber 1..n which part of a splitted gene
//  maxParts   1..n of how many parts consists this gene?
GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_, int partNumber, int maxParts) {
    init(gb_gene_, root_);
    load_positions(partNumber);

    {
        char buffer[30];
        sprintf(buffer, " (%i/%i)", partNumber, maxParts);
        nodeInfo = name+buffer;
    }
}
//  ------------------------------
//      GEN_gene::~GEN_gene()
//  ------------------------------
GEN_gene::~GEN_gene() {
}

//  -----------------------------------------------------------------------------------------------------------------------------
//      GEN_root::GEN_root(const char *species_name_, const char *gene_name_, GBDATA *gb_main_, const char *genom_alignment)
//  -----------------------------------------------------------------------------------------------------------------------------
GEN_root::GEN_root(const char *species_name_, const char *gene_name_, GBDATA *gb_main_, const char *genom_alignment) {
    gb_main      = gb_main_;
    species_name = species_name_;
    gene_name    = gene_name_;
    error_reason = "";
    gb_gene_data = 0;
    change_flag  = 1;

    {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_species = GBT_find_species(gb_main, species_name.c_str());

        if (!gb_species) {
            error_reason = strdup("Please select a species.");
        }
        else {
            GBDATA *gb_data = GBT_read_sequence(gb_species, "ali_genom");
            length          = GB_read_count(gb_data);

            gb_gene_data    = GEN_get_gene_data(gb_species);
            GBDATA *gb_gene = gb_gene_data ? GEN_first_gene_rel_gene_data(gb_gene_data) : 0;

            if (!gb_gene) {
                error_reason = GBS_global_string("Species '%s' has no gene-information", species_name.c_str());
            }
            else {
                while (gb_gene) {
                    GBDATA *gbd = GB_find(gb_gene, "display_hidden", 0, down_level);

                    if (!gbd || !GB_read_byte(gbd)) { // gene is not hidden
                        GBDATA *gbd    = GB_find(gb_gene, "pos_joined", 0, down_level);
                        int     joined = gbd ? GB_read_int(gbd) : 0;

                        if (joined) {
                            for (int j = 1; j <= joined; ++j) { // insert all parts
                                gene_set.insert(GEN_gene(gb_gene, this, j, joined));
                            }
                        }
                        else {
                            gene_set.insert(GEN_gene(gb_gene, this)); // insert normal (unsplitted) gene
                        }
                    }
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


