
#include "island_hopping.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define EXTERN
extern "C" {
#include "i-hopper.h"
}

GB_ERROR IslandHopping::do_align() {

 int nX;
 char *X;
 int *secX;

 int nY;
 char *Y;
 int *secY;

 char *XX=NULL;

 int i,j,k;

 Error=0;

 nX=0; nY=0;
 for(i=0;i<alignment_length;i++) {
  if(ref_sequence[i]!='-') nX++;
  if(toAlign_sequence[i]!='-') nY++;
 }

 X=(char *)malloc((nX+1)*sizeof(char));
 secX=(int *)malloc((nX)*sizeof(int));

 Y=(char *)malloc((nY+1)*sizeof(char));
 secY=(int *)malloc((nY)*sizeof(int));

 j=0; k=0;
 for(i=0;i<alignment_length;i++) {
  if(ref_sequence[i]!='-') X[j]=ref_sequence[i]; secX[j]=0;
  if(toAlign_sequence[i]!='-') Y[k]=toAlign_sequence[i]; secY[k]=0;
  j++; k++;
 }
 X[j]='\0'; Y[k]='\0';

 if(output_sequence) {delete output_sequence; output_sequence=0;}

 Align(
  nX,X,secX,&XX,nY,Y,secY,&output_sequence, 
 freqs,fT,fC,fA,fG,
  rates,rTC,rTA,rTG,rCA,rCG,rAG,
  dist,supp,gap,thres
 ); 

 if(!Error) { FILE *fp; int i,nXX,nYY; char *YY;
   YY=output_sequence;
   nXX=strlen(XX);
   nYY=strlen(YY);
   fp=fopen("alignment.txt","w");
   for(i=0;i<nXX;i++) fprintf(fp,"%c",XX[i]); fprintf(fp,"\n");
   for(i=0;i<nYY;i++) fprintf(fp,"%c",YY[i]); fprintf(fp,"\n");
   fclose(fp);
  }

 free(X);
 free(secX);

 free(Y);
 free(secY);

 free(XX);

 return(Error);
}  
