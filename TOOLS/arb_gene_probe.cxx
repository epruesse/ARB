// =============================================================== //
//                                                                 //
//   File      : arb_gene_probe.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <adGene.h>

#include <map>
#include <list>
#include <set>
#include <string>

#include <unistd.h>
#include <sys/types.h>

#define gp_assert(cond) arb_assert(cond)

using namespace std;

#if defined(DEBUG)
// #define CREATE_DEBUG_FILES
// #define DUMP_OVERLAP_CALC
#endif // DEBUG

// --------------------------------------------------------------------------------

static int gene_counter          = 0; // pre-incremented counters
static int split_gene_counter = 0;
static int intergene_counter     = 0;

struct nameOrder {
    bool operator()(const char *name1, const char *name2) const {
        // Normally it is sufficient to have any order, as long as it is strict.
        // But for UNIT_TESTS we need a reproducable order, which does not
        // depend on memory layout of DB elements.
#if defined(UNIT_TESTS) // UT_DIFF
        return strcmp(name1, name2)<0; // slow, determined by species names
#else
        return (name1-name2)<0;        // fast, but depends on memory layout (e.g. on MEMORY_TEST in gb_memory.h)
#endif
    }
};

typedef map<const char *, string, nameOrder> FullNameMap;
static FullNameMap names;

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
    bool operator ()(const PositionPair& p1, const PositionPair& p2) const {
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

    memcpy(gene_sequence, sequence, seqlen);        // @@@ FIXME: avoid this copy!
    gene_sequence[seqlen] = 0;

    GBDATA *gb_ali     = GB_create_container(gb_species2, "ali_ptgene");
    if (!gb_ali) error = GB_await_error();
    else    error      = GBT_write_string(gb_ali, "data", gene_sequence);

    delete [] gene_sequence;
    return error;
}

#if defined(DEBUG)
static void CHECK_SEMI_ESCAPED(const char *name) {
    // checks whether all ";\\" are escaped
    while (*name) {
        gp_assert(*name != ';'); // oops, unescaped ';'
        if (*name == '\\') ++name;
        ++name;
    }
}
#else
#define CHECK_SEMI_ESCAPED(s)
#endif // DEBUG


static GBDATA *create_gene_species(GBDATA *gb_species_data2, const char *internal_name, const char *long_name, int abspos, const char *sequence, int length) {
    // Note: 'sequence' is not necessarily 0-terminated!

#if defined(DEBUG)
    const char *firstSem = strchr(long_name, ';');
    gp_assert(firstSem);
    CHECK_SEMI_ESCAPED(firstSem+1);
#endif // DEBUG

    GB_ERROR  error       = GB_push_transaction(gb_species_data2);
    GBDATA   *gb_species2 = 0;

    if (!error) {
        gb_species2 = GB_create_container(gb_species_data2, "species");
        if (!gb_species2) error = GB_await_error();
    }

    if (!error) {
        GBDATA *gb_name = GB_create(gb_species2, "name", GB_STRING);

        if (!gb_name) error = GB_await_error();
        else {
            error = GB_write_string(gb_name, internal_name);
            if (!error) {
                const char *static_internal_name = GB_read_char_pntr(gb_name); // use static copy from db as map-index (internal_name is temporary)
                error                            = create_data_entry(gb_species2, sequence, length);
                if (!error) {
                    names[static_internal_name] = long_name;
                    error = GBT_write_int(gb_species2, "abspos", abspos);
                }
            }
        }
    }

    error = GB_end_transaction(gb_species_data2, error);

    if (error) { // be more verbose :
        error       = GBS_global_string("%s (internal_name='%s', long_name='%s')", error, internal_name, long_name);
        GB_export_error(error);
        gb_species2 = NULL;
    }

    return gb_species2;
}

static GB_ERROR create_genelike_entry(const char *internal_name, GBDATA *gb_species_data2, int start_pos, int end_pos, const char *ali_genome, const char *long_name) {
    GBDATA *gb_genespecies = create_gene_species(gb_species_data2, internal_name, long_name, start_pos, ali_genome+start_pos, end_pos-start_pos+1);
    return gb_genespecies ? 0 : GB_await_error();
}

static GB_ERROR create_intergene(GBDATA *gb_species_data2, int start_pos, int end_pos, const char *ali_genome, const char *long_gene_name) {
    if (start_pos <= end_pos) {
        char internal_name[128];
        sprintf(internal_name, "i%x", intergene_counter++);
        return create_genelike_entry(internal_name, gb_species_data2, start_pos, end_pos, ali_genome, long_gene_name);
    }
    return "Illegal inter-gene positions (start behind end)";
}

static GB_ERROR create_gene(GBDATA *gb_species_data2, int start_pos, int end_pos, const char *ali_genome, const char *long_gene_name) {
    if (start_pos <= end_pos) {
        char internal_name[128];
        sprintf(internal_name, "n%x", gene_counter++);
        return create_genelike_entry(internal_name, gb_species_data2, start_pos, end_pos, ali_genome, long_gene_name);
    }
    return "Illegal gene positions (start behind end)";
}

static GB_ERROR create_split_gene(GBDATA *gb_species_data2, PositionPairList& part_list, const char *ali_genome, const char *long_gene_name) {
    GB_ERROR                     error    = 0;
    PositionPairList::iterator list_end = part_list.end();

    int gene_size = 0;
    for (PositionPairList::iterator part = part_list.begin(); part != list_end; ++part) {
        int part_size  = part->end-part->begin+1;
        gp_assert(part_size > 0);
        gene_size     += part_size;
    }
    gp_assert(gene_size > 0);
    char *gene_sequence = new char[gene_size+1];
    int   gene_off      = 0;

    char *split_pos_list = 0;   // contains split information: 'gene pos of part2,abs pos of part2;gene pos of part3,abs pos of part3;...'

    for (PositionPairList::iterator part = part_list.begin(); part != list_end;) {
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
            freeset(split_pos_list, next_split_pos_list);
        }
    }

    char internal_name[128];
    sprintf(internal_name, "s%x", split_gene_counter++);

    const PositionPair&  first_part  = part_list.front();
    GBDATA              *gb_species2 = create_gene_species(gb_species_data2, internal_name, long_gene_name, first_part.begin,
                                                           gene_sequence, first_part.end-first_part.begin+1);

    if (!gb_species2) error = GB_await_error();
    else {
#if defined(DEBUG) && 0
        printf("split gene: long_gene_name='%s' internal_name='%s' split_pos_list='%s'\n",
               long_gene_name, internal_name, split_pos_list);
#endif // DEBUG
        error = GBT_write_string(gb_species2, "splitpos", split_pos_list);
    }

    free(split_pos_list);
    delete [] gene_sequence;

    return error;
}

static GB_ERROR scan_gene_positions(GBDATA *gb_gene, PositionPairList& part_list) {
    GB_ERROR      error    = 0;
    GEN_position *location = GEN_read_position(gb_gene);

    if (!location) error = GB_await_error();
    else {
        GEN_sortAndMergeLocationParts(location);
        int parts = location->parts;
        for (int p = 0; p<parts; ++p) {
            part_list.push_back(PositionPair(location->start_pos[p]-1, location->stop_pos[p]-1));
        }
        GEN_free_position(location);
    }
    return error;
}

static GB_ERROR insert_genes_of_organism(GBDATA *gb_organism, GBDATA *gb_species_data2) {
    // insert all genes of 'gb_organism' as pseudo-species
    // into new 'species_data' (gb_species_data2)

    GB_ERROR    error            = 0;
    const char *organism_name = GBT_read_name(gb_organism);

    GenePositionMap geneRanges;

    int gene_counter_old          = gene_counter; // used for statistics only (see end of function)
    int split_gene_counter_old = split_gene_counter;
    int intergene_counter_old     = intergene_counter;

    GBDATA *gb_ali_genom = GBT_find_sequence(gb_organism, GENOM_ALIGNMENT);
    gp_assert(gb_ali_genom);                                                       // existence has to be checked by caller!

    const char *ali_genom       = GB_read_char_pntr(gb_ali_genom);
    if (!ali_genom) error       = GB_await_error();
    PositionPair::genome_length = GB_read_count(gb_ali_genom);     // this affects checks in PositionPair

    for (GBDATA *gb_gene = GEN_first_gene(gb_organism);
         gb_gene && !error;
         gb_gene = GEN_next_gene(gb_gene))
    {
        const char *gene_name = GBT_read_name(gb_gene);

        PositionPairList part_list;
        error = scan_gene_positions(gb_gene, part_list);

        if (!error && part_list.empty()) error = "empty position list";
        if (!error) {
            int          split_count = part_list.size();
            PositionPair first_part  = *part_list.begin();

            if (!error) {
                char *esc_gene_name = GBS_escape_string(gene_name, ";", '\\');
                char *long_gene_name = GBS_global_string_copy("%s;%s", organism_name, esc_gene_name);
                if (split_count == 1) { // normal gene
                    error = create_gene(gb_species_data2, first_part.begin, first_part.end, ali_genom, long_gene_name);
                    geneRanges.announceGene(first_part);
                }
                else {          // split gene
                    error = create_split_gene(gb_species_data2, part_list, ali_genom, long_gene_name);

                    for (PositionPairList::iterator p = part_list.begin(); p != part_list.end(); ++p) {
                        geneRanges.announceGene(*p);
                    }
                }
                free(long_gene_name);
                free(esc_gene_name);
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
        int new_genes          = gene_counter-gene_counter_old; // only non-split genes
        int new_split_genes = split_gene_counter-split_gene_counter_old;
        int new_intergenes     = intergene_counter-intergene_counter_old;

        unsigned long genesSize    = geneRanges.getAllGeneSize();
        unsigned long overlaps     = geneRanges.getOverlap();
        double        data_grow    = overlaps/double(PositionPair::genome_length)*100;
        double        gene_overlap = overlaps/double(genesSize)*100;

        if (new_split_genes) {

            printf("  - %s: %i genes (%i split), %i intergenes",
                   organism_name, new_genes+new_split_genes, new_split_genes, new_intergenes);
        }
        else {
            printf("  - %s: %i genes, %i intergenes",
                   organism_name, new_genes, new_intergenes);
        }
        printf(" (data grow: %5.2f%%, gene overlap: %5.2f%%=%lu bp)\n", data_grow, gene_overlap, overlaps);
    }

#if defined(DUMP_OVERLAP_CALC)
    geneRanges.dump();
#endif // DUMP_OVERLAP_CALC

    return error;
}

int ARB_main(int argc, char *argv[]) {

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

    // GBK_terminate("test-crash of arb_gene_probe");
    
    printf("Converting '%s' -> '%s' ..\n", inputname, outputname);

    GB_ERROR  error   = 0;
    GB_shell  shell;
    GBDATA   *gb_main = GB_open(inputname, "rw"); // rootzeiger wird gesetzt
    if (!gb_main) {
        error = GBS_global_string("Database '%s' not found", inputname);
    }
    else {
        GB_request_undo_type(gb_main, GB_UNDO_NONE); // disable arbdb builtin undo
        GB_begin_transaction(gb_main);

        GBDATA *gb_species_data     = GBT_get_species_data(gb_main);
        GBDATA *gb_species_data_new = GBT_create(gb_main, "species_data", 7); // create a second 'species_data' container

        if (!gb_species_data_new) error = GB_await_error();

        int non_ali_genom_species = 0;
        int ali_genom_species     = 0;

        for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
             gb_species && !error;
             gb_species = GBT_next_species(gb_species))
        {
            GBDATA *gb_ali_genom = GBT_find_sequence(gb_species, GENOM_ALIGNMENT);
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
        if (!error && ali_genom_species == 0) {
            error = "no species with data in alignment '" GENOM_ALIGNMENT "' were found";
        }

        if (!error) {
            printf("%i species had data in alignment '" GENOM_ALIGNMENT "'.\n"
                   "Found %i genes (%i were split) and %i intergene regions.\n",
                   ali_genom_species, gene_counter, split_gene_counter, intergene_counter);
        }

        if (!error) {
            error = GB_delete(gb_species_data); // delete first (old) 'species_data' container
        }

        if (!error) {
            // create map-string
            char* map_string;
            {
                FullNameMap::iterator NameEnd = names.end();
                FullNameMap::iterator NameIter;

                size_t mapsize = 0;
                for (NameIter = names.begin(); NameIter != NameEnd; ++NameIter) {
                    mapsize += strlen(NameIter->first)+NameIter->second.length()+2;
                }

                map_string  = new char[mapsize+1];
                size_t moff = 0;

                for (NameIter = names.begin(); NameIter != NameEnd; ++NameIter) {
                    int len1 = strlen(NameIter->first);
                    int len2 = NameIter->second.length();

                    memcpy(map_string+moff, NameIter->first, len1);
                    map_string[moff+len1]  = ';';
                    moff                  += len1+1;

                    memcpy(map_string+moff, NameIter->second.c_str(), len2);
                    map_string[moff+len2]  = ';';
                    moff                  += len2+1;
                }
                map_string[moff] = 0;

                gp_assert(moff <= mapsize);
            }

            GBDATA *gb_gene_map     = GB_create_container(gb_main, "gene_map");
            if (!gb_gene_map) error = GB_await_error();
            else    error           = GBT_write_string(gb_gene_map, "map_string", map_string);

            delete [] map_string;
        }

        if (!error) {
            // set default alignment for pt_server
            error = GBT_set_default_alignment(gb_main, "ali_ptgene");

            if (!error) {
                GBDATA *gb_use     = GB_search(gb_main, "presets/alignment/alignment_name", GB_STRING);
                if (!gb_use) error = GB_await_error();
                else {
                    GB_push_my_security(gb_main);
                    error = GB_write_string(gb_use, "ali_ptgene");
                    GB_pop_my_security(gb_main);
                }
            }
        }

        error = GB_end_transaction(gb_main, error);

        if (!error) {
            printf("Saving '%s' ..\n", outputname);
            error = GB_save_as(gb_main, outputname, "bfm");
            if (error) unlink(outputname);
        }

        GB_close(gb_main);
    }

    if (error) {
        printf("Error in arb_gene_probe: %s\n", error);
        return EXIT_FAILURE;
    }

    printf("arb_gene_probe done.\n");
    return EXIT_SUCCESS;
}

