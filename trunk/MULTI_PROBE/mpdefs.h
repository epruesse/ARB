#ifndef MPDEFS
#define MPDEFS

//#include <mpdefs2.h>

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef arbdb_h_included
#include <arbdb.h>
#endif

#include <PT_com.h>
#include <client.h>

#include <servercntrl.h>

#define TRUE 		1
#define FALSE 		0
#define SEPARATOR	"#"

#define NON_WEIGHTED 0

enum
    {
        MP_NO_PROBE = 0,
        MP_PROBE1 	= 1,
        MP_PROBE2 	= 2,
        MP_PROBE3 	= 4,
        MP_PROBE4 	= 8,
        MP_PROBE5 	= 16,
        MP_PROBE6 	= 32,
        MP_PROBE7 	= 64,
        MP_PROBE8 	= 128
    };

class Bakt_Info;
class Hit;
class Sonde;
class MO_Liste;
class Sondentopf;

typedef struct _baktmm
{
    long	nummer;
    double 	mismatch;
} MO_Mismatch;


struct MP_list_elem
{
    void                    *elem;
    MP_list_elem           *next;
};

struct apd_sequence {
	apd_sequence *next;
	char	*sequence;
};

extern struct Params{
	int	DESIGNCPLIPOUTPUT;
	int	SERVERID;
	char *	DESINGNAMES;
	int	DESIGNPROBELENGTH;
	char *	DESIGNSEQUENCE;

	int	MINTEMP;
	int	MAXTEMP;
	int	MINGC;
	int	MAXGC;
	int	MAXBOND;
	int	MINPOS;
	int	MAXPOS;
	int	MISHIT;
	int	MINTARGETS;
	char *	SEQUENCE;
	int	MISMATCHES;
	int	COMPLEMENT;
	int	WEIGHTED;

	apd_sequence *sequence;
} P;


extern struct mp_gl_struct{
	aisc_com *link;
	T_PT_LOCS locs;
	T_PT_MAIN com;
	int	pd_design_id;
} mp_pd_gl;




#endif
