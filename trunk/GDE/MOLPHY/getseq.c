/*
 * getseq.c   Adachi, J.   1994.04.28
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"
#include "prot_tml.h"

void
getsize(ifp, maxspc, numsite, commentp)
FILE *ifp;
int *maxspc;
int *numsite;
char **commentp;
{
	char *cp, *np;
	char line[BUFLINE];

	if (fgets(line, BUFLINE, ifp) != NULL) {
		if (sscanf(line, "%d %d", maxspc, numsite) == 2) {
			for (cp = line; isdigit(*cp) || isspace(*cp) /*&& *cp != '\0'*/; cp++)
				;
			*commentp = new_cvector(strlen(cp) + 1);
			if (*cp != '\0') {
				for (np = *commentp; *cp != '\n' && *cp != '\0'; cp++)
					*np++ = *cp;
				*np = '\0';
			} else {
				**commentp = '\0';
			}
			if (Debug)
				fprintf(stdout, "%d OTUs, %d sites,  %s\n\n",
					*maxspc, *numsite, *commentp);
		} else {
			fputs(line, stderr);
			fprintf(stderr, "\nBad format, first line of input file.\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "\nCan't read input file.\n");
		exit(1);
	}
	return;
} /*_ getsize */


/* getid */
void
getid(ifp, identif, sciname, engname, nl, notu)
FILE *ifp;
char **identif;
char **sciname;
char **engname;
int *nl;
int *notu;
{
	int i;
	char line[BUFLINE];
	char idbuf[BUFLINE];
	char scibuf[BUFLINE];
	char engbuf[BUFLINE];
	char *cp, *np;

	while (fgets(line, BUFLINE, ifp) != NULL) {
		(*nl)++; /* line counter */
		if (line[0] == '#') /* comment line skip */
			continue;
		for (cp = line; isspace(*cp); cp++) /* white line skip */
			;
		if (*cp == '\0')
			continue;
		for (np = idbuf; !isspace(*cp); *np++ = *cp++) /* identifier */
			;
		*np = '\0';
		for (i = 0; i < *notu; i++) { /* ID check */
			if (strcmp(idbuf, identif[i]) == 0) {
				fprintf(stderr,"Identifier \"%s\" is double defined\n",idbuf);
				exit(1);
			}
		}
		identif[*notu] =
			(char *)malloc((unsigned)(strlen(idbuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, identif");
		strcpy(identif[*notu], idbuf);

		for ( ; isspace(*cp); cp++) /* white char skip */
			;
		/* science name */
		if (*cp != '(' && *cp != '\0' && *cp != '#' && *cp != '\n') {
			for (np = scibuf; *cp!='(' && *cp!='#' && *cp!='\0' && *cp!='\n'; *np++ = *cp++)
				;
			for ( ; isspace(*(np-1)); np--)
				;
			*np = '\0';
		} else {
			scibuf[0] = '\0';
		}
		sciname[*notu] =
			(char *)malloc((unsigned)(strlen(scibuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, sciname");
		strcpy(sciname[*notu], scibuf);

		/* english name */
		if (*cp != '\0' && *cp != '#' && *cp != '\n') {
			for (np = engbuf; *cp!='#' && *cp!='\0' && *cp!='\n'; *np++ = *cp++)
				;
			for ( ; isspace(*(np-1)); np--)
				;
			*np = '\0';
		} else {
			engbuf[0] = '\0';
		}
		engname[*notu] =
			(char *)malloc((unsigned)(strlen(engbuf) + 1) * sizeof(char));
		if (identif[*notu] == NULL) maerror("in getid, engname");
		strcpy(engname[*notu], engbuf);

		if (Debug) {
			printf("%3d OTU \"%s\"", *notu+1, identif[*notu]);
			if (sciname[*notu] != '\0') {
				fputs(" sci:", stdout);
				fputs(sciname[*notu], stdout);
			}
			if (engname[*notu] != '\0') {
				putchar(' ');
				fputs(" eng:", stdout);
				fputs(engname[*notu], stdout);
			}
			putchar('\n');
		}
		break;
	}
	return;
} /*_ getid */


/* getsites */
void
getsites(ifp, seqchar, numsite, nl, notu)
FILE *ifp;
cmatrix seqchar;
int numsite, *nl, *notu;
{
	char line[BUFLINE];
	char c;
	int i, imax, nsite;

	nsite = 0;
	while (fgets(line, BUFLINE, ifp) != NULL) {
		(*nl)++; /* line counter */
		if (line[0] == '#') /* comment line skip */
			continue;
		for (i = 0, imax = strlen(line); i < imax && nsite < numsite; i++) {
			c = toupper(line[i]);
			if (isacid(c)) {
				seqchar[*notu][nsite++] = acid2int(c);
			} else if (isspace(c)) {
				/* skip white char */
			} else {
				fputs(line, stderr);
				fprintf(stderr, "Expected %s Acid Character: '%c'\n",
					Infomol, c);
				fprintf(stderr, "%d line %d column, %d OTU %d site\n",
					*nl+1, i+1, *notu+1, nsite+1);
				exit(1);
			}
		}
		if (nsite >= numsite) {
			if (i != imax && !isspace(c = toupper(line[i])) && c != '\0') {
				fputs(line, stderr);
				fprintf(stderr, "Inconsistent size of sequence: '%c'\n", c);
				fprintf(stderr, "%d line %d column, %d OTU %d site\n",
					*nl+1, i+1, *notu+1, nsite+1);
				exit(1);
			}
			break;
		}
	}
	return;
} /* getsites */


void getseq(ifp, identif, sciname, engname, seqchar, maxspc, numsite)
FILE *ifp;
char **identif, **sciname, **engname;
cmatrix seqchar;
int maxspc, numsite;
{
	int notu;
	int nl = 0;

	for (notu = 0; notu < maxspc; notu++) {
		getid(ifp, identif, sciname, engname, &nl, &notu);
		getsites(ifp, seqchar, numsite, &nl, &notu);
	}
	return;
} /*_ getseq */


/* getidsites */
void
getidsites(ifp, identif, seqchar, numsite, nl, notu)
FILE *ifp;
cmatrix identif, seqchar;
int numsite, *nl, *notu;
{
	char line[BUFLINE];
	char idbuf[BUFLINE];
	char c;
	int i, imax, nsite;

	nsite = 0;
	while (fgets(line, BUFLINE, ifp) != NULL) {
		(*nl)++; /* line counter */
		if (line[0] == '#') /* comment line skip */
			continue;
		if (nsite == 0) {
			strncpy(idbuf, line, NMLNGTH);
			i = NMLNGTH;
			while (isspace(idbuf[--i]))
				;
			idbuf[++i] = '\0';
			for (i = 0; i < *notu; i++) { /* ID check */
				if (strcmp(idbuf, identif[i]) == 0) {
					fprintf(stderr,
						"Identifier \"%s\" is double defined\n",idbuf);
					exit(1);
				}
			}
			identif[*notu] =
				(char *)malloc((unsigned)(strlen(idbuf) + 1) * sizeof(char));
			if (identif[*notu] == NULL) maerror("in getidsites, identif");
			strcpy(identif[*notu], idbuf);
			for (i=NMLNGTH, imax=strlen(line); i<imax && nsite<numsite; i++) {
				c = toupper(line[i]);
				if (isacid(c)) {
					seqchar[*notu][nsite++] = acid2int(c);
				} else if (isspace(c)) {
					/* skip white char */
				} else {
					fputs(line, stderr);
					fprintf(stderr, "Expected %s Acid Character: '%c'\n",
						Infomol, c);
					fprintf(stderr, "%d line %d column, %d OTU %d site\n",
						*nl+1, i+1, *notu+1, nsite+1);
					exit(1);
				}
			}
		} else {
			for (i=0, imax=strlen(line); i < imax && nsite < numsite; i++) {
				c = toupper(line[i]);
				if (isacid(c)) {
					seqchar[*notu][nsite++] = acid2int(c);
				} else if (isspace(c)) {
					/* skip white char */
				} else {
					fputs(line, stderr);
					fprintf(stderr, "Expected %s Acid Character: '%c'\n",
						Infomol, c);
					fprintf(stderr, "%d line %d column, %d OTU %d site\n",
						*nl+1, i+1, *notu+1, nsite+1);
					exit(1);
				}
			}
		}
		if (nsite >= numsite) {
			if (i != imax && !isspace(c = toupper(line[i])) && c != '\0') {
				fputs(line, stderr);
				fprintf(stderr, "Inconsistent size of sequence: '%c'\n", c);
				fprintf(stderr, "%d line %d column, %d OTU %d site\n",
					*nl+1, i+1, *notu+1, nsite+1);
				exit(1);
			}
			break;
		}
	}
	return;
}
/*_ getidsites */


void getseqs(ifp, identif, seqchar, maxspc, numsite)
FILE *ifp;
char **identif;
cmatrix seqchar;
int maxspc, numsite;
{
	int notu;
	int nl = 0;

	for (notu = 0; notu < maxspc; notu++)
		getidsites(ifp, identif, seqchar, numsite, &nl, &notu);
	return;
} /*_ getseqs */


void getseqi(ifp, identif, seqchar, maxspc, numsite)
FILE *ifp;
char **identif;
cmatrix seqchar;
int maxspc, numsite;
{
	int notu, nl, nsite, ns, i, imax;
	char line[BUFLINE];
	char idbuf[BUFLINE];
	char c, *cp;

	nsite = 0;
	nl = 0;
	while (nsite < numsite) {
	for (notu = 0; notu < maxspc; notu++) {
		while (fgets(line, BUFLINE, ifp) != NULL) {
			nl++; /* line counter */
			for (cp = line; isspace(*cp); cp++) /* white line skip */
				;
			if (*cp == '\0')
				continue;
			if (nsite == 0) {
				strncpy(idbuf, line, NMLNGTH);
				i = NMLNGTH;
				while (isspace(idbuf[--i]))
					;
				idbuf[++i] = '\0';
				for (i = 0; i < notu; i++) { /* ID check */
					if (strcmp(idbuf, identif[i]) == 0) {
						fprintf(stderr,
							"Identifier \"%s\" is double defined\n",idbuf);
						exit(1);
					}
				}
				identif[notu] =
					(char *)malloc((unsigned)(strlen(idbuf)+1) * sizeof(char));
				if (identif[notu] == NULL) maerror("in getidsites, identif");
				strcpy(identif[notu], idbuf);
				i = NMLNGTH;
			} else {
				i = 0;
			}
			for (ns=nsite, imax=strlen(line); i<imax && ns<numsite; i++) {
				c = toupper(line[i]);
				if (isacid(c)) {
					seqchar[notu][ns++] = acid2int(c);
				} else if (isspace(c)) {
					/* skip white char */
				} else {
					fputs(line, stderr);
					fprintf(stderr, "Expected %s Acid Character: '%c'\n",
						Infomol, c);
					fprintf(stderr,"%d line %d column, %d OTU %d site\n",
						nl+1, i+1, notu+1, ns+1);
					exit(1);
				}
			}
			if (notu == maxspc-1) nsite = ns;
			if (nsite >= numsite) {
				if (i != imax && !isspace(c = toupper(line[i])) && c != '\0') {
					fputs(line, stderr);
					fprintf(stderr, "Inconsistent size of sequence: '%c'\n", c);
					fprintf(stderr, "%d line %d column, %d OTU %d site\n",
						nl+1, i+1, notu+1, nsite+1);
					exit(1);
				}
				break;
			}
			break;
		}
	}
	}
	return;

} /*_ getseqi */


/* put identifier of otu */
void
fputid(ofp, name, maxcolumn)
FILE *ofp;
const char *name;
int maxcolumn;
{
	int i, imax;

	if ((imax = strlen(name)) < maxcolumn) {
		fputs(name, ofp);
		for (i = imax; i < maxcolumn; i++)
			fputc(' ', ofp);
	} else {
		for (i = 0; i < maxcolumn; i++)
			fputc(name[i], ofp);
	}
}


/* print sequences */
void
prsequence(ofp, identif, seqchar, maxspc, maxsite)
FILE *ofp;
char **identif;
char **seqchar;
int maxspc, maxsite;
{
	int i, nst, maxst, nsp;
	char ch;
	ivector counti;
	cvector consenseq;

	counti = new_ivector(Tpmradix + 1);
	consenseq = new_cvector(maxsite);
	for (nst = 0; nst < maxsite; nst++) { /* make consensus sequence */
		for (i = 0; i < Tpmradix + 1; i++) counti[i] = 0;
		for (nsp = 0; nsp < maxspc; nsp++) { /* count char */
			counti[ seqchar[nsp][nst] ]++;
		}
		consenseq[nst] = '.';
		for (i = 0; i < Tpmradix; i++) { /* + 1 */
			if (counti[i] > maxspc / 2) {
				consenseq[nst] = int2acid(i);
				break;
			}
		}
	}

	fputc('\n', ofp);
	for (i = 0; i < maxsite; i += Linesites) {
		maxst = (i + Linesites < maxsite) ? (i + Linesites) : maxsite;
		fputid(ofp, "CONSENSUS", 10);
		for (nst = i; nst < maxst; nst++) {
			if (nst % 10 == 0) fputc(' ', ofp);
			fputc(consenseq[nst], ofp);
		}
		fputc('\n', ofp);
		for (nsp = 0; nsp < maxspc; nsp++) {
			fputid(ofp, identif[nsp], 10);
			for (nst = i; nst < maxst; nst++) {
				if (nst % 10 == 0) fputc(' ', ofp);
				ch = chint2acid(seqchar[nsp][nst]);
				if (ch != consenseq[nst])
					fputc(ch, ofp);
				else
					fputc('.', ofp);
			}
			fputc('\n', ofp);
		}
		fputid(ofp, "", 10);
		for (nst = i; nst < maxst; nst+=10) {
			if (nst+10 <= maxst) printf(" %10d", nst+10);
		}
		fputc('\n', ofp);
	}
	free_cvector(consenseq);
	free_ivector(counti);
	return;
} /*_ prsequence */
