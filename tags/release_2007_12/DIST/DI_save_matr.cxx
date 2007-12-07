#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_tree.hxx>
#include "dist.hxx"
#include <awt_nds.hxx>
#include <awt_csp.hxx>

#include "di_matr.hxx"


const char *PHMATRIX::save(char *filename,enum PH_SAVE_TYPE type)
{
    FILE     *out;
    GB_ERROR  error = 0;

    out = fopen(filename,"w");
    if (!out) return "Cannot save your File";

    size_t row,col;
    switch (type){
        case PH_SAVE_PHYLIP_COMP:
            {
                fprintf(out,"    %li\n",nentries);
                for (row = 0; row<size_t(nentries);row++){
                    fprintf(out,"%-10s ",entries[row]->name);
                    for (col=0; col<row; col++) {
                        fprintf(out,"%6f ",matrix->get(row,col));
                    }
                    fprintf(out,"\n");
                }
            }
            break;
        case PH_SAVE_READABLE:
        case PH_SAVE_TABBED:
            {
                GB_transaction  dummy(gb_main);
                size_t	        app_size = 200; // maximum width for NDS output (and max. height for vertical one)
                size_t	        maxnds   = 0;
                bool            tabbed   = (type == PH_SAVE_TABBED);
                double          min      = matrix->get(1,0) * 100.0;
                double          max      = min;
                double          sum      = 0.0;

                make_node_text_init(gb_main);

                // create all NDS strings
                char **nds_results = new char*[nentries];
                for (col=0; col<size_t(nentries); col++) {
                    const char *buf        = entries[col]->name;
                    GBDATA     *gb_species = GBT_find_species_rel_species_data(gb_species_data,buf);
                    if (gb_species) {
                        buf = make_node_text_nds(gb_main,gb_species,0,0,0);
                        while (buf[0] == ' ') ++buf;
                    }

                    nds_results[col] = strdup(buf);

                    size_t slen             = strlen(buf);
                    if (slen>app_size) slen = app_size;
                    if (slen>maxnds) maxnds = slen;
                }

#if defined(DEBUG)
                fprintf(out, "maxnds=%i\n", maxnds);
#endif // DEBUG

                // print column headers :
                if (tabbed) {
                    for (col=0; col<size_t(nentries); col++) {
                        if ( !tabbed && (col%4) == 0) fputc('\t', out);
                        fprintf(out, "\t%s", nds_results[col]);
                    }
                    fputc('\n', out);
                }
                else {
                    bool *eos_reached = new bool[nentries];
                    for (col = 0; col<size_t(nentries); ++col) eos_reached[col] = false;

                    for (row = 0; row<maxnds; ++row) {
                        for (col = 0; col<(maxnds+2); ++col) fputc(' ', out);

                        for (col = 0; col<size_t(nentries); ++col) {
                            if ( !tabbed && (col%4) == 0) fprintf(out, "  ");
                            if (!eos_reached[col]) {
                                char c = nds_results[col][row];
                                if (c) fprintf(out,"  %c  ", c);
                                else eos_reached[col] = true;
                            }
                        }
                        fputc('\n', out);
                    }

                    free(eos_reached);
                }

                // print data lines :
                for (row = 0; row<size_t(nentries);row++){
                    if (!tabbed && (row%4) == 0) fprintf(out,"\n"); // empty line after 4 lines of data

                    if (tabbed) {
                        fprintf(out, "%s", nds_results[row]);
                    }
                    else {
                        char *nds   = nds_results[row];
                        char  c     = nds[maxnds];
                        nds[maxnds] = 0;
                        fprintf(out,"%-*s ", maxnds, nds);
                        nds[maxnds] = c;
                    }

                    // print the matrix :
                    for (col=0; col<size_t(nentries); col++) {
                        if (tabbed) {
                            fputc('\t', out);
                        }
                        else if ((col%4) == 0) { // empty column after 4 columns
                            fprintf(out,"  ");
                        }

                        double val2 = matrix->get(row,col) * 100.0;
                        {
                            char buf[20];
                            if (val2 > 99.9 || row == col)  sprintf(buf,"%4.0f",val2);
                            else sprintf(buf,"%4.1f",val2);

                            if (tabbed) { // convert '.' -> ','
                                char *dot     = strchr(buf, '.');
                                if (dot) *dot = ',';
                            }
                            fputs(buf, out);
                        }

                        if (!tabbed) fputc(' ', out);

                        if (val2 > max) max  = val2;
                        if (val2 < min) min  = val2;
                        sum                 += val;
                    }
                    fprintf(out,"\n");
                }

                fputc('\n', out);

                fprintf(out,"Minimum:\t%f\n",min);
                fprintf(out,"Maximum:\t%f\n",max);
                fprintf(out,"Average:\t%f\n",sum/(nentries*nentries) );
            }
            break;
        default:
            error = GB_export_error("Unknown Save Type");
    }
    fclose(out);
    return (char *)error;
}
