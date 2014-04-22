// =============================================================
/*                                                               */
//   File      : mem.c
//   Purpose   :
/*                                                               */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                               */
// =============================================================

#include "mem.h"
#include <stdio.h>
#include <attributes.h>

#define MINSIZE 72 // >= sizeof(Node)

static void *M=NULL,*D=NULL;
static size_t A=0;

// ==========================================================================

static void clearUp(void) { void *v;
    while(D) {v=D; D=((void **)v)[0]; free(v);}
    while(M) {v=M; M=((void **)v)[0]; free(v);}
}

// ==========================================================================

__ATTR__NORETURN static void outOfMemory(void) {
    fprintf(stdout,"\n!!! Out of Memory\n");
    clearUp();
    exit(EXIT_FAILURE);
}

// ==========================================================================


void *newBlock(size_t s) {
    void *v; size_t S;

    if(D&&s<=MINSIZE) {
        v=D; D=((void **)v)[0];
    }
    else {
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

//........

void freeBlock_(void **vv) {
    void *v; size_t s;

    v=(void *)(((void **)(*vv))-3);

    if(((void **)v)[0]) ((void ***)v)[0][1]=((void **)v)[1];
    if(((void **)v)[1]) ((void ***)v)[1][0]=((void **)v)[0];
    else                M=((void **)v)[0];

    s=(size_t)(((void **)v)[2]);

    if(s<=MINSIZE) {
        ((void **)v)[0]=D; D=v;
    }
    else {
        A-=s+3*sizeof(void *);
        free(v);
    }

    *vv=NULL;
}

// ==========================================================================

void **newMatrix(size_t nrow,size_t ncol,size_t s) {
    size_t i,p;
    void **m;

    m=(void **)newBlock(nrow*sizeof(void *));

    p=ncol*s; for(i=0;i<nrow;i++) m[i]=newBlock(p);

    return(m);
}

//........

void freeMatrix_(void ***mm) {
    void **m; size_t i,rows;

    m=*mm;
    rows=((size_t)m[-1])/sizeof(void *);
    for(i=0;i<rows;i++) freeBlock(m+i);
    freeBlock((void **)mm);

}
