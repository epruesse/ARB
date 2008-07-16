// =============================================================== //
//                                                                 //
//   File      : trace.h                                           //
//   Purpose   :                                                   //
//   Time-stamp: <Tue May/22/2007 21:06 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef TRACE_H
#define TRACE_H

#if defined(DEBUG)
// #define DUMP_COMMUNICATION
#endif /* DEBUG */

/* -------------------------------------------------------------------------------- */

#if defined(DUMP_COMMUNICATION)

static void aisc_dump_hex(const char *title, const char *data, int datasize) {
    const unsigned char *udata = (const unsigned char *)data;
    int d;

    fprintf(stderr, "%s", title);
    for (d = 0; d<datasize; d++) {
        fprintf(stderr, "%02X", udata[d]);
    }
    fprintf(stderr, "\n");
}

static void aisc_dump_int(const char *where, const char *varname, int var) {
    fprintf(stderr, "AISC_DUMP: %s: int    %s=%i ", where, varname, var);
    aisc_dump_hex("Hex: ", (const char*)&var, sizeof(var));
}
static void aisc_dump_long(const char *where, const char *varname, long var) {
    fprintf(stderr, "AISC_DUMP: %s: long   %s=%li ", where, varname, var);
    aisc_dump_hex("Hex: ", (const char*)&var, sizeof(var));
}
static void aisc_dump_double(const char *where, const char *varname, double var) {
    fprintf(stderr, "AISC_DUMP: %s: double %s=%f ", where, varname, (float)var);
    aisc_dump_hex("Hex: ", (const char*)&var, sizeof(var));
}
static void aisc_dump_charPtr(const char *where, const char *varname, const char *var) {
    fprintf(stderr, "AISC_DUMP: %s: cPtr   %s='%s' ", where, varname, var);
    aisc_dump_hex("Hex: ", var, strlen(var)+1);
}
static void aisc_dump_voidPtr2(const char *where, const char *varname, void *var) {
    fprintf(stderr, "AISC_DUMP: %s: ptr    %s=%p\n", where, varname, var);
}

#define aisc_dump_voidPtr(w, n, v) aisc_dump_voidPtr2(w, n, (void*)v)
#define AISC_DUMP(where, type, var) aisc_dump_##type(#where, #var, var)
#define AISC_DUMP_SEP() fprintf(stderr, "-----------------------------\n")

#else
#define AISC_DUMP(where, type, var) 
#define AISC_DUMP_SEP() 
#endif /* DUMP_COMMUNICATION */

/* -------------------------------------------------------------------------------- */

#else
#error trace.h included twice
#endif // TRACE_H

