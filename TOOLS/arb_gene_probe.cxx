#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <map>
#include <list>
#include <set>

#include <arbdb.h>
#include <arbdbt.h>
#include <adGene.h>

#define gp_assert(cond) arb_assert(cond)

using namespace std;

#if defined(DEBUG)
// #define CREATE_DEBUG_FILES
// #define DUMP_OVERLAP_CALC
#endif // DEBUG

// --------------------------------------------------------------------------------

static int gene_counter          = 0; // pre-incremented counters
static int splitted_gene_counter = 0;
static int intergene_counter     = 0;

static map<const char *,char *> names;

// --------------------------------------------------------------------------------

struct PositionPair {
    int begin;                  // these positions are in range [0 .. genome_length-1]
    int end;

    static int genome_length;

#if defined(DEBUG)
    void check_legal() const {
        gp_assert(begin >= 0);
        gp_assert(begin <= end);
        gp_assert(end < genome_length);
    }
#endif // DEBUG

    PositionPair() : begin(-1), end(-1) {}
    PositionPair(int begin_, int end_) : begin(begin_), end(end_) {
#if defined(DEBUG)
        check_legal();
#endif // DEBUG
    }

    int length() const { return end-begin+1; }

    bool overlapsWith(const PositionPair& other) const {
#if defined(DEBUG)
        check_legal();
        other.check_legal();
#endif // DEBUG
        return ! ((end < other.begin) || (other.end < begin));
    }

#if defined(DUMP_OVERLAP_CALC)
    void dump(const char *note) const {
        printf("%s begin=%i end=%i\n", note, begin, end);
    }
#endif // DUMP_OVERLAP_CALC
};

int PositionPair::genome_length = 0;

typedef list<PositionPair> PositionPairList;

struct ltNonOverlap {
    // sorting with this operator identifies all overlapping PositionPair's as "equal"
    bool operator ()(const PositionPair& p1, const PositionPair& p2) {
        return p1.end < p2.begin;
    }
};

class GenePositionMap {
    typedef set<PositionPair, ltNonOverlap> OverlappingGeneSet;

    OverlappingGeneSet usedRanges;
    unsigned long      overlapSize;
    unsigned long      geneSize;
public:
    GenePositionMap() : overlapSize(0), geneSize(0) {}

    void announceGene(PositionPair gene);
    GB_ERROR buildIntergeneList(const PositionPair& wholeGenome, PositionPairList& intergeneList) const;
    unsigned long getOverlap() const { return overlapSize; }
    unsigned long getAllGeneSize() const { return geneSize; }

#if defined(DUMP_OVERLAP_CALC)
    void dump() const;
#endif // DUMP_OVERLAP_CALC
};

// ____________________________________________________________
// start of implementation of class GenePositionMap:

void GenePositionMap::announceGene(PositionPair gene)
{
    OverlappingGeneSet::iterator found = usedRanges.find(gene);
    if (found == usedRanges.end()) { // gene does not overlap with currently known ranges
        usedRanges.insert(gene); // add to known ranges
    }
    else {
        // 'found' overlaps with 'gene'
        int gene_length = gene.length();

        do {
            gp_assert(gene.overlapsWith(*found));

            gene                = PositionPair(min(found->begin, gene.begin), max(found->end, gene.end)); // calc combined range
            int combined_length = gene.length();

            size_t overlap  = (found->length()+gene_length)-combined_length;
            overlapSize    += overlap;
            geneSize       += gene_length;

            usedRanges.erase(found);

            gene_length = combined_length;
            found       = usedRanges.find(gene); // search for further overlaps
        } while (found != usedRanges.end());

        usedRanges.insert(gene); // insert the combined range
    }
}

GB_ERROR GenePositionMap::buildIntergeneList(const PositionPair& wholeGenome, PositionPairList& intergeneList) const
{
    OverlappingGeneSet::iterator end  = usedRanges.end();
    OverlappingGeneSet::iterator curr = usedRanges.begin();
    OverlappingGeneSet::iterator prev = end;

    if (curr == end) { // nothing defined -> use whole genome as one big intergene
        intergeneList.push_back(wholeGenome);
    }
    else {
        if (curr->begin > wholeGenome.begin) { // intergene before first gene range ?
            intergeneList.push_back(PositionPair(wholeGenome.begin, curr->begin-1));
        }

        prev = curr; ++curr;

        while (curr != end) {
            if (prev->end < curr->begin) {
                if (prev->end != (curr->begin-1)) { // not directly adjacent
                    intergeneList.push_back(PositionPair(prev->end+1, curr->begin-1));
                }
            }
            else {
                return "Internal error: Overlapping gene ranges";
            }

            prev = curr; ++curr;
        }

        if (prev != end && prev->end < wholeGenome.end) {
            intergeneList.push_back(PositionPair(prev->end+1, wholeGenome.end));
        }
    }
    return 0;
}

#if defined(DUMP_OVERLAP_CALC)
void GenePositionMap::dump() const
{
    printf("List of ranges used by genes:\n");
    for (OverlappingGeneSet::iterator g = usedRanges.begin(); g != usedRanges.end(); ++g) {
        g->dump("- ");
    }
    printf("Overlap: %lu bases\n", getOverlap());
}
#endif // DUMP_OVERLAP_CALC

// -end- of implementation of class GenePositionMap.

static GB_ERROR create_data_entry(GBDATA *gb_species2, const char *sequence, int seqlen) {
    GB_ERROR  error         = 0;
    char     *gene_sequence = new char[seqlen+1];
    memcpy(gene_sequence, sequence, seqlen); // @@@ FIXME: avoid this copy!
    gene_sequence[seqlen] = 0;

    GBDATA *gb_ali = GB_create_container(gb_species2, "ali_ptgene");
    if (gb_ali) {
        GBDATA *gb_data = GB_create(gb_ali, "data", GB_STRING);
        if (gb_data) {
            error = GB_write_string(gb_data, gene_sequence);
        }
        else {
            error = GB_get_error();
            if (!error) error = "can't create 'data' entry";
        }
    }
    else {
        error = GB_get_error();
        if (!error) error = "can't create 'ali_ptgene' container";
    }
    delete [] gene_sequence;
    return error;
}


static GBDATA *create_gene_species(GBDATA *gb_species_data2, const char *internal_name, const char *long_name, int abspos, const char *sequence, int length) {
    // Note: 'sequence' is not necessarily 0-terminated!
    GB_ERROR  error       = 0;
    GBDATA   *gb_species2 = GB_create_container(gb_species_data2, "species");

    if (gb_species2) {
        GBDATA *gb_name = GB_create(gb_species2, "name", GB_STRING);
        if (gb_name) {
            error = GB_write_string(gb_name, internal_name);
            if (!error) {
                const char *static_internal_name = GB_read_char_pntr(gb_name); // use static copy from db as map-index (internal_name is temporary)
                error                            = create_data_entry(gb_species2, sequence, length);
                if (!error) {
                    names[static_internal_name] = strdup(long_name);

                    GBDATA *gb_abspos = GB_create(gb_species2, "abspos", GB_INT);
                    if (gb_abspos) {
                        GB_write_int(gb_abspos, abspos); // store absolute position
                    }
                    else {
                        error = GB_get_error();
                        if (!error) error = "can't create 'abspos' entry";
                    }

#if defined(DEBUG)
                    // store realname for debugging purposes
                    if (!error) {
                        GBDATA *gb_realname     = GB_create(gb_species2, "realname_debug", GB_STRING);
                        if (!gb_realname) error = GB_get_error();
                        else    error           = GB_write_string(gb_realname, long_name);
                    }
#endif // DEBUG
                }
            }
        }
        else {
            error = GB_get_error();
            if (!error) error = "can't create 'name' entry";
        }
    }
    else {
        error = GB_get_error();
        if (!error) error = "can't create new 'species'";
    }

    if (error) { // be more verbose :
        error = GBS_global_string("%s (internal_name='%s', long_name='%s')", error, internal_name, long_name);
        GB_export_error(error);
    }

    return gb_species2;
}

static GB_ERROR create_genelike_entry(const char *internal_name, GBDATA *gb_species_data2, int pos_begin, int pos_end, const char *ali_genome, const char *long_name) {
    GBDATA *gb_genespecies = create_gene_species(gb_species_data2, internal_name, long_name, pos_begin, ali_genome+pos_begin, pos_end-pos_begin+1);
    return gb_genespecies ? 0 : GB_get_error();
}

static GB_ERROR create_intergene(GBDATA *gb_species_data2, int pos_begin, int pos_end, const char *ali_genome, const char *long_gene_name) {
    gp_assert(pos_begin <= pos_end);
    char internal_name[128];
    ++intergene_counter;
    sprintf(internal_name, "i%x", intergene_counter);
    return create_genelike_entry(internal_name, gb_species_data2, pos_begin, pos_end, ali_genome, long_gene_name);
}

static GB_ERROR create_gene(GBDATA *gb_species_data2, int pos_begin, int pos_end, const char *ali_genome, const char *long_gene_name) {
    gp_assert(pos_begin <= pos_end);
    char internal_name[128];
    sprintf(internal_name, "n%x", gene_counter++);
    return create_genelike_entry(internal_name, gb_species_data2, pos_begin, pos_end, ali_genome, long_gene_name);
}

static GB_ERROR create_splitted_gene(GBDATA *gb_species_data2, PositionPairList& part_list, const char *ali_genome, const char *long_gene_name) {
    GB_ERROR                     error    = 0;
    PositionPairList::iterator list_end = part_list.end();

    int gene_size = 0;
    for (PositionPairList::iterator part = part_list.begin(); part != list_end; ++part) {
        int part_size  = part->end-part->begin+1;
        gene_size     += part_size;
    }

    char *gene_sequence = new char[gene_size+1];
    int   gene_off      = 0;

    char *split_pos_list = 0;   // contains split information: 'gene pos of part2,abs pos of part2;gene pos of part3,abs pos of part3;...'

    for (PositionPairList::iterator part = part_list.begin(); part != list_end; ) {
        int part_size   = part->end-part->begin+1;
        int genome_pos  = part->begin;
        memcpy(gene_sequence+gene_off, ali_genome+part->begin, part_size);
        gene_off       += part_size;

        ++part;

        if (split_pos_list == 0) { // first part
            split_pos_list = GBS_global_string_copy("%i", gene_off); // gene offset of part 2
        }
        else {                  // next parts
            char *next_split_pos_list;
            if (part != list_end) { // not last
                next_split_pos_list = GBS_global_string_copy("%s,%i;%i", split_pos_list, genome_pos, gene_off);
            }
            else { // last part
                next_split_pos_list = GBS_global_string_copy("%s,%i", split_pos_list, genome_pos);
            }
            free(split_pos_list);
            split_pos_list = next_split_pos_list;
        }
    }

    char internal_name[128];
    sprintf(internal_name, "s%x", splitted_gene_counter++);

    const PositionPair&  first_part  = part_list.front();
    GBDATA              *gb_species2 = create_gene_species(gb_species_data2, internal_name, long_gene_name, first_part.begin,
                                                           gene_sequence, first_part.end-first_part.begin+1);

    if (!gb_species2) {
        error = GB_get_error();
    }
    else {
#if defined(DEBUG) && 0
        printf("splitted gene: long_gene_name='%s' internal_name='%s' split_pos_list='%s'\n",
               long_gene_name, internal_name, split_pos_list);
#endif // DEBUG

        GBDATA *gb_splitpos     = GB_create(gb_species2, "splitpos", GB_STRING);
        if (!gb_splitpos) error = GB_get_error();
        else    error           = GB_write_string(gb_splitpos, split_pos_list);
    }

    free(split_pos_list);
    delete [] gene_sequence;

    return error;
}

static GB_ERROR read_PositionPair(GBDATA *gb_gene, const char *pos_begin, const char *pos_end, PositionPair& pp) {
    GB_ERROR  error        = 0;
    GBDATA   *gb_pos_begin = GB_find(gb_gene, pos_begin, 0, down_level);
    if (!gb_pos_begin) {
        error = GBS_global_string("entry '%s' not found", pos_begin);
    }
    else {
        GBDATA *gb_pos_end = GB_find(gb_gene, pos_end, 0, down_level);
        if (!gb_pos_end) {
            error = GBS_global_string("entry '%s' not found", pos_end);
        }
        else {
            pp.begin = GB_read_int(gb_pos_begin)-1; // Note: gene positions are in range [1..N]
            pp.end   = GB_read_int(gb_pos_end)-1;
        }
    }
    return error;
}

static GB_ERROR scan_gene_positions(GBDATA *gb_gene, PositionPairList& part_list) {
    PositionPair pp;
    GB_ERROR     error = read_PositionPair(gb_gene, "pos_begin", "pos_end", pp);

    if (!error) {
        part_list.push_back(pp);
        GBDATA *gb_pos_joined = GB_find(gb_gene, "pos_joined", 0, down_level);
        if (gb_pos_joined) { // splitted gene
            int parts = GB_read_int(gb_pos_joined);
            gp_assert(parts >= 2);

            char pos_begin_string[20];
            char pos_end_string[20];

            for (int p = 2; p <= parts && !error; ++p) {
                sprintf(pos_begin_string, "pos_begin%i", p);
                sprintf(pos_end_string,   "pos_end%i",   p);

                error = read_PositionPair(gb_gene, pos_begin_string, pos_end_string, pp);
                if (!error) part_list.push_back(pp);
            }
        }
    }

    return error;
}

static GB_ERROR insert_genes_of_organism(GBDATA *gb_organism, GBDATA *gb_species_data2) {
    // insert all genes of 'gb_organism' as pseudo-species
    // into new 'species_data' (gb_species_data2)

    GB_ERROR    error            = 0;
    GBDATA     *gb_organism_name = GB_find(gb_organism,"name",0,down_level); // Name der Spezies
    const char *organism_name    = 0;

    if (!gb_organism_name) error = GBS_global_string("Organism w/o name entry");
    else {
        organism_name             = GB_read_char_pntr(gb_organism_name);
        if (!organism_name) error = GB_get_error();
    }

    GenePositionMap geneRanges;

    int gene_counter_old          = gene_counter; // used for statistics only (see end of function)
    int splitted_gene_counter_old = splitted_gene_counter;
    int intergene_counter_old     = intergene_counter;

    GBDATA     *gb_ali_genom    = GBT_read_sequence(gb_organism, GENOM_ALIGNMENT);
    gp_assert(gb_ali_genom);    // existance has to be checked by caller!
    const char *ali_genom       = GB_read_char_pntr(gb_ali_genom);
    if (!ali_genom) error       = GB_get_error();
    PositionPair::genome_length = GB_read_count(gb_ali_genom); // this affects checks in PositionPair

    for (GBDATA *gb_gene = GEN_first_gene(gb_organism);
         gb_gene && !error;
         gb_gene = GEN_next_gene(gb_gene))
    {
        GBDATA     *gb_gene_name = GB_find(gb_gene,"name",0,down_level);
        const char *gene_name     = 0;

        if (!gb_gene_name) error = "Gene w/o name entry";
        else {
            gene_name             = GB_read_char_pntr(gb_gene_name);
            if (!gene_name) error = GB_get_error();
        }

        PositionPairList part_list;
        if (!error) error                = scan_gene_positions(gb_gene, part_list);
        else if (part_list.empty()) error = "empty position list";

        if (!error) {
            int          split_count = part_list.size();
            PositionPair first_part  = *part_list.begin();

            if (!error) {
                char *long_gene_name = GBS_global_string_copy("%s;%s", organism_name, gene_name);
                if (split_count == 1) { // normal gene
                    error = create_gene(gb_species_data2, first_part.begin, first_part.end, ali_genom, long_gene_name);
                    geneRanges.announceGene(first_part);
                }
                else {          // splitted gene
                    error = create_splitted_gene(gb_species_data2, part_list, ali_genom, long_gene_name);

                    for (PositionPairList::iterator p = part_list.begin(); p != part_list.end(); ++p) {
                        geneRanges.announceGene(*p);
                    }
                }
                free(long_gene_name);
            }
        }

        if (error && gene_name) error = GBS_global_string("in gene '%s': %s", gene_name, error);
    }

    if (!error) { // add intergenes
        PositionPairList intergenes;
        PositionPair     wholeGenome(0, PositionPair::genome_length-1);
        error = geneRanges.buildIntergeneList(wholeGenome, intergenes);

        for (PositionPairList::iterator i = intergenes.begin(); !error && i != intergenes.end(); ++i) {
            char *long_intergene_name = GBS_global_string_copy("%s;intergene_%i_%i", organism_name, i->begin, i->end);
            error                     = create_intergene(gb_species_data2, i->begin, i->end, ali_genom, long_intergene_name);
            free(long_intergene_name);
        }
    }

    if (error && organism_name) error = GBS_global_string("in organism '%s': %s", organism_name, error);

    {
        int new_genes          = gene_counter-gene_counter_old; // only non-splitted genes
        int new_splitted_genes = splitted_gene_counter-splitted_gene_counter_old;
        int new_intergenes     = intergene_counter-intergene_counter_old;

        unsigned long genesSize    = geneRanges.getAllGeneSize();
        unsigned long overlaps     = geneRanges.getOverlap();
        double        data_grow    = overlaps/double(PositionPair::genome_length)*100;
        double        gene_overlap = overlaps/double(genesSize)*100;

        if (new_splitted_genes) {

            printf("  - %s: %u genes (%u splitted), %u intergenes",
                   organism_name, new_genes+new_splitted_genes, new_splitted_genes, new_intergenes);
        }
        else {
            printf("  - %s: %u genes, %u intergenes",
                   organism_name, new_genes, new_intergenes);
        }
        printf(" (data grow: %5.2f%%, gene overlap: %5.2f%%=%lu bp)\n", data_grow, gene_overlap, overlaps);
    }

#if defined(DUMP_OVERLAP_CALC)
    geneRanges.dump();
#endif // DUMP_OVERLAP_CALC

    return error;
}

int main(int argc, char* argv[]) {

    printf("\n"
           "arb_gene_probe 1.2 -- (C) 2003/2004 Lehrstuhl fuer Mikrobiologie - TU Muenchen\n"
           "written by Tom Littschwager, Bernd Spanfelner, Conny Wolf, Ralf Westram.\n");

    if (argc != 3) {
        printf("Usage: arb_gene_probe input_database output_database\n");
        printf("       Prepares a genome database for Gene-PT-Server\n");
        return EXIT_FAILURE;
    }

    const char *inputname  = argv[1];
    const char *outputname = argv[2];

    printf("Converting '%s' -> '%s' ..\n", inputname, outputname);

    GB_ERROR  error   = 0;
    GBDATA   *gb_main = GB_open(inputname, "rw"); // rootzeiger wird gesetzt
    if (!gb_main) {
        error = GBS_global_string("Database '%s' not found", inputname);
    }
    else {
        GB_request_undo_type(gb_main, GB_UNDO_NONE); // disable arbdb builtin undo
        GB_begin_transaction(gb_main);

        GBDATA *gb_species_data     = GB_find(gb_main,"species_data",0,down_level);
        GBDATA *gb_species_data_new = GB_create_container(gb_main,"species_data"); // introducing a second 'species_data' container

        if (!gb_species_data || ! gb_species_data_new) {
            error = GB_get_error();
        }

        int non_ali_genom_species = 0;
        int ali_genom_species     = 0;

        for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
             gb_species && !error;
             gb_species = GBT_next_species(gb_species))
        {
            GBDATA *gb_ali_genom = GBT_read_sequence(gb_species, GENOM_ALIGNMENT);
            if (!gb_ali_genom) {
                // skip species w/o alignment 'GENOM_ALIGNMENT' (genome DBs often contain pseudo species)
                ++non_ali_genom_species;
            }
            else {
                error = insert_genes_of_organism(gb_species, gb_species_data_new);
                ++ali_genom_species;
            }
        }

        if (non_ali_genom_species) {
            printf("%i species had no alignment in '" GENOM_ALIGNMENT "' and have been skipped.\n", non_ali_genom_species);
        }
        if (ali_genom_species == 0) {
            error = "no species with data in alignment '" GENOM_ALIGNMENT "' were found";
        }

        if (!error) {
            printf("%i species had data in alignment '" GENOM_ALIGNMENT "'.\n"
                   "Found %i genes (%i were splitted) and %i intergene regions.\n",
                   ali_genom_species, gene_counter, splitted_gene_counter, intergene_counter);
        }

        if (!error) {
            error = GB_delete(gb_species_data); // delete first (old) 'species_data' container
        }

        if (!error) {
            // create map-string
            char* map_string;
            {
                map<const char*,char*>::iterator NameEnd = names.end();
                map<const char*,char*>::iterator NameIter;

                size_t mapsize = 0;
                for (NameIter = names.begin(); NameIter != NameEnd; ++NameIter) {
                    mapsize += strlen(NameIter->first)+strlen(NameIter->second)+2;
                }

                map_string  = new char[mapsize+1];
                size_t moff = 0;

                for (NameIter = names.begin(); NameIter != NameEnd; ++NameIter) {
                    int len1 = strlen(NameIter->first);
                    int len2 = strlen(NameIter->second);

                    memcpy(map_string+moff, NameIter->first, len1);
                    map_string[moff+len1]  = ';';
                    moff                  += len1+1;

                    memcpy(map_string+moff, NameIter->second, len2);
                    map_string[moff+len2]  = ';';
                    moff                  += len2+1;
                }
                map_string[moff] = 0;

                gp_assert(moff <= mapsize);
            }

            GBDATA *gb_gene_map = GB_create_container(gb_main,"gene_map");
            if (!gb_gene_map) {
                error = GB_get_error();
            }
            else {
                GBDATA *gb_map_string     = GB_create(gb_gene_map,"map_string",GB_STRING);
                if (!gb_map_string) error = GB_get_error();
                else    error             = GB_write_string(gb_map_string, map_string);
            }
        }

        if (!error) {
            // set default alignment for pt_server
            error = GBT_set_default_alignment(gb_main, "ali_ptgene");

            GBDATA *gb_use     = GB_search(gb_main,"presets/alignment/alignment_name",GB_STRING);
            if (!gb_use) error = GB_get_error();
            else {
                error             = GB_push_my_security(gb_main);
                if (!error) error = GB_write_string(gb_use,"ali_ptgene");
                if (!error) error = GB_pop_my_security(gb_main);
            }
        }

        if (error) {
            GB_abort_transaction(gb_main);
        }
        else {
            GB_commit_transaction(gb_main);
            printf("Saving '%s' ..\n", outputname);
            error = GB_save_as(gb_main, outputname, "bfm");
            if (error) unlink(outputname);
        }
    }

    if (error) {
        printf("Error in arb_gene_probe: %s\n", error);
        return EXIT_FAILURE;
    }

    printf("arb_gene_probe done.\n");
    return EXIT_SUCCESS;
}

