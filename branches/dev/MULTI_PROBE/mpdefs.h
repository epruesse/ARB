// =============================================================== //
//                                                                 //
//   File      : mpdefs.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef MPDEFS_H
#define MPDEFS_H

#include <PT_com.h>

#define SEPARATOR "#"

#define NON_WEIGHTED 0

enum
    {
        MP_NO_PROBE = 0,
        MP_PROBE1   = 1,
        MP_PROBE2   = 2,
        MP_PROBE3   = 4,
        MP_PROBE4   = 8,
        MP_PROBE5   = 16,
        MP_PROBE6   = 32,
        MP_PROBE7   = 64,
        MP_PROBE8   = 128
    };

class Bakt_Info;
class Hit;
class Sonde;
class MO_Liste;
class Sondentopf;

typedef struct _baktmm
{
    long   nummer;
    double mismatch;
} MO_Mismatch;


struct MP_list_elem
{
    void         *elem;
    MP_list_elem *next;
};

struct apd_sequence {
    apd_sequence *next;
    char         *sequence;
};

extern struct Params {
    int   DESIGNCPLIPOUTPUT;
    int   SERVERID;
    char *DESIGNNAMES;
#if defined(DEVEL_RALF)
#warning DRY (DESIGNNAMES is completely unused)
#endif
    int   DESIGNPROBELENGTH;
    char *DESIGNSEQUENCE;

    int   MINTEMP;
    int   MAXTEMP;
    int   MINGC;
    int   MAXGC;
    int   MAXBOND;
    int   MINPOS;
    int   MAXPOS;
    int   MISHIT;
    int   MINTARGETS;
    char *SEQUENCE;
    int   MISMATCHES;
    int   COMPLEMENT;
    int   WEIGHTED;

    apd_sequence *sequence;
} P;


extern struct mp_gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
    int        pd_design_id;
} mp_pd_gl;

#else
#error mpdefs.h included twice
#endif // MPDEFS_H
