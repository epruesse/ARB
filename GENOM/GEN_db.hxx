#ifndef GEN_DB_HXX
#define GEN_DB_HXX
class AW_window;
class AW_root;


// genes:

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name); // find existing gene
GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name); // create or find existing gene

GBDATA* GEN_first_gene(GBDATA *gb_species);
GBDATA* GEN_next_gene(GBDATA *gb_gene);

// annotations:

GBDATA* GEN_find_annotation(GBDATA *gb_gene, const char *name); // find existing annotation
GBDATA* GEN_create_annotation(GBDATA *gb_gene, const char *name); // create or find existing annotation

GBDATA* GEN_first_annotation(GBDATA *gb_gene);
GBDATA* GEN_next_annotation(GBDATA *gb_gene);


#else
#error GEN_db.hxx included twice
#endif // GEN_DB_HXX
