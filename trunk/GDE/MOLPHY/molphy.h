#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define boolean int

#ifndef MSDOS
#define SW_CHAR  '-' /* switch charcter '-' (UNIX) */
#define DIR_CHAR '/' /* directory separator '/' (UNIX) */
#else
#define SW_CHAR  '/' /* switch charcter '/' (MSDOS) */
#define DIR_CHAR '\\' /* directory separator '\' (MSDOS) */
#endif

#ifdef RAND_MAX
#define RANDOM_MAX RAND_MAX
#else
#define RANDOM_MAX INT_MAX
#endif

#define A4PAPER 1 /* 1: A4 size,  0: US letter size */

#define DIFFTIME 0
