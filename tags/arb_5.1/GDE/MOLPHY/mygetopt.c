/*
 * mygetopt.c   Adachi, J.   1994.06.30
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

/* #include <stdio.h> */
/* #include <string.h> */
#include "molphy.h"
 
/* #define SW_CHAR '-'   switch character, '-'(UNIX) or '/'(DOS) */
int Optindex = 1; /* index of next argument */
int Opterror = 1; /* error message level */
char *Optargp;    /* pointer to argument of current option */
static char *Optlocp = NULL; /* next option char's location */

int	mygetopt(argc, argv, optstring)
int argc;
char **argv;
char *optstring;
{
	unsigned char ch;
	char *optp;
 
	if (argc > Optindex) {
		if (Optlocp == NULL) {
			if ((Optlocp = argv[Optindex]) == NULL || *(Optlocp++) != SW_CHAR)
				goto lgetopteof;
			if (*Optlocp == SW_CHAR) {
				Optindex++;
				goto lgetopteof;
			}
		}
		if ((ch = *(Optlocp++)) == 0) {
			Optindex++;
			goto lgetopteof;
		}
		if (ch == ':' || (optp = strchr(optstring, ch)) == NULL)  
			goto lgetopterr;
		if (':' == *(++optp)) {
			Optindex++;
			if (0 == *Optlocp) {
				if (argc <= Optindex)
					goto lgetopterr;
				Optlocp = argv[Optindex++];
			}
			Optargp = Optlocp;
			Optlocp = NULL;
		} else {
			if (0 == *Optlocp) {
				Optindex++;
				Optlocp = NULL;
			}
			Optargp = NULL;
		}
		return ch;
	}

lgetopteof:
	Optargp = Optlocp = NULL;  
	return EOF;
 
lgetopterr:
	Optargp = NULL;
	return ('?');
}
