#include <stdio.h>
#include <stdlib.h>

#define M memory_M
#define D memory_D
#define A memory_A

#define MINSIZE 72 /* >= sizeof(Node) */

void *M=NULL,*D=NULL;
size_t A=0;

/* ========================================================================== */

void undispose(void) { void *v; size_t s;
 while(D) {
     v=D; D=((void **)v)[0];
     s=(size_t)(((void **)v)[2]);
     A-=((s<=MINSIZE)?MINSIZE:s)+3*sizeof(void *);
     free(v);
 }
}

/* ========================================================================== */

void clearUp(void) { void *v;
 while(D) {v=D; D=((void **)v)[0]; free(v);}
 while(M) {v=M; M=((void **)v)[0]; free(v);}
}

/* ========================================================================== */

void outOfMemory(void) {
    fprintf(stdout,"\n!!! Out of Memory\n");
    clearUp();
    exit(EXIT_FAILURE);
}

/* ========================================================================== */

size_t allocated(void) {
    return(A);
}


/* ========================================================================== */


void *newBlock(size_t s) {
    void *v; size_t S;

    if(D&&s<=MINSIZE) {
        v=D; D=((void **)v)[0];
    } else {
        S=((s<=MINSIZE)?MINSIZE:s)+3*sizeof(void *);
        v=malloc(S);
        if(v==NULL) outOfMemory();
        A+=S;
    }

    if(M) ((void **)M)[1]=v;
    ((void **)v)[0]=M; M=v;
    ((void **)v)[1]=NULL;
    ((void **)v)[2]=(void *)s;


    return(((void **)v)+3);
}

/*........*/

void freeBlock(void **vv) {
    void *v; size_t s;

    v=(void *)(((void **)(*vv))-3);

    if(((void **)v)[0]) ((void ***)v)[0][1]=((void **)v)[1];
    if(((void **)v)[1]) ((void ***)v)[1][0]=((void **)v)[0];
    else                M=((void **)v)[0];

    s=(size_t)(((void **)v)[2]);

    if(s<=MINSIZE) {
        ((void **)v)[0]=D; D=v;
    } else {
        A-=s+3*sizeof(void *);
        free(v);
    }

    *vv=NULL;
}


void test_mem() {
    void* x[1000];
    int  i;

    for (i = 0; i<1000; ++i) {
        x[i] = malloc(1000+i);
    }
    for (i = 0; i<1000; ++i) {
        free(x[i]);
    }
}


/* ========================================================================== */

void **newMatrix(size_t nrow,size_t ncol,size_t s) {
    size_t i,p;
    void **m;

    m=(void **)newBlock(nrow*sizeof(void *));

    p=ncol*s; for(i=0;i<nrow;i++) m[i]=newBlock(p);

    return(m);
}

/*........*/

void freeMatrix(void ***mm) {
    void **m; size_t i,rows;

    m=*mm;
    rows=((size_t)m[-1])/sizeof(void *);
    for(i=0;i<rows;i++) freeBlock(m+i);
    freeBlock((void **)mm);

}
