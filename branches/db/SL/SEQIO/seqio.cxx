// ============================================================= //
//                                                               //
//   File      : seqio.cxx                                       //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "seqio.hxx"

#include <arbdb.h>

#define sio_assert(cond) arb_assert(cond)

char *SEQIO_fgets(char *s, int size, FILE *stream) {
    // same as fgets but also works with file in MacOS format
    int i;
    for (i = 0; i<(size-1); ++i) {
        int byte = fgetc(stream);
        if (byte == EOF) {
            if (i == 0) return 0;
            break;
        }

        s[i] = byte;
        if (byte == '\n' || byte == '\r') break;
    }
    s[i] = 0;

    return s;
}

bool SEQIO_read_string_pair(FILE *in, char *&s1, char *&s2, size_t& lineNr) {
    // helper function to read import/export filters.
    // returns true if successfully read
    //
    // 's1' is set to a heap-copy of the first token on line
    // 's2' is set to a heap-copy of the rest of the line (or NULL if only one token is present)
    // 'lineNr' is incremented with each line read

    s1 = 0;
    s2 = 0;

    const int  BUFSIZE = 8000;
    char       buffer[BUFSIZE];
    char      *res;

    do {
        res = SEQIO_fgets(&buffer[0], BUFSIZE-1, in);
        if (res) {
            char *p = buffer;

            lineNr++;

            while (*p == ' ' || *p == '\t') p++;
            if (*p != '#') {
                int len = strlen(p)-1;
                while  (len >= 0 && strchr("\t \n\r", p[len])) {
                    p[len--] = 0;
                }

                if (*p) {
                    char *e = strpbrk(p, " \t");

                    if (e) {
                        *(e++) = 0;
                        s1     = strdup(p);

                        e += strspn(e, " \t"); // skip whitespace

                        if (*e == '"') {
                            char *k = strrchr(e, '"');
                            if (k!=e) {
                                e++;
                                *k=0;
                            }
                        }
                        s2 = strdup(e);
                    }
                    else {
                        s1 = strdup(p);
                    }
                }
            }
        }
    } while (res && !s1);                           // read until EOF or something found

    sio_assert(correlated(res, s1));

    return res;
}


