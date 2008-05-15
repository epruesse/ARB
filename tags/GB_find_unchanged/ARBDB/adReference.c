#include <stdlib.h>
#include "adlocal.h"
#include "string.h"


/* this module is currently not used */


typedef struct S_Reffile
{
    char *quickpath;
    char *user;
    struct S_Reffile *next;

} *Reffile;

static char *referencePath(GB_CSTR arbpath)
{
    char *copy = GB_STRDUP(arbpath);
    char *ext =
}

static Reffile addReference(Reffile ref, GB_CSTR quickpath, GB_CSTR user)
{

}
static Reffile removeReference(Reffile ref, GB_CSTR quickpath)
{
    
}

static Reffile readReferences(GB_CSTR refpath)
{
    
}
static void writeReferences(Reffile ref, GB_CSTR refpath)
{
    
}
static void freeReferences(Reffile ref)
{
    while (ref)
    {
	Reffile next = ref->next;

	free(ref->quickpath);
	free(ref->user);
	free(ref);

	ref = next;
    }
}

int gb_announceReference(GB_CSTR arbpath, GB_CSTR quickpath, GB_CSTR user)
{
    char *refpath = referencePath(arbpath);
    Reffile ref = readReferences(refpath);
    
    if (!ref || removeReference(ref,quickpath)==NULL) 
    {
	ref = addReference(ref,quickpath,user);
	writeReferences(ref,refpath);
    }

    freeReferences(ref);
    free(refpath);
    return 0;
}

int gb_deleteReference(GB_CSTR arbpath, GB_CSTR quickpath)
{
    char *refpath = referencePath(arbpath);
    Reffile ref = readReferences(refpath);

    ...

    freeReferences(ref);
    free(refpath);
    return 0;
}


