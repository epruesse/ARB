/* -----------------------------------------------------------------
 * Project:                       ARB
 *
 * Module:                        conservation profile [abbrev.: CPRO]
 *
 * Exported Classes:              x
 *
 * Global Functions:              x
 *
 * Global Variables:
 *                                  AWARS
 *               AW_STRING, "cpro/alignment" : name of alignment
 *               AW_STRING, "cpro/which_species" : all/marked
 *               AW_STRING, "cpro/countgaps" : if off, drop gaps
 *               AW_FLOAT, "cpro/rateofgroup" : how to rate, when two
 *                 characters belong to the same group [ 0.0 means don't rate ]
 *               AW_INT, AWAR_CURSOR_POSITION:
 *                           column shown in func. CPRO_drawstatistic_cb
 *
 * Global Structures:             CPRO
 *
 * Private Classes:               .
 *
 * Private Functions:             .
 *
 * Private Variables:             .
 *
 * Dependencies:             Needs cprofile.fig, CPROdraw.fig, CPROdens.fig
 *                                 CPROxpert.fig CPROcalc.fig
 *
 * Description:                   x
 *
 * Integration Notes: The main function using this module must have a
 *                    callback to the function
 *                    AW_window *AP_open_cprofile_window( AW_root *aw_root)
 *                    and the function void AP_create_consensus_var
 *                    (AW_root *aw_root, AW_default aw_def) has to be called.
 *
 * -----------------------------------------------------------------
 */

#include "NT_local.h"
#include <awt_sel_boxes.hxx>

#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_device.hxx>

#include <arbdbt.h>
#include <arb_defs.h>

#include <iostream>
#include <cmath>

#define ap_assert(cond) arb_assert(cond)

typedef GB_UINT4 STATTYPE;

enum {
    GAP = 1,
    BAS_A = 2,
    BAS_C = 3,
    BAS_G = 4,
    BAS_T = 5,
    MAX_BASES = 6,
    MAX_AMINOS = 27+GAP
};
#define GC_black 7
#define GC_blue 6
#define GC_green 5
#define GC_grid 4


struct CPRO_result_struct {
    STATTYPE **statistic;
    long maxalignlen;          // length of longest sequence
    long resolution;           // resolution of statistic table
    float ratio;               // ratio between transitions and transversion
    long maxaccu;              // highest number of compares per distance
    long memneeded;            // memory required for this statistic
    char which_species[20];    // "marked vs all" , ...
    char drawmode;             // smoothing
    char alignname[80];        // name of alignment
    char statisticexists;      // was there yet a statistic calculated/loaded ?
    long leastcompares;        // if less overlap between two sequences
                               // comparison doesn't contribute to statistic
    long countgaps;            // if 0, comparisons with gaps are not counted
};

struct CPRO_struct {
    long numspecies;           // number of species
    long maxresneeded;         // not yet used (max distance of calculation)
    long partition;            // size of partition in matrix of compares
    long leastaccu;            // if less compares per distance don't show
    char *agonist;             // list of species that contribute to statistic
    char *antagonist;          // -^-
    char convtable[256];         // converts character to a token
    char grouptable[MAX_AMINOS]; // gives number of group, in which token is
                                 // member
    float grouprate;           // ratio between transitions and transversions
    float distancecorrection;  // results out of grouprate
    long column;               // column of alignment that is shown [1..N]
    float maxdistance;         // statistic shows up to this point of distance
    long gridhorizontal;       // grid over statistic
    long gridvertical;         // -^-
    float Z_it_group;          // last value , needed for smoothing
    float Z_it_equal;          // -^-
    struct CPRO_result_struct result[2]; // info needed for each statistic
};

static CPRO_struct CPRO;

/* -----------------------------------------------------------------
 * Function:                     CPRO_readandallocate
 *
 * Arguments:                    char versus,char *align (name of alignment)
 *
 * Returns:                      modifies:
 *                               char **&speciesdata,GBDATA **&speciesdatabase
 *
 * Description:     Memory for 'statistic', 'speciesdata', 'speciesdatabase',
 *                  'agonist' and 'antagonist' is allocated.
 *                  Pointers to the sequences in the database are
 *                  read into array 'speciesdatabase'.
 *
 *
 * NOTE:                         .
 *
 * Global Variables referenced:  .
 *
 * Global Variables modified:    CPRO.agonist,CPRO.antagonist,CPRO.numspecies
 *
 * AWARs referenced:             .
 *
 * AWARs modified:               x
 *
 * Dependencies:                 .
 * -----------------------------------------------------------------
 */
static void CPRO_readandallocate(char **&speciesdata, GBDATA **&speciesdatabase,
                                 char versus, char *align)
{
    GBDATA *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);
    GBDATA *gb_species;

    {
        long nrofspecies=0;
        gb_species = GBT_first_species_rel_species_data(gb_species_data);
        while (gb_species)
        {
            if (GBT_read_sequence(gb_species, align)) {
                nrofspecies++; }
            gb_species = GBT_next_species(gb_species);
        }
        CPRO.numspecies=nrofspecies;
    }

    speciesdata=(char **)calloc((size_t)CPRO.numspecies, sizeof(char *));
    CPRO.agonist=(char *)calloc((size_t)CPRO.numspecies, sizeof(char));
    CPRO.antagonist=(char *)calloc((size_t)CPRO.numspecies, sizeof(char));
    speciesdatabase=(GBDATA **)calloc((size_t)CPRO.numspecies+1, sizeof(GBDATA *)); // Null terminated
    GBDATA *alidata;

    long countspecies=0;
    gb_species = GBT_first_species_rel_species_data(gb_species_data);
    while (gb_species) {
        if ((alidata=GBT_read_sequence(gb_species, align))) {
            speciesdatabase[countspecies++] = alidata;
        }
        gb_species = GBT_next_species(gb_species);
    }

    for (long i=0; i<CPRO.numspecies; i++)
    {
        CPRO.agonist[i]=1;
        CPRO.antagonist[i]=1;
    }

    if (versus!=0)
    {
        for (long j=0; j<CPRO.numspecies; j++)
        {
            CPRO.antagonist[j]=0;
            if (GB_read_flag(GB_get_grandfather(speciesdatabase[j])))
                CPRO.antagonist[j]=(char)1;
        }
    }
    if (versus==1)
    {
        long j;
        for (j=0; j<CPRO.numspecies; j++) CPRO.agonist[j]=CPRO.antagonist[j];
    }
}

// frees memory allocated by function CPRO_readandallocate
static void CPRO_deallocate(char **&speciesdata, GBDATA **&speciesdatabase)
{
    for (long i=0; i<CPRO.numspecies; i++) freenull(speciesdata[i]);
    freenull(speciesdata);

    free(speciesdatabase);
    free(CPRO.agonist);
    free(CPRO.antagonist);
    // 'CPRO.statistic' must not be freed, because it is needed in
    // function CPRO_drawstatistic
}

static void CPRO_allocstatistic(unsigned char which_statistic)
{
    CPRO.result[which_statistic].statistic=(STATTYPE **)calloc((size_t)CPRO.result[which_statistic].resolution*3+3, sizeof(STATTYPE *));
    for (long i=0; i<CPRO.result[which_statistic].resolution*3; i++) {
        CPRO.result[which_statistic].statistic[i]=(STATTYPE *)calloc((size_t)CPRO.result[which_statistic].maxalignlen, sizeof(STATTYPE));
    }
}

static void CPRO_freestatistic(unsigned char which_statistic)
{
    for (long j=0; j<CPRO.result[which_statistic].resolution*3; j++) {
        freenull(CPRO.result[which_statistic].statistic[j]);
    }
    freenull(CPRO.result[which_statistic].statistic);
}

// memory not used is given back to system
static void CPRO_workupstatistic(unsigned char which_statistic)
{
    long base;
    long column, memneeded=0;
    long sum, hits, different, group;
    long hitsc, diffc, groupc;
    CPRO.maxresneeded=0;
    CPRO.result[which_statistic].maxaccu=0;
    for (long res=0; res<CPRO.result[which_statistic].resolution; res++)
    {
        base=res*3;
        hits=0; different=0; group=0;
        for (column=0; column<CPRO.result[which_statistic].maxalignlen; column++)
        {
            hitsc=(long)CPRO.result[which_statistic].statistic[base+0][column];
            groupc=(long)CPRO.result[which_statistic].statistic[base+1][column];
            diffc=(long)CPRO.result[which_statistic].statistic[base+2][column];
            sum=hitsc+groupc+diffc;
            hits+=hitsc; group+=groupc; different+=diffc;

            if (sum) CPRO.maxresneeded=res+1;
            if (sum>CPRO.result[which_statistic].maxaccu)
            {
                CPRO.result[which_statistic].maxaccu=sum;
            }
        }
        if (hits) memneeded += CPRO.result[which_statistic].maxalignlen;
        else freenull(CPRO.result[which_statistic].statistic[base+0]);

        if (group) memneeded += CPRO.result[which_statistic].maxalignlen;
        else freenull(CPRO.result[which_statistic].statistic[base+1]);

        if (different) memneeded += CPRO.result[which_statistic].maxalignlen;
        else freenull(CPRO.result[which_statistic].statistic[base+2]);
    }
    if (!CPRO.result[which_statistic].maxaccu) CPRO.result[which_statistic].maxaccu=1;
    CPRO.result[which_statistic].memneeded=memneeded;
}


/* -----------------------------------------------------------------
 * Function:          CPRO_maketables
 *
 * Arguments:         char isamino
 *
 * Returns:           modifies: char *CPRO.convtable,
 *                              char *CPRO.grouptable
 *
 * Description:       Fills tables CPRO.convtable and CPRO.grouptable, that are
 *                    used later, when making the statistic. Meaning of tables:
 *                    E.g. CPRO.convtable['a']=BAS_A means that char. 'a' is
 *                    converted into number BAS_A. Then CPRO.grouptable[BAS_A]=1
 *                    and CPRO.grouptable[BAS_G]=1 means, that characters 'a'
 *                   and 'g' are both members of group 1.
 *
 * NOTE:                         .
 *
 * Global Variables referenced:  .
 *
 * Global Variables modified:    char *CPRO.convtable, char *CPRO.grouptable
 *
 * AWARs referenced:             .
 *
 * AWARs modified:               x
 *
 * Dependencies:                 .
 * -----------------------------------------------------------------
 */
static void CPRO_maketables(char isamino, char countgaps)
{
    long i;
    for (i=0; i<256; i++) {
        CPRO.convtable[i]=0; }
    if (!isamino)
    {
        if (countgaps) CPRO.convtable[(unsigned char)'-']=GAP;
        CPRO.convtable[(unsigned char)'a']=BAS_A;  CPRO.convtable[(unsigned char)'A']=BAS_A;
        CPRO.convtable[(unsigned char)'c']=BAS_C;  CPRO.convtable[(unsigned char)'C']=BAS_C;
        CPRO.convtable[(unsigned char)'g']=BAS_G;  CPRO.convtable[(unsigned char)'G']=BAS_G;
        CPRO.convtable[(unsigned char)'t']=BAS_T;  CPRO.convtable[(unsigned char)'T']=BAS_T;
        CPRO.convtable[(unsigned char)'u']=BAS_T;  CPRO.convtable[(unsigned char)'U']=BAS_T;

        for (i=0; i<MAX_AMINOS; i++) {
            CPRO.grouptable[i]=0;
        }
        CPRO.grouptable[BAS_A]=1;   CPRO.grouptable[BAS_G]=1;
        CPRO.grouptable[BAS_C]=2;   CPRO.grouptable[BAS_T]=2;
    }
    else
    {
        if (countgaps) CPRO.convtable[(unsigned char)'-'] = GAP;
        for (i=0; i<MAX_AMINOS; i++)
        {
            CPRO.convtable[(unsigned char)'a'+i] = i+1+GAP;
            CPRO.convtable[(unsigned char)'A'+i] = i+1+GAP;
        }
        CPRO.convtable[(unsigned char)'*'] = 10+GAP;    // 'J'

        for (i=0; i<MAX_AMINOS; i++) CPRO.grouptable[i] = 0;

#define SC(x, P) CPRO.grouptable[(unsigned char)P-(unsigned char)'A'+1+GAP] = x
        SC(1, 'P'); SC(1, 'A'); SC(1, 'G'); SC(1, 'S'); SC(1, 'T'); // PAGST
        SC(2, 'Q'); SC(2, 'N'); SC(2, 'E'); SC(2, 'D'); SC(2, 'B'); SC(2, 'Z'); // QNEDBZ
        SC(3, 'H'); SC(3, 'K'); SC(3, 'R'); // HKR
        SC(4, 'L'); SC(4, 'I'); SC(4, 'V'); SC(4, 'M'); // LIVM
        SC(5, 'F'); SC(5, 'Y'); SC(5, 'W'); // FYW
#undef SC
    }
}


static void CPRO_entryinstatistic(char **speciesdata, long elemx, long elemy, unsigned char which_statistic)
{
    unsigned char  value1, value2 = 0;
    char          *firstseq       = speciesdata[elemx];
    char          *secondseq      = speciesdata[elemy];
    float          rate           = 0.0;
    long           numofcolumns   = 0;
    float          distance       = 0.0;


    if (elemx==elemy) return;
    if (!(CPRO.agonist[elemx])) return;
    if (!(CPRO.antagonist[elemy])) return;
    if ((CPRO.agonist[elemy])&&(CPRO.antagonist[elemx])&&(elemy<elemx)) return;

    // add similarities (added 1.0 means equal, 0.0 means different)
    long counter;
    for (counter=0; counter<CPRO.result[which_statistic].maxalignlen; counter++)
    {
        if ((value1=*firstseq)&&(value2=*secondseq))
        {
            numofcolumns++;
            if (value1==value2) {
                rate=rate+1.0; }
            else if (CPRO.grouptable[value1]==CPRO.grouptable[value2]) {
                rate=rate+CPRO.grouprate; } // add transition weighted
            // between 1.0 and 0.0
        }
        firstseq++;
        secondseq++;
    }

    if (numofcolumns<CPRO.result[which_statistic].leastcompares) return;
    distance=((float)numofcolumns-rate)/(float)numofcolumns;
    distance=distance*CPRO.distancecorrection;

    long column=(long)(distance*CPRO.result[which_statistic].resolution);

    if (column < 0 || column >= CPRO.result[which_statistic].resolution) return;

    STATTYPE *equalentry     = CPRO.result[which_statistic].statistic[3*column];
    STATTYPE *samegroupentry = CPRO.result[which_statistic].statistic[3*column+1];
    STATTYPE *differententry = CPRO.result[which_statistic].statistic[3*column+2];

    firstseq  = speciesdata[elemx];
    secondseq = speciesdata[elemy];

    for (counter=0; counter<CPRO.result[which_statistic].maxalignlen; counter++)
    {
        if ((value1=*firstseq)&&(value2=*secondseq))
            // when gap or unaligned base goto next position
        {
            if (value1==value2) { (*equalentry)++; }
            else if (CPRO.grouptable[value1]==CPRO.grouptable[value2]) {
                (*samegroupentry)++; }
            else { (*differententry)++; }
        }
        firstseq++;
        secondseq++;
        equalentry++;
        samegroupentry++;
        differententry++;
    }



}

// is used by function CPRO_makestatistic
// reads sequences of a segment into memory, converts them
// and frees older sequences
static void CPRO_readneededdata(char **speciesdata, GBDATA **speciesdatabase,
                                long elemx1, long elemx2, long elemy1, long elemy2, unsigned char which_statistic)
{
    long i=0, j=0;
    const char *tempdata;
    for (i=0; i<CPRO.numspecies; i++) {
        if ((speciesdata[i]) && (i<elemy1) && (i>elemy2) && (i<elemx1) && (i>elemx2)) {
            delete(speciesdata[i]); speciesdata[i]=0;
        }
    }

    if (elemx1<CPRO.numspecies) {
        for (i=elemx1; (i<=elemx2 && i<CPRO.numspecies); i++) {
            if ((CPRO.agonist[i])&&(!(speciesdata[i]))) {
                tempdata=GB_read_char_pntr(speciesdatabase[i]);
                speciesdata[i]=(char*)calloc((unsigned int) CPRO.result[which_statistic].maxalignlen, 1);
                for (j=0; j<CPRO.result[which_statistic].maxalignlen; j++) {
                    speciesdata[i][j]=CPRO.convtable[(unsigned char)tempdata[j]];
                }
            }
        }
    }
    if (elemy1<CPRO.numspecies) {
        for (i=elemy1; (i<=elemy2 && i<CPRO.numspecies); i++) {
            if ((CPRO.antagonist[i])&&(!(speciesdata[i]))) {
                tempdata=GB_read_char_pntr(speciesdatabase[i]);
                speciesdata[i]=(char*)calloc((unsigned int) CPRO.result[which_statistic].maxalignlen, 1);
                for (j=0; j<CPRO.result[which_statistic].maxalignlen; j++) {
                    speciesdata[i][j]=CPRO.convtable[(unsigned char)tempdata[j]];
                }
            }
        }
    }
}


/* -----------------------------------------------------------------
 * Function:                     CPRO_makestatistic
 *
 * Arguments:                    char **speciesdata,
 *                               GBDATA **speciesdatabase
 *
 * Returns:                      1 if successful, 0 if user abort
 *                                   (without consequences)
 *
 * Description:    This function compares every sequence with every sequence.
 *                 It divides the matrix into segments and goes through each
 *                 segment. Width of a segment depends on CPRO.partition.
 *                 MAX_MEMORY/2 is available.
 *                 When a new segment is entered, the corresponding
 *                 sequences are loaded into array 'speciesdata' by the function
 *                 CPRO_readneededdata. 'speciesdatabase' contains pointers of
 *                 sequences to the database. Comparison and evaluation of two
 *                 sequences is done by function CPRO_entryinstatistic.
 *
 * NOTE:                         .
 *
 * Global Variables referenced:
 *      CPRO.numspecies,CPRO.result[which_statistic].maxalignlen
 *
 * Global Variables modified:    x
 *
 * AWARs referenced:             .
 *
 * AWARs modified:          CPRO.result[which_statistic].statistic  is modified
 *
 * Dependencies:                 CPRO_entryinstatistic , CPRO_readneededdata
 * -----------------------------------------------------------------
 */
static char CPRO_makestatistic(char **speciesdata, GBDATA **speciesdatabase, unsigned char which_statistic)
{
    long widthmatrix    = CPRO.partition;
    long n              = CPRO.numspecies;
    long comparesneeded = n*n;
    long numofsegments  = CPRO.numspecies/widthmatrix + 1;

    if (CPRO.result[which_statistic].statisticexists) {
        CPRO_freestatistic(which_statistic);
    }
    else CPRO.result[which_statistic].statisticexists=1;

    CPRO_allocstatistic(which_statistic);

    arb_progress progress("calculating matrix", comparesneeded);
    bool         aborted = false;

    for (long segmentx=0; segmentx<numofsegments && !aborted; segmentx++) {
        long elemx1 = widthmatrix*segmentx;
        long elemx2 = widthmatrix*(segmentx+1)-1;

        for (long segmenty=0; segmenty<numofsegments && !aborted; segmenty++) {
            long elemy1 = widthmatrix*segmenty;
            long elemy2 = widthmatrix*(segmenty+1)-1;
            
            CPRO_readneededdata(speciesdata, speciesdatabase, elemx1, elemx2, elemy1, elemy2, which_statistic);
            for (long elemx=elemx1; elemx<=elemx2 && !aborted; elemx++) {
                for (long elemy=elemy1; elemy<=elemy2 && !aborted; elemy++) {
                    if ((elemy<CPRO.numspecies)&&(elemx<CPRO.numspecies)) {
                        CPRO_entryinstatistic(speciesdata, elemx, elemy, which_statistic);
                        ++progress;
                        aborted = progress.aborted();
                    }
                }
            }
        }
    }
    return (1);
}



/* -----------------------------------------------------------------
 * Function:                     CPRO_calculate_cb
 *
 * Arguments:                    .
 *
 * Returns:                      .
 *
 * Description:          main callback
 *                       This function calculates the conservative profile.
 *                       Function CPRO_readandallocate allocates memory and
 *                       reads necessary data. Calculation of the statistic
 *                       is done in function CPRO_makestatistic.
 *                       Allocated memory is freed in function CPRO_deallocate.
 *
 * NOTE:                         .
 *
 * Global Variables referenced:  .
 *
 * Global Variables modified:    x
 *
 * AWARs referenced:             cpro/alignment , cpro/which_species
 *                               cpro/countgaps , cpro/rateofgroups
 *
 * AWARs modified:               x
 *
 * Dependencies:   CPRO_readandallocate , CPRO_makestatistic , CPRO_deallocate
 * -----------------------------------------------------------------
 */

static void CPRO_memrequirement_cb(AW_root *aw_root); // prototype

static void CPRO_calculate_cb(AW_window *aw, AW_CL which_statistic)
{
    AW_root *awr=aw->get_root();
    char *align=awr->awar("cpro/alignment")->read_string();
    char *marked=awr->awar("cpro/which_species")->read_string();
    char versus=0; // all vs all
    if (!(strcmp("marked", marked))) versus=1; // marked vs marked
    if (!(strcmp("markedall", marked))) versus=2; // marked vs all
    GB_ERROR faultmessage;
    free(marked);

    if (CPRO.result[which_statistic].statisticexists)
    {
        CPRO_freestatistic((char)which_statistic);
        CPRO.result[which_statistic].statisticexists=0;
    }

    strcpy(CPRO.result[which_statistic].alignname, align);
    CPRO.result[which_statistic].resolution=awr->awar("cpro/resolution")->read_int();
    if (CPRO.result[which_statistic].resolution<=0)
        CPRO.result[which_statistic].resolution=1;
    CPRO.result[which_statistic].leastcompares=
        awr->awar("cpro/leastcompares")->read_int();
    if (CPRO.result[which_statistic].leastcompares<=0) CPRO.result[which_statistic].leastcompares=1;

    if (versus==1) strcpy(CPRO.result[which_statistic].which_species, "m vs m");
    else if (versus==2) strcpy(CPRO.result[which_statistic].which_species, "m vs all");
    else strcpy(CPRO.result[which_statistic].which_species, "all vs all");

    if ((faultmessage=GB_push_transaction(GLOBAL.gb_main))) {
        aw_message(faultmessage);
        free(align);
        return;
    }

    CPRO.result[which_statistic].maxalignlen = GBT_get_alignment_len(GLOBAL.gb_main, align);
    if (CPRO.result[which_statistic].maxalignlen<=0) {
        GB_clear_error();
        GB_pop_transaction(GLOBAL.gb_main);
        aw_message("Error: Select an alignment !");
        delete   align;
        return;
    }

    char isamino = GBT_is_alignment_protein(GLOBAL.gb_main, align);
    arb_progress progress("Calculating conservation profile");

    GBDATA **speciesdatabase; // array of GBDATA-pointers to the species
    char **speciesdata; // array of pointers to strings that hold data of species

    // allocate memory for 'CPRO.statistic','speciesdata' and fill
    //                'speciesdatabase','agonist' and 'antagonist'
    CPRO_readandallocate(speciesdata, speciesdatabase, versus, align);

    char *countgapsstring=awr->awar("cpro/countgaps")->read_string();
    char countgaps=1;
    if (strcmp("on", countgapsstring)) countgaps=0;
    free(countgapsstring);
    CPRO.result[which_statistic].countgaps=(long)countgaps;

    // create convtable and grouptable
    CPRO_maketables(isamino, countgaps);
    CPRO.result[which_statistic].ratio=awr->awar("cpro/transratio")->read_float();
    CPRO.grouprate=1-(0.5/CPRO.result[which_statistic].ratio);
    CPRO.distancecorrection=(CPRO.result[which_statistic].ratio+1)*2.0/3.0;

    // fill the CPRO.statistic table
    CPRO_makestatistic(speciesdata, speciesdatabase, (char)which_statistic);
    // GBUSE(success);
    CPRO_workupstatistic((char)which_statistic);

    CPRO_deallocate(speciesdata, speciesdatabase);
    free(align);
    if ((faultmessage=GB_pop_transaction(GLOBAL.gb_main))) {
        aw_message(faultmessage);
        return;
    }

    CPRO_memrequirement_cb(awr);

}

static void CPRO_memrequirement_cb(AW_root *aw_root)
{
    CPRO.partition  = aw_root->awar("cpro/partition")->read_int();
    long resolution = aw_root->awar("cpro/resolution")->read_int();

    if (CPRO.partition<=0) CPRO.partition = 1;
    if (resolution<=0) resolution         = 1;

    char buf[80];

    aw_root->awar("tmp/cpro/which1")->write_string(CPRO.result[0].which_species);
    aw_root->awar("tmp/cpro/which2")->write_string(CPRO.result[1].which_species);

    sprintf(buf, "%5ld", CPRO.result[0].resolution); aw_root->awar("tmp/cpro/nowres1")->write_string(buf);
    sprintf(buf, "%5ld", CPRO.result[1].resolution); aw_root->awar("tmp/cpro/nowres2")->write_string(buf);

    if (!(CPRO.result[0].statisticexists)) {
        aw_root->awar("tmp/cpro/memfor1")->write_string("0KB");
    }
    else
    {
        sprintf(buf, "%ldKB", CPRO.result[0].memneeded/1024);
        aw_root->awar("tmp/cpro/memfor1")->write_string(buf);
    }

    if (!(CPRO.result[1].statisticexists)) {
        aw_root->awar("tmp/cpro/memfor2")->write_string("0KB");
    }
    else
    {
        sprintf(buf, "%ldKB", CPRO.result[1].memneeded/1024);
        aw_root->awar("tmp/cpro/memfor2")->write_string(buf);
    }

    long len;
    {
        GB_transaction ta(GLOBAL.gb_main);
        char *align  = aw_root->awar("cpro/alignment")->read_string();
    
        len = GBT_get_alignment_len(GLOBAL.gb_main, align);
        free(align);
    }

    if (len<=0) {
        GB_clear_error();
        aw_root->awar("tmp/cpro/mempartition")->write_string("???");
        aw_root->awar("tmp/cpro/memstatistic")->write_string("???");
    }
    else {
        long mem;

        mem=CPRO.partition*len*2;    // *2, because of row and column in matrix
        sprintf(buf, "%li KB", long(mem/1024));
        aw_root->awar("tmp/cpro/mempartition")->write_string(buf);

        mem+=resolution*3*sizeof(STATTYPE)*len;
        sprintf(buf, "%li KB", long(mem/1024));
        aw_root->awar("tmp/cpro/memstatistic")->write_string(buf);
    }
}

void create_cprofile_var(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string("cpro/alignment", "", aw_def);
    aw_root->awar_string("cpro/which_species", "marked", aw_def);
    aw_root->awar_string("cpro/which_result", "transversion", aw_def);
    aw_root->awar_string("cpro/countgaps", "", aw_def);
    aw_root->awar_string("cpro/condensename", "", aw_def);
    aw_root->awar_float("cpro/transratio", 0.5, aw_def);
    aw_root->awar_int(AWAR_CURSOR_POSITION, info2bio(0), GLOBAL.gb_main);
    aw_root->awar_int("cpro/maxdistance", 100, aw_def);
    aw_root->awar_int("cpro/resolution", 100, aw_def);
    aw_root->awar_int("cpro/partition", 100, aw_def);
    aw_root->awar_int("cpro/drawmode0", 0, aw_def);
    aw_root->awar_int("cpro/drawmode1", 0, aw_def);
    aw_root->awar_int("cpro/leastaccu", 3, aw_def);
    aw_root->awar_int("cpro/leastmax", 50, aw_def);
    aw_root->awar_int("cpro/firsttoreach", 50, aw_def);
    aw_root->awar_int("cpro/firstreachedstep", 4, aw_def);
    aw_root->awar_int("cpro/leastcompares", 300, aw_def);
    aw_root->awar_string("tmp/cpro/mempartition", "", aw_def);
    aw_root->awar_int("cpro/gridhorizontal", 20, aw_def);
    aw_root->awar_int("cpro/gridvertical", 20, aw_def);
    aw_root->awar_string("tmp/cpro/which1", "", aw_def);
    aw_root->awar_string("tmp/cpro/which2", "", aw_def);
    aw_root->awar_string("tmp/cpro/nowres1", "", aw_def);
    aw_root->awar_string("tmp/cpro/nowres2", "", aw_def);
    aw_root->awar_string("tmp/cpro/memstatistic", "", aw_def);
    aw_root->awar_string("tmp/cpro/memfor1", "", aw_def);
    aw_root->awar_string("tmp/cpro/memfor2", "", aw_def);

    AW_create_fileselection_awars(aw_root, "cpro/save", ".", ".cpr", "", aw_def);
    AW_create_fileselection_awars(aw_root, "cpro/load", ".", ".cpr", "", aw_def);
    memset((char *)&CPRO, 0, sizeof(CPRO_struct));

}

static float CPRO_statisticaccumulation(long res, long column, unsigned char which_statistic)
{
    STATTYPE hits=0, sum=0, group=0, different=0;
    long base=3*res;

    if (!(CPRO.result[which_statistic].statistic[base+0])) hits=0;
    else hits=CPRO.result[which_statistic].statistic[base+0][column];

    if (!(CPRO.result[which_statistic].statistic[base+1])) group=0;
    else group=CPRO.result[which_statistic].statistic[base+1][column];

    if (!(CPRO.result[which_statistic].statistic[base+2])) different=0;
    else different=CPRO.result[which_statistic].statistic[base+2][column];

    sum=hits+group+different;

    return ((float)sum);
}


// reports how many equal,group and differ entries are at a certain distance
// at a certain column; mode=1 means smoothing is on.
static void CPRO_getfromstatistic(float &equal, float &ingroup, long res, long column, unsigned char which_statistic, char mode)
{
    STATTYPE hits=0, sum=0, group=0, different=0;
    long base=3*res;

    if (!(CPRO.result[which_statistic].statistic[base+0])) hits=0;
    else hits=CPRO.result[which_statistic].statistic[base+0][column];

    if (!(CPRO.result[which_statistic].statistic[base+1])) group=0;
    else group=CPRO.result[which_statistic].statistic[base+1][column];

    if (!(CPRO.result[which_statistic].statistic[base+2])) different=0;
    else different=CPRO.result[which_statistic].statistic[base+2][column];

    sum=hits+group+different;

    if (!(mode))
    {
        if (sum)
        {
            equal=(float)hits/(float)sum;
            ingroup=((float)hits+(float)group)/(float)sum;
        }
        else
        {
            equal=1.0;
            ingroup=1.0;
        }
        return;
    }
    else
    {
        float accu=pow(sum/(float)CPRO.result[which_statistic].maxaccu, 0.0675);
        float distance=(float)CPRO.result[which_statistic].drawmode*.01*
            CPRO.result[which_statistic].resolution;
        float alpha=0.0;   // alpha=0.0 no smoothing; alpha=0.99 high smoothing
        if (distance>0.0000001)
        {
            alpha=1.0-accu/distance;
            if (alpha<0) alpha=0;
        }

        if (res==0)
        {
            CPRO.Z_it_group=1.0;
            CPRO.Z_it_equal=1.0;
        }
        if (sum)
        {
            equal=(float)hits/(float)sum;
            ingroup=((float)hits+(float)group)/(float)sum;
            equal=(1-alpha)*equal+alpha*CPRO.Z_it_equal;
            ingroup=(1-alpha)*ingroup+alpha*CPRO.Z_it_group;
        }
        else
        {
            equal=CPRO.Z_it_equal;
            ingroup=CPRO.Z_it_group;
        }
        CPRO.Z_it_equal=equal;
        CPRO.Z_it_group=ingroup;
    }

}

static void CPRO_box(AW_device *device, int gc, float l, float t, float width, float high) {
    device->box(gc, false, l, t, width, high, 1);
}

static float CPRO_confidinterval(long res, long column, unsigned char which_statistic, char mode)
{
    STATTYPE hits=0, sum=0, group=0, different=0;
    long base=3*res;

    if (!(CPRO.result[which_statistic].statistic[base+0])) hits=0;
    else hits=CPRO.result[which_statistic].statistic[base+0][column];

    if (!(CPRO.result[which_statistic].statistic[base+1])) group=0;
    else group=CPRO.result[which_statistic].statistic[base+1][column];

    if (!(CPRO.result[which_statistic].statistic[base+2])) different=0;
    else different=CPRO.result[which_statistic].statistic[base+2][column];

    sum=hits+group+different;
    if (!(mode)) return (1/sqrt((float)sum)/2.0);
    else return (0.0);
}

static void CPRO_drawstatistic(AW_device *device, unsigned char which_statistic) {
    float topdistance=70.0, leftdistance=40.0;
    float rightdistance=20.0, bottomdistance=10.0;
    float betweendistance=30.0;
    float firstavailable=.65;
    float secondavailable=.35;
    // points are in the areas and without the frame
    float topfirst, leftfirst, widthfirst, highfirst;
    float topsecond, leftsecond, widthsecond, highsecond;
    float highboth;
    float equal, ingroup;
    char mode=CPRO.result[which_statistic].drawmode;

    const AW_screen_area& rect = device->get_area_size();
    device->reset();
    device->clear(-1);

    topfirst=39.0; // @@@ unused
    leftfirst=20.0; // @@@ unused
    widthfirst=(rect.r-rect.l)-leftdistance-1-rightdistance;
    widthsecond=(rect.r-rect.l)-leftdistance-1-rightdistance;
    highboth=(rect.b-rect.t)-topdistance-bottomdistance-betweendistance-4;
    if ((highboth<12.0)||(widthfirst<10.0)) return;

    highfirst=(float)(long)(highboth*firstavailable);
    highsecond=(float)(long)(highboth*secondavailable)+1;

    topfirst=topdistance+1;
    leftfirst=leftdistance;
    topsecond=rect.b-bottomdistance-highsecond;
    leftsecond=leftdistance+1;

    CPRO_box(device, GC_black, leftfirst-1, topfirst-1,
             widthfirst+2, highfirst+2);
    CPRO_box(device, GC_black, leftsecond-1, topsecond-1,
             widthsecond+2, highsecond+2);

    device->text(GC_black, "column", leftdistance+82, 14, 0, 1, 0);

    // draw grid and inscribe axes
    char buf[30];
    long gridx, gridy;
    float xpos=leftdistance, ypos;
    sprintf(buf, " character difference");
    device->text(GC_black, buf, leftdistance-40, topdistance-7, 0, 1, 0);
    device->text(GC_black, "  0%", leftdistance-26, topdistance+4+highfirst, 0, 1, 0);

    for (gridy=CPRO.gridhorizontal; gridy<100; gridy+=CPRO.gridhorizontal)
    {
        ypos=topdistance+1+(1.0-(float)gridy*0.01)*(highfirst-1);
        device->line(GC_grid, xpos, ypos, xpos+widthfirst, ypos, AW_SCREEN);
        sprintf(buf, "%3ld%%", gridy);
        device->text(GC_black, buf, xpos-27, ypos+4, 0, 1, 0);
    }
    device->text(GC_black, "100%", leftdistance-26, topdistance+5, 0, 1, 0);

    device->text(GC_black, "sequence distance", leftdistance+widthfirst-95, topdistance+highfirst+25, 0, 1, 0);
    device->text(GC_black, "  0%", leftdistance-11, topdistance+14+highfirst, 0, 1, 0);
    ypos=topdistance+1;

    for (gridx=CPRO.gridvertical; (float)gridx<=100.0*CPRO.maxdistance;
        gridx+=CPRO.gridvertical)
    {
        xpos=leftdistance+1+(float)gridx*0.01/CPRO.maxdistance*widthfirst;
        if ((float)gridx*0.01<1.0*CPRO.maxdistance) {
            device->line(GC_grid, xpos, ypos, xpos, ypos+highfirst, AW_SCREEN);
        }
        sprintf(buf, "%3ld%%", gridx);
        device->text(GC_black, buf, xpos-12, ypos+13+highfirst, 0, 1, 0);
    }

    if (!CPRO.result[which_statistic].statisticexists) return;
    if ((CPRO.column<1)||(CPRO.column>CPRO.result[which_statistic].maxalignlen))
        return;

    // fill first box
    float accu, confidinterval;
    float step=widthfirst/CPRO.result[which_statistic].resolution;
    float linelength=step/CPRO.maxdistance;
    float ytop, ybottom;

    long firstx;
    for (firstx=0;
         firstx<CPRO.result[which_statistic].resolution*CPRO.maxdistance;
         firstx++)
    {
        CPRO_getfromstatistic(equal, ingroup, (long)firstx, bio2info(CPRO.column), which_statistic, mode);
        accu = CPRO_statisticaccumulation((long)firstx, bio2info(CPRO.column), which_statistic);
        if (accu>=(float)CPRO.leastaccu) {
            xpos=(float)firstx*step/CPRO.maxdistance+leftfirst;
            ypos=topfirst+equal*highfirst;

            // do not draw outside canvas-box
            if (xpos+linelength > leftfirst+widthfirst+1) continue;

            confidinterval = highfirst * CPRO_confidinterval(firstx, bio2info(CPRO.column), which_statistic, mode);

            ytop      = ypos-confidinterval;
            if (ytop>=topfirst) device->line(GC_blue, xpos, ytop, xpos+linelength, ytop, AW_SCREEN);
            else ytop = topfirst;
            device->line(GC_blue, xpos+linelength/2, ytop, xpos+linelength/2, ypos, AW_SCREEN);

            ybottom      = ypos+confidinterval;
            if (ybottom<topfirst+highfirst) device->line(GC_blue, xpos, ybottom, xpos+linelength, ybottom, AW_SCREEN);
            else ybottom = topfirst+highfirst-1;
            device->line(GC_blue, xpos+linelength/2, ybottom, xpos+linelength/2, ypos, AW_SCREEN);

            ypos      = topfirst+ingroup*highfirst;
            ytop      = ypos-confidinterval;
            if (ytop>=topfirst) device->line(GC_green, xpos, ytop, xpos+linelength, ytop, AW_SCREEN);
            else ytop = topfirst;
            device->line(GC_green, xpos+linelength/2, ytop, xpos+linelength/2, ypos, AW_SCREEN);

            ybottom      = ypos+confidinterval;
            if (ybottom<topfirst+highfirst) device->line(GC_green, xpos, ybottom, xpos+linelength, ybottom, AW_SCREEN);
            else ybottom = topfirst+highfirst-1;
            device->line(GC_green, xpos+linelength/2, ybottom, xpos+linelength/2, ypos, AW_SCREEN);
        }
    }

    float resaccu;
    float rate;
    sprintf(buf, " %5ld", CPRO.result[which_statistic].maxaccu);
    device->text(GC_black, "   max", leftsecond-50, topsecond, 0, 1, 0);
    device->text(GC_black, buf, leftsecond-43, topsecond+10*1, 0, 1, 0);
    device->text(GC_black, "  pairs", leftsecond-50, topsecond+10*3, 0, 1, 0);

    // fill second box
    for (firstx=0; firstx<(long)widthsecond; firstx++) {
        long res = firstx*CPRO.result[which_statistic].resolution /widthsecond*CPRO.maxdistance;
        resaccu  = CPRO_statisticaccumulation(res, bio2info(CPRO.column), which_statistic);
        rate     = 1.0-(sqrt(resaccu)/sqrt(CPRO.result[which_statistic].maxaccu));
        device->line(GC_black, firstx+leftsecond, topsecond+rate*highsecond, firstx+leftsecond, topsecond+highsecond, AW_SCREEN);
    }
}

static void CPRO_resize_cb(AW_window *aws, unsigned char which_statistic) {
    AW_root *awr=aws->get_root();
    CPRO.column=awr->awar(AWAR_CURSOR_POSITION)->read_int();
    CPRO.gridvertical=(long)awr->awar("cpro/gridvertical")->read_int();
    CPRO.gridhorizontal=(long)awr->awar("cpro/gridhorizontal")->read_int();
    CPRO.leastaccu=awr->awar("cpro/leastaccu")->read_int();
    if (CPRO.leastaccu<=0) CPRO.leastaccu=1;

    AW_device *device=aws->get_device(AW_INFO_AREA);
    device->reset();
    CPRO_drawstatistic(device, which_statistic);
}

static void CPRO_expose_cb(AW_window *aws, unsigned char which_statistic) {
    AW_root *awr=aws->get_root();
    CPRO.column=awr->awar(AWAR_CURSOR_POSITION)->read_int();
    char buf[80];
    sprintf(buf, "cpro/drawmode%d", (int)which_statistic);
    CPRO.result[which_statistic].drawmode=(char)awr->awar(buf)->read_int();
    CPRO.gridvertical=(long)awr->awar("cpro/gridvertical")->read_int();
    CPRO.gridhorizontal=(long)awr->awar("cpro/gridhorizontal")->read_int();
    CPRO.leastaccu=awr->awar("cpro/leastaccu")->read_int();
    if (CPRO.leastaccu<=0) CPRO.leastaccu=1;

    long maxd=awr->awar("cpro/maxdistance")->read_int();
    if ((maxd>0)&&(maxd<101))CPRO.maxdistance=(float)maxd/100.0;

    AW_device *device=aws->get_device (AW_INFO_AREA);
    CPRO_drawstatistic(device, which_statistic);
}

static void CPRO_column_cb(AW_root *awr, AW_window *aws, unsigned char which_statistic) {
    char buf[80];
    sprintf(buf, "cpro/drawmode%d", (int)which_statistic);
    CPRO.result[which_statistic].drawmode=(char)awr->awar(buf)->read_int();
    CPRO_expose_cb(aws, which_statistic);
}

static void CPRO_columnminus_cb(AW_window *aws) {
    if (CPRO.column>1) {
        aws->get_root()->awar(AWAR_CURSOR_POSITION)->write_int(CPRO.column-1);
    }
}

static void CPRO_columnplus_cb(AW_window *aws, AW_CL /* which_statistic */, AW_CL) {
    aws->get_root()->awar(AWAR_CURSOR_POSITION)->write_int(CPRO.column+1);
}

static void CPRO_savestatistic_cb(AW_window *aw, AW_CL which_statistic)
{
    AW_root  *awr      = aw->get_root();
    char     *filename = awr->awar("cpro/save/file_name")->read_string();
    GB_ERROR  error    = 0;

    if (!filename) {
        error = "no filename";
    }
    else {
        const CPRO_result_struct& curr_stat = CPRO.result[which_statistic];
        if (!(curr_stat.statisticexists)) {
            error = "calculate first!";
        }
        else {
            GBDATA *newbase     = GB_open(filename, "wc");
            if (!newbase) error = GB_await_error();
            else {
                error = GB_begin_transaction(newbase);

                if (!error) error = GBT_write_int   (newbase, "cpro_resolution",    curr_stat.resolution);
                if (!error) error = GBT_write_int   (newbase, "cpro_maxalignlen",   curr_stat.maxalignlen);
                if (!error) error = GBT_write_int   (newbase, "cpro_maxaccu",       curr_stat.maxaccu);
                if (!error) error = GBT_write_int   (newbase, "cpro_memneeded",     curr_stat.memneeded);
                if (!error) error = GBT_write_string(newbase, "cpro_alignname",     curr_stat.alignname);
                if (!error) error = GBT_write_string(newbase, "cpro_which_species", curr_stat.which_species);
                if (!error) error = GBT_write_float (newbase, "cpro_ratio",         curr_stat.ratio);
                if (!error) error = GBT_write_int   (newbase, "cpro_gaps",          curr_stat.countgaps);

                if (!error) {
                    long maxalignlen = curr_stat.maxalignlen;

                    for (long column = 0; column<curr_stat.resolution && !error; column++) {
                        GBDATA   *gb_colrescontainer = GB_create_container(newbase, "column");
                        GBDATA   *gb_colentry;
                        GB_UINT4 *pointer;

                        if (!error && (pointer = curr_stat.statistic[column*3+0])) {
                            gb_colentry = GB_create(gb_colrescontainer, "equal", GB_INTS);
                            error       = GB_write_ints(gb_colentry, pointer, maxalignlen);
                        }
                        if (!error && (pointer = curr_stat.statistic[column*3+1])) {
                            gb_colentry = GB_create(gb_colrescontainer, "group", GB_INTS);
                            error       = GB_write_ints(gb_colentry, pointer, maxalignlen);
                        }
                        if (!error && (pointer = curr_stat.statistic[column*3+2])) {
                            gb_colentry = GB_create(gb_colrescontainer, "different", GB_INTS);
                            error       = GB_write_ints(gb_colentry, pointer, maxalignlen);
                        }
                    }
                }
                error = GB_end_transaction(newbase, error);
                if (!error) error = GB_save(newbase, (char*)0, "b");
            }

            GB_close(newbase);
        }
    }

    free(filename);
    aw->hide_or_notify(error);
}

static void CPRO_loadstatistic_cb(AW_window *aw, AW_CL which_statistic)
{
    AW_root  *awr      = aw->get_root();
    char     *filename = awr->awar("cpro/load/file_name")->read_string();
    GB_ERROR  error    = 0;

    if (!filename) {
        error = "missing filename";
    }
    else {
        GBDATA *oldbase = GB_open(filename, "r");

        if (!oldbase) {
            error = GBS_global_string("can't read DB '%s'", filename);
        }
        else {
            error = GB_begin_transaction(oldbase);
            if (!error) {
                if (CPRO.result[which_statistic].statisticexists) {
                    CPRO_freestatistic((char)which_statistic);
                }

                GBDATA *gb_param     = GB_search(oldbase, "cpro_resolution", GB_FIND);
                if (!gb_param) error = "not a valid statistic";
                else {
                    CPRO.result[which_statistic].resolution = GB_read_int(gb_param);

                    gb_param             = GB_search(oldbase, "cpro_maxalignlen", GB_FIND);
                    CPRO.result[which_statistic].maxalignlen = GB_read_int(gb_param);

                    gb_param=GB_search(oldbase, "cpro_maxaccu", GB_FIND);
                    CPRO.result[which_statistic].maxaccu=GB_read_int(gb_param);

                    gb_param=GB_search(oldbase, "cpro_memneeded", GB_FIND);
                    CPRO.result[which_statistic].memneeded=GB_read_int(gb_param);

                    gb_param=GB_search(oldbase, "cpro_alignname", GB_FIND);
                    strcpy(CPRO.result[which_statistic].alignname, GB_read_char_pntr(gb_param));

                    gb_param=GB_search(oldbase, "cpro_which_species", GB_FIND);
                    if (gb_param) strcpy(CPRO.result[which_statistic].which_species, GB_read_char_pntr(gb_param));

                    CPRO.result[which_statistic].statistic=(STATTYPE **)calloc((size_t)CPRO.result[which_statistic].resolution*3+3, sizeof(STATTYPE *));

                    GBDATA *gb_colrescontainer = GB_entry(oldbase, "column");
                    for (long column=0; column<CPRO.result[which_statistic].resolution; column++) {
                        const char *field[] = { "equal", "group", "different" };
                        for (int i = 0; i<3; i++) {
                            GBDATA *gb_colentry = GB_entry(gb_colrescontainer, field[i]);
                            if (gb_colentry) {
                                CPRO.result[which_statistic].statistic[column*3+i] = (STATTYPE*)GB_read_ints(gb_colentry);
                            }
                        }
                        gb_colrescontainer = GB_nextEntry(gb_colrescontainer);
                    }

                    CPRO.result[which_statistic].statisticexists = 1;
                    CPRO_memrequirement_cb(awr);
                }
            }
            error = GB_end_transaction(oldbase, error);
        }
        GB_close(oldbase);
    }

    aw->hide_or_notify(error);
    free(filename);
}

static AW_window *CPRO_savestatisticwindow_cb(AW_root *aw_root, AW_CL cl_which_statistic) {
    static AW_window *aw[2] = { 0, 0 };             // one window for each value of 'which_statistic'

    int which_statistic = int(cl_which_statistic);
    ap_assert(which_statistic >= 0 && which_statistic <= 1);

    if (!aw[which_statistic]) {
        AW_window_simple *aws       = new AW_window_simple;
        char             *window_id = GBS_global_string_copy("SAVE_CPRO_STATISTIC_%i", which_statistic);

        aws->init(aw_root, window_id, "SAVE STATISTIC");
        aws->load_xfig("sel_box.fig");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("save"); aws->callback(CPRO_savestatistic_cb, which_statistic);
        aws->create_button("SAVE", "SAVE", "S");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("cancel");
        aws->create_button("CANCEL", "CANCEL", "C");

        AW_create_fileselection(aws, "cpro/save");

        free(window_id);

        aw[which_statistic] = aws;
    }
    return aw[which_statistic];
}

static AW_window *CPRO_loadstatisticwindow_cb(AW_root *aw_root, AW_CL cl_which_statistic) {
    static AW_window *aw[2] = { 0, 0 };             // one window for each value of 'which_statistic'

    int which_statistic = int(cl_which_statistic);
    ap_assert(which_statistic >= 0 && which_statistic <= 1);

    if (!aw[which_statistic]) {
        AW_window_simple *aws       = new AW_window_simple;
        char             *window_id = GBS_global_string_copy("LOAD_CPRO_STATISTIC_%i", which_statistic);

        aws->init(aw_root, window_id, "LOAD STATISTIC");
        aws->load_xfig("sel_box.fig");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("save"); aws->callback(CPRO_loadstatistic_cb, which_statistic);
        aws->create_button("LOAD", "LOAD", "S");

        AW_create_fileselection(aws, "cpro/load");

        free(window_id);

        aw[which_statistic] = aws;
    }

    return aw[which_statistic];
}

// search point of resolution when half maximum if reached (for condense)
static float CPRO_gethalfmaximum(long column, float maximum, float firsttoreach,
                          char transversion, unsigned char which_statistic, char mode)
{
    float equal, ingroup, interest;
    float interval, sum;
    float halfmax=0.0;
    long res;
    for (res=0; res<CPRO.result[which_statistic].resolution; res++)
    {
        sum=CPRO_statisticaccumulation(res, column, which_statistic);
        if ((long)sum>=CPRO.leastaccu)
        {
            CPRO_getfromstatistic(equal, ingroup, res, column,
                                  which_statistic, mode);
            if (transversion) interest=1.0-ingroup;
            else interest=1.0-equal;
            interval=CPRO_confidinterval(res, column, which_statistic, mode);
            if (interest-interval>=maximum*firsttoreach)
            {
                res++;
                break;
            }
        }
    }
    halfmax=(float)res/(float)CPRO.result[which_statistic].resolution;
    return (halfmax-(float)CPRO.result[which_statistic].drawmode*0.01);
    // delay depending on drawmode
}

// search maximum distance in character (for condense)
static float CPRO_getmaximum(long column, char transversion,
                      unsigned char which_statistic, char mode)
{
    float maximum=-101.0, equal, ingroup, interest, sum, interval;
    for (long res=0; res<CPRO.result[which_statistic].resolution; res++)
    {
        sum=CPRO_statisticaccumulation(res, column, which_statistic);
        if ((long)sum>=CPRO.leastaccu)
        {
            CPRO_getfromstatistic(equal, ingroup, res, column,
                                  which_statistic, mode);
            if (transversion) interest=1.0-ingroup;
            else interest=1.0-equal;
            interval=CPRO_confidinterval(res, column, which_statistic, mode);
            if (interest-interval>maximum) maximum=interest-interval;
        }
    }
    return (maximum);
}

static void CPRO_condense_cb(AW_window *aw, AW_CL which_statistic)
{
    AW_root *aw_root = aw->get_root();
    
    char mode = CPRO.result[which_statistic].drawmode;
    if (!(CPRO.result[which_statistic].statisticexists)) {
        aw_message("statistic doesn't exist !");
        return;
    }

    float leastmax         = (float)(aw_root->awar("cpro/leastmax")->read_int())/100.0;
    CPRO.leastaccu         = aw_root->awar("cpro/leastaccu")->read_int();
    float firsttoreach     = (float)(aw_root->awar("cpro/firsttoreach")->read_int())/100.0;
    float firstreachedstep = (float) (aw_root->awar("cpro/firstreachedstep")->read_int())/100.0;

    char transversion = 1;
    {
        char *transversionstring = aw_root->awar("cpro/which_result")->read_string();
        if (strcmp(transversionstring, "transversions")) transversion = 0;
        free(transversionstring);
    }


    char *savename = aw_root->awar("cpro/condensename")->read_string();
    if (savename[0]==0) {
        free(savename);
        aw_message("no savename specified");
        return;
    }

    long maxcol = CPRO.result[which_statistic].maxalignlen;
    arb_progress progress("condense statistic", maxcol);

    char *result = (char *)calloc((unsigned int)maxcol+1, 1);
    for (long column=0; column<maxcol; column++) {
        float maximum = CPRO_getmaximum(column, transversion, (char)which_statistic, mode);

        if (maximum<-100.0) result[column]    = '.';
        else if (maximum<=0.0) result[column] = '-';
        else {
            result[column]    = (maximum>=leastmax) ? 'A' : 'a';
            float reachedhalf = CPRO_gethalfmaximum(column, maximum, firsttoreach, transversion, (char)which_statistic, mode);

            char steps;
            for (steps=0; (reachedhalf>firstreachedstep) && (steps<'Y'-'A'); steps++) {
                reachedhalf -= firstreachedstep;
            }
            result[column] += steps;
        }
        ++progress;
    }

    char *align = CPRO.result[which_statistic].alignname;

    GB_begin_transaction(GLOBAL.gb_main);

    GBDATA *gb_extended = GBT_find_or_create_SAI(GLOBAL.gb_main, savename);

    const char *typestring = GBS_global_string("RATES BY DISTANCE:  [%s] [UPPER_CASE%% %li]"
                                               " [ INTERREST%% %li] [STEP/CHAR %li]",
                                               (transversion) ? "transversion" : "all differences",
                                               (long)(leastmax*100.0),
                                               (long)(firsttoreach*100.0),
                                               (long)(firstreachedstep*100.0));

    GBDATA   *gb_param = GBT_add_data(gb_extended, align, "_TYPE", GB_STRING);
    GB_ERROR  error    = GB_write_string(gb_param, typestring);

    if (!error) {
        GBDATA *gb_data = GBT_add_data(gb_extended, align, "data", GB_STRING);
        error           = GB_write_string(gb_data, result);
    }

    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);

    free(result);
    free(savename);
}

static AW_window *CPRO_condensewindow_cb(AW_root *aw_root, AW_CL which_statistic)
{
    char buf[30];
    sprintf(buf, "CONDENSE STATISTIC %ld", (long)which_statistic+1);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, buf, buf);
    aws->load_xfig("cpro/condense.fig");
    aws->button_length(8);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("which_result");
    aws->create_toggle_field("cpro/which_result", NULL, "");
    aws->insert_default_toggle("      all\ndifferences",   "1", "diffs");
    aws->insert_toggle("     only\ntransversions", "1", "transversions");
    aws->update_toggle_field();

    aws->button_length(11);
    aws->at("begin"); aws->callback(CPRO_condense_cb, which_statistic);
    aws->create_button("CONDENSE_AND_EXPORT", "CONDENSE\nAND EXPORT", "E");

    aws->at("name"); aws->create_input_field("cpro/condensename", 11);

    aws->at("save_box");
    awt_create_selection_list_on_sai(GLOBAL.gb_main, aws, "cpro/condensename");

    return (AW_window *)aws;
}

static AW_window *CPRO_xpert_cb(AW_root *aw_root)
{
    static AW_window *expertwindow = 0;
    if (expertwindow) return (expertwindow);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "CPRO_EXPERT_PROPS", "EXPERT PROPERTIES");
    aws->load_xfig("cpro/expert.fig");
    aws->button_length(8);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("partition");
    aws->create_input_field("cpro/partition", 6);

    aws->at("leastaccu");
    aws->create_input_field("cpro/leastaccu", 3);

    aws->at("leastcompares");
    aws->create_input_field("cpro/leastcompares", 6);

    aws->at("gridvertical");
    aws->create_input_field("cpro/gridvertical", 3);

    aws->at("gridhorizontal");
    aws->create_input_field("cpro/gridhorizontal", 3);

    aws->at("leastmax");
    aws->create_input_field("cpro/leastmax", 3);

    aws->at("firsttoreach");
    aws->create_input_field("cpro/firsttoreach", 3);

    aws->at("firstreachedstep");
    aws->create_input_field("cpro/firstreachedstep", 3);

    aws->label_length(8);
    aws->button_length(8);
    aws->at("mempartition"); aws->create_button(0, "tmp/cpro/mempartition");

    return expertwindow=(AW_window *)aws;
}

static AW_window *CPRO_showstatistic_cb(AW_root *aw_root, unsigned char which_statistic) {
    char buf[20];
    sprintf(buf, "SHOW STATISTIC %d\n", (int)which_statistic+1);
    AW_window_simple *aws=new AW_window_simple;
    aws->init(aw_root, buf, buf);
    aws->load_xfig("cpro/show.fig");
    aws->button_length(6);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("column");
    aws->create_input_field(AWAR_CURSOR_POSITION, 4);

    aws->button_length(3);
    aws->at("d"); aws->callback((AW_CB0)CPRO_columnminus_cb);
    aws->create_button(0, "-", "1");
    aws->at("u"); aws->callback((AW_CB2)CPRO_columnplus_cb, which_statistic, 0);
    aws->create_button(0, "+", "2");

    sprintf(buf, "cpro/drawmode%d", (int)which_statistic);
    aws->at("drawmode"); aws->create_option_menu(buf);
    aws->insert_option("no smoothing", "n", 0);
    aws->insert_option("smoothing 1", "1", 1);
    aws->insert_option("smoothing 2", "2", 2);
    aws->insert_option("smoothing 3", "3", 3);
    aws->insert_option("smoothing 5", "5", 5);
    aws->insert_option("smoothing 10", "6", 10);
    aws->insert_option("smoothing 15", "7", 15);
    aws->update_option_menu();

    {
        RootCallback column_cb = makeRootCallback(CPRO_column_cb, static_cast<AW_window*>(aws), which_statistic);

        aw_root->awar(buf)->add_callback(column_cb);
        aw_root->awar("cpro/gridhorizontal")->add_callback(column_cb);
        aw_root->awar("cpro/gridvertical")->add_callback(column_cb);

        aws->at("maxdistance");
        aws->create_input_field("cpro/maxdistance", 3);

        aws->set_resize_callback(AW_INFO_AREA, makeWindowCallback(CPRO_resize_cb, which_statistic));
        aws->set_expose_callback(AW_INFO_AREA, makeWindowCallback(CPRO_expose_cb, which_statistic));
        aw_root->awar(AWAR_CURSOR_POSITION)->add_callback(column_cb);
        aw_root->awar("cpro/maxdistance")->add_callback(column_cb);
        aw_root->awar("cpro/maxdistance")->add_callback(column_cb);
    }

    aws->button_length(6);

    AW_device *device=aws->get_device (AW_INFO_AREA);
    device->reset();

    device->new_gc(GC_black);
    device->set_line_attributes(GC_black, 1, AW_SOLID);
    device->set_foreground_color(GC_black, AW_WINDOW_FG);
#ifndef ARB_GTK
    device->set_font(GC_black, 0, 10, 0);
#endif
    device->new_gc(GC_blue);
    device->set_line_attributes(GC_blue, 1, AW_SOLID);
    device->set_foreground_color(GC_blue, AW_WINDOW_C1);
    device->new_gc(GC_green);
    device->set_line_attributes(GC_green, 1, AW_SOLID);
    device->set_foreground_color(GC_green, AW_WINDOW_C2);
    device->new_gc(GC_grid);
    device->set_line_attributes(GC_grid, 1, AW_DASHED);
    device->set_foreground_color(GC_grid, AW_WINDOW_C3);

    return aws;
}

static AW_window *CPRO_calculatewin_cb(AW_root *aw_root, AW_CL which_statistic)
{
    char buf[30];
    sprintf(buf, "CALCULATE STATISTIC %ld", (long)which_statistic+1);
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, buf, buf);
    aws->load_xfig("cpro/calc.fig");
    aws->button_length(10);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("resolution");
    aws->create_input_field("cpro/resolution", 8);

    aws->at("transratio");
    aws->create_input_field("cpro/transratio", 8);

    aws->at("which_alignment");
    awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, "cpro/alignment", "*=");

    aws->button_length(10);
    aws->at("xpert"); aws->callback(AW_POPUP, (AW_CL)CPRO_xpert_cb, 0);
    aws->create_button("EXPERT_OPTIONS", "expert...", "x");

    aws->at("calculate");
    aws->callback(CPRO_calculate_cb, (AW_CL)which_statistic);
    aws->create_button("CALCULATE", "CALCULATE", "A");

    aws->at("which_species");
    aws->create_toggle_field("cpro/which_species", NULL, "");
    aws->insert_toggle("all vs all", "1", "all");
    aws->insert_toggle("marked vs marked",   "1", "marked");
    aws->insert_default_toggle("marked vs all",   "1", "markedall");
    aws->update_toggle_field();

    aws->at("countgaps");
    aws->create_toggle_field("cpro/countgaps", NULL, "");
    aws->insert_toggle("on", "1", "on");
    aws->insert_default_toggle("off",   "1", "off");
    aws->update_toggle_field();

    aws->label_length(8);
    aws->button_length(8);
    aws->at("memstatistic");
    aws->create_button(0, "tmp/cpro/memstatistic");

    return aws;
}

/* -----------------------------------------------------------------
 * Function:                 AP_open_cprofile_window
 *
 * Arguments:                    .
 *
 * Returns:                      .
 *
 * Description:      Draws window, initializes callback for most important
 *                   function, CPRO_begin_cb
 *
 * NOTE:                         .
 *
 * Global Variables referenced:  .
 *
 * Global Variables modified:    x
 *
 * AWARs referenced:             .
 *
 * AWARs modified:               x
 *
 * Dependencies:      Needs xfig file cprofile.fig
 * -----------------------------------------------------------------
 */
AW_window *
AP_open_cprofile_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "CPR_MAIN", "Conservation Profile: Distance Matrix");
    aws->load_xfig("cpro/main.fig");
    aws->button_length(10);

    GB_push_transaction(GLOBAL.gb_main);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("pos_variability.ps"));
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(10);
    aws->at("xpert"); aws->callback(AW_POPUP, (AW_CL)CPRO_xpert_cb, 0);
    aws->create_button("EXPERT_OPTIONS", "expert...", "x");

    // start action by button
    aws->button_length(17);
    aws->at("calculate1"); aws->callback(AW_POPUP, (AW_CL)CPRO_calculatewin_cb, 0);
    aws->create_button("GO_STAT_1", "calculate as\nstatistic 1 ...", "c");
    aws->at("calculate2"); aws->callback(AW_POPUP, (AW_CL)CPRO_calculatewin_cb, 1);
    aws->create_button("GO_STAT_2", "calculate as\nstatistic 2 ...", "a");

    aws->button_length(17);
    aws->at("save1"); aws->callback(AW_POPUP, (AW_CL)CPRO_savestatisticwindow_cb, 0);
    aws->create_button("SAVE_STAT_1", "save\nstatistic 1 ...", "s");
    aws->at("save2"); aws->callback(AW_POPUP, (AW_CL)CPRO_savestatisticwindow_cb, 1);
    aws->create_button("SAVE_STAT_2", "save\nstatistic 2 ...", "v");

    aws->button_length(17);
    aws->at("show1"); aws->callback(AW_POPUP, (AW_CL)CPRO_showstatistic_cb, 0);
    aws->create_button("SHOW_STAT_!", "show\ngraph 1 ...", "h");
    aws->at("show2"); aws->callback(AW_POPUP, (AW_CL)CPRO_showstatistic_cb, 1);
    aws->create_button("SHOW_STAT_2", "show\ngraph 2 ...", "w");

    aws->button_length(17);
    aws->at("load1"); aws->callback(AW_POPUP, (AW_CL)CPRO_loadstatisticwindow_cb, 0);
    aws->create_button("LOAD_STAT_1", "load\nstatistic 1 ... ", "l");
    aws->at("load2"); aws->callback(AW_POPUP, (AW_CL)CPRO_loadstatisticwindow_cb, 1);
    aws->create_button("LOAD_STAT_2", "load\nstatistic 2 ...", "d");

    aws->at("condense1"); aws->callback(AW_POPUP,   (AW_CL)CPRO_condensewindow_cb, 0);
    aws->create_button("CONDENSE_STAT_1", "condense\nstatistic 1 ...", "o");
    aws->at("condense2"); aws->callback(AW_POPUP,   (AW_CL)CPRO_condensewindow_cb, 1);
    aws->create_button("CONDENSE_STAT_2",   "condense\nstatistic 2 ...", "n");

    aws->at("memfor1"); aws->create_button(0, "tmp/cpro/memfor1");
    aws->at("memfor2"); aws->create_button(0, "tmp/cpro/memfor2");
    aws->at("which1"); aws->create_button(0, "tmp/cpro/which1");
    aws->at("which2"); aws->create_button(0, "tmp/cpro/which2");
    aws->at("resolution1"); aws->create_button(0, "tmp/cpro/nowres1");
    aws->at("resolution2"); aws->create_button(0, "tmp/cpro/nowres2");

    aw_root->awar("cpro/alignment")->add_callback(CPRO_memrequirement_cb);
    aw_root->awar("cpro/partition")->add_callback(CPRO_memrequirement_cb);
    aw_root->awar("cpro/resolution")->add_callback(CPRO_memrequirement_cb);
    aw_root->awar("cpro/which_species")->add_callback(CPRO_memrequirement_cb);

    GB_pop_transaction(GLOBAL.gb_main);

    CPRO_memrequirement_cb(aw_root);

    return (AW_window *)aws;
}
