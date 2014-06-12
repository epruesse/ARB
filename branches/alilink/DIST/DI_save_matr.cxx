// =============================================================== //
//                                                                 //
//   File      : DI_save_matr.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_matr.hxx"
#include <nds.h>

const char *DI_MATRIX::save(char *filename, enum DI_SAVE_TYPE type)
{
    FILE     *out;
    GB_ERROR  error = 0;

    out = fopen(filename, "w");
    if (!out) return "Cannot save your File";

    size_t row, col;
    switch (type) {
        case DI_SAVE_PHYLIP_COMP:
            {
                fprintf(out, "    %zu\n", nentries);
                for (row = 0; row<nentries; row++) {
                    fprintf(out, "%-10s ", entries[row]->name);
                    for (col=0; col<row; col++) {
                        fprintf(out, "%6f ", matrix->get(row, col));
                    }
                    fprintf(out, "\n");
                }
            }
            break;
        case DI_SAVE_READABLE:
        case DI_SAVE_TABBED:
            {
                GBDATA         *gb_main  = get_gb_main();
                GB_transaction  ta(gb_main);
                size_t          app_size = 200;     // maximum width for NDS output (and max. height for vertical one)
                size_t          maxnds   = 0;
                bool            tabbed   = (type == DI_SAVE_TABBED);
                double          min      = matrix->get(1, 0) * 100.0;
                double          max      = min;
                double          sum      = 0.0;

                make_node_text_init(gb_main);

                // create all NDS strings
                char **nds_results = new char*[nentries];
                for (col=0; col<nentries; col++) {
                    const char *buf        = entries[col]->name;
                    GBDATA     *gb_species = GBT_find_species_rel_species_data(gb_species_data, buf);
                    if (gb_species) {
                        buf = make_node_text_nds(gb_main, gb_species, NDS_OUTPUT_LEAFTEXT, 0, 0);
                        while (buf[0] == ' ') ++buf;
                    }

                    nds_results[col] = strdup(buf);

                    size_t slen             = strlen(buf);
                    if (slen>app_size) slen = app_size;
                    if (slen>maxnds) maxnds = slen;
                }

#if defined(DEBUG)
                fprintf(out, "maxnds=%zu\n", maxnds);
#endif // DEBUG

                // print column headers :
                if (tabbed) {
                    for (col=0; col<nentries; col++) {
                        if (!tabbed && (col%4) == 0) fputc('\t', out);
                        fprintf(out, "\t%s", nds_results[col]);
                    }
                    fputc('\n', out);
                }
                else {
                    bool *eos_reached = new bool[nentries];
                    for (col = 0; col<nentries; ++col) eos_reached[col] = false;

                    for (row = 0; row<maxnds; ++row) {
                        for (col = 0; col<(maxnds+2); ++col) fputc(' ', out);

                        for (col = 0; col<nentries; ++col) {
                            if (!tabbed && (col%4) == 0) fprintf(out, "  ");
                            if (!eos_reached[col]) {
                                char c = nds_results[col][row];
                                if (c) fprintf(out, "  %c  ", c);
                                else eos_reached[col] = true;
                            }
                        }
                        fputc('\n', out);
                    }

                    delete [] eos_reached;
                }

                // print data lines :
                for (row = 0; row<nentries; row++) {
                    if (!tabbed && (row%4) == 0) fprintf(out, "\n"); // empty line after 4 lines of data

                    if (tabbed) {
                        fprintf(out, "%s", nds_results[row]);
                    }
                    else {
                        char *nds   = nds_results[row];
                        char  c     = nds[maxnds];
                        nds[maxnds] = 0;
                        fprintf(out, "%-*s ", int(maxnds), nds);
                        nds[maxnds] = c;
                    }

                    // print the matrix :
                    for (col=0; col<nentries; col++) {
                        if (tabbed) {
                            fputc('\t', out);
                        }
                        else if ((col%4) == 0) { // empty column after 4 columns
                            fprintf(out, "  ");
                        }

                        double val2 = matrix->get(row, col) * 100.0;
                        {
                            char buf[20];
                            if (val2 > 99.9 || row == col)  sprintf(buf, "%4.0f", val2);
                            else sprintf(buf, "%4.1f", val2);

                            if (tabbed) { // convert '.' -> ','
                                char *dot     = strchr(buf, '.');
                                if (dot) *dot = ',';
                            }
                            fputs(buf, out);
                        }

                        if (!tabbed) fputc(' ', out);

                        if (val2 > max) max = val2;
                        if (val2 < min) min = val2;

                        sum += val2; // ralf: before this added 'val' (which was included somehow)
                    }
                    fprintf(out, "\n");
                }

                fputc('\n', out);

                fprintf(out, "Minimum:\t%f\n", min);
                fprintf(out, "Maximum:\t%f\n", max);
                fprintf(out, "Average:\t%f\n", sum/(nentries*nentries));

                for (col=0; col<nentries; col++) free(nds_results[col]);
                free(nds_results);
            }
            break;
        default:
            error = GB_export_error("Unknown Save Type");
    }
    fclose(out);
    return (char *)error;
}
