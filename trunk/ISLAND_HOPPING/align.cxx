// =============================================================
/*                                                               */
//   File      : align.c
//   Purpose   :
/*                                                               */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                               */
// =============================================================

#include "mem.h"
#include "trnsprob.h"

#define EXTERN
#include "i-hopper.h"

#include <stdio.h>
#include <math.h>

#define MSIZE          512
#define ESIZE          36

//============================================================================

#define MINDIST        0.001
#define MAXDIST        1.000

#define MAXGAP         16
#define NOWHERE        (MAXGAP+1)

#define MINLENGTH      5

typedef struct score {
    double score;
    int up,down;
} Score;


typedef struct fragment {
    int beginX,beginY,length;
    struct fragment *next;
} Fragment;

typedef struct island {
    double score,upperScore,lowerScore;
    int beginX,beginY,endX,endY;
    int hasUpper,hasLower;
    struct island *next,*nextBeginIndex,*nextEndIndex,*nextSelected,*upper,*lower;
    struct fragment *fragments;
} Island;

static double **GP,**GS;

// #define TEST

#ifdef TEST

#define TELEN 193
static double TE[TELEN][TELEN];

static int te=TRUE;

#endif

static Score **U;

static double Thres,LogThres,Supp,GapA,GapB,GapC,expectedScore,**GE;

static Island *Z,**ZB,**ZE;

//============================================================================

static void initScore(double ***pP,double ***pS) {

    *pP=newDoubleMatrix(N,N);
    *pS=newDoubleMatrix(N1,N1);

}

//............................................................................

static void uninitScore(double ***pP,double ***pS) {

    freeMatrix(pS);
    freeMatrix(pP);

}

//............................................................................

static void updateScore(
                 double **P,double **S,
                 double F[N],double X[6],double dist
                 ) {
    int i,j;
    double ***SS,s,smin,smax;

    P[T][T]=F[T]*F[T]; P[T][C]=F[T]*F[C]; P[T][A]=F[T]*F[A]; P[T][G]=F[T]*F[G];
    P[C][T]=F[C]*F[T]; P[C][C]=F[C]*F[C]; P[C][A]=F[C]*F[A]; P[C][G]=F[C]*F[G];
    P[A][T]=F[A]*F[T]; P[A][C]=F[A]*F[C]; P[A][A]=F[A]*F[A]; P[A][G]=F[A]*F[G];
    P[G][T]=F[G]*F[T]; P[G][C]=F[G]*F[C]; P[G][A]=F[G]*F[A]; P[G][G]=F[G]*F[G];

    initTrnsprob(&SS);
    updateTrnsprob(SS,F,X,REV);
    getTrnsprob(S,SS,dist);
    uninitTrnsprob(&SS);
    S[T][T]/=F[T]; S[T][C]/=F[C]; S[T][A]/=F[A]; S[T][G]/=F[G];
    S[C][T]/=F[T]; S[C][C]/=F[C]; S[C][A]/=F[A]; S[C][G]/=F[G];
    S[A][T]/=F[T]; S[A][C]/=F[C]; S[A][A]/=F[A]; S[A][G]/=F[G];
    S[G][T]/=F[T]; S[G][C]/=F[C]; S[G][A]/=F[A]; S[G][G]/=F[G];

    for(i=0;i<N;i++) for(j=0;j<N;j++) S[i][j]=log(S[i][j]);

    S[T][N]=F[T]*S[T][T]+F[C]*S[T][C]+F[A]*S[T][A]+F[G]*S[T][G];
    S[C][N]=F[T]*S[C][T]+F[C]*S[C][C]+F[A]*S[C][A]+F[G]*S[C][G];
    S[A][N]=F[T]*S[A][T]+F[C]*S[A][C]+F[A]*S[A][A]+F[G]*S[A][G];
    S[G][N]=F[T]*S[G][T]+F[C]*S[G][C]+F[A]*S[G][A]+F[G]*S[G][G];
    S[N][T]=F[T]*S[T][T]+F[C]*S[C][T]+F[A]*S[A][T]+F[G]*S[G][T];
    S[N][C]=F[T]*S[T][C]+F[C]*S[C][C]+F[A]*S[A][C]+F[G]*S[G][C];
    S[N][A]=F[T]*S[T][A]+F[C]*S[C][A]+F[A]*S[A][A]+F[G]*S[G][A];
    S[N][G]=F[T]*S[T][G]+F[C]*S[C][G]+F[A]*S[A][G]+F[G]*S[G][G];
    S[N][N]=F[T]*S[T][N]+F[C]*S[C][N]+F[A]*S[A][N]+F[G]*S[G][N];

    smin=0.; smax=0.;
    for(i=0;i<N;i++)
        for(j=0;j<N;j++) {
            s=S[i][j];
            if(s<smin) smin=s; else
                if(s>smax) smax=s;
        }

}

//============================================================================

static void initEntropy(double ***EE) {
    *EE=newDoubleMatrix(ESIZE+1,MSIZE);
}

//............................................................................

static void uninitEntropy(double ***EE) {
    freeMatrix(EE);
}

//............................................................................

static double getEntropy(double **E,double m,int l) {
    int k,ia,ib,ja,jb;
    double *M,mmin,idm,la,lb,ma,mb;

    M=E[0]; ma=0.5*(M[1]-M[0]); mmin=M[0]-ma; idm=0.5/ma;

    k=floor((m-mmin)*idm);
    if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;

    if(k==0)       {ja=k;   jb=k+1;} else
        if(k==MSIZE-1) {ja=k-1; jb=k;  } else
            if(m>M[k])     {ja=k;   jb=k+1;} else
            {ja=k-1; jb=k;  }

    ma=M[ja]; mb=M[jb];

    if(l<=16) {
        return(
               ((mb-m)*E[l][ja]+(m-ma)*E[l][jb])/(mb-ma)
               );
    }
    else {
        if(l<=32) {
            if(l<=18) {la=16; lb=18; ia=16; ib=17;} else
                if(l<=20) {la=18; lb=20; ia=17; ib=18;} else
                    if(l<=22) {la=20; lb=22; ia=18; ib=19;} else
                        if(l<=24) {la=22; lb=24; ia=19; ib=20;} else
                            if(l<=26) {la=24; lb=26; ia=20; ib=21;} else
                                if(l<=28) {la=26; lb=28; ia=21; ib=22;} else
                                    if(l<=30) {la=28; lb=30; ia=22; ib=23;} else
                                        if(l<=32) {la=30; lb=32; ia=23; ib=24;}
        } else
            if(l<=64) {
                if(l<=36) {la=32; lb=36; ia=24; ib=25;} else
                    if(l<=40) {la=36; lb=40; ia=25; ib=26;} else
                        if(l<=44) {la=40; lb=44; ia=26; ib=27;} else
                            if(l<=48) {la=44; lb=48; ia=27; ib=28;} else
                                if(l<=52) {la=48; lb=52; ia=28; ib=29;} else
                                    if(l<=56) {la=52; lb=56; ia=29; ib=30;} else
                                        if(l<=60) {la=56; lb=60; ia=30; ib=31;} else
                                            if(l<=64) {la=60; lb=64; ia=31; ib=32;}
            }
            else {
                if(l<= 128) {la=  64; lb= 128; ia=32; ib=33;} else
                    if(l<= 256) {la= 128; lb= 256; ia=33; ib=34;} else
                        if(l<= 512) {la= 256; lb= 512; ia=34; ib=35;} else
                        {la= 512; lb=1024; ia=35; ib=36;}
            }
        return(
               (
                +(lb-l)*((mb-m)*E[ia][ja]+(m-ma)*E[ia][jb])
                +(l-la)*((mb-m)*E[ib][ja]+(m-ma)*E[ib][jb])
                )
               /((lb-la)*(mb-ma))
               );
    }

}

//............................................................................

static void updateEntropy(double **P,double **S,double **E) {
    int i,j,k,l,ll,lll;
    double *M,m,dm,idm,mmin,mmax;

    mmin=0.; mmax=0.;
    for(i=0;i<N;i++)
        for(j=0;j<N;j++) {m=S[i][j]; if(m<mmin) mmin=m; else if(m>mmax) mmax=m;}
    dm=(mmax-mmin)/MSIZE;
    idm=1./dm;

    M=E[0];
    for(i=0,m=mmin+0.5*dm;i<MSIZE;i++,m+=dm) M[i]=m;

    for(i=0;i<MSIZE;i++) E[1][i]=0.;
    for(i=0;i<N;i++)
        for(j=0;j<N;j++) {
            k=floor((S[i][j]-mmin)*idm);
            if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
            E[1][k]+=P[i][j];
        }

    for(k=0;k<MSIZE;k++) E[2][k]=0.;
    for(i=0;i<MSIZE;i++)
        for(j=0;j<MSIZE;j++) {
            m=(M[i]+M[j])/2;
            k=floor((m-mmin)*idm);
            if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
            E[2][k]+=E[1][i]*E[1][j];
        }

    for(ll=2,lll=1;ll<=8;ll++,lll++) {
        l=ll+lll;
        for(k=0;k<MSIZE;k++) E[l][k]=0.;
        for(i=0;i<MSIZE;i++)
            for(j=0;j<MSIZE;j++) {
                m=(ll*M[i]+lll*M[j])/l;
                k=floor((m-mmin)*idm);
                if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
                E[l][k]+=E[ll][i]*E[lll][j];
            }
        l=ll+ll;
        for(k=0;k<MSIZE;k++) E[l][k]=0.;
        for(i=0;i<MSIZE;i++)
            for(j=0;j<MSIZE;j++) {
                m=(M[i]+M[j])/2;
                k=floor((m-mmin)*idm);
                if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
                E[l][k]+=E[ll][i]*E[ll][j];
            }
    }

    for(l=17,ll=l-8;l<=24;l++,ll++) {
        for(k=0;k<MSIZE;k++) E[l][k]=0.;
        for(i=0;i<MSIZE;i++)
            for(j=0;j<MSIZE;j++) {
                m=(M[i]+M[j])/2;
                k=floor((m-mmin)*idm);
                if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
                E[l][k]+=E[ll][i]*E[ll][j];
            }
    }

    for(l=25,ll=l-8;l<=32;l++,ll++) {
        for(k=0;k<MSIZE;k++) E[l][k]=0.;
        for(i=0;i<MSIZE;i++)
            for(j=0;j<MSIZE;j++) {
                m=(M[i]+M[j])/2;
                k=floor((m-mmin)*idm);
                if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
                E[l][k]+=E[ll][i]*E[ll][j];
            }
    }

    for(l=33,ll=l-1;l<=36;l++,ll++) {
        for(k=0;k<MSIZE;k++) E[l][k]=0.;
        for(i=0;i<MSIZE;i++)
            for(j=0;j<MSIZE;j++) {
                m=(M[i]+M[j])/2;
                k=floor((m-mmin)*idm);
                if(k>=MSIZE) k=MSIZE-1; else if(k<0) k=0;
                E[l][k]+=E[ll][i]*E[ll][j];
            }
    }

    for(l=1;l<=ESIZE;l++) {m=0.; for(k=MSIZE-1;k>=0;k--) m=E[l][k]+=m;}

    for(i=1;i<=ESIZE;i++)
        for(j=0;j<MSIZE;j++) E[i][j]=(E[i][j]>REAL_MIN)?log(E[i][j]):LOG_REAL_MIN;

    {
        FILE *fp;
        int nbp;
        double m0,dm2;

        nbp=22;
        fp=fopen("distrib.txt","wt");
        fprintf(fp,"\n(* score per basepair distribution for gap-less random path 1=Smin 100=Smax *)\nListPlot3D[({");
        m0=M[0]; dm2=(M[MSIZE-1]-M[0])/99.;
        for(i=1;i<=nbp;i++) {
            fprintf(fp,"\n{%f",exp(getEntropy(E,m0,i)));
            for(j=1;j<=99;j++) fprintf(fp,",%f",exp(getEntropy(E,m0+dm2*j,i)));
            fprintf(fp,"}"); if(i<nbp) fprintf(fp,",");
        }
        fprintf(fp,"}),PlotRange->{{1,100},{1,%d},{0,1}},ViewPoint->{1.5,1.1,0.8}]\n",nbp);
        fclose(fp);

    }

}

//============================================================================

static Island *newIsland(char *X,char *Y,int i,int j,int d) {
    Island *p; Fragment *f,**ff,*L; int k,ii,jj,iii,jjj,l; double s;

    p=(Island*)newBlock(sizeof(Island));

    ii=i; jj=j; l=0; s=0.;

    if(d>0) {
        ff=&L;
        for(;;) {
            s+=GS[(int)X[ii]][(int)Y[jj]];
            if(++l==1) {iii=ii; jjj=jj;}
            k=U[ii][jj].up;
            if(k) {
                f=(Fragment*)newBlock(sizeof(Fragment));
                f->beginX=iii; f->beginY=jjj; f->length=l;
                l=0; *ff=f; ff=&f->next;
            }
            if(k==NOWHERE) break;
            ii++; jj++; if(k>=0) ii+=k; else jj-=k;
        }
        *ff=NULL;
        p->beginX= i; p->beginY= j;
        p->endX  =ii; p->endY  =jj;
    }
    else {
        L=NULL;
        for(;;) {
            s+=GS[(int)X[ii]][(int)Y[jj]];
            if(++l==1) {iii=ii; jjj=jj;}
            k=U[ii][jj].down;
            if(k) {
                f=(Fragment*)newBlock(sizeof(Fragment));
                f->beginX=iii-l+1; f->beginY=jjj-l+1; f->length=l;
                l=0; f->next=L; L=f;
            }
            if(k==NOWHERE) break;
            ii--; jj--; if(k>=0) ii-=k; else jj+=k;
        }
        p->beginX=ii; p->beginY=jj;
        p->endX  = i; p->endY  = j;
    }

    p->fragments=L;

    p->score=s+GapC*expectedScore;

    return(p);
}

//............................................................................

static void freeIsland(Island **pp) {
    Island *p; Fragment *q;

    p=*pp;

    while(p->fragments) {q=p->fragments; p->fragments=q->next; freeBlock(&q);}

    freeBlock(pp);

}

//............................................................................

static void registerIsland(Island *f) {
    Island **pli;

    f->next=Z; Z=f;

    pli=&ZB[f->beginX+f->beginY];
    f->nextBeginIndex=*pli; *pli=f;

    pli=&ZE[f->endX+f->endY];
    f->nextEndIndex=*pli; *pli=f;

}

//............................................................................

static Island *selectUpperIslands(Island *f,int nX,int nY,int *incomplete) {
    int i,j; Island *l,*g;

    l=NULL;

    for(i=f->endX+f->endY+2,j=nX+nY-2;i<=j;i++)
        for(g=ZB[i];g;g=g->nextBeginIndex)
            if(g->beginX>f->endX&&g->beginY>f->endY) {
                if(!g->hasUpper) {*incomplete=TRUE; return(NULL);}
                g->nextSelected=l; l=g;
            }

    *incomplete=FALSE;

    return(l);
}

//............................................................................

static Island *selectLowerIslands(Island *f,int *incomplete) {
    int i; Island *l,*g;

    l=NULL;

    for(i=f->beginX+f->beginY-2;i>=0;i--)
        for(g=ZE[i];g;g=g->nextEndIndex)
            if(g->endX<f->beginX&&g->endY<f->beginY) {
                if(!g->hasLower) {*incomplete=TRUE; return(NULL);}
                g->nextSelected=l; l=g;
            }

    *incomplete=FALSE;

    return(l);
}

//............................................................................

static int areEqual(Island *a,Island *b) {
    Fragment *fa,*fb;

    if( a->beginX != b->beginX ) return(FALSE);
    if( a->beginY != b->beginY ) return(FALSE);
    if( a->endX   != b->endX   ) return(FALSE);
    if( a->endY   != b->endY   ) return(FALSE);

    fa=a->fragments; fb=b->fragments;
    while(fa&&fb) {
        if( fa->beginX != fb->beginX ) return(FALSE);
        if( fa->beginY != fb->beginY ) return(FALSE);
        if( fa->length != fb->length ) return(FALSE);
        fa=fa->next; fb=fb->next;
    }
    if(fa||fb) return(FALSE);

    return(TRUE);
}

//............................................................................

static int isUnique(Island *f) {
    Island *v;

    for(v=ZB[f->beginX+f->beginY];v;v=v->nextBeginIndex)
        if(areEqual(f,v)) return(FALSE);

    return(TRUE);
}

//............................................................................

static int isSignificant(Island *f) {
    int l;
    Fragment *q;

    l=0; for(q=f->fragments;q;q=q->next) l+=q->length;

    if(l<MINLENGTH) return(FALSE);

    if(getEntropy(GE,f->score/l,l)<=LogThres) return(TRUE);

    return(FALSE);
}

//............................................................................

static int I,J,K;

//....

static void drawLowerPath(Island *f,int nX,char *X,char *XX,int nY,char *Y,char *YY) {
    int k;
    Fragment *q;

    if(f->lower) drawLowerPath(f->lower,nX,X,XX,nY,Y,YY);

    for(q=f->fragments;q;q=q->next) {
        while(I<q->beginX) {XX[K]=decodeBase(X[I++]); YY[K]='-';                K++;}
        while(J<q->beginY) {XX[K]='-';                YY[K]=decodeBase(Y[J++]); K++;}
        for(k=0;k<q->length;k++)
        {XX[K]=decodeBase(X[I++]); YY[K]=decodeBase(Y[J++]); K++;}
    }

}

//....

static void drawPath(Island *f,int nX,char *X,char *XX,int nY,char *Y,char *YY) {
    int k;
    Island *p;
    Fragment *q;

    I=0; J=0; K=0;

    if(f->lower) drawLowerPath(f->lower,nX,X,XX,nY,Y,YY);

    for(p=f;p;p=p->upper)
        for(q=p->fragments;q;q=q->next) {
            while(I<q->beginX) {XX[K]=decodeBase(X[I++]); YY[K]='-';                K++;}
            while(J<q->beginY) {XX[K]='-';                YY[K]=decodeBase(Y[J++]); K++;}
            for(k=0;k<q->length;k++)
            {XX[K]=decodeBase(X[I++]); YY[K]=decodeBase(Y[J++]); K++;}
        }

    while(I<nX) {XX[K]=decodeBase(X[I++]); YY[K]='-';                K++;}
    while(J<nY) {XX[K]='-';                YY[K]=decodeBase(Y[J++]); K++;}

    XX[K]='\0';
    YY[K]='\0';

}

//............................................................................

static void drawIsland(Island *f) {
    int i,j,k;
    Fragment *q;

#ifdef TEST
    double score;
    score=f->score;
#endif

    for(q=f->fragments;q;q=q->next) {
        for(i=q->beginX,j=q->beginY,k=0;k<q->length;k++,i++,j++) {

#ifdef TEST
            if(
               i>=0&&i<TELEN&&j>=0&&j<TELEN // &&score>TE[i][j]
               ) {
                TE[i][j]=score;
            }
#endif

        }
    }

}

//============================================================================

static double secS(int i,int j,char X[],int secX[],char Y[],int secY[]) {

    if(secX[i]||secY[j]) {
        if(secX[i]&&secY[j]) return(GS[(int)X[i]][(int)Y[j]]-Supp*expectedScore);
        return(-1.e34);
    }

    return(GS[(int)X[i]][(int)Y[j]]);
}

//============================================================================

static void AlignTwo(
              int nX,char X[],int secX[],char XX[],int nY,char Y[],int secY[],char YY[]
              ) {
    int r,changed,i,j,k,ii,jj,startx,stopx,starty,stopy;
    double s,ss;
    Island *z,*zz,*zzz,*best;

    //OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

    Z=NULL; for(i=nX+nY-2;i>=0;i--) {ZB[i]=NULL; ZE[i]=NULL;}

    startx=0; stopx=nX-1;
    for(i=startx,ii=i-1;i<=stopx;i++,ii++) {
        starty=0; stopy=nY-1;
        for(j=starty,jj=j-1;j<=stopy;j++,jj++) {
            s=0.; r=NOWHERE;
            if(i>startx&&j>starty) {
                ss=U[ii][jj].score; if(ss>s) {s=ss; r=0;}
                for(k=1;k<=MAXGAP&&ii-k>=0;k++) {
                    ss=U[ii-k][jj].score+(GapA+k*GapB)*expectedScore; if(ss>s) {s=ss; r= k;}
                }
                for(k=1;k<=MAXGAP&&jj-k>=0;k++) {
                    ss=U[ii][jj-k].score+(GapA+k*GapB)*expectedScore; if(ss>s) {s=ss; r=-k;}
                }
            }
            s+=secS(i,j,X,secX,Y,secY); if(s<0.) {s=0.; r=NOWHERE;}
            U[i][j].score=s;
            U[i][j].down=r;
        }
    }

    startx=0; stopx=nX-1;
    for(i=startx;i<=stopx;i++) {
        starty=0; stopy=nY-1;
        for(j=starty;j<=stopy;j++) {
            ii=i; jj=j; s=U[ii][jj].score; U[ii][jj].up=NOWHERE;
            for(;;) {
                k=U[ii][jj].down; if(k==NOWHERE) break;
                ii--; jj--; if(k>=0) ii-=k; else jj+=k;
                if(U[ii][jj].score>s) break;
                U[ii][jj].score=s; U[ii][jj].up=k;
            }
        }
    }

    startx=0; stopx=nX-1;
    for(i=startx;i<=stopx;i++) {
        starty=0; stopy=nY-1;
        for(j=starty;j<=stopy;j++) {
            if(U[i][j].down!=NOWHERE) continue;
            if(U[i][j].up==NOWHERE) continue;
            z=newIsland(X,Y,i,j,1);
            if(isUnique(z)&&isSignificant(z)) registerIsland(z);
            else                              freeIsland(&z);
        }
    }

    //----

    startx=nX-1; stopx=0;
    for(i=startx,ii=i+1;i>=stopx;i--,ii--) {
        starty=nY-1; stopy=0;
        for(j=starty,jj=j+1;j>=stopy;j--,jj--) {
            s=0.; r=NOWHERE;
            if(i<startx&&j<starty) {
                ss=U[ii][jj].score; if(ss>s) {s=ss; r=0;}
                for(k=1;k<=MAXGAP&&ii+k<nX;k++) {
                    ss=U[ii+k][jj].score+(GapA+k*GapB)*expectedScore; if(ss>s) {s=ss; r= k;}
                }
                for(k=1;k<=MAXGAP&&jj+k<nY;k++) {
                    ss=U[ii][jj+k].score+(GapA+k*GapB)*expectedScore; if(ss>s) {s=ss; r=-k;}
                }
            }
            s+=secS(i,j,X,secX,Y,secY); if(s<0.) {s=0.; r=NOWHERE;}
            U[i][j].score=s;
            U[i][j].up=r;
        }
    }

    startx=nX-1; stopx=0;
    for(i=startx;i>=stopx;i--) {
        starty=nY-1; stopy=0;
        for(j=starty;j>=stopy;j--) {
            ii=i; jj=j; s=U[ii][jj].score; U[ii][jj].down=NOWHERE;
            for(;;) {
                k=U[ii][jj].up; if(k==NOWHERE) break;
                ii++; jj++; if(k>=0) ii+=k; else jj-=k;
                if(U[ii][jj].score>s) break;
                U[ii][jj].score=s; U[ii][jj].down=k;
            }
        }
    }

    startx=nX-1; stopx=0;
    for(i=startx;i>=stopx;i--) {
        starty=nY-1; stopy=0;
        for(j=starty;j>=stopy;j--) {
            if(U[i][j].up!=NOWHERE) continue;
            if(U[i][j].down==NOWHERE) continue;
            z=newIsland(X,Y,i,j,-1);
            if(isUnique(z)&&isSignificant(z)) registerIsland(z);
            else                              freeIsland(&z);
        }
    }

    //*****************

    for(z=Z;z;z=z->next) {z->hasUpper=FALSE; z->upper=NULL; z->upperScore=0.;}
    do { changed=FALSE;
        for(z=Z;z;z=z->next) {
            if(z->hasUpper) continue;
            zz=selectUpperIslands(z,nX,nY,&i); if(i) continue;
            if(zz) {
                s=0.; best=NULL;
                for(zzz=zz;zzz;zzz=zzz->nextSelected) {
                    if(zzz->upperScore+zzz->score>s) {s=zzz->upperScore+zzz->score; best=zzz;}
                }
                if(best) {z->upper=best; z->upperScore=s;}
            }
            z->hasUpper=TRUE;
            changed=TRUE;
        }
    } while(changed);

    for(z=Z;z;z=z->next) {z->hasLower=FALSE; z->lower=NULL; z->lowerScore=0.;}
    do { changed=FALSE;
        for(z=Z;z;z=z->next) {
            if(z->hasLower) continue;
            zz=selectLowerIslands(z,&i); if(i) continue;
            if(zz) {
                s=0.; best=NULL;
                for(zzz=zz;zzz;zzz=zzz->nextSelected) {
                    if(zzz->lowerScore+zzz->score>s) {s=zzz->lowerScore+zzz->score; best=zzz;}
                }
                if(best) {z->lower=best; z->lowerScore=s;}
            }
            z->hasLower=TRUE;
            changed=TRUE;
        }
    } while(changed);

    //*****************

    s=0.; best=NULL;
    for(z=Z;z;z=z->next) {
        ss=z->score+z->upperScore+z->lowerScore;
        if(ss>s) {best=z; s=ss;}
    }

#ifdef TEST
    for(i=0;i<TELEN;i++) for(j=0;j<TELEN;j++) TE[i][j]=0.;
#endif

    if(best) { drawPath(best,nX,X,XX,nY,Y,YY);
        drawIsland(best);
        for(z=best->upper;z;z=z->upper) drawIsland(z);
        for(z=best->lower;z;z=z->lower) drawIsland(z);
    }

    //OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

#ifdef TEST

    if(te) {FILE *fp; te=FALSE;

        fp=fopen("subst.txt","wt");
        fprintf(fp,"\n(* substitution matrix *)\nListDensityPlot[{");
        for(i=0;i<N;i++) {
            fprintf(fp,"\n{%f",GS[i][0]);
            for(j=1;j<N;j++) fprintf(fp,",%f",GS[i][j]);
            fprintf(fp,"}"); if(i<N-1) fprintf(fp,",");
        }
        fprintf(fp,"}]\n");
        fclose(fp);

        fp=fopen("correl.txt","w");
        fprintf(fp,"\n(* correlation matrix *)\n{");
        for(i=0;i<TELEN&&i<nX;i++) {
            fprintf(fp,"\n{");
            fprintf(fp,"%f",secS(i,0,X,secX,Y,secY));
            for(j=1;j<TELEN&&j<nY;j++) fprintf(fp,",%f",secS(i,j,X,secX,Y,secY));
            if(i==TELEN-1||i==nX-1)fprintf(fp,"}"); else fprintf(fp,"},");
        }
        fprintf(fp,"}//ListDensityPlot\n");
        fclose(fp);

        fp=fopen("path.txt","w");
        fprintf(fp,"\n(* pairwise alignment *)\n{");
        for(i=0;i<TELEN&&i<nX;i++) {
            fprintf(fp,"\n{");
            fprintf(fp,"%f\n",TE[i][0]);
            for(j=1;j<TELEN&&j<nY;j++) fprintf(fp,",%f",TE[i][j]);
            if(i==TELEN-1||i==nX-1)fprintf(fp,"}"); else fprintf(fp,"},");
        }
        fprintf(fp,"}//ListDensityPlot\n");
        fclose(fp);

        for(i=0;i<TELEN;i++) for(j=0;j<TELEN;j++) TE[i][j]=0.;
        for(z=Z;z;z=z->next) drawIsland(z);

        fp=fopen("islands.txt","w");
        fprintf(fp,"\n(* smith-waterman-islands *)\n{");
        for(i=0;i<TELEN&&i<nX;i++) {
            fprintf(fp,"\n{");
            fprintf(fp,"%f\n",TE[i][0]);
            for(j=1;j<TELEN&&j<nY;j++) fprintf(fp,",%f",TE[i][j]);
            if(i==TELEN-1||i==nX-1)fprintf(fp,"}"); else fprintf(fp,"},");
        }
        fprintf(fp,"}//ListDensityPlot\n");
        fclose(fp);

    }

#endif

    //OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

    while(Z) {z=Z; Z=Z->next; freeIsland(&z);}

}

//............................................................................

void Align(
           int nX,char X[],int secX[],char **XX,int nY,char Y[],int secY[],char **YY,
           int freqs,double fT,double fC,double fA,double fG,
           double rTC,double rTA,double rTG,double rCA,double rCG,double rAG,
           double dist,double supp,double gapA,double gapB,double gapC,double thres
           ) {
    double F[N],R[6];
    char *s;
    int i,j,maxlen;

    *XX = (char*)newBlock((nX+nY+1)*sizeof(char));
    *YY = (char*)newBlock((nX+nY+1)*sizeof(char));

    Supp=supp;
    GapA=gapA;
    GapB=gapB;
    GapC=gapC;
    Thres=thres;

    if(dist>MAXDIST||dist<MINDIST) {Error="Bad argument"; return;}

    if(GapA<0.||GapB<0.) {Error="Bad argument"; return;}

    if(Thres>1.||Thres<=0.) {Error="Bad argument"; return;}
    LogThres=log(Thres);

    if(freqs) {
        if(fT<=0.||fC<=0.||fA<=0.||fG<=0.) {Error="Bad argument"; return;}
    }
    else {
        fT=0.; fC=0.; fA=0.; fG=0.;
        for(s=X;*s;s++) {
            switch(*s) {
                case 'T': fT++; break; case 'C': fC++; break;
                case 'A': fA++; break; case 'G': fG++; break;
                default: fT+=0.25; fC+=0.25; fA+=0.25; fG+=0.25;
            }
        }
        for(s=Y;*s;s++) {
            switch(*s) {
                case 'T': fT++; break; case 'C': fC++; break;
                case 'A': fA++; break; case 'G': fG++; break;
                default: fT+=0.25; fC+=0.25; fA+=0.25; fG+=0.25;
            }
        }
    }

    if(rTC<=0.||rTA<=0.||rTG<=0.||rCA<=0.||rCG<=0.||rAG<=0.) {Error="Bad argument"; return;}

    normalizeBaseFreqs(F,fT,fC,fA,fG);
    normalizeRateParams(R,rTC,rTA,rTG,rCA,rCG,rAG);

    maxlen=nX>nY?nX:nY;

    for(i=0;i<nX;i++) X[i]=encodeBase(X[i]);
    for(i=0;i<nY;i++) Y[i]=encodeBase(Y[i]);

    U=(Score **)newMatrix(maxlen,maxlen,sizeof(Score));
    ZB=(Island **)newVector(2*maxlen,sizeof(Island *));
    ZE=(Island **)newVector(2*maxlen,sizeof(Island *));

    initScore(&GP,&GS);
    initEntropy(&GE);

    updateScore(GP,GS,F,R,dist);
    updateEntropy(GP,GS,GE);

    expectedScore=0.;
    for(i=0;i<N;i++) for(j=0;j<N;j++) expectedScore+=GP[i][j]*GS[i][j];

    AlignTwo(nX,X,secX,*XX,nY,Y,secY,*YY);

    uninitEntropy(&GE);
    uninitScore(&GP,&GS);

    freeBlock(&ZE);
    freeBlock(&ZB);
    freeMatrix(&U);

    for(i=0;i<nX;i++) X[i]=decodeBase(X[i]);
    for(i=0;i<nY;i++) Y[i]=decodeBase(Y[i]);

}
