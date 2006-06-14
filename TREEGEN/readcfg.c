#include "readcfg.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define COMMENT     ';'
#define MAXLEN  2048

static cstr error_message;   /* Kann von decode gesetzt werden */
                             /* und wird dann als Fehlermeldung */
                             /* ausgegeben */

/* -------------------------------------------------------------------------- */
/*      static void scanKeywords(struct S_cfgLine line[], int *lineanz, int... */
/* ------------------------------------------------------ 18.05.95 02.10 ---- */
/* */
/*  Anzahl und maximale L„nge der Keywords bestimmen */
/* */
static void scanKeywords(struct S_cfgLine line[], int *lineanz, int *maxlen)
{
    while (1)
    {
        cstr keyword    = line[*lineanz].keyword;
        int  keywordlen;

        if (!keyword) break;
        keywordlen = strlen(keyword);
        if (keywordlen>*maxlen) *maxlen = keywordlen;
        (*lineanz)++;
    }
}
/* -------------------------------------------------------------------------- */
/*      static void cfgReadWarning(cstr fname, int lineno) */
/* ------------------------------------------------------ 18.05.95 02.23 ---- */
static void cfgReadWarning(cstr fname, int lineno)
{
    warningf("Error in line %i of '%s'", lineno, fname);
}
/* -------------------------------------------------------------------------- */
/*      int readCfg(cstr fname, struct S_cfgLine line[]) */
/* ------------------------------------------------------ 17.05.95 21:23 ---- */
/* */
/*  liest und dekodiert CFG-Datei */
/* */
int readCfg(cstr fname, struct S_cfgLine line[])
{
    FILE *in = fopen(fname, "r");
    int   ok = in!=NULL;

    if (!ok && errno==ENOENT)                              /* Datei nicht vorhanden */
    {
        FILE *out = fopen(fname, "w");

        if (out)
        {
            fprintf(out, ";\n; arb_treegen CFG-File '%s'\n;\n\n", fname);
            fclose(out);

            warningf("'%s' has been generated.", fname);

            in = fopen(fname, "r");
            ok = in!=NULL;
        }
    }

    if (ok)
    {
        char  linebuf[MAXLEN];
        int   readLines        = 0,                        /* Anzahl gelesener Zeilen */
              keywords         = 0,                        /* Anzahl keywords */
              maxKeywordlen    = 0,                        /* Laenge des laengsten keywords */
              lineno           = 0,                        /* fortlaufende Zeilennummer */
             *wordRead,                                    /* welche Worte wurden in welcher Zeile gelesen */
              x;

        scanKeywords(line, &keywords, &maxKeywordlen);

        wordRead = (int*)malloc(keywords*sizeof(int));
        for (x = 0; x<keywords; x++) wordRead[x] = 0;

        /* /----------------------------\ */
        /* |  CFG lesen und decodieren  | */
        /* \----------------------------/ */

        while (1)
        {
            str firstWord;

            if (!fgets(linebuf, MAXLEN, in)) break;
            lineno++;

            firstWord = strtok(linebuf, " \n");

            if (firstWord && firstWord[0]!=COMMENT && firstWord[0]!=0)
            {
                int search;

                for (search = 0; search<keywords; search++)
                {
                    if (strcmp(line[search].keyword, firstWord)==0)
                    {
                        str restDerZeile = strtok(NULL, "\n");
                        int decoded;

                        error_message = NULL;
                        decoded       = line[search].decode(restDerZeile, line[search].varPointer);

                        if (!decoded)
                        {
                            cfgReadWarning(fname, lineno);
                            if (error_message) warning(error_message);
                            else               warningf("Can't interpret '%s'", line[search].keyword);
                            ok = 0;
                        }
                        else
                        {
                            if (wordRead[search])          /* schonmal gelesen? */
                            {
                                if (decoded!=2)            /* mehrfache Verwendung erlaubt? */
                                {
                                    cfgReadWarning(fname, lineno);
                                    warningf("Keyword '%s' duplicated (already specified in line %i)", firstWord, wordRead[search]);
                                    ok = 0;
                                }
                            }
                            else                           /* nein, dann merken! */
                            {
                                wordRead[search] = lineno;
                                readLines++;
                            }
                        }

                        break;
                    }
                }

                if (search==keywords)                      /* keyword nicht gefunden! */
                {
                    cfgReadWarning(fname, lineno);
                    warningf("Unknown Keyword '%s'", firstWord);
                    ok = 0;
                }
            }
        }

        fclose(in);

        /* /----------------------------------------------\ */
        /* |  Waren alle keywords im CFG-File vorhanden?  | */
        /* \----------------------------------------------/ */

        if (ok && readLines<keywords)                      /* nein :-( */
        {
            FILE *out = fopen(fname, "a");

            if (out)
            {
                for (x = 0; x<keywords; x++)
                {
                    if (!wordRead[x])
                    {
                        fprintf(out, ";%s\n"
                                     "\n"
                                     "%-*s %s\n"
                                     "\n",
                                     line[x].description,
                                     maxKeywordlen,
                                     line[x].keyword,
                                     line[x].defaultVal);
                    }
                }

                fclose(out);
                warningf("Missing keywords appended to '%s'.", fname);
            }

            ok = 0;
        }

        free(wordRead);
    }

    return ok;
}
/* -------------------------------------------------------------------------- */
/*      void setCfgError(cstr message) */
/* ------------------------------------------------------ 17.05.95 22.01 ---- */
void setCfgError(cstr message)
{
    error_message = message;
}


