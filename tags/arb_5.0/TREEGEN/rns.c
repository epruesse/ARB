#include <limits.h>
#include <stdlib.h>
#include <memory.h>

#include "rns.h"
#include "spreadin.h"


/* /------------------------\ */
/* |  Erzeugung der Ur-RNS  | */
/* \------------------------/ */

int        orgLen;         /* LÑnge der Ur-RNS */
double     orgHelixPart;   /* Anteil Helix-Bereich */
static int rnsCreated; /* Anzahl bisher erzeugter RNS-Sequenzen */

/* /------------\ */
/* |  Mutation  | */
/* \------------/ */

int    timeSteps;        /* Anzahl Zeitschritte */
Frand  mrpb_Init,        /* Initialisierungsfunktion fÅr 'mutationRatePerBase' */
       l2hrpb_Init,      /* Initialisierungsfunktion fÅr 'loop2helixRatePerBase' */
       pairPart,         /* Anteil paarender Helix-Bindungen */
       mutationRate,     /* Mutationsrate */
       splitRate,        /* Spaltungsrate */
       helixGcDruck,     /* G-C-Druck       im Helix-Bereich */
       helixGcRate,      /* VerhÑltnis G:C  im Helix-Bereich */
       helixAtRate,      /* VerhÑltnis A:T  im Helix-Bereich */
       loopGcDruck,      /* G-C-Druck       im Loop-Bereich */
       loopGcRate,       /* VerhÑltnis G:C  im Loop-Bereich */
       loopAtRate;       /* VerhÑltnis A:T  im Loop-Bereich */
double transitionRate,   /* Transition-Rate */
       transversionRate; /* Transversion-Rate */

static double     *mutationRatePerBase,   /* positionsspez. Mutationsrate (wird nur einmal bestimmt und bleibt dann konstant) */
                  *loop2helixRatePerBase; /* positionsspez. Rate fÅr Wechsel Loop-Base in Helix-Base und vv. (wird nur einmal bestimmt und bleibt dann konstant) */
static int         mrpb_anz,              /* Anzahl Positionen */
                   mrpb_allocated,        /* wirklich Grî·e des Arrays */
                   l2hrpb_anz,            /* Anzahl Positionen */
                   l2hrpb_allocated;      /* wirklich Grî·e des Arrays */
static DoubleProb  helixMutationMatrix,   /* Mutationsmatrix fÅr Helix-Bereiche */
                   loopMutationMatrix;    /* Mutationsmatrix fÅr Loop-Bereiche */

/* /----------------------\ */
/* |  Ausgabefilepointer  | */
/* \----------------------/ */

FILE *topo, /* Topologie */
     *seq;  /* Sequenzen */

/* /-------------\ */
/* |  Sonstiges  | */
/* \-------------/ */

static int minDepth = INT_MAX, /* minimale Tiefe (Astanzahl) der Blattspitzen */
           maxDepth = INT_MIN; /* maximale Tiefe der Blattspitzen */

/* -------------------------------------------------------------------------- */
/*      void dumpDepths(void) */
/* ------------------------------------------------------ 24.05.95 22.27 ---- */
void dumpDepths(void)
{
    printf("Minimale Baumtiefe = %i\n", minDepth);
    printf("Maximale Baumtiefe = %i\n", maxDepth);
}
/* -------------------------------------------------------------------------- */
/*      static void dumpRNS(RNS rns) */
/* ------------------------------------------------------ 26.05.95 11.29 ---- */
static void dumpRNS(RNS rns)
{
    int        b,
               b1,
               b2;
    static int cleared,
               h_cnt[BASETYPES+1][BASETYPES+1],
               l_cnt[BASETYPES+1],
               loop,
               helix;

    if (!cleared)
    {
        for (b1 = 0; b1<(BASETYPES+1); b1++)
        {
            for (b2 = 0; b2<(BASETYPES+1); b2++) h_cnt[b1][b2] = 0;
            l_cnt[b1] = 0;
        }

        loop  = 0;
        helix = 0;

        cleared = 1;
    }

    if (rns)
    {
        for (b = 0; b<(rns->bases); b++)
        {
            char base = rns->base[b];

            if (isHelical(base))
            {
                int bt1 = char2BaseType(base),
                    bt2 = char2BaseType(rns->base[b+1]);

                h_cnt[bt1][bt2]++;
                helix++;
                b++;
            }
            else
            {
                int bt = char2BaseType(base);

                l_cnt[bt]++;
                loop++;
            }
        }
    }
    else
    {
        printf("Helix-Basenpaare = %i\n"
               "Loop-Basen       = %i\n"
               "Helix:Loop       = %f\n",
               helix,
               loop,
               (double)helix/(double)loop);

        {
            int gc      = h_cnt[BASE_C][BASE_G]+h_cnt[BASE_G][BASE_C],
                at      = h_cnt[BASE_A][BASE_T]+h_cnt[BASE_T][BASE_A],
                paarend = gc+at;

            printf("GC-Paare              = %i\n"
                   "AT-Paare              = %i\n"
                   "Paare:Helix-Bindungen = %f\n"
                   "GC-Paare:Paare        = %f\n",
                   gc,
                   at,
                   (double)paarend/(double)helix,
                   (double)gc/(double)paarend);
        }

        printf("\n");
    }
}
/* -------------------------------------------------------------------------- */
/*      static void initBaseSpecificProbs(int bases) */
/* ------------------------------------------------------ 24.05.95 12.51 ---- */
static void initBaseSpecificProbs(int bases)
{
    int b;

    mrpb_anz            = bases;
    mrpb_allocated      = bases;
    mutationRatePerBase = malloc(bases*sizeof(double));

    l2hrpb_anz            = bases;
    l2hrpb_allocated      = bases;
    loop2helixRatePerBase = malloc(bases*sizeof(double));

    if (!mutationRatePerBase || !loop2helixRatePerBase) outOfMemory();

    for (b = 0; b<bases; b++)
    {
        mutationRatePerBase[b]   = getFrand(mrpb_Init);
        loop2helixRatePerBase[b] = getFrand(l2hrpb_Init);
    }
}
/* -------------------------------------------------------------------------- */
/*      static RNS allocRNS(int len) */
/* ------------------------------------------------------ 20.05.95 16.04 ---- */
static RNS allocRNS(int len)
{
    RNS rns = malloc(sizeof(*rns));

    if (!rns) outOfMemory();

/*     rns->bases = orgLen; */
/*     rns->base  = malloc(sizeof(*(rns->base))*orgLen); */
    rns->bases = len;
    rns->base  = malloc(sizeof(*(rns->base))*len);

    if (!rns->base) outOfMemory();

    return rns;
}
/* -------------------------------------------------------------------------- */
/*      RNS createOriginRNS(void) */
/* ------------------------------------------------------ 14.05.95 14:54 ---- */
/* */
/*  Erzeugt eine Ur-RNS */
/* */
RNS createOriginRNS(void)
{
    RNS rns      = allocRNS(orgLen);
    int helixLen = orgLen*orgHelixPart,
        l;
    str base     = rns->base;

    printf("Generating origin species..\n");

    initBaseSpecificProbs(orgLen);

    rns->laufNr = rnsCreated++;

    /* /------------------\ */
    /* |  Helix erzeugen  |                                                             im Loop-Bereich */
    /* \------------------/ */

    if (helixLen%1) helixLen--;                            /* mu· gerade LÑnge haben, da nur Paare! */

    assert(helixLen<=orgLen);

    rns->helix   = helixLen/2;
    rns->pairing = 0;

    {
        DoubleProb orgHelixProb;
        Spreading  s;
        int        b1,
                   b2;
        double     actPairPart     = getFrand(pairPart),
                   actHelixGcDruck = getFrand(helixGcDruck),
                   actHelixGcRate  = getFrand(helixGcRate),
                   actHelixAtRate  = getFrand(helixAtRate),
                   nonPairProb     = (1.0-actPairPart)/2.0;

        for (b1 = 0; b1<BASETYPES; b1++)
        {
            for (b2 = 0; b2<BASETYPES; b2++)
            {
                if (isPairing(b1, b2))
                {
                    switch (b1)
                    {
                        case BASE_A:
                        case BASE_T:
                        {
                            orgHelixProb[b1][b2] = (actPairPart*(1.0-actHelixGcDruck))/2.0;
                            break;
                        }
                        case BASE_C:
                        case BASE_G:
                        {
                            orgHelixProb[b1][b2] = (actPairPart*actHelixGcDruck)/2.0;
                            break;
                        }
                    }
                }
                else
                {
                    double prob = nonPairProb;
                    int    b    = b1;

                    while (1)                              /* wird je einmal mit b1 und b2 ausgefÅhrt */
                    {
                        switch (b)
                        {
                            case BASE_A:
                            {
                                prob = prob*(1.0-actHelixGcDruck)*actHelixAtRate;
                                break;
                            }
                            case BASE_C:
                            {
                                prob = prob*actHelixGcDruck*(1.0-actHelixGcRate);
                                break;
                            }
                            case BASE_G:
                            {
                                prob = prob*actHelixGcDruck*actHelixGcRate;
                                break;
                            }
                            case BASE_T:
                            {
                                prob = prob*(1.0-actHelixGcDruck)*(1.0-actHelixAtRate);
                                break;
                            }
                        }

                        if (b==b2) break;
                        b = b2;
                    }

                    orgHelixProb[b1][b2] = prob;
                }
            }
        }

        s = newSpreading((double*)orgHelixProb, BASEQUAD);

        for (l = 0; l<helixLen; l+=2)
        {
            int val = spreadRand(s),
                B1  = val%BASETYPES,
                B2  = val/BASETYPES;

            base[l]   = helixBaseChar[B1];
            base[l+1] = helixBaseChar[B2];

            rns->pairing += isPairing(B1, B2);
        }

        freeSpreading(s);
    }

    /* /-----------------\ */
    /* |  Loop erzeugen  | */
    /* \-----------------/ */

    {
        SingleProb orgLoopProb;
        Spreading  s;
        double     actLoopGcDruck = getFrand(loopGcDruck),
                   actLoopGcRate  = getFrand(loopGcRate),
                   actLoopAtRate  = getFrand(loopAtRate);

        orgLoopProb[BASE_A] = (1.0-actLoopGcDruck)*actLoopAtRate;
        orgLoopProb[BASE_C] = actLoopGcDruck*(1.0-actLoopGcRate);
        orgLoopProb[BASE_G] = actLoopGcDruck*actLoopGcRate;
        orgLoopProb[BASE_T] = (1.0-actLoopGcDruck)*(1.0-actLoopAtRate);

        s = newSpreading((double*)orgLoopProb, BASETYPES);
        for (; l<orgLen; l++) base[l] = loopBaseChar[spreadRand(s)];
        freeSpreading(s);
    }

    return rns;
}
/* -------------------------------------------------------------------------- */
/*      void freeRNS(RNS rns) */
/* ------------------------------------------------------ 20.05.95 19.45 ---- */
void freeRNS(RNS rns)
{
    free(rns->base);
    free(rns);
}
/* -------------------------------------------------------------------------- */
/*      static RNS dupRNS(RNS rns) */
/* ------------------------------------------------------ 20.05.95 20.32 ---- */
static RNS dupRNS(RNS rns)
{
    RNS neu = malloc(sizeof(*rns));

    if (!neu) outOfMemory();

    memcpy(neu, rns, sizeof(*rns));

    neu->base = malloc(rns->bases*sizeof(*(neu->base)));
    memcpy(neu->base, rns->base, rns->bases);

    neu->laufNr = rnsCreated++;

    return neu;
}
/* -------------------------------------------------------------------------- */
/*      static void dumpDoubleProb(double *d, int anz) */
/* ------------------------------------------------------ 25.05.95 01.31 ---- */
/*static void dumpDoubleProb(double *d, int anz) */
/*{ */
/*    while (anz--) printf("%-10f", *d++); */
/*    printf("\n\n"); */
/*} */
/* -------------------------------------------------------------------------- */
/*      static void calcMutationMatrix(DoubleProb mutationMatrix, double mu... */
/* ------------------------------------------------------ 24.05.95 13.58 ---- */
static void calcMutationMatrix(DoubleProb mutationMatrix, double muteRate, double gcDruck, double gcRate, double atRate, double *pairProb)
{
    double k   = transitionRate/transversionRate,
           fa  = (1.0-gcDruck)*atRate,
           fc  = gcDruck*(1.0-gcRate),
           fg  = gcDruck*gcRate,
           ft  = (1.0-gcDruck)*(1.0-atRate),
           bfa = transversionRate*fa,
           bfc = transversionRate*fc,
           bfg = transversionRate*fg,
           bft = transversionRate*ft,
           kag = k/(fa+fg),
           kct = k/(fc+ft);
/*           sa  = (kag+3.0)*bfa,                            // Summe der "mutierenden" Positionen jeder Zeile */
/*           sc  = (kct+3.0)*bfc, */
/*           sg  = (kag+3.0)*bfg, */
/*           st  = (kct+3.0)*bft; */

    /* Auf aktuelle Mutationsrate normieren */

/*    bfa = bfa*muteRate/sa; */
/*    bfc = bfc*muteRate/sc; */
/*    bfg = bfg*muteRate/sg; */
/*    bft = bft*muteRate/st; */

/*    printf("bfa=%f\n", bfa); */
/*    printf("bfc=%f\n", bfc); */
/*    printf("bfg=%f\n", bfg); */
/*    printf("bft=%f\n", bft); */

    /* Matrix besetzen */

    mutationMatrix[BASE_A][BASE_A] = 1.0-(kag+3.0)*bfa;
    mutationMatrix[BASE_C][BASE_A] = bfa;
    mutationMatrix[BASE_G][BASE_A] = (kag+1.0)*bfa;
    mutationMatrix[BASE_T][BASE_A] = bfa;

    mutationMatrix[BASE_A][BASE_C] = bfc;
    mutationMatrix[BASE_C][BASE_C] = 1.0-(kct+3.0)*bfc;
    mutationMatrix[BASE_G][BASE_C] = bfc;
    mutationMatrix[BASE_T][BASE_C] = (kct+1.0)*bfc;

    mutationMatrix[BASE_A][BASE_G] = (kag+1.0)*bfg;
    mutationMatrix[BASE_C][BASE_G] = bfg;
    mutationMatrix[BASE_G][BASE_G] = 1.0-(kag+3.0)*bfg;
    mutationMatrix[BASE_T][BASE_G] = bfg;

    mutationMatrix[BASE_A][BASE_T] = bft;
    mutationMatrix[BASE_C][BASE_T] = (kct+1.0)*bft;
    mutationMatrix[BASE_G][BASE_T] = bft;
    mutationMatrix[BASE_T][BASE_T] = 1.0-(kct+3.0)*bft;

/*    { */
/*        int von,                                         // Matrix ausgeben */
/*            nach; */
/* */
/*        printf("       von %c     von %c     von %c     von %c     \n", */
/*                helixBaseChar[0], */
/*                helixBaseChar[1], */
/*                helixBaseChar[2], */
/*                helixBaseChar[3] ); */
/* */
/*        for (nach = BASE_A; nach<=BASE_T; nach++) */
/*        { */
/*            double sum = 0.0; */
/* */
/*            printf("nach %c ", helixBaseChar[nach]); */
/* */
/*            for (von = BASE_A; von<=BASE_T; von++) */
/*            { */
/*                printf("%-10f", mutationMatrix[von][nach]); */
/*                sum += mutationMatrix[von][nach]; */
/*            } */
/* */
/*            printf("    sum = %-10f\n", sum); */
/*        } */
/* */
/*        printf("\n"); */
/*    } */

    if (pairProb)                                          /* soll pairProb berechnet werden? */
    {
        double mutatesTo[BASETYPES],
               freq[BASETYPES];                            /* HÑufigkeit der einzelnen Basen */
        int    von,
               nach;

        freq[BASE_A] = fa;
        freq[BASE_C] = fc;
        freq[BASE_G] = fg;
        freq[BASE_T] = ft;

        for (nach = 0; nach<BASETYPES; nach++)
            mutatesTo[nach] = 0.0;

        for (von = 0; von<BASETYPES; von++)
            for (nach = 0; nach<BASETYPES; nach++)
                mutatesTo[nach] += mutationMatrix[von][nach]*freq[von];

        *pairProb = 2.0*mutatesTo[BASE_A]*mutatesTo[BASE_T] + 2.0*mutatesTo[BASE_C]*mutatesTo[BASE_G];
    }
}
/* -------------------------------------------------------------------------- */
/*      static int calcPairTrials(double pairProb, double actPairPart) */
/* ------------------------------------------------------ 25.05.95 13.31 ---- */
/* */
/*  Berechnet die Anzahl Mutations-Wiederholungen, die notwendig sind, um */
/*  mindestens 'actPairPart' Prozent paarende Bindungen zu erhalten, falls */
/*  die Wahrscheinlichkeit eine paarende Bindung zu erzeugen gleich */
/*  'pairProb' ist. */
/* */
static int calcPairTrials(double pairProb, double actPairPart)
{
    int    trials   = 1;
    double failProb = 1.0-pairProb,
           succProb = pairProb;

    while (succProb<actPairPart)
    {
        pairProb *= failProb;
        succProb += pairProb;
        trials++;

/*        printf("trials=%i succProb=%f actPairPart=%f\n", trials, succProb, actPairPart); */
    }

    return trials;
}
/* -------------------------------------------------------------------------- */
/*      static void indent(int depth) */
/* ------------------------------------------------------ 24.05.95 21.08 ---- */
/*static void indent(int depth) */
/*{ */
/*    while (depth--) fputc(' ', topo); */
/*} */
/* -------------------------------------------------------------------------- */
/*      static void mutateRNS(RNS rns, int steps, int depth) */
/* ------------------------------------------------------ 20.05.95 19.50 ---- */
/* */
/*  Mutiert eine RNS bis zur nÑchsten Spaltung */
/* */
/*  'steps'     Anzahl noch zu durchlaufender Zeitschritte */
/* */
static void mutateRNS(int no_of_father, RNS rns, int steps, int depth)
{
    int    splitInSteps,
           s;
    double mutationTime = 0.0;

    /* /---------------------------------------\ */
    /* |  Schritte bis zur Spaltung berechnen  | */
    /* \---------------------------------------/ */

    {
        double actualSplitRate = getFrand(splitRate);

        assert(actualSplitRate!=0);

        splitInSteps = (int)(1.0/actualSplitRate);
        if (splitInSteps>steps) splitInSteps = steps;

        assert(splitInSteps>=1);
    }

    /* /----------------------------\ */
    /* |  Zeitschritte durchlaufen  | */
    /* \----------------------------/ */

    for (s = 0; s<splitInSteps; s++)
    {
        int       b,
                  pairTrials;                              /* Anzahl Versuche eine paarende Helixbindung herzustellen */
        double    actMutationRate    = getFrand(mutationRate),
                  actPairPart        = getFrand(pairPart);
        Spreading s_helix[BASETYPES],
                  s_loop[BASETYPES];

        {
            double pairProb;                               /* Wahrscheinlichkeit, da· ein Paar im helikalen Bereich entsteht */

            calcMutationMatrix(helixMutationMatrix, 1.0, getFrand(helixGcDruck), getFrand(helixGcRate), getFrand(helixAtRate), &pairProb);
            calcMutationMatrix(loopMutationMatrix, actMutationRate, getFrand(loopGcDruck), getFrand(loopGcRate), getFrand(loopAtRate), NULL);

            pairTrials = calcPairTrials(pairProb, actPairPart);
/*            printf("pairProb=%f pairTrials=%i\n", pairProb, pairTrials); */
        }

        for (b = 0; b<BASETYPES; b++)
        {
            s_helix[b] = newSpreading(&(helixMutationMatrix[b][0]), BASETYPES);
            s_loop[b]  = newSpreading(&(loopMutationMatrix[b][0]), BASETYPES);
        }

        mutationTime += actMutationRate;                   /* Mutationszeit aufaddieren (Einheit ist Mutationsrate*Zeitschritte) */

        /* /----------------------------------\ */
        /* |  Alle Basen(-paare) durchlaufen  | */
        /* \----------------------------------/ */

        for (b = 0; b<(rns->bases); )
        {
            char base = rns->base[b];

            if (!isDeleted(base))                          /* Deletes ignorieren */
            {
                /* /---------------------\ */
                /* |  Helicale Bereiche  | */
                /* \---------------------/ */

                if (isHelical(base))
                {
                    int  trials = pairTrials,
                         mut1   = randProb()<mutationRatePerBase[b]*actMutationRate,
                         mut2   = randProb()<mutationRatePerBase[b+1]*actMutationRate;
                    char base2  = rns->base[b+1];

                    assert(isHelical(base2));

                    if (mut1 || mut2)
                    {
                        int bt1 = char2BaseType(base),
                            bt2 = char2BaseType(base2);

                        if (isPairing(bt1, bt2))
                        {
                            rns->pairing--;
                        }

                        while (trials--)
                        {
                            if (mut1)
                            {
                                if (mut2)                  /* beide Basen mutieren */
                                {
                                    bt1 = spreadRand(s_helix[bt1]);
                                    bt2 = spreadRand(s_helix[bt2]);
                                }
                                else                       /* nur 1.Base mutieren */
                                {
                                    bt1 = spreadRand(s_helix[bt1]);
                                }
                            }
                            else                           /* nur 2.Base mutieren */
                            {
                                bt2 = spreadRand(s_helix[bt2]);
                            }

                            if (isPairing(bt1, bt2))       /* paarend? ja->abbrechen */
                            {
                                rns->pairing++;
                                break;
                            }
                        }

                        rns->base[b]   = helixBaseChar[bt1];
                        rns->base[b+1] = helixBaseChar[bt2];
                    }

                    b++;
                }

                /* /-----------------\ */
                /* |  Loop-Bereiche  | */
                /* \-----------------/ */

                else
                {
                    double mutationProb = actMutationRate*mutationRatePerBase[b];

                    if (randProb()<mutationProb)
                    {
                        rns->base[b] = loopBaseChar[spreadRand(s_loop[char2BaseType(base)])];
                    }
                }
            }

            b++;
        }

        for (b = 0; b<BASETYPES; b++)
        {
            freeSpreading(s_helix[b]);
            freeSpreading(s_loop[b]);
        }
    }

    splitRNS(no_of_father, rns, mutationTime, steps-splitInSteps, depth+1);
}
/* -------------------------------------------------------------------------- */
/*      void splitRNS(RNS origin, double age, int steps, int depth) */
/* ------------------------------------------------------ 20.05.95 20.13 ---- */
/* */
/*  Spaltet eine RNS in zwei Species auf */
/* */
void splitRNS(int no_of_father, RNS origin, double age, int steps, int depth)
{
    int x;

    dumpRNS(origin);

    /* /---------------------\ */
    /* |  Sequenz schreiben  | */
    /* \---------------------/ */

    if (no_of_father != -1) {
        fprintf(seq, ">no%i son of no%i\n", origin->laufNr, no_of_father);
    }
    else {
        fprintf(seq, ">no%i father of all species\n", origin->laufNr);
    }
    no_of_father = origin->laufNr; /* now i'm the father */
    for (x = 0; x<(origin->bases); x++) fputc(origin->base[x], seq);
    fputc('\n', seq);

    if (steps)                                             /* Species splitten! */
    {
        double gcDruck_val      = helixGcDruck->val,       /* Frand-Werte merken */
               pairPart_val     = pairPart->val,
               mutationRate_val = mutationRate->val,
               splitRate_val    = splitRate->val;

/*        indent(depth); */
        fprintf(topo, "(no%i:%f,\n", origin->laufNr, age);

        {
            RNS left = dupRNS(origin);                     /* linker Sohn */

            mutateRNS(no_of_father, left, steps, depth);
            freeRNS(left);
        }

        fputs(",\n", topo);

        helixGcDruck->val = gcDruck_val;                   /* Frand-Werte wiederherstellen */
        pairPart->val     = pairPart_val;
        mutationRate->val = mutationRate_val;
        splitRate->val    = splitRate_val;

        {
            RNS right = dupRNS(origin);                    /* rechter Sohn */

            mutateRNS(no_of_father, right, steps, depth);
            freeRNS(right);
        }

        fputc(')', topo);
    }
    else                                                   /* Baumspitze */
    {
        if      (depth>maxDepth) maxDepth = depth;
        else if (depth<minDepth) minDepth = depth;

/*        indent(depth); */
        fprintf(topo, "no%i:%f", origin->laufNr, age);

        if ((origin->laufNr%100) == 0) {
            printf("generated Species: %i\n", origin->laufNr);
        }
    }

    if (age==0.0) dumpRNS(NULL);
}


