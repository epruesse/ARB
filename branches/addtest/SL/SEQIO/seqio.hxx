// ============================================================= //
//                                                               //
//   File      : seqio.hxx                                       //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef SEQIO_HXX
#define SEQIO_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class AP_filter;

bool  SEQIO_read_string_pair(FILE *in, char *&s1, char *&s2, size_t& lineNr);
char *SEQIO_fgets(char *s, int size, FILE *stream);

GB_ERROR SEQIO_export_by_format(GBDATA *gb_main, int marked_only, AP_filter *filter,
                                int cut_stop_codon, int compress, const char *dbname,
                                const char *formname, const char *outname, int multiple,
                                char **real_outname);

char *SEQIO_exportFormat_get_outfile_default_suffix(const char *formname, GB_ERROR& error);

#else
#error seqio.hxx included twice
#endif // SEQIO_HXX
