#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "help.hxx"

char *Vglstr = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ\0"};

int GBS_read_hash(long dummy, char *suchstr)
{
    char *cp;

    dummy = dummy;
    cp = strpbrk(Vglstr, suchstr);
    return (int)cp - (int)Vglstr;
}

long GBS_create_hash(int nc)
{
    nc=nc;
    return NULL;
}

void GBS_write_hash(long ptr, STR names, int idx)
{
    ;
}
