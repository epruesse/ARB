/*
 * protproc.c   Adachi, J.   1994.01.31
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "protml.h"


char
*Cacid1[] = {
	"A", "R", "N", "D", "C", "Q", "E", "G", "H", "I",
	"L", "K", "M", "F", "P", "S", "T", "W", "Y", "V",
	"B", "Z", "X", "*", "-"
};


char
*Cacid3[] = {
	"Ala", "Arg", "Asn", "Asp", "Cys", "Gln", "Glu", "Gly", "His", "Ile",
	"Leu", "Lys", "Met", "Phe", "Pro", "Ser", "Thr", "Trp", "Tyr", "Val",
	"Asx", "Glx", "Xxx", " * ", " - "
};


int
isacid(c)
char c;
{
	/* amino acid */
	switch (c) {
		case 'A':  case 'R':  case 'N':  case 'D':  case 'C':
		case 'Q':  case 'E':  case 'G':  case 'H':  case 'I':
		case 'L':  case 'K':  case 'M':  case 'F':  case 'P':
		case 'S':  case 'T':  case 'W':  case 'Y':  case 'V':
		case 'B':  case 'Z':  case 'X':  case '*':  case '-':
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
		case 'A': return 0;
		case 'R': return 1;
		case 'N': return 2;
		case 'D': return 3;
		case 'C': return 4;
		case 'Q': return 5;
		case 'E': return 6;
		case 'G': return 7;
		case 'H': return 8;
		case 'I': return 9;
		case 'L': return 10;
		case 'K': return 11;
		case 'M': return 12;
		case 'F': return 13;
		case 'P': return 14;
		case 'S': return 15;
		case 'T': return 16;
		case 'W': return 17;
		case 'Y': return 18;
		case 'V': return 19;
		default : return 20;
	}
} /*_ acid2int */


char
acid2chint(c)
char c;
{
	switch (c) {
		case 'A': return 0;
		case 'R': return 1;
		case 'N': return 2;
		case 'D': return 3;
		case 'C': return 4;
		case 'Q': return 5;
		case 'E': return 6;
		case 'G': return 7;
		case 'H': return 8;
		case 'I': return 9;
		case 'L': return 10;
		case 'K': return 11;
		case 'M': return 12;
		case 'F': return 13;
		case 'P': return 14;
		case 'S': return 15;
		case 'T': return 16;
		case 'W': return 17;
		case 'Y': return 18;
		case 'V': return 19;
		default : return -1;
	}
} /*_ acid2chint */


char
chint2acid(c)
char c;
{
	switch (c) {
		case  0 : return 'A';
		case  1 : return 'R';
		case  2 : return 'N';
		case  3 : return 'D';
		case  4 : return 'C';
		case  5 : return 'Q';
		case  6 : return 'E';
		case  7 : return 'G';
		case  8 : return 'H';
		case  9 : return 'I';
		case 10 : return 'L';
		case 11 : return 'K';
		case 12 : return 'M';
		case 13 : return 'F';
		case 14 : return 'P';
		case 15 : return 'S';
		case 16 : return 'T';
		case 17 : return 'W';
		case 18 : return 'Y';
		case 19 : return 'V';
		default : return '-';
	}
} /*_ chint2acid */


char
int2acid(i)
int i;
{
	switch (i) {
		case  0 : return 'A';
		case  1 : return 'R';
		case  2 : return 'N';
		case  3 : return 'D';
		case  4 : return 'C';
		case  5 : return 'Q';
		case  6 : return 'E';
		case  7 : return 'G';
		case  8 : return 'H';
		case  9 : return 'I';
		case 10 : return 'L';
		case 11 : return 'K';
		case 12 : return 'M';
		case 13 : return 'F';
		case 14 : return 'P';
		case 15 : return 'S';
		case 16 : return 'T';
		case 17 : return 'W';
		case 18 : return 'Y';
		case 19 : return 'V';
		default : return '-';
	}
} /*_ int2acid */
