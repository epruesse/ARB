#include "island_hopping.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define EXTERN
extern "C" {
#include "i-hopper.h"
#include "memory.h"
}

IslandHoppingParameter *IslandHopping::para = 0;

IslandHoppingParameter::IslandHoppingParameter(bool    use_user_freqs_,
                                               double fT_, double fC_, double fA_, double fG_,
                                               double rTC_, double rTA_, double rTG_, double rCA_, double rCG_, double rAG_,
                                               double dist_, double supp_, double gapA_, double gapB_, double gapC_, double thres_)
{
    use_user_freqs = use_user_freqs_;
    fT    = fT_;
    fC    = fC_;
    fA    = fA_;
    fG    = fG_;

    rTC   = rTC_;
    rTA   = rTA_;
    rTG   = rTG_;
    rCA   = rCA_;
    rCG   = rCG_;
    rAG   = rAG_;

    dist  = dist_;
    supp  = supp_;
    gapA   = gapA_;
    gapB   = gapB_;
    gapC   = gapC_;
    thres = thres_;

    //@@@ init();
}

IslandHoppingParameter::~IslandHoppingParameter() {
    // @@ uninit();
}


//  -------------------------------------------
//      GB_ERROR IslandHopping::do_align()
//  -------------------------------------------
GB_ERROR IslandHopping::do_align() {

    if (!para) {
        para = new IslandHoppingParameter(0, 0.25, 0.25, 0.25, 0.25, 0, 4.0, 1.0, 1.0, 1.0, 1.0, 4.0, 0.3, 0.5, 8.0, 4.0, 0.001);
    }

    int   nX;
    char *X;
    int  *secX;

    int nY;
    char *Y;
    int *secY;

    char *XX=NULL;
    char *YY=NULL;

    int i,j,k;

    Error = 0;

    nX = 0; nY=0;
    for(i=0;i<alignment_length;i++) {
        if(ref_sequence[i]!='-' && ref_sequence[i] != '.') nX++;
        if(toAlign_sequence[i]!='-' && toAlign_sequence[i] != '.') nY++;
    }

    X=(char *)malloc((nX+1)*sizeof(char));
    secX=(int *)malloc((nX)*sizeof(int));

    Y    = (char *)malloc((nY+1)*sizeof(char));
    secY = (int *)malloc((nY)*sizeof(int));

    // @@@ helix?

    j = 0;
    k = 0;

    for(i=0;i<alignment_length;i++) {
        if(ref_sequence[i]!='-' && ref_sequence[i]!='.') {
            X[j] = ref_sequence[i]; secX[j]=0;
            j++;
        }
        if(toAlign_sequence[i]!='-' && toAlign_sequence[i]!='.') {
            Y[k] = toAlign_sequence[i]; secY[k]=0;
            k++;
        }
    }
    X[j]='\0'; Y[k]='\0';

    if(output_sequence) {delete output_sequence; output_sequence=0;}
    if(aligned_ref_sequence) {delete aligned_ref_sequence; aligned_ref_sequence=0;}

    Align(
          nX,X,secX,&XX,nY,Y,secY,&YY,
          para->use_user_freqs,para->fT,para->fC,para->fA,para->fG,
          para->rTC,para->rTA,para->rTG,para->rCA,para->rCG,para->rAG,
          para->dist,para->supp,para->gapA,para->gapB,para->gapC,para->thres
          );

    if(!Error) {
        int nXY                 = strlen(XX);
        int i;
        output_alignment_length = nXY;

        {
            FILE *fp;
            fp = fopen("alignment.txt","w");
            for(i=0;i<nXY;i++) fprintf(fp,"%c",XX[i]); fprintf(fp,"\n");
            for(i=0;i<nXY;i++) fprintf(fp,"%c",YY[i]); fprintf(fp,"\n");
            fclose(fp);
        }

        aligned_ref_sequence = new char[nXY+1];
        output_sequence      = new char[nXY+1];

        for (i = 0;i<nXY;++i) {
            aligned_ref_sequence[i] = XX[i] == '-' ? '-' : '*';
            output_sequence[i]      = YY[i] == '-' ? '-' : '*';
        }
        aligned_ref_sequence[i] = 0;
        output_sequence[i]      = 0;


        //         memcpy(aligned_ref_sequence, XX, nXY+1);
        //         memcpy(output_sequence, YY, nXY+1);
    }

    free(X);
    free(secX);

    free(Y);
    free(secY);

    freeBlock(&XX);
    freeBlock(&YY);

    return(Error);
}
