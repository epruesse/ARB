/*
 * matrixut.c - numerical matrix utility
 * Ver. 1.2  Aug 26, 1993  Adachi, J.
 *
 * Copyright (C) 1992, 1993 J. Adachi & M. Hasegawa, All rights reserved.
 */

#ifndef MATRIX_UTILITY
#define MATRIX_UTILITY

#include <stdio.h>
#include <stddef.h>
#include "mo_assert.h"
#define mo_assert(cond) arb_assert(cond)

#include "matrixut.h"

void
maerror(message)
const char *message;
/* memory allocation error handler */
{
	fprintf(stderr, "\nmemory allocation failure %s\n", message);
	exit(1);
}

/*
 * float matrix utility
 */

fvector
new_fvector(n)
int n;
/* memory allocate a float vector */
{
	fvector v;

    mo_assert(n>0);

	v = (fvector) malloc((unsigned) (n * sizeof(float)));
	if (v == NULL) maerror("in fvector().");
	return v;
}

fmatrix
new_fmatrix(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a float matrix */
{
	int i;
	fmatrix m;

    mo_assert(nrow>0);
    mo_assert(ncol>0);

	m = (fmatrix) malloc((unsigned) (nrow * sizeof(fvector)));
	if (m == NULL) maerror("1 in fmatrix().");
	*m = (fvector) malloc((unsigned) (nrow * ncol * sizeof(float)));
	if (*m == NULL) maerror("2 in fmatrix().");
	for (i = 1; i < nrow; i++) m[i] = m[i-1] + ncol;
	return m;
}

fcube
new_fcube(ntri, nrow, ncol)
int ntri;
int nrow;
int ncol;
/* memory allocate a float cube */
{
	int i, j;
	fcube c;

    mo_assert(ntri>0);
    mo_assert(nrow>0);
    mo_assert(ncol>0);

	c = (fcube) malloc((unsigned) (ntri * sizeof(fmatrix)));
	if (c == NULL) maerror("1 in fcube().");
	*c = (fmatrix) malloc((unsigned) (ntri * nrow * sizeof(fvector)));
	if (*c == NULL) maerror("2 in fcube().");
	**c = (fvector) malloc((unsigned) (ntri * nrow * ncol * sizeof(float)));
	if (**c == NULL) maerror("3 in fcube().");
	for (j = 1; j < nrow; j++) c[0][j] = c[0][j-1] + ncol;
	for (i = 1; i < ntri; i++) {
		c[i] = c[i-1] + nrow;
		c[i][0] = c[i-1][0] + nrow * ncol;
		for (j = 1; j < nrow; j++) c[i][j] = c[i][j-1] + ncol;
	}
	return c;
}

void
free_fvector(v)
fvector v;
{
	free((char *) v);
}

void
free_fmatrix(m)
fmatrix m;
{
	free((char *) *m);
	free((char *) m);
}

void
free_fcube(c)
fcube c;
{
	free((char *) **c);
	free((char *) *c);
	free((char *) c);
}


/*
 * double matrix utility
 */

dvector
new_dvector(n)
int n;
/* memory allocate a double vector */
{
	dvector v;

    mo_assert(n>0);

	v = (dvector) malloc((unsigned) (n * sizeof(double)));
	if (v == NULL) maerror("in dvector().");
	return v;
}

dmatrix
new_dmatrix(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a double matrix */
{
    int     i;
    dmatrix m;

    mo_assert(nrow>0);
    mo_assert(ncol>0);

    m = (dmatrix) malloc((unsigned) (nrow * sizeof(dvector)));
    if (m == NULL) maerror("1 in dmatrix().");
    *m = (dvector) malloc((unsigned) (nrow * ncol * sizeof(double)));
    if (*m == NULL) maerror("2 in dmatrix().");
    for (i = 1; i < nrow; i++) m[i] = m[i-1] + ncol;
    return m;
}

dcube
new_dcube(ntri, nrow, ncol)
int ntri;
int nrow;
int ncol;
/* memory allocate a double cube */
{
	int i, j;
	dcube c;

    mo_assert(ntri>0);
    mo_assert(nrow>0);
    mo_assert(ncol>0);
    
	c = (dcube) malloc((unsigned) (ntri * sizeof(dmatrix)));
	if (c == NULL) maerror("1 in dcube().");
	*c = (dmatrix) malloc((unsigned) (ntri * nrow * sizeof(dvector)));
	if (*c == NULL) maerror("2 in dcube().");
	**c = (dvector) malloc((unsigned) (ntri * nrow * ncol * sizeof(double)));
	if (**c == NULL) maerror("3 in dcube().");
	for (j = 1; j < nrow; j++) c[0][j] = c[0][j-1] + ncol;
	for (i = 1; i < ntri; i++) {
		c[i] = c[i-1] + nrow;
		c[i][0] = c[i-1][0] + nrow * ncol;
		for (j = 1; j < nrow; j++) c[i][j] = c[i][j-1] + ncol;
	}
	return c;
}

void
free_dvector(v)
dvector v;
{
	free((char *) v);
}

void
free_dmatrix(m)
dmatrix m;
{
	free((char *) *m);
	free((char *) m);
}

void
free_dcube(c)
dcube c;
{
	free((char *) **c);
	free((char *) *c);
	free((char *) c);
}


/*
 * char matrix utility
 */

cvector
new_cvector(n)
int n;
/* memory allocate a char vector */
{
	cvector v;
    
    mo_assert(n>0);

	v = (cvector) malloc((unsigned)n * sizeof(char));
	if (v == NULL) maerror("in cvector().");
	return v;
}

cmatrix
new_cmatrix(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a char matrix */
{
	int i;
	cmatrix m;

    mo_assert(nrow>0);
    mo_assert(ncol>0);
    
	m = (cmatrix) malloc((unsigned) (nrow * sizeof(cvector)));
	if (m == NULL) maerror("1 in cmatrix().");
	*m = (cvector) malloc((unsigned) (nrow * ncol * sizeof(char)));
	if (*m == NULL) maerror("2 in cmatrix().");
	for (i = 1; i < nrow; i++) m[i] = m[i-1] + ncol;
	return m;
}

ccube
new_ccube(ntri, nrow, ncol)
int ntri;
int nrow;
int ncol;
/* memory allocate a char cube */
{
	int i, j;
	ccube c;

    mo_assert(ntri>0);
    mo_assert(nrow>0);
    mo_assert(ncol>0);
    
	c = (ccube) malloc((unsigned) (ntri * sizeof(cmatrix)));
	if (c == NULL) maerror("1 in ccube().");
	*c = (cmatrix) malloc((unsigned) (ntri * nrow * sizeof(cvector)));
	if (*c == NULL) maerror("2 in ccube().");
	**c = (cvector) malloc((unsigned) (ntri * nrow * ncol * sizeof(char)));
	if (**c == NULL) maerror("3 in ccube().");
	for (j = 1; j < nrow; j++) c[0][j] = c[0][j-1] + ncol;
	for (i = 1; i < ntri; i++) {
		c[i] = c[i-1] + nrow;
		c[i][0] = c[i-1][0] + nrow * ncol;
		for (j = 1; j < nrow; j++) c[i][j] = c[i][j-1] + ncol;
	}
	return c;
}

void
free_cvector(v)
cvector v;
{
	free((char *) v);
}

void
free_cmatrix(m)
cmatrix m;
{
	free((char *) *m);
	free((char *) m);
}

void
free_ccube(c)
ccube c;
{
	free((char *) **c);
	free((char *) *c);
	free((char *) c);
}


/*
 * int matrix utility
 */

ivector
new_ivector(n)
int n;
/* memory allocate a int vector */
{
	ivector v;

    mo_assert(n>0);
    
	v = (ivector) malloc((unsigned) (n * sizeof(int)));
	if (v == NULL) maerror("in ivector().");
	return v;
}

imatrix
new_imatrix(nrow, ncol)
int nrow;
int ncol;
/* memory allocate a int matrix */
{
	int i;
	imatrix m;

    mo_assert(nrow>0);
    mo_assert(ncol>0);
    
	m = (imatrix) malloc((unsigned) (nrow * sizeof(ivector)));
	if (m == NULL) maerror("1 in imatrix().");
	*m = (ivector) malloc((unsigned) (nrow * ncol * sizeof(int)));
	if (*m == NULL) maerror("2 in imatrix().");
	for (i = 1; i < nrow; i++) m[i] = m[i-1] + ncol;
	return m;
}

icube
new_icube(ntri, nrow, ncol)
int ntri;
int nrow;
int ncol;
/* memory allocate a int cube */
{
	int i, j;
	icube c;

    mo_assert(ntri>0);
    mo_assert(nrow>0);
    mo_assert(ncol>0);
    
	c = (icube) malloc((unsigned) (ntri * sizeof(imatrix)));
	if (c == NULL) maerror("1 in icube().");
	*c = (imatrix) malloc((unsigned) (ntri * nrow * sizeof(ivector)));
	if (*c == NULL) maerror("2 in icube().");
	**c = (ivector) malloc((unsigned) (ntri * nrow * ncol * sizeof(int)));
	if (**c == NULL) maerror("3 in icube().");
	for (j = 1; j < nrow; j++) c[0][j] = c[0][j-1] + ncol;
	for (i = 1; i < ntri; i++) {
		c[i] = c[i-1] + nrow;
		c[i][0] = c[i-1][0] + nrow * ncol;
		for (j = 1; j < nrow; j++) c[i][j] = c[i][j-1] + ncol;
	}
	return c;
}

void
free_ivector(v)
ivector v;
{
	free((char *) v);
}

void
free_imatrix(m)
imatrix m;
{
	free((char *) *m);
	free((char *) m);
}

void
free_icube(c)
icube c;
{
	free((char *) **c);
	free((char *) *c);
	free((char *) c);
}


#endif /* MATRIX_UTILITY */
