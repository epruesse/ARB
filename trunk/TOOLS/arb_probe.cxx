#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <PT_com.h>
#include <client.h>

#include <arbdb.h>
#include <servercntrl.h>

struct apd_sequence {
    apd_sequence *next;
    const char *sequence;
};

struct Params{
    int         DESIGNCPLIPOUTPUT;
    int         SERVERID;
    const char *DESINGNAMES;
    int         DESIGNPROBELENGTH;
    const char *DESIGNSEQUENCE;

    int         MINTEMP;
    int         MAXTEMP;
    int         MINGC;
    int         MAXGC;
    int         MAXBOND;
    int         MINPOS;
    int         MAXPOS;
    int         MISHIT;
    int         MINTARGETS;
    const char *SEQUENCE;
    int         MISMATCHES;
    int         COMPLEMENT;
    int         WEIGHTED;

    apd_sequence *sequence;
} P;


struct gl_struct {
    aisc_com *link;
    T_PT_LOCS locs;
    T_PT_MAIN com;
    int pd_design_id;
} pd_gl;


int init_local_com_struct()
{
    const char *user = GB_getenvUSER();

    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

void aw_message(const char *error){
    printf("%s\n",error);
}

char *probe_pt_look_for_server()
{
    char choice[256];
    sprintf(choice,"ARB_PT_SERVER%i",P.SERVERID);
    GB_ERROR error;
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER,choice,0);
    if (error) {
        aw_message((char *)error);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}


int probe_design_send_data(T_PT_PDC  pdc) {
    //    int i;
    //    char buffer[256];

    if (aisc_put(pd_gl.link,PT_PDC, pdc, PDC_CLIPRESULT, P.DESIGNCPLIPOUTPUT, 0))
        return 1;

    return 0;
}

void AP_probe_design_event() {
    char *servername;
    //    char  buffer[256];
    T_PT_PDC  pdc;
    T_PT_TPROBE tprobe;
    bytestring bs;
    char *match_info;

    if( !(servername=(char *)probe_pt_look_for_server()) ){
        return;
    }

    pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com,AISC_MAGIC_NUMBER);
    free (servername); servername = 0;

    if (!pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        return;
    }
    if (init_local_com_struct() ) {
        aw_message ("Cannot contact Probe bank server (2)");
        return;
    }

    bs.data = (char*)(P.DESINGNAMES);
    bs.size = strlen(bs.data)+1;

    aisc_create(pd_gl.link,PT_LOCS, pd_gl.locs,
                LOCS_PROBE_DESIGN_CONFIG, PT_PDC,   &pdc,
                PDC_PROBELENGTH,P.DESIGNPROBELENGTH,
                PDC_MINTEMP,    (double)P.MINTEMP,
                PDC_MAXTEMP,    (double)P.MAXTEMP,
                PDC_MINGC,  P.MINGC/100.0,
                PDC_MAXGC,  P.MAXGC/100.0,
                PDC_MAXBOND,    P.MAXBOND,
                0);
    aisc_put(pd_gl.link,PT_PDC, pdc,
             PDC_MINPOS,    P.MINPOS,
             PDC_MAXPOS,    P.MAXPOS,
             PDC_MISHIT,    P.MISHIT,
             PDC_MINTARGETS,    P.MINTARGETS/100.0,
             0);

    if (probe_design_send_data(pdc)) {
        aw_message ("Connection to PT_SERVER lost (1)");
        return;
    }
    apd_sequence *s;
    for ( s= P.sequence; s; s = s->next){
        bytestring bs_seq;
        T_PT_SEQUENCE pts;
        bs_seq.data = (char*)s->sequence;
        bs_seq.size = strlen(bs_seq.data)+1;
        aisc_create(pd_gl.link, PT_PDC, pdc,
                    PDC_SEQUENCE, PT_SEQUENCE, &pts,
                    SEQUENCE_SEQUENCE, &bs_seq,
                    0);
    }

    aisc_put(pd_gl.link,PT_PDC, pdc,
             PDC_NAMES,&bs,
             PDC_GO, 0,
             0);

    {
        char *locs_error = 0;
        if (aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
                      LOCS_ERROR    ,&locs_error,
                      0)){
            aw_message ("Connection to PT_SERVER lost (1)");
            return;
        }
        if (*locs_error) {
            aw_message(locs_error);
        }
        delete locs_error;
    }

    aisc_get( pd_gl.link, PT_PDC, pdc,
              PDC_TPROBE, &tprobe,
              0);


    if (tprobe) {
        aisc_get( pd_gl.link, PT_TPROBE, tprobe,
                  TPROBE_INFO_HEADER,   &match_info,
                  0);
        printf("%s\n",match_info);
        free(match_info);
    }


    while ( tprobe ){
        if (aisc_get( pd_gl.link, PT_TPROBE, tprobe,
                      TPROBE_NEXT,      &tprobe,
                      TPROBE_INFO,      &match_info,
                      0)) break;
        printf("%s\n",match_info);
    }
    aisc_close(pd_gl.link); pd_gl.link = 0;
    return;
}

void AP_probe_match_event()
{
    char           *servername;
    //    char  buffer[256];
    //    char  result[1024];
    T_PT_PDC        pdc;
    T_PT_MATCHLIST  match_list;
    //    char *match_info, *match_name;
    //    int   mark;
    char           *probe = 0;
    char           *locs_error;


    if( !(servername=(char *)probe_pt_look_for_server()) ){
        return;
    }

    pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com,AISC_MAGIC_NUMBER);
    free (servername); servername = 0;

    if (!pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        return;
    }
    if (init_local_com_struct() ) {
        aw_message ("Cannot contact Probe bank server (2)");
        return;
    }

    aisc_create(pd_gl.link,PT_LOCS, pd_gl.locs,
                LOCS_PROBE_DESIGN_CONFIG, PT_PDC,   &pdc,
                0);
    if (probe_design_send_data(pdc)) {
        aw_message ("Connection to PT_SERVER lost (2)");
        return;
    }



    if (aisc_nput(pd_gl.link,PT_LOCS, pd_gl.locs,
                  LOCS_MATCH_REVERSED,      P.COMPLEMENT,
                  LOCS_MATCH_SORT_BY,       P.WEIGHTED,
                  LOCS_MATCH_COMPLEMENT,        0,
                  LOCS_MATCH_MAX_MISMATCHES,    P.MISMATCHES,
                  LOCS_MATCH_MAX_SPECIES,       100000,
                  LOCS_SEARCHMATCH,     P.SEQUENCE,
                  0)){
        free(probe);
        aw_message ("Connection to PT_SERVER lost (2)");
        return;
    }

    long match_list_cnt;
    bytestring bs;
    bs.data = 0;
    aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
              LOCS_MATCH_LIST,  &match_list,
              LOCS_MATCH_LIST_CNT,  &match_list_cnt,
              LOCS_MATCH_STRING,    &bs,
              LOCS_ERROR,       &locs_error,
              0);
    if (*locs_error) {
        aw_message(locs_error);
    }
    free(locs_error);

    char toksep[2];
    toksep[0] = 1;
    toksep[1] = 0;
    printf("%s\n",bs.data);

    aisc_close(pd_gl.link);
    free(bs.data);

    return;
}

int pargc;
char **pargv;
int helpflag;

int getInt(const char *param, int val, int min, int max, const char *description){
    if (helpflag){
        printf("    %s=%3i [%3i:%3i] %s\n",param,val,min,max,description);
        return 0;
    }
    int   i;
    char *s = 0;

    arb_assert(pargc >= 1);     // otherwise s stays 0

    for (i=1;i<pargc;i++){
        s = pargv[i];
        if (*s == '-') s++;
        if (!strncasecmp(s,param,strlen(param))) break;
    }
    if (i==pargc) return val;
    s+= strlen(param);
    if (*s != '=') return val;
    s++;
    val = atoi(s);
    pargc--;        // remove parameter
    for (;i<pargc;i++){
        pargv[i] = pargv[i+1];
    }

    if (val<min) val = min;
    if (val>max) val = max;
    return val;
}

const char *getString(const char *param, const char *val, const char *description){
    if (helpflag){
        if (!val) val = "";
        printf("    %s=%s   %s\n",param,val,description);
        return 0;
    }
    int   i;
    char *s = 0;

    arb_assert(pargc >= 1);     // otherwise s stays 0

    for (i=1;i<pargc;i++){
        s = pargv[i];
        if (*s == '-') s++;
        if (!strncasecmp(s,param,strlen(param))) break;
    }
    if (i==pargc) return val;
    s+= strlen(param);
    if (*s != '=') return val;
    s++;
    pargc--;        // remove parameter
    for (;i<pargc;i++){
        pargv[i] = pargv[i+1];
    }
    return s;
}

int main(int argc,char ** argv){
    pargc = argc;
    pargv = argv;
    if (argc<=1) helpflag = 1;
    else        helpflag = 0;

    P.SERVERID = getInt("serverid" ,0,0,100,   "Server Id, look into $ARBHOME/lib/arb_tcp.dat");

    P.DESIGNCPLIPOUTPUT = getInt("designmaxhits",100,10,10000,  "Maximum Number of Probe Design Suggestions");
    P.DESINGNAMES       = getString("designnames","",       "List of short names seperated by '#'");
    P.sequence          = 0;
    while  ((P.DESIGNSEQUENCE = getString("designsequence",0,       "Additional Sequences, will be added to the target group"))) {
        apd_sequence *s  = new apd_sequence;
        s->next          = P.sequence;
        P.sequence       = s;
        s->sequence      = P.DESIGNSEQUENCE;
        P.DESIGNSEQUENCE = 0;
    }
    P.DESIGNPROBELENGTH = getInt("designprobelength",18,10,100, "Length of probe");
    P.MINTEMP           = getInt("designmintemp",0,0,400,   "Minimum melting temperature of probe");
    P.MAXTEMP           = getInt("designmaxtemp",400,0,400, "Maximum melting temperature of probe");
    P.MINGC             = getInt("desingmingc",30,0,100,    "Minimum gc content");
    P.MAXGC             = getInt("desingmaxgc",80,0,100,    "Maximum gc content");
    P.MAXBOND           = getInt("desingmaxbond",0,0,10,    "Not implemented");

    P.MINPOS = getInt("desingminpos",0,0,10000,  "Minimum ecoli position");
    P.MAXPOS = getInt("desingmaxpos",10000,0,10000,  "Maximumm ecoli position");

    P.MISHIT     = getInt("designmishit",0,0,10000,  "Number of allowed hits outside the selected group");
    P.MINTARGETS = getInt("designmintargets",50,0,100,   "Minimum percentage of hits within the selected species");

    P.SEQUENCE   = getString("matchsequence","agtagtagt","The sequence to search for");
    P.MISMATCHES = getInt("matchmismatches", 0,0,5,  "Maximum Number of allowed mismatches");
    P.COMPLEMENT = getInt("matchcomplement", 0,0,1,  "Match reversed and complemented probe");
    P.WEIGHTED   = getInt("matchweighted", 0,0,1,    "Use weighted mismatches");


    if (pargc>1){
        printf("Unknown Parameter %s\n", pargv[1]);
        exit(-1);
    }
    if (helpflag) return 0;

    if (*P.DESINGNAMES || P.sequence){
        AP_probe_design_event();
    }else{
        AP_probe_match_event();
    }
    return 0;
}
