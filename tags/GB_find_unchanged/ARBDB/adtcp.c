/* =============================================================== */
/*                                                                 */
/*   File      : adtcp.c                                           */
/*   Purpose   : arb_tcp.dat handling                              */
/*   Time-stamp: <Fri May/04/2007 17:56 MET Coder@ReallySoft.de>   */
/*                                                                 */
/*   Coded by Ralf Westram (coder@reallysoft.de) in April 2007     */
/*   Institute of Microbiology (Technical University Munich)       */
/*   www.arb-home.de                                               */
/* =============================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "adlocal.h"
#include "arbdbt.h"

#if defined(DEBUG)
/* #define DUMP_ATD_ACCESS */
#endif /* DEBUG */

/* ------------------------------------------------------------ */
/* Data representing current content of arb_tcp.dat */

static GB_ULONG  ATD_modtime  = -1; /* modification time of read-in arb_tcp.dat */
static char     *ATD_filename = 0; /* pathname of loaded arb_tcp.dat */

static char **ATD_content = 0; /* zero-pointer terminated array of multi-separated strings
                                * (strings have same format as the result of
                                * GBS_read_arb_tcp(), but also contain the first element,
                                * i.e. the server id)
                                */

/* ------------------------------------------------------------ */

static const char *get_ATD_entry(const char *serverID) {
    const char *result = 0;
    if (ATD_content) {
        int c;
        for (c = 0; ATD_content[c]; c++) {
            const char *id = ATD_content[c];

            if (strcmp(id, serverID) == 0) {
                result = strchr(id, 0)+1; /* return pointer to first parameter */
                break;
            }
        }
    }
    return result;
}

/* ------------------------------------------------------------ */
#if defined(DUMP_ATD_ACCESS)
static void dump_ATD() {
    printf("ATD_filename='%s'\n", ATD_filename);
    printf("ATD_modtime='%lu'\n", ATD_modtime);
    if (ATD_content) {
        int c;
        for (c = 0; ATD_content[c]; c++) {
            char *data = ATD_content[c];
            char *tok  = data;
            printf("Entry #%i:\n", c);

            while (tok[0]) {
                printf("- '%s'\n", tok);
                tok = strchr(tok, 0)+1;
            }
        }
    }
    else {
        printf("No ATD_content\n");
    }
}
#endif /* DUMP_ATD_ACCESS */
/* ------------------------------------------------------------ */

static void freeContent() {
    if (ATD_content) {
        int c;
        for (c = 0; ATD_content[c]; c++) free(ATD_content[c]);
        free(ATD_content);
        ATD_content = 0;
    }
}

#define MAXLINELEN 512
#define MAXTOKENS  10

static GB_ERROR read_arb_tcp_dat(const char *filename, int *versionFound) {
    /* used to read arb_tcp.dat or arb_tcp_org.dat */
    GB_ERROR  error = 0;
    FILE     *in    = fopen(filename, "rt");

    *versionFound = 1; // default to version 1 (old format)

#if defined(DUMP_ATD_ACCESS)
    printf("(re)reading %s\n", filename);
#endif /* DUMP_ATD_ACCESS */

    freeContent();

    if (!in) {
        error = GBS_global_string("Can't read '%s'", filename);
    }
    else {
        char   buffer[MAXLINELEN+1];
        char  *lp;
        int    lineNumber = 0;
        char **tokens     = malloc(MAXTOKENS*sizeof(*tokens));

        int    entries_allocated = 30;
        int    entries           = 0;
        char **entry             = malloc(entries_allocated*sizeof(*entry));

        if (!tokens || !entry) error = "Out of memory";

        for (lp = fgets(buffer, MAXLINELEN, in);
             lp && !error;
             lp = fgets(buffer, MAXLINELEN, in))
        {
            char *tok;
            int   tokCount = 0;
            int   t;

            lineNumber++;

            while ((tok = strtok(lp, " \t\n"))) {
                if (tok[0] == '#') break; /* EOL comment -> stop */
                if (tokCount >= MAXTOKENS) { error = "Too many tokens"; break; }
                tokens[tokCount] = tokCount ? GBS_eval_env(tok) : GB_strdup(tok);
                if (!tokens[tokCount]) { error = GB_get_error(); break; }
                tokCount++;
                lp = 0;
            }

            if (!error && tokCount>0) {
                if (strcmp(tokens[0], "ARB_TCP_DAT_VERSION") == 0) {
                    if (tokCount>1) *versionFound = atoi(tokens[1]);
                }
                else {
                    char *data;
                    {
                        int allsize = 0;
                        int size[MAXTOKENS];

                        for (t = 0; t<tokCount; t++) {
                            size[t]  = strlen(tokens[t])+1;
                            allsize += size[t];
                        }
                        allsize++;      /* additional zero byte */

                        data = malloc(allsize);
                        {
                            char *d = data;
                            for (t = 0; t<tokCount; t++) {
                                memmove(d, tokens[t], size[t]);
                                d += size[t];
                            }
                            d[0] = 0;
                        }
                    }

                    if (entries == entries_allocated) {
                        char **entry2;
                        entries_allocated = (int)(entries_allocated*1.5);
                        entry2            = realloc(entry, entries_allocated*sizeof(*entry));

                        if (!entry2) error = "Out of memory";
                        else entry = entry2;
                    }
                    if (!error) {
                        entry[entries++] = data;
                        data             = 0;
                    }
                    free(data);
                }
            }

            if (error) {
                error = GBS_global_string("%s (in line %i of '%s')", error, lineNumber, filename);
            }

            for (t = 0; t<tokCount; t++) {
                free(tokens[t]);
                tokens[t] = 0;
            }
        }

        ATD_content = realloc(entry, (entries+1)*sizeof(*entry));
        if (!ATD_content) error = "Out of memory";
        else ATD_content[entries] = 0;

        free(tokens);
        fclose(in);
    }

#if defined(DUMP_ATD_ACCESS)
    dump_ATD();
#endif /* DUMP_ATD_ACCESS */
    return error;
}

static GB_ERROR load_arb_tcp_dat() {
    /* read arb_tcp.dat (once or if changed on disk) */
    GB_ERROR  error    = 0;
    char     *filename = GBS_find_lib_file("arb_tcp.dat","", 1);

    if (!filename) {
        error = GBS_global_string("File $ARBHOME/lib/arb_tcp.dat not found");
    }
    else {
        struct stat st;
        if (stat(filename, &st) == 0) {
            GB_ULONG modtime = st.st_mtime;
            if (ATD_modtime != modtime) { // read once or when changed
                int arb_tcp_version;

                error = read_arb_tcp_dat(filename, &arb_tcp_version);
                if (!error) {
                    int expected_version = 2;
                    if (arb_tcp_version != expected_version) {
                        error = GBS_global_string("Expected arb_tcp.dat version %i\n"
                                                  "Your '%s' has version %i\n"
                                                  "To solve the problem\n"
                                                  "- either reinstall ARB and do not select\n"
                                                  "  'Use information of already installed ARB'\n"
                                                  "  (any changes to arb_tcp.dat will be lost)\n"
                                                  "- or backup your changed %s,\n"
                                                  "  replace it by the contents from $ARBHOME/lib/arb_tcp_org.dat\n"
                                                  "  and edit it to fit your needs.",
                                                  expected_version,
                                                  filename, arb_tcp_version,
                                                  filename);
                    }
                }

                free(ATD_filename);
                if (!error) {
                    ATD_modtime  = modtime;
                    ATD_filename = filename;
                    filename     = 0;
                }
                else {
                    ATD_modtime  = -1;
                    ATD_filename = 0;
                }
            }
        }
        else {
            error = GBS_global_string("Can't stat '%s'", filename);
        }
    }
    free(filename);

#if defined(DUMP_ATD_ACCESS)
    printf("error=%s\n", error);
#endif /* DUMP_ATD_ACCESS */

    return error;
}

const char *GBS_scan_arb_tcp_param(const char *ipPort, const char *wantedParam) {
    /* search a specific server parameter in result from GBS_read_arb_tcp()
     * wantedParam may be sth like '-d' (case is ignored!)
     */
    const char *result = 0;
    if (ipPort) {
        const char *exe   = strchr(ipPort, 0)+1;
        const char *param = strchr(exe, 0)+1;
        size_t      plen  = strlen(param);
        size_t      wplen = strlen(wantedParam);

        while (plen) {
            if (gbs_strnicmp(param, wantedParam, wplen) == 0) { /* starts with wantedParam */
                result = param+wplen;
                break;
            }
            param += plen+1;    /* position behind 0-delimiter */
            plen   = strlen(param);
        }
    }
    return result;
}

/********************************************************************************************
            Find an entry in the $ARBHOME/lib/arb_tcp.dat file
********************************************************************************************/

/* Be aware: GBS_read_arb_tcp returns a quite unusual string containing several string delimiters (0-characters).
   It contains all words (separated by space or tab in arb_tcp.dat) of the corresponding line in arb_tcp.dat.
   These words get concatenated (separated by 0 characters).

   The first word (which matches the parameter env) is skipped.
   The second word (the socket info = "host:port") is returned directly as result.
   The third word is the server executable name.
   Thr fourth and following words are parameters to the executable. 

   To access these words follow this example:

   const char *ipPort    = GBS_read_arb_tcp("ARB_NAME_SERVER");
   if (ipPort) {
   const char *serverExe = strchr(ipPort, 0)+1;
   const char *para1     = strchr(serverExe, 0)+1; // always exists!
   const char *para2     = strchr(para1, 0)+1;
   if (para2[0]) {
   // para2 exists
   }
   }

   see also GBS_read_arb_tcp_param() above
*/

const char *GBS_read_arb_tcp(const char *env) {
    const char *result = 0;
    GB_ERROR    error  = 0;

    if (strchr(env,':')){
        static char *resBuf = 0;

        free(resBuf);
        resBuf = GB_STRDUP(env);
        result = resBuf;
    }
    else {
        error = load_arb_tcp_dat(); /* load once or reload after change on disk */
        if (!error) {
            const char *user = GB_getenvUSER();
            if (!user) {
                error = "Environment variable 'USER' not defined";
            }
            else {
                char *envuser = GBS_global_string_copy("%s:%s", user, env);
                result        = get_ATD_entry(envuser);

                if (!result) { /* no user-specific entry found */
                    result = get_ATD_entry(env);
                    if (!result) {
                        error = GBS_global_string("Expected entry '%s' or '%s' in '%s'",
                                                  env, envuser, ATD_filename);
                    }
                }
                free(envuser);
            }
        }
    }

    if (error) {
        GB_export_error(error);
        result = 0;
    }
    return result;
}

const char * const *GBS_get_arb_tcp_entries(const char *matching) {
    /* returns a list of all matching non-user-specific entries found in arb_tcp.dat
     * match is performed by GBS_string_cmp (e.g. use "ARB_PT_SERVER*")
     */
    static const char **matchingEntries     = 0;
    static int          matchingEntriesSize = 0;

    GB_ERROR error = 0;

    error = load_arb_tcp_dat();
    if (!error) {
        int count   = 0;
        int matched = 0;
        int c;
        
        while (ATD_content[count]) count++;

        if (matchingEntriesSize != count) {
            free(matchingEntries);
            matchingEntries     = malloc((count+1)*sizeof(*matchingEntries));
            matchingEntriesSize = count;
        }

        for (c = 0; c<count; c++) {
            const char *id = ATD_content[c];

            if (strchr(id, ':') == 0) { /* not user-specific */
                if (GBS_string_cmp(id, matching, 0) == 0) { /* matches */
                    matchingEntries[matched++] = id;
                }
            }
        }
        matchingEntries[matched] = 0; /* end of array */
    }
    if (error) GB_export_error(error);
    return error ? 0 : matchingEntries;
}

/* --------------------------- */
/*      pt server related      */
/* --------------------------- */

const char *GBS_ptserver_logname() {
    static char *serverlog = 0;
    if (!serverlog) serverlog = GBS_eval_env("$(ARBHOME)/lib/pts/ptserver.log");
    return serverlog;
}

void GBS_add_ptserver_logentry(const char *entry) {
    FILE *log = fopen(GBS_ptserver_logname(), "at");
    if (log) {
        char       atime[256];
        time_t     t   = time(0);
        struct tm *tms = localtime(&t);
        strftime(atime, 255, "%Y/%m/%d %k:%M:%S", tms);
        fprintf(log, "%s %s\n", atime, entry);
        fclose(log);
    }
    else {
        fprintf(stderr, "Failed to write to '%s'\n", GBS_ptserver_logname());
    }
}

char *GBS_ptserver_id_to_choice(int i, int showBuild) {
    /* Return a readable name for PTserver number 'i'
       if 'showBuild' then show build info as well
    */
    char       *serverID = GBS_global_string_copy("ARB_PT_SERVER%i", i);
    const char *ipPort   = GBS_read_arb_tcp(serverID);
    char       *result   = 0;

    if (ipPort) {
        const char *file     = GBS_scan_arb_tcp_param(ipPort, "-d");
        const char *nameOnly = strrchr(file, '/');

        if (nameOnly) nameOnly++;   /* position behind '/' */
        else nameOnly = file;       /* otherwise show complete file */

        {
            char *remote      = GB_strdup(ipPort);
            char *colon       = strchr(remote, ':');
            if (colon) *colon = 0; /* hide port */

            if (strcmp(remote, "localhost") == 0) { /* hide localhost */
                result = GB_strdup(nameOnly);
            }
            else {
                result = GBS_global_string_copy("%s: %s", remote, nameOnly);
            }
            free(remote);
        }


        if (showBuild) {
            struct stat  st;

            if (stat(file, &st) == 0) { // xxx.arb present
                time_t  fileMod   = st.st_mtime;
                char   *serverDB  = GBS_global_string_copy("%s.pt", file);
                char   *newResult = 0;

                if (stat(serverDB, &st) == 0) { // pt-database present
                    if (st.st_mtime < fileMod) { // DB is newer than pt-database
                        newResult = GBS_global_string_copy("%s [starting or failed update]", result);
                    }
                    else {
                        char       atime[256];
                        struct tm *tms  = localtime(&st.st_mtime);

                        strftime(atime, 255,"%Y/%m/%d %k:%M", tms);
                        newResult = GBS_global_string_copy("%s [%s]", result, atime);
                    }
                }
                else { // check for running build
                    char *serverDB_duringBuild = GBS_global_string_copy("%s%%", serverDB);
                    if (stat(serverDB_duringBuild, &st) == 0) {
                        newResult = GBS_global_string_copy("%s [building..]", result);
                    }
                    free(serverDB_duringBuild);
                }

                if (newResult) { free(result); result = newResult; }
                free(serverDB);
            }
        }
    }
    free(serverID);

    return result;
}

