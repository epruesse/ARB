/*  ====================================================================  */
/*                                                                        */
/*    File      : adGene.c                                                */
/*    Purpose   : Basic gene access functions                             */
/*                                                                        */
/*                                                                        */
/*  Coded by Ralf Westram (coder@reallysoft.de) in July 2002              */
/*  Copyright Department of Microbiology (Technical University Munich)    */
/*                                                                        */
/*  Visit our web site at: http://www.arb-home.de/                        */
/*                                                                        */
/*                                                                        */
/*  ====================================================================  */

#include <adGene.h>
#include <arbdbt.h>
#include "gb_local.h"


GB_BOOL GEN_is_genome_db(GBDATA *gb_main, int default_value) {
    // default_value ==  0 -> default to normal database
    //               ==  1 -> default to GENOM database
    //               == -1 -> assume that type is already defined
    GBDATA *gb_genom_db = GB_entry(gb_main, GENOM_DB_TYPE);

    if (!gb_genom_db) {         // no DB-type entry -> create one with default
        GB_ERROR error = NULL;

        assert_or_exit(default_value != -1); // first call to GEN_is_genome_db has to provide a 'default_value'

        gb_genom_db             = GB_create(gb_main, GENOM_DB_TYPE, GB_INT);
        if (!gb_genom_db) error = GB_await_error();
        else error              = GB_write_int(gb_genom_db, default_value);

        if (error) GBK_terminatef("Fatal in GEN_is_genome_db: %s", error);
    }

    return GB_read_int(gb_genom_db) != 0;
}

//  --------------
//      genes:
//  --------------

GBDATA* GEN_findOrCreate_gene_data(GBDATA *gb_species) {
    GBDATA *gb_gene_data = GB_search(gb_species, "gene_data", GB_CREATE_CONTAINER);
    gb_assert(gb_gene_data);
    return gb_gene_data;
}

GBDATA* GEN_find_gene_data(GBDATA *gb_species) {
    return GB_search(gb_species, "gene_data", GB_FIND);
}

GBDATA* GEN_expect_gene_data(GBDATA *gb_species) {
    GBDATA *gb_gene_data = GB_search(gb_species, "gene_data", GB_FIND);
    gb_assert(gb_gene_data);
    return gb_gene_data;
}

GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GBDATA *gb_name = GB_find_string(gb_gene_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD); 

    if (gb_name) return GB_get_father(gb_name); // found existing gene
    return 0;
}

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name) {
    // find existing gene. returns 0 if it does not exist.
    GBDATA *gb_gene_data = GEN_find_gene_data(gb_species);
    return gb_gene_data ? GEN_find_gene_rel_gene_data(gb_gene_data, name) : 0;
}

GBDATA* GEN_create_nonexisting_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GB_ERROR  error   = GB_push_transaction(gb_gene_data);
    GBDATA   *gb_gene = 0;

    gb_assert(!GEN_find_gene_rel_gene_data(gb_gene_data, name)); // don't call this function if you are not sure that the gene does not exists!

    if (!error) {
        gb_gene = GB_create_container(gb_gene_data, "gene");
        error   = gb_gene ? GBT_write_string(gb_gene, "name", name) : GB_await_error();
    }

    gb_assert(gb_gene || error);
    error = GB_end_transaction(gb_gene_data, error);
    if (error) GB_export_error(error);

    return gb_gene;
}

GBDATA* GEN_create_nonexisting_gene(GBDATA *gb_species, const char *name) {
    return GEN_create_nonexisting_gene_rel_gene_data(GEN_findOrCreate_gene_data(gb_species), name);
}

GBDATA* GEN_find_or_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GBDATA *gb_gene = 0;

    /* Search for a gene, when gene does not exist create it */
    if (!name || !name[0]) {
        GB_export_error("Missing gene name");
    }
    else {
        GBDATA *gb_name = GB_find_string(gb_gene_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

        if (gb_name) {
            gb_gene = GB_get_father(gb_name); // found existing gene
        }
        else {
            GB_ERROR error = GB_push_transaction(gb_gene_data);

            if (!error) {
                gb_gene = GB_create_container(gb_gene_data, "gene");
                error   = GBT_write_string(gb_gene, "name", name);
            }
            error = GB_end_transaction(gb_gene_data, error);
            if (error) {
                gb_gene = NULL;
                GB_export_error(error);
            }
        }
    }
    return gb_gene;
}

GBDATA* GEN_find_or_create_gene(GBDATA *gb_species, const char *name) {
    return GEN_find_or_create_gene_rel_gene_data(GEN_findOrCreate_gene_data(gb_species), name);
}

GBDATA* GEN_first_gene(GBDATA *gb_species) {
    return GB_entry(GEN_expect_gene_data(gb_species), "gene");
}

GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data) {
    return GB_entry(gb_gene_data, "gene");
}

GBDATA* GEN_next_gene(GBDATA *gb_gene) {
    gb_assert(GB_has_key(gb_gene, "gene"));
    return GB_nextEntry(gb_gene);
}

GBDATA *GEN_first_marked_gene(GBDATA *gb_species) {
    return GB_first_marked(GEN_expect_gene_data(gb_species), "gene");
}
GBDATA *GEN_next_marked_gene(GBDATA *gb_gene) {
    return GB_next_marked(gb_gene,"gene");
}

/* ----------------------- */
/*      gene position      */
/* ----------------------- */

static struct GEN_position *lastFreedPosition = 0;

struct GEN_position *GEN_new_position(int parts, GB_BOOL joinable) {
    struct GEN_position *pos;

    size_t pos_size  = parts*sizeof(pos->start_pos[0]);
    size_t comp_size = parts*sizeof(pos->complement[0]);
    size_t data_size = 2*pos_size+3*comp_size;

    gb_assert(parts>0);

    if (lastFreedPosition && lastFreedPosition->parts == parts) {
        pos               = lastFreedPosition;
        lastFreedPosition = 0;
        memset(pos->start_pos, 0, data_size);
    }
    else {
        pos             = (GEN_position*)GB_calloc(1, sizeof(*pos));
        pos->parts      = parts;
        pos->start_pos  = (size_t*)GB_calloc(1, data_size);
        pos->stop_pos   = pos->start_pos+parts;
        pos->complement = (unsigned char*)(pos->stop_pos+parts);
    }

    pos->joinable        = joinable;
    pos->start_uncertain = 0;
    pos->stop_uncertain  = 0;

    return pos;
}

void GEN_use_uncertainties(struct GEN_position *pos) {
    if (pos->start_uncertain == 0) {
        // space was already allocated in GEN_new_position
        pos->start_uncertain = pos->complement+pos->parts;
        pos->stop_uncertain  = pos->start_uncertain+pos->parts;

        size_t comp_size = pos->parts*sizeof(pos->complement[0]);
        memset(pos->start_uncertain, '=', 2*comp_size);
    }
}

void GEN_free_position(struct GEN_position *pos) {
    if (lastFreedPosition) {
        free(lastFreedPosition->start_pos); // rest is allocated together with start_pos
        free(lastFreedPosition);
    }

    lastFreedPosition = pos;
}

static void clearParseTable(char **parseTable, int parts) {
    int p;
    free(parseTable[0]);
    for (p = 0; p<parts; p++) parseTable[p] = 0;
}

static GB_ERROR parseCSV(GBDATA *gb_gene, const char *field_name, int parts, char **parseTable) {
    // reads a field and splits the content at ','
    // results are put into parseTable (only first entry in parseTable is allocated, other entries
    // are simply pointers into the same string)

    GB_ERROR  error      = 0;
    GBDATA   *gb_field   = GB_entry(gb_gene, field_name);
    if (!gb_field) error = GBS_global_string("Expected entry '%s' missing", field_name);
    else {
        char *content       = GB_read_string(gb_field);
        if (!content) error = GB_await_error();
        else {
            int   p;
            char *pos = content;

            clearParseTable(parseTable, parts);

            for (p = 0; p<(parts-1) && !error; p++) {
                char *comma = strchr(pos, ',');
                if (comma) {
                    comma[0]      = 0;
                    parseTable[p] = pos;
                    pos           = comma+1;
                }
                else {
                    error = "comma expected";
                }
            }

            if (!error) {
                parseTable[p] = pos; // rest

                if (strchr(pos, ',') != 0) error = "comma found where none expected";
            }

            if (error) {
                error         = GBS_global_string("%s in '%s' (while parsing %i values from '%s')",
                                                  error, pos, parts, GB_read_char_pntr(gb_field));
                parseTable[0] = content; // ensure content gets freed
            }
        }
    }

    if (error) clearParseTable(parseTable, parts);
    return error;
}

static GB_ERROR parsePositions(GBDATA *gb_gene, const char *field_name, int parts, size_t *results, char **parseTable) {
    GB_ERROR error = parseCSV(gb_gene, field_name, parts, parseTable);
    if (!error) {
        int p;
        for (p = 0; p<parts && !error; p++) {
            char *end;
            results[p] = strtol(parseTable[p], &end, 10);
            if (end == parseTable[p]) { // error
                error = GBS_global_string("can't convert '%s' to number (while parsing '%s')", parseTable[p], field_name);
            }
        }
    }
    return error;
}

struct GEN_position *GEN_read_position(GBDATA *gb_gene) {
    int                  parts         = 1;
    GB_BOOL              joinable      = GB_FALSE;
    GBDATA              *gb_pos_joined = GB_entry(gb_gene, "pos_joined");
    struct GEN_position *pos           = 0;
    GB_ERROR             error         = 0;

    if (gb_pos_joined) {
        parts = GB_read_int(gb_pos_joined);
        if (parts != 1) { // splitted
            if (parts>1) joinable = GB_TRUE;
            else if (parts<-1) parts = -parts; // neg value means "not joinable" (comes from feature location 'order(...)')
            else error = GBS_global_string("Illegal value %i in 'pos_joined'", parts);
        }
    }

    if (!error) {
        pos = GEN_new_position(parts, joinable);

        char **parseTable = (char**)GB_calloc(parts, sizeof(*parseTable));

        error =             parsePositions(gb_gene, "pos_start", parts, pos->start_pos, parseTable);
        if (!error) error = parsePositions(gb_gene, "pos_stop",  parts, pos->stop_pos,  parseTable);

        int p;
        if (!error) {
            error = parseCSV(gb_gene, "pos_complement",  parts, parseTable);
            for (p = 0; p<parts && !error; p++) {
                const char *val = parseTable[p];
                if ((val[0] != '0' && val[0] != '1') || val[1] != 0) {
                    error = GBS_global_string("Invalid content '%s' in 'pos_complement' (expected: \"01\")", val);
                }
                else {
                    pos->complement[p] = (unsigned char)atoi(val);
                }
            }
        }

        if (!error) {
            GBDATA *gb_pos_certain = GB_entry(gb_gene, "pos_certain");

            if (gb_pos_certain) {
                error = parseCSV(gb_gene, "pos_certain",  parts, parseTable);
                GEN_use_uncertainties(pos);
                for (p = 0; p<parts && !error; p++) {
                    const unsigned char *val = (unsigned char *)(parseTable[p]);
                    int                  vp;

                    for (vp = 0; vp<2; vp++) {
                        unsigned char c = val[vp];
                        if (c != '<' && c != '=' && c != '>' && (c != "+-"[vp])) {
                            error = GBS_global_string("Invalid content '%s' in 'pos_certain' (expected 2 from \"<=>\")", val);
                        }
                    }
                    if (!error) {
                        pos->start_uncertain[p] = val[0];
                        pos->stop_uncertain[p]  = val[1];
                    }
                }
            }
        }

        clearParseTable(parseTable, parts);
        free(parseTable);
    }

    gb_assert(error || pos);
    if (error) {
        GB_export_error(error);
        if (pos) {
            GEN_free_position(pos);
            pos = 0;
        }
    }
    return pos;
}

GB_ERROR GEN_write_position(GBDATA *gb_gene, const struct GEN_position *pos) {
    GB_ERROR  error          = 0;
    GBDATA   *gb_pos_joined  = GB_entry(gb_gene, "pos_joined");
    GBDATA   *gb_pos_certain = GB_entry(gb_gene, "pos_certain");
    GBDATA   *gb_pos_start;
    GBDATA   *gb_pos_stop;
    GBDATA   *gb_pos_complement;
    int       p;

    gb_assert(pos);

    gb_pos_start             = GB_search(gb_gene, "pos_start", GB_STRING);
    if (!gb_pos_start) error = GB_await_error();

    if (!error) {
        gb_pos_stop             = GB_search(gb_gene, "pos_stop", GB_STRING);
        if (!gb_pos_stop) error = GB_await_error();
    }
    if (!error) {
        gb_pos_complement             = GB_search(gb_gene, "pos_complement", GB_STRING);
        if (!gb_pos_complement) error = GB_await_error();
    }

    if (!error) {
        if (pos->start_uncertain) {
            if (!gb_pos_certain) {
                gb_pos_certain             = GB_search(gb_gene, "pos_certain", GB_STRING);
                if (!gb_pos_certain) error = GB_await_error();
            }
        }
        else {
            if (gb_pos_certain) {
                error          = GB_delete(gb_pos_certain);
                gb_pos_certain = 0;
            }
        }
    }

#if defined(DEBUG)
    /* test data */
    if (!error) {
        for (p = 0; p<pos->parts; ++p) {
            char c;

            c = pos->complement[p]; gb_assert(c == 0 || c == 1);
            gb_assert(pos->start_pos[p] <= pos->stop_pos[p]);
            if (pos->start_uncertain) {
                c = pos->start_uncertain[p]; gb_assert(strchr("<=>+", c) != 0);
                c = pos->stop_uncertain[p]; gb_assert(strchr("<=>-", c) != 0);
            }
        }
    }
#endif /* DEBUG */

    if (!error) {
        if (pos->parts == 1) {
            if (gb_pos_joined) error = GB_delete(gb_pos_joined);
            
            if (!error) error = GB_write_string(gb_pos_start,      GBS_global_string("%zu", pos->start_pos[0]));
            if (!error) error = GB_write_string(gb_pos_stop,       GBS_global_string("%zu", pos->stop_pos[0]));
            if (!error) error = GB_write_string(gb_pos_complement, GBS_global_string("%c", pos->complement[0]+'0'));

            if (!error && gb_pos_certain) {
                error = GB_write_string(gb_pos_certain, GBS_global_string("%c%c", pos->start_uncertain[0], pos->stop_uncertain[0]));
            }
        }
        else {
            if (!gb_pos_joined) {
                gb_pos_joined             = GB_search(gb_gene, "pos_joined", GB_INT);
                if (!gb_pos_joined) error = GB_await_error();
            }
            if (!error) error = GB_write_int(gb_pos_joined, pos->parts * (pos->joinable ? 1 : -1)); // neg. parts means not joinable

            if (!error) {
                GBS_strstruct *start      = GBS_stropen(12*pos->parts);
                GBS_strstruct *stop       = GBS_stropen(12*pos->parts);
                GBS_strstruct *complement = GBS_stropen(2*pos->parts);
                GBS_strstruct *uncertain  = GBS_stropen(3*pos->parts);

                for (p = 0; p<pos->parts; ++p) {
                    if (p>0) {
                        GBS_chrcat(start, ',');
                        GBS_chrcat(stop, ',');
                        GBS_chrcat(complement, ',');
                        GBS_chrcat(uncertain, ',');
                    }
                    GBS_strcat(start, GBS_global_string("%zu", pos->start_pos[p]));
                    GBS_strcat(stop,  GBS_global_string("%zu", pos->stop_pos[p]));
                    GBS_chrcat(complement, pos->complement[p]+'0');
                    if (gb_pos_certain) {
                        GBS_chrcat(uncertain, pos->start_uncertain[p]);
                        GBS_chrcat(uncertain, pos->stop_uncertain[p]);
                    }
                }

                char *sstart      = GBS_strclose(start);
                char *sstop       = GBS_strclose(stop);
                char *scomplement = GBS_strclose(complement);
                char *suncertain  = GBS_strclose(uncertain);

                error             = GB_write_string(gb_pos_start, sstart);
                if (!error) error = GB_write_string(gb_pos_stop, sstop);
                if (!error) error = GB_write_string(gb_pos_complement, scomplement);
                if (!error && gb_pos_certain) error = GB_write_string(gb_pos_certain, suncertain);

                free(suncertain);
                free(scomplement);
                free(sstop);
                free(sstart);
            }
        }
    }

    return error;
}

static struct GEN_position *location2sort = 0;

static int cmp_location_parts(const void *v1, const void *v2) {
    int i1 = *(int*)v1;
    int i2 = *(int*)v2;

    int cmp = location2sort->start_pos[i1]-location2sort->start_pos[i2];
    if (!cmp) {
        cmp = location2sort->stop_pos[i1]-location2sort->stop_pos[i2];
    }
    return cmp;
}

void GEN_sortAndMergeLocationParts(struct GEN_position *location) {
    // Note: makes location partly invalid (only start_pos + stop_pos are valid afterwards)
    int  parts = location->parts;
    int *idx   = (int*)malloc(parts*sizeof(*idx)); // idx[newpos] = oldpos
    int  i, p;

    for (p = 0; p<parts; ++p) idx[p] = p;

    location2sort = location;
    qsort(idx, parts, sizeof(*idx), cmp_location_parts);
    location2sort = 0;

    for (p = 0; p<parts; ++p) {
        i = idx[p];

#define swap(a, b, type) do { type tmp = (a); (a) = (b); (b) = (tmp); } while (0)

        if (i != p) {
            swap(location->start_pos[i],  location->start_pos[p],  size_t);
            swap(location->stop_pos[i],   location->stop_pos[p],   size_t);
            swap(idx[i], idx[p], int);
        }
    }

#if defined(DEBUG) && 0
    printf("Locations sorted:\n");
    for (p = 0; p<parts; ++p) {
        printf("  [%i] %i - %i %i\n", p, location->start_pos[p], location->stop_pos[p], (int)(location->complement[p]));
    }
#endif /* DEBUG */
    
    i = 0;
    for (p = 1; p<parts; p++) {
        if ((location->stop_pos[i]+1) >= location->start_pos[p]) {
            // parts overlap or are directly consecutive

            location->stop_pos[i]  = location->stop_pos[p];
        }
        else {
            i++;
            location->start_pos[i] = location->start_pos[p];
            location->stop_pos[i]  = location->stop_pos[p];
        }
    }
    location->parts = i+1;

#if defined(DEBUG) && 0
    parts = location->parts;
    printf("Locations merged:\n");
    for (p = 0; p<parts; ++p) {
        printf("  [%i] %i - %i %i\n", p, location->start_pos[p], location->stop_pos[p], (int)(location->complement[p]));
    }
#endif /* DEBUG */

    free(idx);
}



//  -----------------------------------------
//      test if species is pseudo-species
//  -----------------------------------------

const char *GEN_origin_organism(GBDATA *gb_pseudo) {
    GBDATA *gb_origin = GB_entry(gb_pseudo, "ARB_origin_species");
    return gb_origin ? GB_read_char_pntr(gb_origin) : 0;
}
const char *GEN_origin_gene(GBDATA *gb_pseudo) {
    GBDATA *gb_origin = GB_entry(gb_pseudo, "ARB_origin_gene");
    return gb_origin ? GB_read_char_pntr(gb_origin) : 0;
}

GB_BOOL GEN_is_pseudo_gene_species(GBDATA *gb_species) {
    return GEN_origin_organism(gb_species) != 0;
}

//  ------------------------------------------------
//      find organism or gene for pseudo-species
//  ------------------------------------------------

GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo) {
    GB_ERROR error = 0;

    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));
    gb_assert(GEN_find_origin_organism(gb_pseudo, 0) == 0);

    error = GB_export_errorf("The gene-species '%s' refers to an unknown organism (%s)\n"
                             "This occurs if you rename or delete the organism or change the entry\n"
                             "'ARB_origin_species' and will most likely cause serious problems.",
                             GBT_read_name(gb_pseudo),
                             GEN_origin_organism(gb_pseudo));

    return error;
}

// @@@ FIXME: missing: GEN_gene_not_found (like GEN_organism_not_found)

/* ---------------------------------- */
/*      searching pseudo species      */
/* ---------------------------------- */

static const char *pseudo_species_hash_key(const char *organism_name, const char *gene_name) {
    return GBS_global_string("%s*%s", organism_name, gene_name);
}

GBDATA *GEN_read_pseudo_species_from_hash(GB_HASH *pseudo_hash, const char *organism_name, const char *gene_name) {
    return (GBDATA*)GBS_read_hash(pseudo_hash, pseudo_species_hash_key(organism_name, gene_name));
}

void GEN_add_pseudo_species_to_hash(GBDATA *gb_pseudo, GB_HASH *pseudo_hash) {
    const char *organism_name = GEN_origin_organism(gb_pseudo);
    const char *gene_name     = GEN_origin_gene(gb_pseudo);

    gb_assert(organism_name);
    gb_assert(gene_name);

    GBS_write_hash(pseudo_hash, pseudo_species_hash_key(organism_name, gene_name), (long)gb_pseudo);
}

GB_HASH *GEN_create_pseudo_species_hash(GBDATA *gb_main, int additionalSize) {
    GB_HASH *pseudo_hash = GBS_create_hash(GBT_get_species_hash_size(gb_main)+2*additionalSize, GB_IGNORE_CASE);
    GBDATA  *gb_pseudo;

    for (gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GEN_add_pseudo_species_to_hash(gb_pseudo, pseudo_hash);
    }

    return pseudo_hash;
}

GBDATA *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name, GB_HASH *pseudo_hash) {
    // parameter pseudo_hash :
    // 0 -> use slow direct search [if you only search one]
    // otherwise it shall be a hash generated by GEN_create_pseudo_species_hash() [if you search several times]
    // Note : use GEN_add_pseudo_species_to_hash to keep hash up-to-date
    GBDATA *gb_pseudo;

    if (pseudo_hash) {
        gb_pseudo = GEN_read_pseudo_species_from_hash(pseudo_hash, organism_name, gene_name);
    }
    else {
        for (gb_pseudo = GEN_first_pseudo_species(gb_main);
             gb_pseudo;
             gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
        {
            const char *origin_gene_name = GEN_origin_gene(gb_pseudo);
            if (strcmp(gene_name, origin_gene_name) == 0) {
                const char *origin_species_name = GEN_origin_organism(gb_pseudo);
                if (strcmp(organism_name, origin_species_name) == 0) {
                    break; // found pseudo species
                }
            }
        }
    }
    return gb_pseudo;
}

/* --------------------------- */
/*      searching origins      */
/* --------------------------- */

GBDATA *GEN_find_origin_organism(GBDATA *gb_pseudo, GB_HASH *organism_hash) {
    // parameter organism_hash:
    // 0 -> use slow direct search [if you only search one or two]
    // otherwise it shall be a hash generated by GBT_create_organism_hash() [if you search several times]
    // Note : use GBT_add_item_to_hash() to keep hash up-to-date
    
    const char *origin_species_name;
    GBDATA     *gb_organism = 0;
    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));

    origin_species_name = GEN_origin_organism(gb_pseudo);
    if (origin_species_name) {
        if (organism_hash) {
            gb_organism = (GBDATA*)GBS_read_hash(organism_hash, origin_species_name);
        }
        else {
            gb_organism = GBT_find_species_rel_species_data(GB_get_father(gb_pseudo), origin_species_name);
        }
    }

    return gb_organism;
}

GBDATA *GEN_find_origin_gene(GBDATA *gb_pseudo, GB_HASH *organism_hash) {
    const char *origin_gene_name;

    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));

    origin_gene_name = GEN_origin_gene(gb_pseudo);
    if (origin_gene_name) {
        GBDATA *gb_organism = GEN_find_origin_organism(gb_pseudo, organism_hash);
        gb_assert(gb_organism);

        return GEN_find_gene(gb_organism, origin_gene_name);
    }
    return 0;
}

//  --------------------------------
//      find pseudo-species
//  --------------------------------

GBDATA* GEN_first_pseudo_species(GBDATA *gb_main) {
    GBDATA *gb_species = GBT_first_species(gb_main);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_pseudo_species(gb_species);
}

GBDATA* GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data) {
    GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_pseudo_species(gb_species);
}

GBDATA* GEN_next_pseudo_species(GBDATA *gb_species) {
    if (gb_species) {
        while (1) {
            gb_species = GBT_next_species(gb_species);
            if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) break;
        }
    }
    return gb_species;
}

GBDATA *GEN_first_marked_pseudo_species(GBDATA *gb_main) {
    GBDATA *gb_species = GBT_first_marked_species(gb_main);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_marked_pseudo_species(gb_species);
}

GBDATA* GEN_next_marked_pseudo_species(GBDATA *gb_species) {
    if (gb_species) {
        while (1) {
            gb_species = GBT_next_marked_species(gb_species);
            if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) break;
        }
    }
    return gb_species;
}



/* ------------------------ */
/*        organisms         */
/* ------------------------ */

GB_BOOL GEN_is_organism(GBDATA *gb_species) {
    gb_assert(GEN_is_genome_db(GB_get_root(gb_species), -1)); /* assert this is a genome db */
    /* otherwise it is an error to use GEN_is_organism (or its callers)!!!! */
    
    return GB_entry(gb_species, GENOM_ALIGNMENT) != 0;
}

GBDATA *GEN_find_organism(GBDATA *gb_main, const char *name) {
    GBDATA *gb_orga = GBT_find_species(gb_main, name);
    if (gb_orga) {
        if (!GEN_is_organism(gb_orga)) {
            fprintf(stderr, "ARBDB-warning: found unspecific species named '%s', but expected an 'organism' with that name\n", name);
            gb_orga = 0;
        }
    }
    return gb_orga;
}

GBDATA *GEN_first_organism(GBDATA *gb_main) {
    GBDATA *gb_organism = GBT_first_species(gb_main);

    if (!gb_organism || GEN_is_organism(gb_organism)) return gb_organism;
    return GEN_next_organism(gb_organism);
}
GBDATA *GEN_next_organism(GBDATA *gb_organism) {
    if (gb_organism) {
        while (1) {
            gb_organism = GBT_next_species(gb_organism);
            if (!gb_organism || GEN_is_organism(gb_organism)) break;
        }
    }
    return gb_organism;

}

long GEN_get_organism_count(GBDATA *gb_main) {
    long    count       = 0;
    GBDATA *gb_organism = GEN_first_organism(gb_main);
    while (gb_organism) {
        count++;
        gb_organism = GEN_next_organism(gb_organism);
    }
    return count;
}


GBDATA *GEN_first_marked_organism(GBDATA *gb_main) {
    GBDATA *gb_organism = GBT_first_marked_species(gb_main);

    if (!gb_organism || GEN_is_organism(gb_organism)) return gb_organism;
    return GEN_next_marked_organism(gb_organism);
}
GBDATA *GEN_next_marked_organism(GBDATA *gb_organism) {
    if (gb_organism) {
        while (1) {
            gb_organism = GBT_next_marked_species(gb_organism);
            if (!gb_organism || GEN_is_organism(gb_organism)) break;
        }
    }
    return gb_organism;
}

char *GEN_global_gene_identifier(GBDATA *gb_gene, GBDATA *gb_organism) {
    if (!gb_organism) {
        gb_organism = GB_get_grandfather(gb_gene);
        gb_assert(gb_organism);
    }

    return GBS_global_string_copy("%s/%s", GBT_read_name(gb_organism), GBT_read_name(gb_gene));
}
