/*
 * nucproc.c   Adachi, J.   1994.01.28
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"


char
*Cacid1[] = {
	"T", "C", "A", "G",
	"N", "*", "-"
};


char
*Cacid3[] = {
	"Thy", "Cyt", "Ade", "Gua",
	"Nnn", " * ", " - "
};


int isacid(char c)
{
	/* nuc */
	switch (c) {
		case 'T':  case 'C':  case 'A':  case 'G':  case 'U':
		case 'R':  case 'Y':  case 'N':  case '*':  case '-':
			return 1;
		default:
			return 0;
	}
} /*_ isacid */


int
acid2int(c)
char c;
{
	switch (c) {
		case 'T': return 0;
		case 'U': return 0;
		case 'C': return 1;
		case 'A': return 2;
		case 'G': return 3;
		default : return 4;
	}
} /*_ acid2int */


char
acid2chint(c)
char c;
{
	switch (c) {
		case 'T': return 0;
		case 'U': return 0;
		case 'C': return 1;
		case 'A': return 2;
		case 'G': return 3;
		default : return -1;
	}
} /*_ acid2chint */


char
chint2acid(c)
char c;
{
	switch (c) {
		case  0 : return 'T';
		case  1 : return 'C';
		case  2 : return 'A';
		case  3 : return 'G';
		default : return '-';
	}
} /*_ chint2acid */


char
int2acid(i)
int i;
{
	switch (i) {
		case  0 : return 'T';
		case  1 : return 'C';
		case  2 : return 'A';
		case  3 : return 'G';
		default : return '-';
	}
} /*_ int2acid */
