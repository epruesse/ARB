// =============================================================== //
//                                                                 //
//   File      : GEN_gene.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "GEN_gene.hxx"
#include "GEN_local.hxx"
#include "GEN_nds.hxx"

#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <adGene.h>

// Standard fields of a gb_gene entry:
// -----------------------------------
// name                  = short name of gene (unique in one species)
// type                  = type of gene (e.g. 'gene', 'CDS', 'tRNA', 'misc_feature')
// pos_start             = start-position(s) of gene(-parts); range is 1...genomeLength
// pos_stop              = end-position(s)   of gene(-parts); range is 1...genomeLength
// pos_certain           = contains pairs of chars (1. for start-pos, 2. for end-pos)
//
//                         '=' means 'pos is exact'
//                         '<' means 'pos may be lower'
//                         '>' means 'pos may be higher'
//                         '+' means 'pos is directly behind'
//                         '-' means 'pos is directly before'
//
//                         if pos_certain is missing -> like '=='
//
// pos_complement        = 1 -> CDS is on opposite strand

// fields for split genes:
// --------------------------
// pos_joined         = xxx -> gene consists of abs(xxx) parts (if missing xxx == 1 is assumed)
//
// if abs(xxx)>1, the gene consists of several parts.
// In that case the fields 'pos_start', 'pos_stop',  'pos_certain' and 'pos_complement'
// contain multiple comma-separated values - one for each joined part.
//
// if xxx is < -1, then joining the parts does not make sense (or nothing is known about it)
//
// Note: Please do not access these fields manually - use GEN_read_position!

// other fields added by importer:
// -------------------------------
//
// During import ARB tries to reproduce existing translations.
// If it succeeds, it removes the translation.
//
// ARB_translation      = written if ARB translation differs from original translation
//                        (original translation is not deleted in this case)
// ARB_translation_note = additional info about failed translation
// ARB_translation_rm   = 1 -> translation was reproduced and deleted
//
// if a gene with type 'gene' exists and another gene with different type, but
// identical location exists as well, ARB sets ARB_display_hidden to 1 for
// the 'gene'. For the other gene with diff. type ARB sets a reference to the
// hidden 'gene':
//
// ARB_is_gene          = shortname of related hidden gene


// fields used for display:
// ------------------------
// ARB_display_hidden = 1 -> do not display this gene (depends on AWAR_GENMAP_SHOW_HIDDEN too)
// ARB_color          = color group



// Old format standard fields of a gb_gene entry:
// ----------------------------------------------
// name          = short name of gene (unique in one species)
// pos_begin     = start-position of gene
// pos_end       = end-position of gene
// pos_uncertain = contains 2 chars (1. for start-pos, 2. for end-pos); = means 'pos is exact'; < means 'pos may be lower'; > means 'pos may be higher'; missing -> like ==
// complement    = 1 -> encoding from right to left
//
// fields for split genes:
// --------------------------
// pos_joined               = xxx -> gene consists of xxx parts (may not exist if xxx == 1)
// pos_beginxxx, pos_endxxx = start-/end-positions for parts 2...n
// pos_uncertainxxx         = like above for parts 2...n
//

using namespace std;

static const GEN_position *loadPositions4gene(GBDATA *gb_gene) {
    static GEN_position *loaded_position     = 0;
    static GBDATA       *positionLoaded4gene = 0;

    if (positionLoaded4gene != gb_gene) {
        if (loaded_position) {
            GEN_free_position(loaded_position);
            loaded_position     = 0;
            positionLoaded4gene = 0;
        }

        if (gb_gene) {
            loaded_position = GEN_read_position(gb_gene);
            if (loaded_position) positionLoaded4gene = gb_gene;
        }
    }
    return loaded_position;
}

void GEN_gene::init() {
    name = GBT_read_name(gb_gene);

    GBDATA *gbd = GB_entry(gb_gene, "complement");
    complement  = gbd ? GB_read_byte(gbd) == 1 : false;
}

void GEN_gene::load_location(int part, const GEN_position *location) {
    gen_assert(part >= 1);
    gen_assert(part <= location->parts);

    pos1       = location->start_pos[part-1];
    pos2       = location->stop_pos[part-1];
    complement = location->complement[part-1];

    gen_assert(pos1 <= pos2);
}

GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_, const GEN_position *location) :
    gb_gene(gb_gene_),
    root(root_)
{
    init();
    load_location(1, location);
    nodeInfo = GEN_make_node_text_nds(root->GbMain(), gb_gene, 0);
}

GEN_gene::GEN_gene(GBDATA *gb_gene_, GEN_root *root_, const GEN_position *location, int partNumber) :
    gb_gene(gb_gene_),
    root(root_)
{
    //  partNumber 1..n which part of a split gene
    //  maxParts   1..n of how many parts consists this gene?

    init();
    load_location(partNumber, location);

    {
        char buffer[30];
        sprintf(buffer, " (%i/%i)", partNumber, location->parts);
        nodeInfo = name+buffer;
    }
}

void GEN_gene::reinit_NDS() const {
    nodeInfo = GEN_make_node_text_nds(root->GbMain(), gb_gene, 0);
}

// ------------------
//      GEN_root

GEN_root::GEN_root(const char *organism_name_, const char *gene_name_, GBDATA *gb_main_, AW_root *aw_root, GEN_graphic *gen_graphic_)
    : gb_main(gb_main_)
    , gen_graphic(gen_graphic_)
    , organism_name(organism_name_)
    , gene_name(gene_name_)
    , error_reason("")
    , length(-1)
    , gb_gene_data(0)
{
    GB_transaction  ta(gb_main);
    GBDATA         *gb_organism = GBT_find_species(gb_main, organism_name.c_str());

    if (!gb_organism) {
        error_reason = ARB_strdup("Please select a species.");
    }
    else {
        GBDATA *gb_data = GBT_find_sequence(gb_organism, GENOM_ALIGNMENT);
        if (!gb_data) {
            error_reason = GBS_global_string_copy("'%s' has no data in '%s'", organism_name.c_str(), GENOM_ALIGNMENT);
        }
        else {
            length = GB_read_count(gb_data);

            gb_gene_data    = GEN_find_gene_data(gb_organism);
            GBDATA *gb_gene = gb_gene_data ? GEN_first_gene_rel_gene_data(gb_gene_data) : 0;

            if (!gb_gene) {
                error_reason = GBS_global_string("Species '%s' has no gene-information", organism_name.c_str());
            }
            else {
                bool show_hidden = aw_root->awar(AWAR_GENMAP_SHOW_HIDDEN)->read_int() != 0;

                while (gb_gene) {
                    bool show_this = show_hidden;

                    if (!show_this) {
                        GBDATA *gbd = GB_entry(gb_gene, ARB_HIDDEN);

                        if (!gbd || !GB_read_byte(gbd)) { // gene is not hidden
                            show_this = true;
                        }
                    }

                    if (show_this) {
                        const GEN_position *location = loadPositions4gene(gb_gene);

                        if (!location) {
                            GB_ERROR  warning = GB_await_error();
                            char     *id      = GEN_global_gene_identifier(gb_gene, gb_organism);
                            aw_message(GBS_global_string("Can't load gene '%s':\nReason: %s", id, warning));
                            free(id);
                        }
                        else {
                            int parts = location->parts;
                            if (parts == 1) {
                                gene_set.insert(GEN_gene(gb_gene, this, location));
                            }
                            else { // joined gene
                                for (int p = 1; p <= parts; ++p) {
                                    gene_set.insert(GEN_gene(gb_gene, this, location, p));
                                }
                            }
                        }
                    }
                    gb_gene = GEN_next_gene(gb_gene);
                }
            }
        }
    }
}

void GEN_root::reinit_NDS() const {
    GEN_iterator end  = gene_set.end();
    for (GEN_iterator gene = gene_set.begin(); gene != end; ++gene) {
        gene->reinit_NDS();
    }
}
