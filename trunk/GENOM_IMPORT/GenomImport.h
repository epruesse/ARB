#ifndef GENOME_IMPORT_H
#define GENOME_IMPORT_H

// #include <GenomEmbl.h>
// #include <GenomArbConnection.h>

GB_ERROR Genom_read_embl_universal(GBDATA *gb_main, const char *filename, const char *ali_name);

#else
#error Genome_Import.h included twice
#endif

