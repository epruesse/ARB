/*
 * matrixut.h - numerical matrix utility
 *
 * Ver. 1.1  july 23, 1993  Adachi, J.
 */


/* Environment Dependent Definitions */


/* Public Definitions */

typedef float *fvector, **fmatrix, ***fcube;
typedef double *dvector, **dmatrix, ***dcube;
typedef int *ivector, **imatrix, ***icube;
typedef char *cvector, **cmatrix, ***ccube;


/* Function Prototype Declarations */

#if defined(__STDC__) || defined(ANSI) /* GCC Ver.2.x */

void maerror(const char *message);
fvector new_fvector(int n);
fmatrix new_fmatrix(int nrow, int ncol);
fcube new_fcube(int ntri, int nrow, int ncol);
void free_fvector(fvector v);
void free_fmatrix(fmatrix m);
void free_fcube(fcube c);
dvector new_dvector(int n);
dmatrix new_dmatrix(int nrow, int ncol);
dcube new_dcube(int ntri, int nrow, int ncol);
void free_dvector(dvector v);
void free_dmatrix(dmatrix m);
void free_dcube(dcube c);
cvector new_cvector(int n);
cmatrix new_cmatrix(int nrow, int ncol);
ccube new_ccube(int ntri, int nrow, int ncol);
void free_cvector(cvector v);
void free_cmatrix(cmatrix m);
void free_ccube(ccube c);
ivector new_ivector(int n);
imatrix new_imatrix(int nrow, int ncol);
icube new_icube(int ntri, int nrow, int ncol);
void free_ivector(ivector v);
void free_imatrix(imatrix m);
void free_icube(icube c);

#else /* Sun OS Ver.4.1.3 */

void maerror();
fvector new_fvector();
fmatrix new_fmatrix();
fcube new_fcube();
void free_fvector();
void free_fmatrix();
void free_fcube();
dvector new_dvector();
dmatrix new_dmatrix();
dcube new_dcube();
void free_dvector();
void free_dmatrix();
void free_dcube();
cvector new_cvector();
cmatrix new_cmatrix();
ccube new_ccube();
void free_cvector();
void free_cmatrix();
void free_ccube();
ivector new_ivector();
imatrix new_imatrix();
icube new_icube();
void free_ivector();
void free_imatrix();
void free_icube();

#endif
