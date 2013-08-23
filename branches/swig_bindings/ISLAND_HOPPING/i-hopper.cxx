// =============================================================
/*                                                               */
//   File      : i-hopper.c
//   Purpose   :
/*                                                               */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                               */
// =============================================================

#include "i-hopper.h"
#include "mem.h"

// ============================================================================

#ifndef ARB
int main(void) {

    Error=NULL;

    fprintf(stdout,"\nAligning ...");

    {
        char X[]="CTTCGCTTTGGATCCTTACTAGGATCTGCCTAGTACATTCAAATCTTAACAGGCTTATTTCTGTGTGGGTGTGTGTGTGAATACATTACACATCAGACACATCAACTG";
        int secX[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        char Y[]="GGTCTTTATTAGGAATATGCCTAATTATTCAAATTCTCACCGGACTATTCACACCCCAACACACACACACAATACACTACACCTCTGACACATTCACCGCCTT";
        int secY[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        int freqs=FALSE;
        double fT=0.25;
        double fC=0.25;
        double fA=0.25;
        double fG=0.25;

        double rTC=4.0;
        double rTA=1.0;
        double rTG=1.0;
        double rCA=1.0;
        double rCG=1.0;
        double rAG=4.0;

        double dist=0.3;
        double supp=0.5;
        double gapA=8.;
        double gapB=4.;
        double gapC=7.;
        double thres=0.005;

        char *XX=NULL;
        char *YY=NULL;

        int nX;
        int nY;

        nX=strlen(X);
        nY=strlen(Y);

        Align(
              nX,X,secX,&XX,nY,Y,secY,&YY,
              freqs,fT,fC,fA,fG,
              rTC,rTA,rTG,rCA,rCG,rAG,
              dist,supp,gapA,gapB,gapC,thres
              );

        if(Error) goto error;

        { FILE *fp; int i,nXX,nYY;
            nXX=strlen(XX);
            nYY=strlen(YY);
            fp=fopen("alignment.txt","w");
            for(i=0;i<nXX;i++) fprintf(fp,"%c",XX[i]); fprintf(fp,"\n");
            for(i=0;i<nYY;i++) fprintf(fp,"%c",YY[i]); fprintf(fp,"\n");
            fclose(fp);
        }

        freeBlock(&XX);
        freeBlock(&YY);

    }

    clearUp();

    return(EXIT_SUCCESS);

 error:

    fprintf(stdout,"\n!!! %s\n",Error);
    fprintf(stdout,"\nPress RETURN to exit\n");
    getc(stdin);

    clearUp();

    return(EXIT_FAILURE);

}
#endif
