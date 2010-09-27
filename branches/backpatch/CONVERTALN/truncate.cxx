#include <stdio.h>
#include <string.h>
#include "convert.h"
#include "global.h"


/* ------------------------------------------------------------ */
/*  Function truncate_over_80().
    /*      Initially used for truncating line over 80 char.
    /*      of AE2 file.  Now might not be useful anymore...
*/
void truncate_over_80(FILE * inf, FILE * outf)
{
    char *eof, line[80], c;

    eof = fgets(line, 80, inf);
    for (; eof != NULL;) {
        if (strlen(line) >= 79) {
            fprintf(stderr, "OVERFLOW LINE: %s", line);
            for (; (c = fgetc(inf)) != '\n' && c != 0;) ;
        }
        eof = fgets(line, 80, inf);
    }
}
