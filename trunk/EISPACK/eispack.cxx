/* eispack.f -- translated by f2c (version 19950110).
   You must link the resulting object file with the libraries:
        -lf2c -lm   (in that order)
*/

#ifdef __cplusplus
extern "C" {
#endif
#include "f2c.h"

/* Table of constant values */

static doublereal c_b141 = 1.;
static doublereal c_b550 = 0.;

/* Subroutine */ int cdiv_(doublereal *ar, doublereal *ai, doublereal *br, 
        doublereal *bi, doublereal *cr, doublereal *ci)
{
    /* System generated locals */
    doublereal d_1, d_2;

    /* Local variables */
    static doublereal s, ais, bis, ars, brs;


/*     COMPLEX DIVISION, (CR,CI) = (AR,AI)/(BR,BI) */

    s = abs(*br) + abs(*bi);
    ars = *ar / s;
    ais = *ai / s;
    brs = *br / s;
    bis = *bi / s;
/* Computing 2nd power */
    d_1 = brs;
/* Computing 2nd power */
    d_2 = bis;
    s = d_1 * d_1 + d_2 * d_2;
    *cr = (ars * brs + ais * bis) / s;
    *ci = (ais * brs - ars * bis) / s;
    return 0;
} /* cdiv_ */

/* Subroutine */ int csroot_(doublereal *xr, doublereal *xi, doublereal *yr, 
        doublereal *yi)
{
    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal s, ti, tr;
    extern doublereal pythag_(doublereal *, doublereal *);


/*     (YR,YI) = COMPLEX DSQRT(XR,XI) */
/*     BRANCH CHOSEN SO THAT YR .GE. 0.0 AND SIGN(YI) .EQ. SIGN(XI) */

    tr = *xr;
    ti = *xi;
    s = sqrt((pythag_(&tr, &ti) + abs(tr)) * .5);
    if (tr >= 0.) {
        *yr = s;
    }
    if (ti < 0.) {
        s = -s;
    }
    if (tr <= 0.) {
        *yi = s;
    }
    if (tr < 0.) {
        *yr = ti / *yi * .5;
    }
    if (tr > 0.) {
        *yi = ti / *yr * .5;
    }
    return 0;
} /* csroot_ */

doublereal epslon_(doublereal *x)
{
    /* System generated locals */
    doublereal ret_val, d_1;

    /* Local variables */
    static doublereal a, b, c, eps;


/*     ESTIMATE UNIT ROUNDOFF IN QUANTITIES OF SIZE X. */


/*     THIS PROGRAM SHOULD FUNCTION PROPERLY ON ALL SYSTEMS */
/*     SATISFYING THE FOLLOWING TWO ASSUMPTIONS, */
/*        1.  THE BASE USED IN REPRESENTING FLOATING POINT */
/*            NUMBERS IS NOT A POWER OF THREE. */
/*        2.  THE QUANTITY  A  IN STATEMENT 10 IS REPRESENTED TO */
/*            THE ACCURACY USED IN FLOATING POINT VARIABLES */
/*            THAT ARE STORED IN MEMORY. */
/*     THE STATEMENT NUMBER 10 AND THE GO TO 10 ARE INTENDED TO */
/*     FORCE OPTIMIZING COMPILERS TO GENERATE CODE SATISFYING */
/*     ASSUMPTION 2. */
/*     UNDER THESE ASSUMPTIONS, IT SHOULD BE TRUE THAT, */
/*            A  IS NOT EXACTLY EQUAL TO FOUR-THIRDS, */
/*            B  HAS A ZERO FOR ITS LAST BIT OR DIGIT, */
/*            C  IS NOT EXACTLY EQUAL TO ONE, */
/*            EPS  MEASURES THE SEPARATION OF 1.0 FROM */
/*                 THE NEXT LARGER FLOATING POINT NUMBER. */
/*     THE DEVELOPERS OF EISPACK WOULD APPRECIATE BEING INFORMED */
/*     ABOUT ANY SYSTEMS WHERE THESE ASSUMPTIONS DO NOT HOLD. */

/*     THIS VERSION DATED 4/6/83. */

    a = 1.3333333333333333;
L10:
    b = a - 1.;
    c = b + b + b;
    eps = (d_1 = c - 1., abs(d_1));
    if (eps == 0.) {
        goto L10;
    }
    ret_val = eps * abs(*x);
    return ret_val;
} /* epslon_ */

doublereal pythag_(doublereal *a, doublereal *b)
{
    /* System generated locals */
    doublereal ret_val, d_1, d_2, d_3;

    /* Local variables */
    static doublereal p, r, s, t, u;


/*     FINDS DSQRT(A**2+B**2) WITHOUT OVERFLOW OR DESTRUCTIVE UNDERFLOW */

/* Computing MAX */
    d_1 = abs(*a), d_2 = abs(*b);
    p = max(d_1,d_2);
    if (p == 0.) {
        goto L20;
    }
/* Computing MIN */
    d_2 = abs(*a), d_3 = abs(*b);
/* Computing 2nd power */
    d_1 = min(d_2,d_3) / p;
    r = d_1 * d_1;
L10:
    t = r + 4.;
    if (t == 4.) {
        goto L20;
    }
    s = r / t;
    u = s * 2. + 1.;
    p = u * p;
/* Computing 2nd power */
    d_1 = s / u;
    r = d_1 * d_1 * r;
    goto L10;
L20:
    ret_val = p;
    return ret_val;
} /* pythag_ */

/* Subroutine */ int bakvec_(integer *nm, integer *n, doublereal *t, 
        doublereal *e, integer *m, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer t_dim1, t_offset, z_dim1, z_offset, i_1, i_2;

    /* Local variables */
    static integer i, j;



/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A NONSYMMETRIC */
/*     TRIDIAGONAL MATRIX BY BACK TRANSFORMING THOSE OF THE */
/*     CORRESPONDING SYMMETRIC MATRIX DETERMINED BY  FIGI. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        T CONTAINS THE NONSYMMETRIC MATRIX.  ITS SUBDIAGONAL IS */
/*          STORED IN THE LAST N-1 POSITIONS OF THE FIRST COLUMN, */
/*          ITS DIAGONAL IN THE N POSITIONS OF THE SECOND COLUMN, */
/*          AND ITS SUPERDIAGONAL IN THE FIRST N-1 POSITIONS OF */
/*          THE THIRD COLUMN.  T(1,1) AND T(N,3) ARE ARBITRARY. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE SYMMETRIC */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        T IS UNALTERED. */

/*        E IS DESTROYED. */

/*        Z CONTAINS THE TRANSFORMED EIGENVECTORS */
/*          IN ITS FIRST M COLUMNS. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          2*N+I      IF E(I) IS ZERO WITH T(I,1) OR T(I-1,3) NON-ZERO. 
*/
/*                     IN THIS CASE, THE SYMMETRIC MATRIX IS NOT SIMILAR 
*/
/*                     TO THE ORIGINAL MATRIX, AND THE EIGENVECTORS */
/*                     CANNOT BE FOUND BY THIS PROGRAM. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    t_dim1 = *nm;
    t_offset = t_dim1 + 1;
    t -= t_offset;
    --e;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    *ierr = 0;
    if (*m == 0) {
        goto L1001;
    }
    e[1] = 1.;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
        if (e[i] != 0.) {
            goto L80;
        }
        if (t[i + t_dim1] != 0. || t[i - 1 + t_dim1 * 3] != 0.) {
            goto L1000;
        }
        e[i] = 1.;
        goto L100;
L80:
        e[i] = e[i - 1] * e[i] / t[i - 1 + t_dim1 * 3];
L100:
        ;
    }

    i_1 = *m;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (i = 2; i <= i_2; ++i) {
            z[i + j * z_dim1] *= e[i];
/* L120: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- EIGENVECTORS CANNOT BE */
/*                FOUND BY THIS PROGRAM .......... */
L1000:
    *ierr = (*n << 1) + i;
L1001:
    return 0;
} /* bakvec_ */

/* Subroutine */ int balanc_(integer *nm, integer *n, doublereal *a, integer *
        low, integer *igh, doublereal *scale)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2;
    doublereal d_1;

    /* Local variables */
    static integer iexc;
    static doublereal c, f, g;
    static integer i, j, k, l, m;
    static doublereal r, s, radix, b2;
    static integer jj;
    static logical noconv;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE BALANCE, */
/*     NUM. MATH. 13, 293-304(1969) BY PARLETT AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 315-326(1971). */

/*     THIS SUBROUTINE BALANCES A REAL MATRIX AND ISOLATES */
/*     EIGENVALUES WHENEVER POSSIBLE. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE INPUT MATRIX TO BE BALANCED. */

/*     ON OUTPUT */

/*        A CONTAINS THE BALANCED MATRIX. */

/*        LOW AND IGH ARE TWO INTEGERS SUCH THAT A(I,J) */
/*          IS EQUAL TO ZERO IF */
/*           (1) I IS GREATER THAN J AND */
/*           (2) J=1,...,LOW-1 OR I=IGH+1,...,N. */

/*        SCALE CONTAINS INFORMATION DETERMINING THE */
/*           PERMUTATIONS AND SCALING FACTORS USED. */

/*     SUPPOSE THAT THE PRINCIPAL SUBMATRIX IN ROWS LOW THROUGH IGH */
/*     HAS BEEN BALANCED, THAT P(J) DENOTES THE INDEX INTERCHANGED */
/*     WITH J DURING THE PERMUTATION STEP, AND THAT THE ELEMENTS */
/*     OF THE DIAGONAL MATRIX USED ARE DENOTED BY D(I,J).  THEN */
/*        SCALE(J) = P(J),    FOR J = 1,...,LOW-1 */
/*                 = D(J,J),      J = LOW,...,IGH */
/*                 = P(J)         J = IGH+1,...,N. */
/*     THE ORDER IN WHICH THE INTERCHANGES ARE MADE IS N TO IGH+1, */
/*     THEN 1 TO LOW-1. */

/*     NOTE THAT 1 IS RETURNED FOR IGH IF IGH IS ZERO FORMALLY. */

/*     THE ALGOL PROCEDURE EXC CONTAINED IN BALANCE APPEARS IN */
/*     BALANC  IN LINE.  (NOTE THAT THE ALGOL ROLES OF IDENTIFIERS */
/*     K,L HAVE BEEN REVERSED.) */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --scale;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    radix = 16.;

    b2 = radix * radix;
    k = 1;
    l = *n;
    goto L100;
/*     .......... IN-LINE PROCEDURE FOR ROW AND */
/*                COLUMN EXCHANGE .......... */
L20:
    scale[m] = (doublereal) j;
    if (j == m) {
        goto L50;
    }

    i_1 = l;
    for (i = 1; i <= i_1; ++i) {
        f = a[i + j * a_dim1];
        a[i + j * a_dim1] = a[i + m * a_dim1];
        a[i + m * a_dim1] = f;
/* L30: */
    }

    i_1 = *n;
    for (i = k; i <= i_1; ++i) {
        f = a[j + i * a_dim1];
        a[j + i * a_dim1] = a[m + i * a_dim1];
        a[m + i * a_dim1] = f;
/* L40: */
    }

L50:
    switch (iexc) {
        case 1:  goto L80;
        case 2:  goto L130;
    }
/*     .......... SEARCH FOR ROWS ISOLATING AN EIGENVALUE */
/*                AND PUSH THEM DOWN .......... */
L80:
    if (l == 1) {
        goto L280;
    }
    --l;
/*     .......... FOR J=L STEP -1 UNTIL 1 DO -- .......... */
L100:
    i_1 = l;
    for (jj = 1; jj <= i_1; ++jj) {
        j = l + 1 - jj;

        i_2 = l;
        for (i = 1; i <= i_2; ++i) {
            if (i == j) {
                goto L110;
            }
            if (a[j + i * a_dim1] != 0.) {
                goto L120;
            }
L110:
            ;
        }

        m = l;
        iexc = 1;
        goto L20;
L120:
        ;
    }

    goto L140;
/*     .......... SEARCH FOR COLUMNS ISOLATING AN EIGENVALUE */
/*                AND PUSH THEM LEFT .......... */
L130:
    ++k;

L140:
    i_1 = l;
    for (j = k; j <= i_1; ++j) {

        i_2 = l;
        for (i = k; i <= i_2; ++i) {
            if (i == j) {
                goto L150;
            }
            if (a[i + j * a_dim1] != 0.) {
                goto L170;
            }
L150:
            ;
        }

        m = k;
        iexc = 2;
        goto L20;
L170:
        ;
    }
/*     .......... NOW BALANCE THE SUBMATRIX IN ROWS K TO L .......... */
    i_1 = l;
    for (i = k; i <= i_1; ++i) {
/* L180: */
        scale[i] = 1.;
    }
/*     .......... ITERATIVE LOOP FOR NORM REDUCTION .......... */
L190:
    noconv = FALSE_;

    i_1 = l;
    for (i = k; i <= i_1; ++i) {
        c = 0.;
        r = 0.;

        i_2 = l;
        for (j = k; j <= i_2; ++j) {
            if (j == i) {
                goto L200;
            }
            c += (d_1 = a[j + i * a_dim1], abs(d_1));
            r += (d_1 = a[i + j * a_dim1], abs(d_1));
L200:
            ;
        }
/*     .......... GUARD AGAINST ZERO C OR R DUE TO UNDERFLOW .........
. */
        if (c == 0. || r == 0.) {
            goto L270;
        }
        g = r / radix;
        f = 1.;
        s = c + r;
L210:
        if (c >= g) {
            goto L220;
        }
        f *= radix;
        c *= b2;
        goto L210;
L220:
        g = r * radix;
L230:
        if (c < g) {
            goto L240;
        }
        f /= radix;
        c /= b2;
        goto L230;
/*     .......... NOW BALANCE .......... */
L240:
        if ((c + r) / f >= s * .95) {
            goto L270;
        }
        g = 1. / f;
        scale[i] *= f;
        noconv = TRUE_;

        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
/* L250: */
            a[i + j * a_dim1] *= g;
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L260: */
            a[j + i * a_dim1] *= f;
        }

L270:
        ;
    }

    if (noconv) {
        goto L190;
    }

L280:
    *low = k;
    *igh = l;
    return 0;
} /* balanc_ */

/* Subroutine */ int balbak_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *scale, integer *m, doublereal *z)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2;

    /* Local variables */
    static integer i, j, k;
    static doublereal s;
    static integer ii;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE BALBAK, */
/*     NUM. MATH. 13, 293-304(1969) BY PARLETT AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 315-326(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A REAL GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     BALANCED MATRIX DETERMINED BY  BALANC. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY  BALANC. */

/*        SCALE CONTAINS INFORMATION DETERMINING THE PERMUTATIONS */
/*          AND SCALING FACTORS USED BY  BALANC. */

/*        M IS THE NUMBER OF COLUMNS OF Z TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGEN- */
/*          VECTORS TO BE BACK TRANSFORMED IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE */
/*          TRANSFORMED EIGENVECTORS IN ITS FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --scale;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    if (*igh == *low) {
        goto L120;
    }

    i_1 = *igh;
    for (i = *low; i <= i_1; ++i) {
        s = scale[i];
/*     .......... LEFT HAND EIGENVECTORS ARE BACK TRANSFORMED */
/*                IF THE FOREGOING STATEMENT IS REPLACED BY */
/*                S=1.0D0/SCALE(I). .......... */
        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
/* L100: */
            z[i + j * z_dim1] *= s;
        }

/* L110: */
    }
/*     ......... FOR I=LOW-1 STEP -1 UNTIL 1, */
/*               IGH+1 STEP 1 UNTIL N DO -- .......... */
L120:
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = ii;
        if (i >= *low && i <= *igh) {
            goto L140;
        }
        if (i < *low) {
            i = *low - ii;
        }
        k = (integer) scale[i];
        if (k == i) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            s = z[i + j * z_dim1];
            z[i + j * z_dim1] = z[k + j * z_dim1];
            z[k + j * z_dim1] = s;
/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* balbak_ */

/* Subroutine */ int bandr_(integer *nm, integer *n, integer *mb, doublereal *
        a, doublereal *d, doublereal *e, doublereal *e2, logical *matz, 
        doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3, i_4, i_5, 
            i_6;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal dmin_;
    static integer maxl, maxr;
    static doublereal g;
    static integer j, k, l, r;
    static doublereal u, b1, b2, c2, f1, f2;
    static integer i1, i2, j1, j2, m1, n2, r1;
    static doublereal s2;
    static integer kr, mr;
    static doublereal dminrt;
    static integer ugl;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE BANDRD, */
/*     NUM. MATH. 12, 231-241(1968) BY SCHWARZ. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 273-283(1971). */

/*     THIS SUBROUTINE REDUCES A REAL SYMMETRIC BAND MATRIX */
/*     TO A SYMMETRIC TRIDIAGONAL MATRIX USING AND OPTIONALLY */
/*     ACCUMULATING ORTHOGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        MB IS THE (HALF) BAND WIDTH OF THE MATRIX, DEFINED AS THE */
/*          NUMBER OF ADJACENT DIAGONALS, INCLUDING THE PRINCIPAL */
/*          DIAGONAL, REQUIRED TO SPECIFY THE NON-ZERO PORTION OF THE */
/*          LOWER TRIANGLE OF THE MATRIX. */

/*        A CONTAINS THE LOWER TRIANGLE OF THE SYMMETRIC BAND INPUT */
/*          MATRIX STORED AS AN N BY MB ARRAY.  ITS LOWEST SUBDIAGONAL */
/*          IS STORED IN THE LAST N+1-MB POSITIONS OF THE FIRST COLUMN, */
/*          ITS NEXT SUBDIAGONAL IN THE LAST N+2-MB POSITIONS OF THE */
/*          SECOND COLUMN, FURTHER SUBDIAGONALS SIMILARLY, AND FINALLY */
/*          ITS PRINCIPAL DIAGONAL IN THE N POSITIONS OF THE LAST COLUMN. 
*/
/*          CONTENTS OF STORAGES NOT PART OF THE MATRIX ARE ARBITRARY. */

/*        MATZ SHOULD BE SET TO .TRUE. IF THE TRANSFORMATION MATRIX IS */
/*          TO BE ACCUMULATED, AND TO .FALSE. OTHERWISE. */

/*     ON OUTPUT */

/*        A HAS BEEN DESTROYED, EXCEPT FOR ITS LAST TWO COLUMNS WHICH */
/*          CONTAIN A COPY OF THE TRIDIAGONAL MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE TRIDIAGONAL MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*        Z CONTAINS THE ORTHOGONAL TRANSFORMATION MATRIX PRODUCED IN */
/*          THE REDUCTION IF MATZ HAS BEEN SET TO .TRUE.  OTHERWISE, Z */
/*          IS NOT REFERENCED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e2;
    --e;
    --d;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    dmin_ = 5.4210108624275222e-20;
    dminrt = 2.3283064365386963e-10;
/*     .......... INITIALIZE DIAGONAL SCALING MATRIX .......... */
    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {
/* L30: */
        d[j] = 1.;
    }

    if (! (*matz)) {
        goto L60;
    }

    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (k = 1; k <= i_2; ++k) {
/* L40: */
            z[j + k * z_dim1] = 0.;
        }

        z[j + j * z_dim1] = 1.;
/* L50: */
    }

L60:
    m1 = *mb - 1;
    if ((i_1 = m1 - 1) < 0) {
        goto L900;
    } else if (i_1 == 0) {
        goto L800;
    } else {
        goto L70;
    }
L70:
    n2 = *n - 2;

    i_1 = n2;
    for (k = 1; k <= i_1; ++k) {
/* Computing MIN */
        i_2 = m1, i_3 = *n - k;
        maxr = min(i_2,i_3);
/*     .......... FOR R=MAXR STEP -1 UNTIL 2 DO -- .......... */
        i_2 = maxr;
        for (r1 = 2; r1 <= i_2; ++r1) {
            r = maxr + 2 - r1;
            kr = k + r;
            mr = *mb - r;
            g = a[kr + mr * a_dim1];
            a[kr - 1 + a_dim1] = a[kr - 1 + (mr + 1) * a_dim1];
            ugl = k;

            i_3 = *n;
            i_4 = m1;
            for (j = kr; i_4 < 0 ? j >= i_3 : j <= i_3; j += i_4) {
                j1 = j - 1;
                j2 = j1 - 1;
                if (g == 0.) {
                    goto L600;
                }
                b1 = a[j1 + a_dim1] / g;
                b2 = b1 * d[j1] / d[j];
                s2 = 1. / (b1 * b2 + 1.);
                if (s2 >= .5) {
                    goto L450;
                }
                b1 = g / a[j1 + a_dim1];
                b2 = b1 * d[j] / d[j1];
                c2 = 1. - s2;
                d[j1] = c2 * d[j1];
                d[j] = c2 * d[j];
                f1 = a[j + m1 * a_dim1] * 2.;
                f2 = b1 * a[j1 + *mb * a_dim1];
                a[j + m1 * a_dim1] = -b2 * (b1 * a[j + m1 * a_dim1] - a[j + *
                        mb * a_dim1]) - f2 + a[j + m1 * a_dim1];
                a[j1 + *mb * a_dim1] = b2 * (b2 * a[j + *mb * a_dim1] + f1) + 
                        a[j1 + *mb * a_dim1];
                a[j + *mb * a_dim1] = b1 * (f2 - f1) + a[j + *mb * a_dim1];

                i_5 = j2;
                for (l = ugl; l <= i_5; ++l) {
                    i2 = *mb - j + l;
                    u = a[j1 + (i2 + 1) * a_dim1] + b2 * a[j + i2 * a_dim1];
                    a[j + i2 * a_dim1] = -b1 * a[j1 + (i2 + 1) * a_dim1] + a[
                            j + i2 * a_dim1];
                    a[j1 + (i2 + 1) * a_dim1] = u;
/* L200: */
                }

                ugl = j;
                a[j1 + a_dim1] += b2 * g;
                if (j == *n) {
                    goto L350;
                }
/* Computing MIN */
                i_5 = m1, i_6 = *n - j1;
                maxl = min(i_5,i_6);

                i_5 = maxl;
                for (l = 2; l <= i_5; ++l) {
                    i1 = j1 + l;
                    i2 = *mb - l;
                    u = a[i1 + i2 * a_dim1] + b2 * a[i1 + (i2 + 1) * a_dim1];
                    a[i1 + (i2 + 1) * a_dim1] = -b1 * a[i1 + i2 * a_dim1] + a[
                            i1 + (i2 + 1) * a_dim1];
                    a[i1 + i2 * a_dim1] = u;
/* L300: */
                }

                i1 = j + m1;
                if (i1 > *n) {
                    goto L350;
                }
                g = b2 * a[i1 + a_dim1];
L350:
                if (! (*matz)) {
                    goto L500;
                }

                i_5 = *n;
                for (l = 1; l <= i_5; ++l) {
                    u = z[l + j1 * z_dim1] + b2 * z[l + j * z_dim1];
                    z[l + j * z_dim1] = -b1 * z[l + j1 * z_dim1] + z[l + j * 
                            z_dim1];
                    z[l + j1 * z_dim1] = u;
/* L400: */
                }

                goto L500;

L450:
                u = d[j1];
                d[j1] = s2 * d[j];
                d[j] = s2 * u;
                f1 = a[j + m1 * a_dim1] * 2.;
                f2 = b1 * a[j + *mb * a_dim1];
                u = b1 * (f2 - f1) + a[j1 + *mb * a_dim1];
                a[j + m1 * a_dim1] = b2 * (b1 * a[j + m1 * a_dim1] - a[j1 + *
                        mb * a_dim1]) + f2 - a[j + m1 * a_dim1];
                a[j1 + *mb * a_dim1] = b2 * (b2 * a[j1 + *mb * a_dim1] + f1) 
                        + a[j + *mb * a_dim1];
                a[j + *mb * a_dim1] = u;

                i_5 = j2;
                for (l = ugl; l <= i_5; ++l) {
                    i2 = *mb - j + l;
                    u = b2 * a[j1 + (i2 + 1) * a_dim1] + a[j + i2 * a_dim1];
                    a[j + i2 * a_dim1] = -a[j1 + (i2 + 1) * a_dim1] + b1 * a[
                            j + i2 * a_dim1];
                    a[j1 + (i2 + 1) * a_dim1] = u;
/* L460: */
                }

                ugl = j;
                a[j1 + a_dim1] = b2 * a[j1 + a_dim1] + g;
                if (j == *n) {
                    goto L480;
                }
/* Computing MIN */
                i_5 = m1, i_6 = *n - j1;
                maxl = min(i_5,i_6);

                i_5 = maxl;
                for (l = 2; l <= i_5; ++l) {
                    i1 = j1 + l;
                    i2 = *mb - l;
                    u = b2 * a[i1 + i2 * a_dim1] + a[i1 + (i2 + 1) * a_dim1];
                    a[i1 + (i2 + 1) * a_dim1] = -a[i1 + i2 * a_dim1] + b1 * a[
                            i1 + (i2 + 1) * a_dim1];
                    a[i1 + i2 * a_dim1] = u;
/* L470: */
                }

                i1 = j + m1;
                if (i1 > *n) {
                    goto L480;
                }
                g = a[i1 + a_dim1];
                a[i1 + a_dim1] = b1 * a[i1 + a_dim1];
L480:
                if (! (*matz)) {
                    goto L500;
                }

                i_5 = *n;
                for (l = 1; l <= i_5; ++l) {
                    u = b2 * z[l + j1 * z_dim1] + z[l + j * z_dim1];
                    z[l + j * z_dim1] = -z[l + j1 * z_dim1] + b1 * z[l + j * 
                            z_dim1];
                    z[l + j1 * z_dim1] = u;
/* L490: */
                }

L500:
                ;
            }

L600:
            ;
        }

        if (k % 64 != 0) {
            goto L700;
        }
/*     .......... RESCALE TO AVOID UNDERFLOW OR OVERFLOW .......... */
        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            if (d[j] >= dmin_) {
                goto L650;
            }
/* Computing MAX */
            i_4 = 1, i_3 = *mb + 1 - j;
            maxl = max(i_4,i_3);

            i_4 = m1;
            for (l = maxl; l <= i_4; ++l) {
/* L610: */
                a[j + l * a_dim1] = dminrt * a[j + l * a_dim1];
            }

            if (j == *n) {
                goto L630;
            }
/* Computing MIN */
            i_4 = m1, i_3 = *n - j;
            maxl = min(i_4,i_3);

            i_4 = maxl;
            for (l = 1; l <= i_4; ++l) {
                i1 = j + l;
                i2 = *mb - l;
                a[i1 + i2 * a_dim1] = dminrt * a[i1 + i2 * a_dim1];
/* L620: */
            }

L630:
            if (! (*matz)) {
                goto L645;
            }

            i_4 = *n;
            for (l = 1; l <= i_4; ++l) {
/* L640: */
                z[l + j * z_dim1] = dminrt * z[l + j * z_dim1];
            }

L645:
            a[j + *mb * a_dim1] = dmin_ * a[j + *mb * a_dim1];
            d[j] /= dmin_;
L650:
            ;
        }

L700:
        ;
    }
/*     .......... FORM SQUARE ROOT OF SCALING MATRIX .......... */
L800:
    i_1 = *n;
    for (j = 2; j <= i_1; ++j) {
/* L810: */
        e[j] = sqrt(d[j]);
    }

    if (! (*matz)) {
        goto L840;
    }

    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (k = 2; k <= i_2; ++k) {
/* L820: */
            z[j + k * z_dim1] = e[k] * z[j + k * z_dim1];
        }

/* L830: */
    }

L840:
    u = 1.;

    i_1 = *n;
    for (j = 2; j <= i_1; ++j) {
        a[j + m1 * a_dim1] = u * e[j] * a[j + m1 * a_dim1];
        u = e[j];
/* Computing 2nd power */
        d_1 = a[j + m1 * a_dim1];
        e2[j] = d_1 * d_1;
        a[j + *mb * a_dim1] = d[j] * a[j + *mb * a_dim1];
        d[j] = a[j + *mb * a_dim1];
        e[j] = a[j + m1 * a_dim1];
/* L850: */
    }

    d[1] = a[*mb * a_dim1 + 1];
    e[1] = 0.;
    e2[1] = 0.;
    goto L1001;

L900:
    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {
        d[j] = a[j + *mb * a_dim1];
        e[j] = 0.;
        e2[j] = 0.;
/* L950: */
    }

L1001:
    return 0;
} /* bandr_ */

/* Subroutine */ int bandv_(integer *nm, integer *n, integer *mbw, doublereal 
        *a, doublereal *e21, integer *m, doublereal *w, doublereal *z, 
        integer *ierr, integer */*nv*/, doublereal *rv, doublereal *rv6)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3, i_4, i_5;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static integer maxj, maxk;
    static doublereal norm;
    static integer i, j, k, r;
    static doublereal u, v, order;
    static integer group, m1;
    static doublereal x0, x1;
    static integer mb, m21, ii, ij, jj, kj;
    static doublereal uk, xu;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer ij1, kj1, its;
    static doublereal eps2, eps3, eps4;



/*     THIS SUBROUTINE FINDS THOSE EIGENVECTORS OF A REAL SYMMETRIC */
/*     BAND MATRIX CORRESPONDING TO SPECIFIED EIGENVALUES, USING INVERSE 
*/
/*     ITERATION.  THE SUBROUTINE MAY ALSO BE USED TO SOLVE SYSTEMS */
/*     OF LINEAR EQUATIONS WITH A SYMMETRIC OR NON-SYMMETRIC BAND */
/*     COEFFICIENT MATRIX. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        MBW IS THE NUMBER OF COLUMNS OF THE ARRAY A USED TO STORE THE */
/*          BAND MATRIX.  IF THE MATRIX IS SYMMETRIC, MBW IS ITS (HALF) */
/*          BAND WIDTH, DENOTED MB AND DEFINED AS THE NUMBER OF ADJACENT 
*/
/*          DIAGONALS, INCLUDING THE PRINCIPAL DIAGONAL, REQUIRED TO */
/*          SPECIFY THE NON-ZERO PORTION OF THE LOWER TRIANGLE OF THE */
/*          MATRIX.  IF THE SUBROUTINE IS BEING USED TO SOLVE SYSTEMS */
/*          OF LINEAR EQUATIONS AND THE COEFFICIENT MATRIX IS NOT */
/*          SYMMETRIC, IT MUST HOWEVER HAVE THE SAME NUMBER OF ADJACENT */
/*          DIAGONALS ABOVE THE MAIN DIAGONAL AS BELOW, AND IN THIS */
/*          CASE, MBW=2*MB-1. */

/*        A CONTAINS THE LOWER TRIANGLE OF THE SYMMETRIC BAND INPUT */
/*          MATRIX STORED AS AN N BY MB ARRAY.  ITS LOWEST SUBDIAGONAL */
/*          IS STORED IN THE LAST N+1-MB POSITIONS OF THE FIRST COLUMN, */
/*          ITS NEXT SUBDIAGONAL IN THE LAST N+2-MB POSITIONS OF THE */
/*          SECOND COLUMN, FURTHER SUBDIAGONALS SIMILARLY, AND FINALLY */
/*          ITS PRINCIPAL DIAGONAL IN THE N POSITIONS OF COLUMN MB. */
/*          IF THE SUBROUTINE IS BEING USED TO SOLVE SYSTEMS OF LINEAR */
/*          EQUATIONS AND THE COEFFICIENT MATRIX IS NOT SYMMETRIC, A IS */
/*          N BY 2*MB-1 INSTEAD WITH LOWER TRIANGLE AS ABOVE AND WITH */
/*          ITS FIRST SUPERDIAGONAL STORED IN THE FIRST N-1 POSITIONS OF 
*/
/*          COLUMN MB+1, ITS SECOND SUPERDIAGONAL IN THE FIRST N-2 */
/*          POSITIONS OF COLUMN MB+2, FURTHER SUPERDIAGONALS SIMILARLY, */
/*          AND FINALLY ITS HIGHEST SUPERDIAGONAL IN THE FIRST N+1-MB */
/*          POSITIONS OF THE LAST COLUMN. */
/*          CONTENTS OF STORAGES NOT PART OF THE MATRIX ARE ARBITRARY. */

/*        E21 SPECIFIES THE ORDERING OF THE EIGENVALUES AND CONTAINS */
/*            0.0D0 IF THE EIGENVALUES ARE IN ASCENDING ORDER, OR */
/*            2.0D0 IF THE EIGENVALUES ARE IN DESCENDING ORDER. */
/*          IF THE SUBROUTINE IS BEING USED TO SOLVE SYSTEMS OF LINEAR */
/*          EQUATIONS, E21 SHOULD BE SET TO 1.0D0 IF THE COEFFICIENT */
/*          MATRIX IS SYMMETRIC AND TO -1.0D0 IF NOT. */

/*        M IS THE NUMBER OF SPECIFIED EIGENVALUES OR THE NUMBER OF */
/*          SYSTEMS OF LINEAR EQUATIONS. */

/*        W CONTAINS THE M EIGENVALUES IN ASCENDING OR DESCENDING ORDER. 
*/
/*          IF THE SUBROUTINE IS BEING USED TO SOLVE SYSTEMS OF LINEAR */
/*          EQUATIONS (A-W(R)*I)*X(R)=B(R), WHERE I IS THE IDENTITY */
/*          MATRIX, W(R) SHOULD BE SET ACCORDINGLY, FOR R=1,2,...,M. */

/*        Z CONTAINS THE CONSTANT MATRIX COLUMNS (B(R),R=1,2,...,M), IF */
/*          THE SUBROUTINE IS USED TO SOLVE SYSTEMS OF LINEAR EQUATIONS. 
*/

/*        NV MUST BE SET TO THE DIMENSION OF THE ARRAY PARAMETER RV */
/*          AS DECLARED IN THE CALLING PROGRAM DIMENSION STATEMENT. */

/*     ON OUTPUT */

/*        A AND W ARE UNALTERED. */

/*        Z CONTAINS THE ASSOCIATED SET OF ORTHOGONAL EIGENVECTORS. */
/*          ANY VECTOR WHICH FAILS TO CONVERGE IS SET TO ZERO.  IF THE */
/*          SUBROUTINE IS USED TO SOLVE SYSTEMS OF LINEAR EQUATIONS, */
/*          Z CONTAINS THE SOLUTION MATRIX COLUMNS (X(R),R=1,2,...,M). */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          -R         IF THE EIGENVECTOR CORRESPONDING TO THE R-TH */
/*                     EIGENVALUE FAILS TO CONVERGE, OR IF THE R-TH */
/*                     SYSTEM OF LINEAR EQUATIONS IS NEARLY SINGULAR. */

/*        RV AND RV6 ARE TEMPORARY STORAGE ARRAYS.  NOTE THAT RV IS */
/*          OF DIMENSION AT LEAST N*(2*MB-1).  IF THE SUBROUTINE */
/*          IS BEING USED TO SOLVE SYSTEMS OF LINEAR EQUATIONS, THE */
/*          DETERMINANT (UP TO SIGN) OF A-W(M)*I IS AVAILABLE, UPON */
/*          RETURN, AS THE PRODUCT OF THE FIRST N ELEMENTS OF RV. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv6;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    --rv;

    /* Function Body */
    *ierr = 0;
    if (*m == 0) {
        goto L1001;
    }
    mb = *mbw;
    if (*e21 < 0.) {
        mb = (*mbw + 1) / 2;
    }
    m1 = mb - 1;
    m21 = m1 + mb;
    order = 1. - abs(*e21);
/*     .......... FIND VECTORS BY INVERSE ITERATION .......... */
    i_1 = *m;
    for (r = 1; r <= i_1; ++r) {
        its = 1;
        x1 = w[r];
        if (r != 1) {
            goto L100;
        }
/*     .......... COMPUTE NORM OF MATRIX .......... */
        norm = 0.;

        i_2 = mb;
        for (j = 1; j <= i_2; ++j) {
            jj = mb + 1 - j;
            kj = jj + m1;
            ij = 1;
            v = 0.;

            i_3 = *n;
            for (i = jj; i <= i_3; ++i) {
                v += (d_1 = a[i + j * a_dim1], abs(d_1));
                if (*e21 >= 0.) {
                    goto L40;
                }
                v += (d_1 = a[ij + kj * a_dim1], abs(d_1));
                ++ij;
L40:
                ;
            }

            norm = max(norm,v);
/* L60: */
        }

        if (*e21 < 0.) {
            norm *= .5;
        }
/*     .......... EPS2 IS THE CRITERION FOR GROUPING, */
/*                EPS3 REPLACES ZERO PIVOTS AND EQUAL */
/*                ROOTS ARE MODIFIED BY EPS3, */
/*                EPS4 IS TAKEN VERY SMALL TO AVOID OVERFLOW .........
. */
        if (norm == 0.) {
            norm = 1.;
        }
        eps2 = norm * .001 * abs(order);
        eps3 = epslon_(&norm);
        uk = (doublereal) (*n);
        uk = sqrt(uk);
        eps4 = uk * eps3;
L80:
        group = 0;
        goto L120;
/*     .......... LOOK FOR CLOSE OR COINCIDENT ROOTS .......... */
L100:
        if ((d_1 = x1 - x0, abs(d_1)) >= eps2) {
            goto L80;
        }
        ++group;
        if (order * (x1 - x0) <= 0.) {
            x1 = x0 + order * eps3;
        }
/*     .......... EXPAND MATRIX, SUBTRACT EIGENVALUE, */
/*                AND INITIALIZE VECTOR .......... */
L120:
        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* Computing MIN */
            i_3 = 0, i_4 = i - m1;
            ij = i + min(i_3,i_4) * *n;
            kj = ij + mb * *n;
            ij1 = kj + m1 * *n;
            if (m1 == 0) {
                goto L180;
            }

            i_3 = m1;
            for (j = 1; j <= i_3; ++j) {
                if (ij > m1) {
                    goto L125;
                }
                if (ij > 0) {
                    goto L130;
                }
                rv[ij1] = 0.;
                ij1 += *n;
                goto L130;
L125:
                rv[ij] = a[i + j * a_dim1];
L130:
                ij += *n;
                ii = i + j;
                if (ii > *n) {
                    goto L150;
                }
                jj = mb - j;
                if (*e21 >= 0.) {
                    goto L140;
                }
                ii = i;
                jj = mb + j;
L140:
                rv[kj] = a[ii + jj * a_dim1];
                kj += *n;
L150:
                ;
            }

L180:
            rv[ij] = a[i + mb * a_dim1] - x1;
            rv6[i] = eps4;
            if (order == 0.) {
                rv6[i] = z[i + r * z_dim1];
            }
/* L200: */
        }

        if (m1 == 0) {
            goto L600;
        }
/*     .......... ELIMINATION WITH INTERCHANGES .......... */
        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            ii = i + 1;
/* Computing MIN */
            i_3 = i + m1 - 1;
            maxk = min(i_3,*n);
/* Computing MIN */
            i_3 = *n - i, i_4 = m21 - 2;
            maxj = min(i_3,i_4) * *n;

            i_3 = maxk;
            for (k = i; k <= i_3; ++k) {
                kj1 = k;
                j = kj1 + *n;
                jj = j + maxj;

                i_4 = jj;
                i_5 = *n;
                for (kj = j; i_5 < 0 ? kj >= i_4 : kj <= i_4; kj += i_5) {
                    rv[kj1] = rv[kj];
                    kj1 = kj;
/* L340: */
                }

                rv[kj1] = 0.;
/* L360: */
            }

            if (i == *n) {
                goto L580;
            }
            u = 0.;
/* Computing MIN */
            i_3 = i + m1;
            maxk = min(i_3,*n);
/* Computing MIN */
            i_3 = *n - ii, i_5 = m21 - 2;
            maxj = min(i_3,i_5) * *n;

            i_3 = maxk;
            for (j = i; j <= i_3; ++j) {
                if ((d_1 = rv[j], abs(d_1)) < abs(u)) {
                    goto L450;
                }
                u = rv[j];
                k = j;
L450:
                ;
            }

            j = i + *n;
            jj = j + maxj;
            if (k == i) {
                goto L520;
            }
            kj = k;

            i_3 = jj;
            i_5 = *n;
            for (ij = i; i_5 < 0 ? ij >= i_3 : ij <= i_3; ij += i_5) {
                v = rv[ij];
                rv[ij] = rv[kj];
                rv[kj] = v;
                kj += *n;
/* L500: */
            }

            if (order != 0.) {
                goto L520;
            }
            v = rv6[i];
            rv6[i] = rv6[k];
            rv6[k] = v;
L520:
            if (u == 0.) {
                goto L580;
            }

            i_5 = maxk;
            for (k = ii; k <= i_5; ++k) {
                v = rv[k] / u;
                kj = k;

                i_3 = jj;
                i_4 = *n;
                for (ij = j; i_4 < 0 ? ij >= i_3 : ij <= i_3; ij += i_4) {
                    kj += *n;
                    rv[kj] -= v * rv[ij];
/* L540: */
                }

                if (order == 0.) {
                    rv6[k] -= v * rv6[i];
                }
/* L560: */
            }

L580:
            ;
        }
/*     .......... BACK SUBSTITUTION */
/*                FOR I=N STEP -1 UNTIL 1 DO -- .......... */
L600:
        i_2 = *n;
        for (ii = 1; ii <= i_2; ++ii) {
            i = *n + 1 - ii;
            maxj = min(ii,m21);
            if (maxj == 1) {
                goto L620;
            }
            ij1 = i;
            j = ij1 + *n;
            jj = j + (maxj - 2) * *n;

            i_5 = jj;
            i_4 = *n;
            for (ij = j; i_4 < 0 ? ij >= i_5 : ij <= i_5; ij += i_4) {
                ++ij1;
                rv6[i] -= rv[ij] * rv6[ij1];
/* L610: */
            }

L620:
            v = rv[i];
            if (abs(v) >= eps3) {
                goto L625;
            }
/*     .......... SET ERROR -- NEARLY SINGULAR LINEAR SYSTEM .....
..... */
            if (order == 0.) {
                *ierr = -r;
            }
            v = d_sign(&eps3, &v);
L625:
            rv6[i] /= v;
/* L630: */
        }

        xu = 1.;
        if (order == 0.) {
            goto L870;
        }
/*     .......... ORTHOGONALIZE WITH RESPECT TO PREVIOUS */
/*                MEMBERS OF GROUP .......... */
        if (group == 0) {
            goto L700;
        }

        i_2 = group;
        for (jj = 1; jj <= i_2; ++jj) {
            j = r - group - 1 + jj;
            xu = 0.;

            i_4 = *n;
            for (i = 1; i <= i_4; ++i) {
/* L640: */
                xu += rv6[i] * z[i + j * z_dim1];
            }

            i_4 = *n;
            for (i = 1; i <= i_4; ++i) {
/* L660: */
                rv6[i] -= xu * z[i + j * z_dim1];
            }

/* L680: */
        }

L700:
        norm = 0.;

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L720: */
            norm += (d_1 = rv6[i], abs(d_1));
        }

        if (norm >= .1) {
            goto L840;
        }
/*     .......... IN-LINE PROCEDURE FOR CHOOSING */
/*                A NEW STARTING VECTOR .......... */
        if (its >= *n) {
            goto L830;
        }
        ++its;
        xu = eps4 / (uk + 1.);
        rv6[1] = eps4;

        i_2 = *n;
        for (i = 2; i <= i_2; ++i) {
/* L760: */
            rv6[i] = xu;
        }

        rv6[its] -= eps4 * uk;
        goto L600;
/*     .......... SET ERROR -- NON-CONVERGED EIGENVECTOR .......... */
L830:
        *ierr = -r;
        xu = 0.;
        goto L870;
/*     .......... NORMALIZE SO THAT SUM OF SQUARES IS */
/*                1 AND EXPAND TO FULL ORDER .......... */
L840:
        u = 0.;

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L860: */
            u = pythag_(&u, &rv6[i]);
        }

        xu = 1. / u;

L870:
        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L900: */
            z[i + r * z_dim1] = rv6[i] * xu;
        }

        x0 = x1;
/* L920: */
    }

L1001:
    return 0;
} /* bandv_ */

/* Subroutine */ int bisect_(integer *n, doublereal *eps1, doublereal *d, 
        doublereal *e, doublereal *e2, doublereal *lb, doublereal *ub, 
        integer *mm, integer *m, doublereal *w, integer *ind, integer *ierr, 
        doublereal *rv4, doublereal *rv5)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2, d_3;

    /* Local variables */
    static integer i, j, k, l, p, q, r, s;
    static doublereal u, v;
    static integer m1, m2;
    static doublereal t1, t2, x0, x1;
    static integer ii;
    static doublereal xu;
    extern doublereal epslon_(doublereal *);
    static integer isturm, tag;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE BISECTION TECHNIQUE */
/*     IN THE ALGOL PROCEDURE TRISTURM BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 418-439(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVALUES OF A TRIDIAGONAL */
/*     SYMMETRIC MATRIX WHICH LIE IN A SPECIFIED INTERVAL, */
/*     USING BISECTION. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        EPS1 IS AN ABSOLUTE ERROR TOLERANCE FOR THE COMPUTED */
/*          EIGENVALUES.  IF THE INPUT EPS1 IS NON-POSITIVE, */
/*          IT IS RESET FOR EACH SUBMATRIX TO A DEFAULT VALUE, */
/*          NAMELY, MINUS THE PRODUCT OF THE RELATIVE MACHINE */
/*          PRECISION AND THE 1-NORM OF THE SUBMATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2(1) IS ARBITRARY. */

/*        LB AND UB DEFINE THE INTERVAL TO BE SEARCHED FOR EIGENVALUES. */
/*          IF LB IS NOT LESS THAN UB, NO EIGENVALUES WILL BE FOUND. */

/*        MM SHOULD BE SET TO AN UPPER BOUND FOR THE NUMBER OF */
/*          EIGENVALUES IN THE INTERVAL.  WARNING. IF MORE THAN */
/*          MM EIGENVALUES ARE DETERMINED TO LIE IN THE INTERVAL, */
/*          AN ERROR RETURN IS MADE WITH NO EIGENVALUES FOUND. */

/*     ON OUTPUT */

/*        EPS1 IS UNALTERED UNLESS IT HAS BEEN RESET TO ITS */
/*          (LAST) DEFAULT VALUE. */

/*        D AND E ARE UNALTERED. */

/*        ELEMENTS OF E2, CORRESPONDING TO ELEMENTS OF E REGARDED */
/*          AS NEGLIGIBLE, HAVE BEEN REPLACED BY ZERO CAUSING THE */
/*          MATRIX TO SPLIT INTO A DIRECT SUM OF SUBMATRICES. */
/*          E2(1) IS ALSO SET TO ZERO. */

/*        M IS THE NUMBER OF EIGENVALUES DETERMINED TO LIE IN (LB,UB). */

/*        W CONTAINS THE M EIGENVALUES IN ASCENDING ORDER. */

/*        IND CONTAINS IN ITS FIRST M POSITIONS THE SUBMATRIX INDICES */
/*          ASSOCIATED WITH THE CORRESPONDING EIGENVALUES IN W -- */
/*          1 FOR EIGENVALUES BELONGING TO THE FIRST SUBMATRIX FROM */
/*          THE TOP, 2 FOR THOSE BELONGING TO THE SECOND SUBMATRIX, ETC.. 
*/

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          3*N+1      IF M EXCEEDS MM. */

/*        RV4 AND RV5 ARE TEMPORARY STORAGE ARRAYS. */

/*     THE ALGOL PROCEDURE STURMCNT CONTAINED IN TRISTURM */
/*     APPEARS IN BISECT IN-LINE. */

/*     NOTE THAT SUBROUTINE TQL1 OR IMTQL1 IS GENERALLY FASTER THAN */
/*     BISECT, IF MORE THAN N/4 EIGENVALUES ARE TO BE FOUND. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv5;
    --rv4;
    --e2;
    --e;
    --d;
    --ind;
    --w;

    /* Function Body */
    *ierr = 0;
    tag = 0;
    t1 = *lb;
    t2 = *ub;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ENTRIES .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i == 1) {
            goto L20;
        }
        tst1 = (d_1 = d[i], abs(d_1)) + (d_2 = d[i - 1], abs(d_2));
        tst2 = tst1 + (d_1 = e[i], abs(d_1));
        if (tst2 > tst1) {
            goto L40;
        }
L20:
        e2[i] = 0.;
L40:
        ;
    }
/*     .......... DETERMINE THE NUMBER OF EIGENVALUES */
/*                IN THE INTERVAL .......... */
    p = 1;
    q = *n;
    x1 = *ub;
    isturm = 1;
    goto L320;
L60:
    *m = s;
    x1 = *lb;
    isturm = 2;
    goto L320;
L80:
    *m -= s;
    if (*m > *mm) {
        goto L980;
    }
    q = 0;
    r = 0;
/*     .......... ESTABLISH AND PROCESS NEXT SUBMATRIX, REFINING */
/*                INTERVAL BY THE GERSCHGORIN BOUNDS .......... */
L100:
    if (r == *m) {
        goto L1001;
    }
    ++tag;
    p = q + 1;
    xu = d[p];
    x0 = d[p];
    u = 0.;

    i_1 = *n;
    for (q = p; q <= i_1; ++q) {
        x1 = u;
        u = 0.;
        v = 0.;
        if (q == *n) {
            goto L110;
        }
        u = (d_1 = e[q + 1], abs(d_1));
        v = e2[q + 1];
L110:
/* Computing MIN */
        d_1 = d[q] - (x1 + u);
        xu = min(d_1,xu);
/* Computing MAX */
        d_1 = d[q] + (x1 + u);
        x0 = max(d_1,x0);
        if (v == 0.) {
            goto L140;
        }
/* L120: */
    }

L140:
/* Computing MAX */
    d_2 = abs(xu), d_3 = abs(x0);
    d_1 = max(d_2,d_3);
    x1 = epslon_(&d_1);
    if (*eps1 <= 0.) {
        *eps1 = -x1;
    }
    if (p != q) {
        goto L180;
    }
/*     .......... CHECK FOR ISOLATED ROOT WITHIN INTERVAL .......... */
    if (t1 > d[p] || d[p] >= t2) {
        goto L940;
    }
    m1 = p;
    m2 = p;
    rv5[p] = d[p];
    goto L900;
L180:
    x1 *= q - p + 1;
/* Computing MAX */
    d_1 = t1, d_2 = xu - x1;
    *lb = max(d_1,d_2);
/* Computing MIN */
    d_1 = t2, d_2 = x0 + x1;
    *ub = min(d_1,d_2);
    x1 = *lb;
    isturm = 3;
    goto L320;
L200:
    m1 = s + 1;
    x1 = *ub;
    isturm = 4;
    goto L320;
L220:
    m2 = s;
    if (m1 > m2) {
        goto L940;
    }
/*     .......... FIND ROOTS BY BISECTION .......... */
    x0 = *ub;
    isturm = 5;

    i_1 = m2;
    for (i = m1; i <= i_1; ++i) {
        rv5[i] = *ub;
        rv4[i] = *lb;
/* L240: */
    }
/*     .......... LOOP FOR K-TH EIGENVALUE */
/*                FOR K=M2 STEP -1 UNTIL M1 DO -- */
/*                (-DO- NOT USED TO LEGALIZE -COMPUTED GO TO-) .......... 
*/
    k = m2;
L250:
    xu = *lb;
/*     .......... FOR I=K STEP -1 UNTIL M1 DO -- .......... */
    i_1 = k;
    for (ii = m1; ii <= i_1; ++ii) {
        i = m1 + k - ii;
        if (xu >= rv4[i]) {
            goto L260;
        }
        xu = rv4[i];
        goto L280;
L260:
        ;
    }

L280:
    if (x0 > rv5[k]) {
        x0 = rv5[k];
    }
/*     .......... NEXT BISECTION STEP .......... */
L300:
    x1 = (xu + x0) * .5;
    if (x0 - xu <= abs(*eps1)) {
        goto L420;
    }
    tst1 = (abs(xu) + abs(x0)) * 2.;
    tst2 = tst1 + (x0 - xu);
    if (tst2 == tst1) {
        goto L420;
    }
/*     .......... IN-LINE PROCEDURE FOR STURM SEQUENCE .......... */
L320:
    s = p - 1;
    u = 1.;

    i_1 = q;
    for (i = p; i <= i_1; ++i) {
        if (u != 0.) {
            goto L325;
        }
        v = (d_1 = e[i], abs(d_1)) / epslon_(&c_b141);
        if (e2[i] == 0.) {
            v = 0.;
        }
        goto L330;
L325:
        v = e2[i] / u;
L330:
        u = d[i] - x1 - v;
        if (u < 0.) {
            ++s;
        }
/* L340: */
    }

    switch (isturm) {
        case 1:  goto L60;
        case 2:  goto L80;
        case 3:  goto L200;
        case 4:  goto L220;
        case 5:  goto L360;
    }
/*     .......... REFINE INTERVALS .......... */
L360:
    if (s >= k) {
        goto L400;
    }
    xu = x1;
    if (s >= m1) {
        goto L380;
    }
    rv4[m1] = x1;
    goto L300;
L380:
    rv4[s + 1] = x1;
    if (rv5[s] > x1) {
        rv5[s] = x1;
    }
    goto L300;
L400:
    x0 = x1;
    goto L300;
/*     .......... K-TH EIGENVALUE FOUND .......... */
L420:
    rv5[k] = x1;
    --k;
    if (k >= m1) {
        goto L250;
    }
/*     .......... ORDER EIGENVALUES TAGGED WITH THEIR */
/*                SUBMATRIX ASSOCIATIONS .......... */
L900:
    s = r;
    r = r + m2 - m1 + 1;
    j = 1;
    k = m1;

    i_1 = r;
    for (l = 1; l <= i_1; ++l) {
        if (j > s) {
            goto L910;
        }
        if (k > m2) {
            goto L940;
        }
        if (rv5[k] >= w[l]) {
            goto L915;
        }

        i_2 = s;
        for (ii = j; ii <= i_2; ++ii) {
            i = l + s - ii;
            w[i + 1] = w[i];
            ind[i + 1] = ind[i];
/* L905: */
        }

L910:
        w[l] = rv5[k];
        ind[l] = tag;
        ++k;
        goto L920;
L915:
        ++j;
L920:
        ;
    }

L940:
    if (q < *n) {
        goto L100;
    }
    goto L1001;
/*     .......... SET ERROR -- UNDERESTIMATE OF NUMBER OF */
/*                EIGENVALUES IN INTERVAL .......... */
L980:
    *ierr = *n * 3 + 1;
L1001:
    *lb = t1;
    *ub = t2;
    return 0;
} /* bisect_ */

/* Subroutine */ int bqr_(integer *nm, integer *n, integer *mb, doublereal *a,
         doublereal *t, doublereal *r, integer *ierr, integer */*nv*/, doublereal 
        *rv)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;
    doublereal d_1;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *), sqrt(doublereal);

    /* Local variables */
    static doublereal f, g;
    static integer i, j, k, l, m;
    static doublereal q, s, scale;
    static integer imult, m1, m2, m3, m4, m21, m31, ii, ik, jk, kj, jm, kk, 
            km, ll, mk, mn, ni, mz;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer kj1, its;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE BQR, */
/*     NUM. MATH. 16, 85-92(1970) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL II-LINEAR ALGEBRA, 266-272(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUE OF SMALLEST (USUALLY) */
/*     MAGNITUDE OF A REAL SYMMETRIC BAND MATRIX USING THE */
/*     QR ALGORITHM WITH SHIFTS OF ORIGIN.  CONSECUTIVE CALLS */
/*     CAN BE MADE TO FIND FURTHER EIGENVALUES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        MB IS THE (HALF) BAND WIDTH OF THE MATRIX, DEFINED AS THE */
/*          NUMBER OF ADJACENT DIAGONALS, INCLUDING THE PRINCIPAL */
/*          DIAGONAL, REQUIRED TO SPECIFY THE NON-ZERO PORTION OF THE */
/*          LOWER TRIANGLE OF THE MATRIX. */

/*        A CONTAINS THE LOWER TRIANGLE OF THE SYMMETRIC BAND INPUT */
/*          MATRIX STORED AS AN N BY MB ARRAY.  ITS LOWEST SUBDIAGONAL */
/*          IS STORED IN THE LAST N+1-MB POSITIONS OF THE FIRST COLUMN, */
/*          ITS NEXT SUBDIAGONAL IN THE LAST N+2-MB POSITIONS OF THE */
/*          SECOND COLUMN, FURTHER SUBDIAGONALS SIMILARLY, AND FINALLY */
/*          ITS PRINCIPAL DIAGONAL IN THE N POSITIONS OF THE LAST COLUMN. 
*/
/*          CONTENTS OF STORAGES NOT PART OF THE MATRIX ARE ARBITRARY. */
/*          ON A SUBSEQUENT CALL, ITS OUTPUT CONTENTS FROM THE PREVIOUS */
/*          CALL SHOULD BE PASSED. */

/*        T SPECIFIES THE SHIFT (OF EIGENVALUES) APPLIED TO THE DIAGONAL 
*/
/*          OF A IN FORMING THE INPUT MATRIX. WHAT IS ACTUALLY DETERMINED 
*/
/*          IS THE EIGENVALUE OF A+TI (I IS THE IDENTITY MATRIX) NEAREST 
*/
/*          TO T.  ON A SUBSEQUENT CALL, THE OUTPUT VALUE OF T FROM THE */
/*          PREVIOUS CALL SHOULD BE PASSED IF THE NEXT NEAREST EIGENVALUE 
*/
/*          IS SOUGHT. */

/*        R SHOULD BE SPECIFIED AS ZERO ON THE FIRST CALL, AND AS ITS */
/*          OUTPUT VALUE FROM THE PREVIOUS CALL ON A SUBSEQUENT CALL. */
/*          IT IS USED TO DETERMINE WHEN THE LAST ROW AND COLUMN OF */
/*          THE TRANSFORMED BAND MATRIX CAN BE REGARDED AS NEGLIGIBLE. */

/*        NV MUST BE SET TO THE DIMENSION OF THE ARRAY PARAMETER RV */
/*          AS DECLARED IN THE CALLING PROGRAM DIMENSION STATEMENT. */

/*     ON OUTPUT */

/*        A CONTAINS THE TRANSFORMED BAND MATRIX.  THE MATRIX A+TI */
/*          DERIVED FROM THE OUTPUT PARAMETERS IS SIMILAR TO THE */
/*          INPUT A+TI TO WITHIN ROUNDING ERRORS.  ITS LAST ROW AND */
/*          COLUMN ARE NULL (IF IERR IS ZERO). */

/*        T CONTAINS THE COMPUTED EIGENVALUE OF A+TI (IF IERR IS ZERO). */

/*        R CONTAINS THE MAXIMUM OF ITS INPUT VALUE AND THE NORM OF THE */
/*          LAST COLUMN OF THE INPUT MATRIX A. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          N          IF THE EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*        RV IS A TEMPORARY STORAGE ARRAY OF DIMENSION AT LEAST */
/*          (2*MB**2+4*MB-3).  THE FIRST (3*MB-2) LOCATIONS CORRESPOND */
/*          TO THE ALGOL ARRAY B, THE NEXT (2*MB-1) LOCATIONS CORRESPOND 
*/
/*          TO THE ALGOL ARRAY H, AND THE FINAL (2*MB**2-MB) LOCATIONS */
/*          CORRESPOND TO THE MB BY (2*MB-1) ALGOL ARRAY U. */

/*     NOTE. FOR A SUBSEQUENT CALL, N SHOULD BE REPLACED BY N-1, BUT */
/*     MB SHOULD NOT BE ALTERED EVEN WHEN IT EXCEEDS THE CURRENT N. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    --rv;

    /* Function Body */
    *ierr = 0;
    m1 = min(*mb,*n);
    m = m1 - 1;
    m2 = m + m;
    m21 = m2 + 1;
    m3 = m21 + m;
    m31 = m3 + 1;
    m4 = m31 + m2;
    mn = m + *n;
    mz = *mb - m1;
    its = 0;
/*     .......... TEST FOR CONVERGENCE .......... */
L40:
    g = a[*n + *mb * a_dim1];
    if (m == 0) {
        goto L360;
    }
    f = 0.;

    i_1 = m;
    for (k = 1; k <= i_1; ++k) {
        mk = k + mz;
        f += (d_1 = a[*n + mk * a_dim1], abs(d_1));
/* L50: */
    }

    if (its == 0 && f > *r) {
        *r = f;
    }
    tst1 = *r;
    tst2 = tst1 + f;
    if (tst2 <= tst1) {
        goto L360;
    }
    if (its == 30) {
        goto L1000;
    }
    ++its;
/*     .......... FORM SHIFT FROM BOTTOM 2 BY 2 MINOR .......... */
    if (f > *r * .25 && its < 5) {
        goto L90;
    }
    f = a[*n + (*mb - 1) * a_dim1];
    if (f == 0.) {
        goto L70;
    }
    q = (a[*n - 1 + *mb * a_dim1] - g) / (f * 2.);
    s = pythag_(&q, &c_b141);
    g -= f / (q + d_sign(&s, &q));
L70:
    *t += g;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L80: */
        a[i + *mb * a_dim1] -= g;
    }

L90:
    i_1 = m4;
    for (k = m31; k <= i_1; ++k) {
/* L100: */
        rv[k] = 0.;
    }

    i_1 = mn;
    for (ii = 1; ii <= i_1; ++ii) {
        i = ii - m;
        ni = *n - ii;
        if (ni < 0) {
            goto L230;
        }
/*     .......... FORM COLUMN OF SHIFTED MATRIX A-G*I .......... */
/* Computing MAX */
        i_2 = 1, i_3 = 2 - i;
        l = max(i_2,i_3);

        i_2 = m3;
        for (k = 1; k <= i_2; ++k) {
/* L110: */
            rv[k] = 0.;
        }

        i_2 = m1;
        for (k = l; k <= i_2; ++k) {
            km = k + m;
            mk = k + mz;
            rv[km] = a[ii + mk * a_dim1];
/* L120: */
        }

        ll = min(m,ni);
        if (ll == 0) {
            goto L135;
        }

        i_2 = ll;
        for (k = 1; k <= i_2; ++k) {
            km = k + m21;
            ik = ii + k;
            mk = *mb - k;
            rv[km] = a[ik + mk * a_dim1];
/* L130: */
        }
/*     .......... PRE-MULTIPLY WITH HOUSEHOLDER REFLECTIONS ..........
 */
L135:
        ll = m2;
        imult = 0;
/*     .......... MULTIPLICATION PROCEDURE .......... */
L140:
        kj = m4 - m1;

        i_2 = ll;
        for (j = 1; j <= i_2; ++j) {
            kj += m1;
            jm = j + m3;
            if (rv[jm] == 0.) {
                goto L170;
            }
            f = 0.;

            i_3 = m1;
            for (k = 1; k <= i_3; ++k) {
                ++kj;
                jk = j + k - 1;
                f += rv[kj] * rv[jk];
/* L150: */
            }

            f /= rv[jm];
            kj -= m1;

            i_3 = m1;
            for (k = 1; k <= i_3; ++k) {
                ++kj;
                jk = j + k - 1;
                rv[jk] -= rv[kj] * f;
/* L160: */
            }

            kj -= m1;
L170:
            ;
        }

        if (imult != 0) {
            goto L280;
        }
/*     .......... HOUSEHOLDER REFLECTION .......... */
        f = rv[m21];
        s = 0.;
        rv[m4] = 0.;
        scale = 0.;

        i_2 = m3;
        for (k = m21; k <= i_2; ++k) {
/* L180: */
            scale += (d_1 = rv[k], abs(d_1));
        }

        if (scale == 0.) {
            goto L210;
        }

        i_2 = m3;
        for (k = m21; k <= i_2; ++k) {
/* L190: */
/* Computing 2nd power */
            d_1 = rv[k] / scale;
            s += d_1 * d_1;
        }

        s = scale * scale * s;
        d_1 = sqrt(s);
        g = -d_sign(&d_1, &f);
        rv[m21] = g;
        rv[m4] = s - f * g;
        kj = m4 + m2 * m1 + 1;
        rv[kj] = f - g;

        i_2 = m1;
        for (k = 2; k <= i_2; ++k) {
            ++kj;
            km = k + m2;
            rv[kj] = rv[km];
/* L200: */
        }
/*     .......... SAVE COLUMN OF TRIANGULAR FACTOR R .......... */
L210:
        i_2 = m1;
        for (k = l; k <= i_2; ++k) {
            km = k + m;
            mk = k + mz;
            a[ii + mk * a_dim1] = rv[km];
/* L220: */
        }

L230:
/* Computing MAX */
        i_2 = 1, i_3 = m1 + 1 - i;
        l = max(i_2,i_3);
        if (i <= 0) {
            goto L300;
        }
/*     .......... PERFORM ADDITIONAL STEPS .......... */
        i_2 = m21;
        for (k = 1; k <= i_2; ++k) {
/* L240: */
            rv[k] = 0.;
        }

/* Computing MIN */
        i_2 = m1, i_3 = ni + m1;
        ll = min(i_2,i_3);
/*     .......... GET ROW OF TRIANGULAR FACTOR R .......... */
        i_2 = ll;
        for (kk = 1; kk <= i_2; ++kk) {
            k = kk - 1;
            km = k + m1;
            ik = i + k;
            mk = *mb - k;
            rv[km] = a[ik + mk * a_dim1];
/* L250: */
        }
/*     .......... POST-MULTIPLY WITH HOUSEHOLDER REFLECTIONS .........
. */
        ll = m1;
        imult = 1;
        goto L140;
/*     .......... STORE COLUMN OF NEW A MATRIX .......... */
L280:
        i_2 = m1;
        for (k = l; k <= i_2; ++k) {
            mk = k + mz;
            a[i + mk * a_dim1] = rv[k];
/* L290: */
        }
/*     .......... UPDATE HOUSEHOLDER REFLECTIONS .......... */
L300:
        if (l > 1) {
            --l;
        }
        kj1 = m4 + l * m1;

        i_2 = m2;
        for (j = l; j <= i_2; ++j) {
            jm = j + m3;
            rv[jm] = rv[jm + 1];

            i_3 = m1;
            for (k = 1; k <= i_3; ++k) {
                ++kj1;
                kj = kj1 - m1;
                rv[kj] = rv[kj1];
/* L320: */
            }
        }

/* L350: */
    }

    goto L40;
/*     .......... CONVERGENCE .......... */
L360:
    *t += g;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L380: */
        a[i + *mb * a_dim1] -= g;
    }

    i_1 = m1;
    for (k = 1; k <= i_1; ++k) {
        mk = k + mz;
        a[*n + mk * a_dim1] = 0.;
/* L400: */
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = *n;
L1001:
    return 0;
} /* bqr_ */

/* Subroutine */ int cbabk2_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *scale, integer *m, doublereal *zr, doublereal *zi)
{
    /* System generated locals */
    integer zr_dim1, zr_offset, zi_dim1, zi_offset, i_1, i_2;

    /* Local variables */
    static integer i, j, k;
    static doublereal s;
    static integer ii;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE */
/*     CBABK2, WHICH IS A COMPLEX VERSION OF BALBAK, */
/*     NUM. MATH. 13, 293-304(1969) BY PARLETT AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 315-326(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A COMPLEX GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     BALANCED MATRIX DETERMINED BY  CBAL. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY  CBAL. */

/*        SCALE CONTAINS INFORMATION DETERMINING THE PERMUTATIONS */
/*          AND SCALING FACTORS USED BY  CBAL. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVECTORS TO BE */
/*          BACK TRANSFORMED IN THEIR FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE TRANSFORMED EIGENVECTORS */
/*          IN THEIR FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --scale;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    if (*igh == *low) {
        goto L120;
    }

    i_1 = *igh;
    for (i = *low; i <= i_1; ++i) {
        s = scale[i];
/*     .......... LEFT HAND EIGENVECTORS ARE BACK TRANSFORMED */
/*                IF THE FOREGOING STATEMENT IS REPLACED BY */
/*                S=1.0D0/SCALE(I). .......... */
        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            zr[i + j * zr_dim1] *= s;
            zi[i + j * zi_dim1] *= s;
/* L100: */
        }

/* L110: */
    }
/*     .......... FOR I=LOW-1 STEP -1 UNTIL 1, */
/*                IGH+1 STEP 1 UNTIL N DO -- .......... */
L120:
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = ii;
        if (i >= *low && i <= *igh) {
            goto L140;
        }
        if (i < *low) {
            i = *low - ii;
        }
        k = (integer) scale[i];
        if (k == i) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            s = zr[i + j * zr_dim1];
            zr[i + j * zr_dim1] = zr[k + j * zr_dim1];
            zr[k + j * zr_dim1] = s;
            s = zi[i + j * zi_dim1];
            zi[i + j * zi_dim1] = zi[k + j * zi_dim1];
            zi[k + j * zi_dim1] = s;
/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* cbabk2_ */

/* Subroutine */ int cbal_(integer *nm, integer *n, doublereal *ar, 
        doublereal *ai, integer *low, integer *igh, doublereal *scale)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, i_1, i_2;
    doublereal d_1, d_2;

    /* Local variables */
    static integer iexc;
    static doublereal c, f, g;
    static integer i, j, k, l, m;
    static doublereal r, s, radix, b2;
    static integer jj;
    static logical noconv;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE */
/*     CBALANCE, WHICH IS A COMPLEX VERSION OF BALANCE, */
/*     NUM. MATH. 13, 293-304(1969) BY PARLETT AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 315-326(1971). */

/*     THIS SUBROUTINE BALANCES A COMPLEX MATRIX AND ISOLATES */
/*     EIGENVALUES WHENEVER POSSIBLE. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX MATRIX TO BE BALANCED. */

/*     ON OUTPUT */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE BALANCED MATRIX. */

/*        LOW AND IGH ARE TWO INTEGERS SUCH THAT AR(I,J) AND AI(I,J) */
/*          ARE EQUAL TO ZERO IF */
/*           (1) I IS GREATER THAN J AND */
/*           (2) J=1,...,LOW-1 OR I=IGH+1,...,N. */

/*        SCALE CONTAINS INFORMATION DETERMINING THE */
/*           PERMUTATIONS AND SCALING FACTORS USED. */

/*     SUPPOSE THAT THE PRINCIPAL SUBMATRIX IN ROWS LOW THROUGH IGH */
/*     HAS BEEN BALANCED, THAT P(J) DENOTES THE INDEX INTERCHANGED */
/*     WITH J DURING THE PERMUTATION STEP, AND THAT THE ELEMENTS */
/*     OF THE DIAGONAL MATRIX USED ARE DENOTED BY D(I,J).  THEN */
/*        SCALE(J) = P(J),    FOR J = 1,...,LOW-1 */
/*                 = D(J,J)       J = LOW,...,IGH */
/*                 = P(J)         J = IGH+1,...,N. */
/*     THE ORDER IN WHICH THE INTERCHANGES ARE MADE IS N TO IGH+1, */
/*     THEN 1 TO LOW-1. */

/*     NOTE THAT 1 IS RETURNED FOR IGH IF IGH IS ZERO FORMALLY. */

/*     THE ALGOL PROCEDURE EXC CONTAINED IN CBALANCE APPEARS IN */
/*     CBAL  IN LINE.  (NOTE THAT THE ALGOL ROLES OF IDENTIFIERS */
/*     K,L HAVE BEEN REVERSED.) */

/*     ARITHMETIC IS REAL THROUGHOUT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --scale;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;

    /* Function Body */
    radix = 16.;

    b2 = radix * radix;
    k = 1;
    l = *n;
    goto L100;
/*     .......... IN-LINE PROCEDURE FOR ROW AND */
/*                COLUMN EXCHANGE .......... */
L20:
    scale[m] = (doublereal) j;
    if (j == m) {
        goto L50;
    }

    i_1 = l;
    for (i = 1; i <= i_1; ++i) {
        f = ar[i + j * ar_dim1];
        ar[i + j * ar_dim1] = ar[i + m * ar_dim1];
        ar[i + m * ar_dim1] = f;
        f = ai[i + j * ai_dim1];
        ai[i + j * ai_dim1] = ai[i + m * ai_dim1];
        ai[i + m * ai_dim1] = f;
/* L30: */
    }

    i_1 = *n;
    for (i = k; i <= i_1; ++i) {
        f = ar[j + i * ar_dim1];
        ar[j + i * ar_dim1] = ar[m + i * ar_dim1];
        ar[m + i * ar_dim1] = f;
        f = ai[j + i * ai_dim1];
        ai[j + i * ai_dim1] = ai[m + i * ai_dim1];
        ai[m + i * ai_dim1] = f;
/* L40: */
    }

L50:
    switch (iexc) {
        case 1:  goto L80;
        case 2:  goto L130;
    }
/*     .......... SEARCH FOR ROWS ISOLATING AN EIGENVALUE */
/*                AND PUSH THEM DOWN .......... */
L80:
    if (l == 1) {
        goto L280;
    }
    --l;
/*     .......... FOR J=L STEP -1 UNTIL 1 DO -- .......... */
L100:
    i_1 = l;
    for (jj = 1; jj <= i_1; ++jj) {
        j = l + 1 - jj;

        i_2 = l;
        for (i = 1; i <= i_2; ++i) {
            if (i == j) {
                goto L110;
            }
            if (ar[j + i * ar_dim1] != 0. || ai[j + i * ai_dim1] != 0.) {
                goto L120;
            }
L110:
            ;
        }

        m = l;
        iexc = 1;
        goto L20;
L120:
        ;
    }

    goto L140;
/*     .......... SEARCH FOR COLUMNS ISOLATING AN EIGENVALUE */
/*                AND PUSH THEM LEFT .......... */
L130:
    ++k;

L140:
    i_1 = l;
    for (j = k; j <= i_1; ++j) {

        i_2 = l;
        for (i = k; i <= i_2; ++i) {
            if (i == j) {
                goto L150;
            }
            if (ar[i + j * ar_dim1] != 0. || ai[i + j * ai_dim1] != 0.) {
                goto L170;
            }
L150:
            ;
        }

        m = k;
        iexc = 2;
        goto L20;
L170:
        ;
    }
/*     .......... NOW BALANCE THE SUBMATRIX IN ROWS K TO L .......... */
    i_1 = l;
    for (i = k; i <= i_1; ++i) {
/* L180: */
        scale[i] = 1.;
    }
/*     .......... ITERATIVE LOOP FOR NORM REDUCTION .......... */
L190:
    noconv = FALSE_;

    i_1 = l;
    for (i = k; i <= i_1; ++i) {
        c = 0.;
        r = 0.;

        i_2 = l;
        for (j = k; j <= i_2; ++j) {
            if (j == i) {
                goto L200;
            }
            c = c + (d_1 = ar[j + i * ar_dim1], abs(d_1)) + (d_2 = ai[j + 
                    i * ai_dim1], abs(d_2));
            r = r + (d_1 = ar[i + j * ar_dim1], abs(d_1)) + (d_2 = ai[i + 
                    j * ai_dim1], abs(d_2));
L200:
            ;
        }
/*     .......... GUARD AGAINST ZERO C OR R DUE TO UNDERFLOW .........
. */
        if (c == 0. || r == 0.) {
            goto L270;
        }
        g = r / radix;
        f = 1.;
        s = c + r;
L210:
        if (c >= g) {
            goto L220;
        }
        f *= radix;
        c *= b2;
        goto L210;
L220:
        g = r * radix;
L230:
        if (c < g) {
            goto L240;
        }
        f /= radix;
        c /= b2;
        goto L230;
/*     .......... NOW BALANCE .......... */
L240:
        if ((c + r) / f >= s * .95) {
            goto L270;
        }
        g = 1. / f;
        scale[i] *= f;
        noconv = TRUE_;

        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            ar[i + j * ar_dim1] *= g;
            ai[i + j * ai_dim1] *= g;
/* L250: */
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            ar[j + i * ar_dim1] *= f;
            ai[j + i * ai_dim1] *= f;
/* L260: */
        }

L270:
        ;
    }

    if (noconv) {
        goto L190;
    }

L280:
    *low = k;
    *igh = l;
    return 0;
} /* cbal_ */

/* Subroutine */ int cg_(integer *nm, integer *n, doublereal *ar, doublereal *
        ai, doublereal *wr, doublereal *wi, integer *matz, doublereal *zr, 
        doublereal *zi, doublereal *fv1, doublereal *fv2, doublereal *fv3, 
        integer *ierr)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset;

    /* Local variables */
    extern /* Subroutine */ int cbal_(integer *, integer *, doublereal *, 
            doublereal *, integer *, integer *, doublereal *), corth_(integer 
            *, integer *, integer *, integer *, doublereal *, doublereal *, 
            doublereal *, doublereal *), comqr_(integer *, integer *, integer 
            *, integer *, doublereal *, doublereal *, doublereal *, 
            doublereal *, integer *), cbabk2_(integer *, integer *, integer *,
             integer *, doublereal *, integer *, doublereal *, doublereal *), 
            comqr2_(integer *, integer *, integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *, doublereal *, 
            doublereal *, doublereal *, doublereal *, integer *);
    static integer is1, is2;



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A COMPLEX GENERAL MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A=(AR,AI). */

/*        AR  AND  AI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE COMPLEX GENERAL MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        WR  AND  WI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE EIGENVALUES. */

/*        ZR  AND  ZI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR COMQR */
/*           AND COMQR2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1, FV2, AND  FV3  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv3;
    --fv2;
    --fv1;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;
    --wi;
    --wr;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    cbal_(nm, n, &ar[ar_offset], &ai[ai_offset], &is1, &is2, &fv1[1]);
    corth_(nm, n, &is1, &is2, &ar[ar_offset], &ai[ai_offset], &fv2[1], &fv3[1]
            );
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    comqr_(nm, n, &is1, &is2, &ar[ar_offset], &ai[ai_offset], &wr[1], &wi[1], 
            ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    comqr2_(nm, n, &is1, &is2, &fv2[1], &fv3[1], &ar[ar_offset], &ai[
            ai_offset], &wr[1], &wi[1], &zr[zr_offset], &zi[zi_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    cbabk2_(nm, n, &is1, &is2, &fv1[1], n, &zr[zr_offset], &zi[zi_offset]);
L50:
    return 0;
} /* cg_ */

/* Subroutine */ int ch_(integer *nm, integer *n, doublereal *ar, doublereal *
        ai, doublereal *w, integer *matz, doublereal *zr, doublereal *zi, 
        doublereal *fv1, doublereal *fv2, doublereal *fm1, integer *ierr)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2;

    /* Local variables */
    static integer i, j;
    extern /* Subroutine */ int htridi_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *, doublereal *, 
            doublereal *), htribk_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, integer *, doublereal *, doublereal *)
            , tqlrat_(integer *, doublereal *, doublereal *, integer *), 
            tql2_(integer *, integer *, doublereal *, doublereal *, 
            doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A COMPLEX HERMITIAN MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A=(AR,AI). */

/*        AR  AND  AI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE COMPLEX HERMITIAN MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        ZR  AND  ZI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1, FV2, AND  FM1  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    fm1 -= 3;
    --fv2;
    --fv1;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;
    --w;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    htridi_(nm, n, &ar[ar_offset], &ai[ai_offset], &w[1], &fv1[1], &fv2[1], &
            fm1[3]);
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            zr[j + i * zr_dim1] = 0.;
/* L30: */
        }

        zr[i + i * zr_dim1] = 1.;
/* L40: */
    }

    tql2_(nm, n, &w[1], &fv1[1], &zr[zr_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    htribk_(nm, n, &ar[ar_offset], &ai[ai_offset], &fm1[3], n, &zr[zr_offset],
             &zi[zi_offset]);
L50:
    return 0;
} /* ch_ */

/* Subroutine */ int cinvit_(integer *nm, integer *n, doublereal *ar, 
        doublereal *ai, doublereal *wr, doublereal *wi, logical *select, 
        integer *mm, integer *m, doublereal *zr, doublereal *zi, integer *
        ierr, doublereal *rm1, doublereal *rm2, doublereal *rv1, doublereal *
        rv2)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, rm1_dim1, rm1_offset, rm2_dim1, rm2_offset, 
            i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, k, s;
    static doublereal x, y, normv;
    static integer ii;
    static doublereal ilambd;
    static integer mp, uk;
    static doublereal rlambd;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer km1, ip1;
    static doublereal growto, ukroot;
    static integer its;
    static doublereal eps3;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE CX INVIT */
/*     BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP. VOL.II-LINEAR ALGEBRA, 418-439(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVECTORS OF A COMPLEX UPPER */
/*     HESSENBERG MATRIX CORRESPONDING TO SPECIFIED EIGENVALUES, */
/*     USING INVERSE ITERATION. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE HESSENBERG MATRIX. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, RESPECTIVELY, */
/*          OF THE EIGENVALUES OF THE MATRIX.  THE EIGENVALUES MUST BE */
/*          STORED IN A MANNER IDENTICAL TO THAT OF SUBROUTINE  COMLR, */
/*          WHICH RECOGNIZES POSSIBLE SPLITTING OF THE MATRIX. */

/*        SELECT SPECIFIES THE EIGENVECTORS TO BE FOUND.  THE */
/*          EIGENVECTOR CORRESPONDING TO THE J-TH EIGENVALUE IS */
/*          SPECIFIED BY SETTING SELECT(J) TO .TRUE.. */

/*        MM SHOULD BE SET TO AN UPPER BOUND FOR THE NUMBER OF */
/*          EIGENVECTORS TO BE FOUND. */

/*     ON OUTPUT */

/*        AR, AI, WI, AND SELECT ARE UNALTERED. */

/*        WR MAY HAVE BEEN ALTERED SINCE CLOSE EIGENVALUES ARE PERTURBED 
*/
/*          SLIGHTLY IN SEARCHING FOR INDEPENDENT EIGENVECTORS. */

/*        M IS THE NUMBER OF EIGENVECTORS ACTUALLY FOUND. */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, RESPECTIVELY, */
/*          OF THE EIGENVECTORS.  THE EIGENVECTORS ARE NORMALIZED */
/*          SO THAT THE COMPONENT OF LARGEST MAGNITUDE IS 1. */
/*          ANY VECTOR WHICH FAILS THE ACCEPTANCE TEST IS SET TO ZERO. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          -(2*N+1)   IF MORE THAN MM EIGENVECTORS HAVE BEEN SPECIFIED, 
*/
/*          -K         IF THE ITERATION CORRESPONDING TO THE K-TH */
/*                     VALUE FAILS, */
/*          -(N+K)     IF BOTH ERROR SITUATIONS OCCUR. */

/*        RM1, RM2, RV1, AND RV2 ARE TEMPORARY STORAGE ARRAYS. */

/*     THE ALGOL PROCEDURE GUESSVEC APPEARS IN CINVIT IN LINE. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv2;
    --rv1;
    rm2_dim1 = *n;
    rm2_offset = rm2_dim1 + 1;
    rm2 -= rm2_offset;
    rm1_dim1 = *n;
    rm1_offset = rm1_dim1 + 1;
    rm1 -= rm1_offset;
    --select;
    --wi;
    --wr;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    *ierr = 0;
    uk = 0;
    s = 1;

    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {
        if (! select[k]) {
            goto L980;
        }
        if (s > *mm) {
            goto L1000;
        }
        if (uk >= k) {
            goto L200;
        }
/*     .......... CHECK FOR POSSIBLE SPLITTING .......... */
        i_2 = *n;
        for (uk = k; uk <= i_2; ++uk) {
            if (uk == *n) {
                goto L140;
            }
            if (ar[uk + 1 + uk * ar_dim1] == 0. && ai[uk + 1 + uk * ai_dim1] 
                    == 0.) {
                goto L140;
            }
/* L120: */
        }
/*     .......... COMPUTE INFINITY NORM OF LEADING UK BY UK */
/*                (HESSENBERG) MATRIX .......... */
L140:
        norm = 0.;
        mp = 1;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            x = 0.;

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
/* L160: */
                x += pythag_(&ar[i + j * ar_dim1], &ai[i + j * ai_dim1]);
            }

            if (x > norm) {
                norm = x;
            }
            mp = i;
/* L180: */
        }
/*     .......... EPS3 REPLACES ZERO PIVOT IN DECOMPOSITION */
/*                AND CLOSE ROOTS ARE MODIFIED BY EPS3 .......... */
        if (norm == 0.) {
            norm = 1.;
        }
        eps3 = epslon_(&norm);
/*     .......... GROWTO IS THE CRITERION FOR GROWTH .......... */
        ukroot = (doublereal) uk;
        ukroot = sqrt(ukroot);
        growto = .1 / ukroot;
L200:
        rlambd = wr[k];
        ilambd = wi[k];
        if (k == 1) {
            goto L280;
        }
        km1 = k - 1;
        goto L240;
/*     .......... PERTURB EIGENVALUE IF IT IS CLOSE */
/*                TO ANY PREVIOUS EIGENVALUE .......... */
L220:
        rlambd += eps3;
/*     .......... FOR I=K-1 STEP -1 UNTIL 1 DO -- .......... */
L240:
        i_2 = km1;
        for (ii = 1; ii <= i_2; ++ii) {
            i = k - ii;
            if (select[i] && (d_1 = wr[i] - rlambd, abs(d_1)) < eps3 && (
                    d_2 = wi[i] - ilambd, abs(d_2)) < eps3) {
                goto L220;
            }
/* L260: */
        }

        wr[k] = rlambd;
/*     .......... FORM UPPER HESSENBERG (AR,AI)-(RLAMBD,ILAMBD)*I */
/*                AND INITIAL COMPLEX VECTOR .......... */
L280:
        mp = 1;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
                rm1[i + j * rm1_dim1] = ar[i + j * ar_dim1];
                rm2[i + j * rm2_dim1] = ai[i + j * ai_dim1];
/* L300: */
            }

            rm1[i + i * rm1_dim1] -= rlambd;
            rm2[i + i * rm2_dim1] -= ilambd;
            mp = i;
            rv1[i] = eps3;
/* L320: */
        }
/*     .......... TRIANGULAR DECOMPOSITION WITH INTERCHANGES, */
/*                REPLACING ZERO PIVOTS BY EPS3 .......... */
        if (uk == 1) {
            goto L420;
        }

        i_2 = uk;
        for (i = 2; i <= i_2; ++i) {
            mp = i - 1;
            if (pythag_(&rm1[i + mp * rm1_dim1], &rm2[i + mp * rm2_dim1]) <= 
                    pythag_(&rm1[mp + mp * rm1_dim1], &rm2[mp + mp * rm2_dim1]
                    )) {
                goto L360;
            }

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
                y = rm1[i + j * rm1_dim1];
                rm1[i + j * rm1_dim1] = rm1[mp + j * rm1_dim1];
                rm1[mp + j * rm1_dim1] = y;
                y = rm2[i + j * rm2_dim1];
                rm2[i + j * rm2_dim1] = rm2[mp + j * rm2_dim1];
                rm2[mp + j * rm2_dim1] = y;
/* L340: */
            }

L360:
            if (rm1[mp + mp * rm1_dim1] == 0. && rm2[mp + mp * rm2_dim1] == 
                    0.) {
                rm1[mp + mp * rm1_dim1] = eps3;
            }
            cdiv_(&rm1[i + mp * rm1_dim1], &rm2[i + mp * rm2_dim1], &rm1[mp + 
                    mp * rm1_dim1], &rm2[mp + mp * rm2_dim1], &x, &y);
            if (x == 0. && y == 0.) {
                goto L400;
            }

            i_3 = uk;
            for (j = i; j <= i_3; ++j) {
                rm1[i + j * rm1_dim1] = rm1[i + j * rm1_dim1] - x * rm1[mp + 
                        j * rm1_dim1] + y * rm2[mp + j * rm2_dim1];
                rm2[i + j * rm2_dim1] = rm2[i + j * rm2_dim1] - x * rm2[mp + 
                        j * rm2_dim1] - y * rm1[mp + j * rm1_dim1];
/* L380: */
            }

L400:
            ;
        }

L420:
        if (rm1[uk + uk * rm1_dim1] == 0. && rm2[uk + uk * rm2_dim1] == 0.) {
            rm1[uk + uk * rm1_dim1] = eps3;
        }
        its = 0;
/*     .......... BACK SUBSTITUTION */
/*                FOR I=UK STEP -1 UNTIL 1 DO -- .......... */
L660:
        i_2 = uk;
        for (ii = 1; ii <= i_2; ++ii) {
            i = uk + 1 - ii;
            x = rv1[i];
            y = 0.;
            if (i == uk) {
                goto L700;
            }
            ip1 = i + 1;

            i_3 = uk;
            for (j = ip1; j <= i_3; ++j) {
                x = x - rm1[i + j * rm1_dim1] * rv1[j] + rm2[i + j * rm2_dim1]
                         * rv2[j];
                y = y - rm1[i + j * rm1_dim1] * rv2[j] - rm2[i + j * rm2_dim1]
                         * rv1[j];
/* L680: */
            }

L700:
            cdiv_(&x, &y, &rm1[i + i * rm1_dim1], &rm2[i + i * rm2_dim1], &
                    rv1[i], &rv2[i]);
/* L720: */
        }
/*     .......... ACCEPTANCE TEST FOR EIGENVECTOR */
/*                AND NORMALIZATION .......... */
        ++its;
        norm = 0.;
        normv = 0.;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            x = pythag_(&rv1[i], &rv2[i]);
            if (normv >= x) {
                goto L760;
            }
            normv = x;
            j = i;
L760:
            norm += x;
/* L780: */
        }

        if (norm < growto) {
            goto L840;
        }
/*     .......... ACCEPT VECTOR .......... */
        x = rv1[j];
        y = rv2[j];

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            cdiv_(&rv1[i], &rv2[i], &x, &y, &zr[i + s * zr_dim1], &zi[i + s * 
                    zi_dim1]);
/* L820: */
        }

        if (uk == *n) {
            goto L940;
        }
        j = uk + 1;
        goto L900;
/*     .......... IN-LINE PROCEDURE FOR CHOOSING */
/*                A NEW STARTING VECTOR .......... */
L840:
        if (its >= uk) {
            goto L880;
        }
        x = ukroot;
        y = eps3 / (x + 1.);
        rv1[1] = eps3;

        i_2 = uk;
        for (i = 2; i <= i_2; ++i) {
/* L860: */
            rv1[i] = y;
        }

        j = uk - its + 1;
        rv1[j] -= eps3 * x;
        goto L660;
/*     .......... SET ERROR -- UNACCEPTED EIGENVECTOR .......... */
L880:
        j = 1;
        *ierr = -k;
/*     .......... SET REMAINING VECTOR COMPONENTS TO ZERO .......... 
*/
L900:
        i_2 = *n;
        for (i = j; i <= i_2; ++i) {
            zr[i + s * zr_dim1] = 0.;
            zi[i + s * zi_dim1] = 0.;
/* L920: */
        }

L940:
        ++s;
L980:
        ;
    }

    goto L1001;
/*     .......... SET ERROR -- UNDERESTIMATE OF EIGENVECTOR */
/*                SPACE REQUIRED .......... */
L1000:
    if (*ierr != 0) {
        *ierr -= *n;
    }
    if (*ierr == 0) {
        *ierr = -((*n << 1) + 1);
    }
L1001:
    *m = s - 1;
    return 0;
} /* cinvit_ */

/* Subroutine */ int combak_(integer *nm, integer *low, integer *igh, 
        doublereal *ar, doublereal *ai, integer *int_, integer *m, 
        doublereal *zr, doublereal *zi)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer i, j, la, mm, mp;
    static doublereal xi, xr;
    static integer kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE COMBAK, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A COMPLEX GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     UPPER HESSENBERG MATRIX DETERMINED BY  COMHES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1 AND IGH EQUAL TO THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN THE MULTIPLIERS WHICH WERE USED IN THE */
/*          REDUCTION BY  COMHES  IN THEIR LOWER TRIANGLES */
/*          BELOW THE SUBDIAGONAL. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS */
/*          INTERCHANGED IN THE REDUCTION BY  COMHES. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVECTORS TO BE */
/*          BACK TRANSFORMED IN THEIR FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE TRANSFORMED EIGENVECTORS */
/*          IN THEIR FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --int_;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = la;
    for (mm = kp1; mm <= i_1; ++mm) {
        mp = *low + *igh - mm;
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
            xr = ar[i + (mp - 1) * ar_dim1];
            xi = ai[i + (mp - 1) * ai_dim1];
            if (xr == 0. && xi == 0.) {
                goto L110;
            }

            i_3 = *m;
            for (j = 1; j <= i_3; ++j) {
                zr[i + j * zr_dim1] = zr[i + j * zr_dim1] + xr * zr[mp + j * 
                        zr_dim1] - xi * zi[mp + j * zi_dim1];
                zi[i + j * zi_dim1] = zi[i + j * zi_dim1] + xr * zi[mp + j * 
                        zi_dim1] + xi * zr[mp + j * zr_dim1];
/* L100: */
            }

L110:
            ;
        }

        i = int_[mp];
        if (i == mp) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            xr = zr[i + j * zr_dim1];
            zr[i + j * zr_dim1] = zr[mp + j * zr_dim1];
            zr[mp + j * zr_dim1] = xr;
            xi = zi[i + j * zi_dim1];
            zi[i + j * zi_dim1] = zi[mp + j * zi_dim1];
            zi[mp + j * zi_dim1] = xi;
/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* combak_ */

/* Subroutine */ int comhes_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *ar, doublereal *ai, integer *int_)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static integer i, j, m, la;
    static doublereal xi, yi, xr, yr;
    static integer mm1, kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE COMHES, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     GIVEN A COMPLEX GENERAL MATRIX, THIS SUBROUTINE */
/*     REDUCES A SUBMATRIX SITUATED IN ROWS AND COLUMNS */
/*     LOW THROUGH IGH TO UPPER HESSENBERG FORM BY */
/*     STABILIZED ELEMENTARY SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX INPUT MATRIX. */

/*     ON OUTPUT */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE HESSENBERG MATRIX.  THE */
/*          MULTIPLIERS WHICH WERE USED IN THE REDUCTION */
/*          ARE STORED IN THE REMAINING TRIANGLES UNDER THE */
/*          HESSENBERG MATRIX. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS */
/*          INTERCHANGED IN THE REDUCTION. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    --int_;

    /* Function Body */
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }

    i_1 = la;
    for (m = kp1; m <= i_1; ++m) {
        mm1 = m - 1;
        xr = 0.;
        xi = 0.;
        i = m;

        i_2 = *igh;
        for (j = m; j <= i_2; ++j) {
            if ((d_1 = ar[j + mm1 * ar_dim1], abs(d_1)) + (d_2 = ai[j + 
                    mm1 * ai_dim1], abs(d_2)) <= abs(xr) + abs(xi)) {
                goto L100;
            }
            xr = ar[j + mm1 * ar_dim1];
            xi = ai[j + mm1 * ai_dim1];
            i = j;
L100:
            ;
        }

        int_[m] = i;
        if (i == m) {
            goto L130;
        }
/*     .......... INTERCHANGE ROWS AND COLUMNS OF AR AND AI ..........
 */
        i_2 = *n;
        for (j = mm1; j <= i_2; ++j) {
            yr = ar[i + j * ar_dim1];
            ar[i + j * ar_dim1] = ar[m + j * ar_dim1];
            ar[m + j * ar_dim1] = yr;
            yi = ai[i + j * ai_dim1];
            ai[i + j * ai_dim1] = ai[m + j * ai_dim1];
            ai[m + j * ai_dim1] = yi;
/* L110: */
        }

        i_2 = *igh;
        for (j = 1; j <= i_2; ++j) {
            yr = ar[j + i * ar_dim1];
            ar[j + i * ar_dim1] = ar[j + m * ar_dim1];
            ar[j + m * ar_dim1] = yr;
            yi = ai[j + i * ai_dim1];
            ai[j + i * ai_dim1] = ai[j + m * ai_dim1];
            ai[j + m * ai_dim1] = yi;
/* L120: */
        }
/*     .......... END INTERCHANGE .......... */
L130:
        if (xr == 0. && xi == 0.) {
            goto L180;
        }
        mp1 = m + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
            yr = ar[i + mm1 * ar_dim1];
            yi = ai[i + mm1 * ai_dim1];
            if (yr == 0. && yi == 0.) {
                goto L160;
            }
            cdiv_(&yr, &yi, &xr, &xi, &yr, &yi);
            ar[i + mm1 * ar_dim1] = yr;
            ai[i + mm1 * ai_dim1] = yi;

            i_3 = *n;
            for (j = m; j <= i_3; ++j) {
                ar[i + j * ar_dim1] = ar[i + j * ar_dim1] - yr * ar[m + j * 
                        ar_dim1] + yi * ai[m + j * ai_dim1];
                ai[i + j * ai_dim1] = ai[i + j * ai_dim1] - yr * ai[m + j * 
                        ai_dim1] - yi * ar[m + j * ar_dim1];
/* L140: */
            }

            i_3 = *igh;
            for (j = 1; j <= i_3; ++j) {
                ar[j + m * ar_dim1] = ar[j + m * ar_dim1] + yr * ar[j + i * 
                        ar_dim1] - yi * ai[j + i * ai_dim1];
                ai[j + m * ai_dim1] = ai[j + m * ai_dim1] + yr * ai[j + i * 
                        ai_dim1] + yi * ar[j + i * ar_dim1];
/* L150: */
            }

L160:
            ;
        }

L180:
        ;
    }

L200:
    return 0;
} /* comhes_ */

/* Subroutine */ int comlr_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *hr, doublereal *hi, doublereal *wr, doublereal *wi, 
        integer *ierr)
{
    /* System generated locals */
    integer hr_dim1, hr_offset, hi_dim1, hi_offset, i_1, i_2;
    doublereal d_1, d_2, d_3, d_4;

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static integer i, j, l, m, en, ll, mm;
    static doublereal si, ti, xi, yi, sr, tr, xr, yr;
    static integer im1;
    extern /* Subroutine */ int csroot_(doublereal *, doublereal *, 
            doublereal *, doublereal *);
    static integer mp1, itn, its;
    static doublereal zzi, zzr;
    static integer enm1;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE COMLR, */
/*     NUM. MATH. 12, 369-376(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 396-403(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A COMPLEX */
/*     UPPER HESSENBERG MATRIX BY THE MODIFIED LR METHOD. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        HR AND HI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX UPPER HESSENBERG MATRIX. */
/*          THEIR LOWER TRIANGLES BELOW THE SUBDIAGONAL CONTAIN THE */
/*          MULTIPLIERS WHICH WERE USED IN THE REDUCTION BY  COMHES, */
/*          IF PERFORMED. */

/*     ON OUTPUT */

/*        THE UPPER HESSENBERG PORTIONS OF HR AND HI HAVE BEEN */
/*          DESTROYED.  THEREFORE, THEY MUST BE SAVED BEFORE */
/*          CALLING  COMLR  IF SUBSEQUENT CALCULATION OF */
/*          EIGENVECTORS IS TO BE PERFORMED. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  IF AN ERROR */
/*          EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS CSROOT FOR COMPLEX SQUARE ROOT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --wi;
    --wr;
    hi_dim1 = *nm;
    hi_offset = hi_dim1 + 1;
    hi -= hi_offset;
    hr_dim1 = *nm;
    hr_offset = hr_dim1 + 1;
    hr -= hr_offset;

    /* Function Body */
    *ierr = 0;
/*     .......... STORE ROOTS ISOLATED BY CBAL .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i >= *low && i <= *igh) {
            goto L200;
        }
        wr[i] = hr[i + i * hr_dim1];
        wi[i] = hi[i + i * hi_dim1];
L200:
        ;
    }

    en = *igh;
    tr = 0.;
    ti = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUE .......... */
L220:
    if (en < *low) {
        goto L1001;
    }
    its = 0;
    enm1 = en - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW D0 -- .......... */
L240:
    i_1 = en;
    for (ll = *low; ll <= i_1; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L300;
        }
        tst1 = (d_1 = hr[l - 1 + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[
                l - 1 + (l - 1) * hi_dim1], abs(d_2)) + (d_3 = hr[l + l * 
                hr_dim1], abs(d_3)) + (d_4 = hi[l + l * hi_dim1], abs(d_4))
                ;
        tst2 = tst1 + (d_1 = hr[l + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = 
                hi[l + (l - 1) * hi_dim1], abs(d_2));
        if (tst2 == tst1) {
            goto L300;
        }
/* L260: */
    }
/*     .......... FORM SHIFT .......... */
L300:
    if (l == en) {
        goto L660;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its == 10 || its == 20) {
        goto L320;
    }
    sr = hr[en + en * hr_dim1];
    si = hi[en + en * hi_dim1];
    xr = hr[enm1 + en * hr_dim1] * hr[en + enm1 * hr_dim1] - hi[enm1 + en * 
            hi_dim1] * hi[en + enm1 * hi_dim1];
    xi = hr[enm1 + en * hr_dim1] * hi[en + enm1 * hi_dim1] + hi[enm1 + en * 
            hi_dim1] * hr[en + enm1 * hr_dim1];
    if (xr == 0. && xi == 0.) {
        goto L340;
    }
    yr = (hr[enm1 + enm1 * hr_dim1] - sr) / 2.;
    yi = (hi[enm1 + enm1 * hi_dim1] - si) / 2.;
/* Computing 2nd power */
    d_2 = yr;
/* Computing 2nd power */
    d_3 = yi;
    d_1 = d_2 * d_2 - d_3 * d_3 + xr;
    d_4 = yr * 2. * yi + xi;
    csroot_(&d_1, &d_4, &zzr, &zzi);
    if (yr * zzr + yi * zzi >= 0.) {
        goto L310;
    }
    zzr = -zzr;
    zzi = -zzi;
L310:
    d_1 = yr + zzr;
    d_2 = yi + zzi;
    cdiv_(&xr, &xi, &d_1, &d_2, &xr, &xi);
    sr -= xr;
    si -= xi;
    goto L340;
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
L320:
    sr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hr[enm1 + (en 
            - 2) * hr_dim1], abs(d_2));
    si = (d_1 = hi[en + enm1 * hi_dim1], abs(d_1)) + (d_2 = hi[enm1 + (en 
            - 2) * hi_dim1], abs(d_2));

L340:
    i_1 = en;
    for (i = *low; i <= i_1; ++i) {
        hr[i + i * hr_dim1] -= sr;
        hi[i + i * hi_dim1] -= si;
/* L360: */
    }

    tr += sr;
    ti += si;
    ++its;
    --itn;
/*     .......... LOOK FOR TWO CONSECUTIVE SMALL */
/*                SUB-DIAGONAL ELEMENTS .......... */
    xr = (d_1 = hr[enm1 + enm1 * hr_dim1], abs(d_1)) + (d_2 = hi[enm1 + 
            enm1 * hi_dim1], abs(d_2));
    yr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hi[en + enm1 * 
            hi_dim1], abs(d_2));
    zzr = (d_1 = hr[en + en * hr_dim1], abs(d_1)) + (d_2 = hi[en + en * 
            hi_dim1], abs(d_2));
/*     .......... FOR M=EN-1 STEP -1 UNTIL L DO -- .......... */
    i_1 = enm1;
    for (mm = l; mm <= i_1; ++mm) {
        m = enm1 + l - mm;
        if (m == l) {
            goto L420;
        }
        yi = yr;
        yr = (d_1 = hr[m + (m - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[m + (
                m - 1) * hi_dim1], abs(d_2));
        xi = zzr;
        zzr = xr;
        xr = (d_1 = hr[m - 1 + (m - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[m 
                - 1 + (m - 1) * hi_dim1], abs(d_2));
        tst1 = zzr / yi * (zzr + xr + xi);
        tst2 = tst1 + yr;
        if (tst2 == tst1) {
            goto L420;
        }
/* L380: */
    }
/*     .......... TRIANGULAR DECOMPOSITION H=L*R .......... */
L420:
    mp1 = m + 1;

    i_1 = en;
    for (i = mp1; i <= i_1; ++i) {
        im1 = i - 1;
        xr = hr[im1 + im1 * hr_dim1];
        xi = hi[im1 + im1 * hi_dim1];
        yr = hr[i + im1 * hr_dim1];
        yi = hi[i + im1 * hi_dim1];
        if (abs(xr) + abs(xi) >= abs(yr) + abs(yi)) {
            goto L460;
        }
/*     .......... INTERCHANGE ROWS OF HR AND HI .......... */
        i_2 = en;
        for (j = im1; j <= i_2; ++j) {
            zzr = hr[im1 + j * hr_dim1];
            hr[im1 + j * hr_dim1] = hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = zzr;
            zzi = hi[im1 + j * hi_dim1];
            hi[im1 + j * hi_dim1] = hi[i + j * hi_dim1];
            hi[i + j * hi_dim1] = zzi;
/* L440: */
        }

        cdiv_(&xr, &xi, &yr, &yi, &zzr, &zzi);
        wr[i] = 1.;
        goto L480;
L460:
        cdiv_(&yr, &yi, &xr, &xi, &zzr, &zzi);
        wr[i] = -1.;
L480:
        hr[i + im1 * hr_dim1] = zzr;
        hi[i + im1 * hi_dim1] = zzi;

        i_2 = en;
        for (j = i; j <= i_2; ++j) {
            hr[i + j * hr_dim1] = hr[i + j * hr_dim1] - zzr * hr[im1 + j * 
                    hr_dim1] + zzi * hi[im1 + j * hi_dim1];
            hi[i + j * hi_dim1] = hi[i + j * hi_dim1] - zzr * hi[im1 + j * 
                    hi_dim1] - zzi * hr[im1 + j * hr_dim1];
/* L500: */
        }

/* L520: */
    }
/*     .......... COMPOSITION R*L=H .......... */
    i_1 = en;
    for (j = mp1; j <= i_1; ++j) {
        xr = hr[j + (j - 1) * hr_dim1];
        xi = hi[j + (j - 1) * hi_dim1];
        hr[j + (j - 1) * hr_dim1] = 0.;
        hi[j + (j - 1) * hi_dim1] = 0.;
/*     .......... INTERCHANGE COLUMNS OF HR AND HI, */
/*                IF NECESSARY .......... */
        if (wr[j] <= 0.) {
            goto L580;
        }

        i_2 = j;
        for (i = l; i <= i_2; ++i) {
            zzr = hr[i + (j - 1) * hr_dim1];
            hr[i + (j - 1) * hr_dim1] = hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = zzr;
            zzi = hi[i + (j - 1) * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = hi[i + j * hi_dim1];
            hi[i + j * hi_dim1] = zzi;
/* L540: */
        }

L580:
        i_2 = j;
        for (i = l; i <= i_2; ++i) {
            hr[i + (j - 1) * hr_dim1] = hr[i + (j - 1) * hr_dim1] + xr * hr[i 
                    + j * hr_dim1] - xi * hi[i + j * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = hi[i + (j - 1) * hi_dim1] + xr * hi[i 
                    + j * hi_dim1] + xi * hr[i + j * hr_dim1];
/* L600: */
        }

/* L640: */
    }

    goto L240;
/*     .......... A ROOT FOUND .......... */
L660:
    wr[en] = hr[en + en * hr_dim1] + tr;
    wi[en] = hi[en + en * hi_dim1] + ti;
    en = enm1;
    goto L220;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* comlr_ */

/* Subroutine */ int comlr2_(integer *nm, integer *n, integer *low, integer *
        igh, integer *int_, doublereal *hr, doublereal *hi, doublereal *wr, 
        doublereal *wi, doublereal *zr, doublereal *zi, integer *ierr)
{
    /* System generated locals */
    integer hr_dim1, hr_offset, hi_dim1, hi_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Local variables */
    static integer iend;
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, k, l, m, ii, en, jj, ll, mm, nn;
    static doublereal si, ti, xi, yi, sr, tr, xr, yr;
    static integer im1;
    extern /* Subroutine */ int csroot_(doublereal *, doublereal *, 
            doublereal *, doublereal *);
    static integer ip1, mp1, itn, its;
    static doublereal zzi, zzr;
    static integer enm1;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE COMLR2, */
/*     NUM. MATH. 16, 181-204(1970) BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 372-395(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES AND EIGENVECTORS */
/*     OF A COMPLEX UPPER HESSENBERG MATRIX BY THE MODIFIED LR */
/*     METHOD.  THE EIGENVECTORS OF A COMPLEX GENERAL MATRIX */
/*     CAN ALSO BE FOUND IF  COMHES  HAS BEEN USED TO REDUCE */
/*     THIS GENERAL MATRIX TO HESSENBERG FORM. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS INTERCHANGED */
/*          IN THE REDUCTION BY  COMHES, IF PERFORMED.  ONLY ELEMENTS */
/*          LOW THROUGH IGH ARE USED.  IF THE EIGENVECTORS OF THE HESSEN- 
*/
/*          BERG MATRIX ARE DESIRED, SET INT(J)=J FOR THESE ELEMENTS. */

/*        HR AND HI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX UPPER HESSENBERG MATRIX. */
/*          THEIR LOWER TRIANGLES BELOW THE SUBDIAGONAL CONTAIN THE */
/*          MULTIPLIERS WHICH WERE USED IN THE REDUCTION BY  COMHES, */
/*          IF PERFORMED.  IF THE EIGENVECTORS OF THE HESSENBERG */
/*          MATRIX ARE DESIRED, THESE ELEMENTS MUST BE SET TO ZERO. */

/*     ON OUTPUT */

/*        THE UPPER HESSENBERG PORTIONS OF HR AND HI HAVE BEEN */
/*          DESTROYED, BUT THE LOCATION HR(1,1) CONTAINS THE NORM */
/*          OF THE TRIANGULARIZED MATRIX. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  IF AN ERROR */
/*          EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVECTORS.  THE EIGENVECTORS */
/*          ARE UNNORMALIZED.  IF AN ERROR EXIT IS MADE, NONE OF */
/*          THE EIGENVECTORS HAS BEEN FOUND. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */


/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS CSROOT FOR COMPLEX SQUARE ROOT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;
    --wi;
    --wr;
    hi_dim1 = *nm;
    hi_offset = hi_dim1 + 1;
    hi -= hi_offset;
    hr_dim1 = *nm;
    hr_offset = hr_dim1 + 1;
    hr -= hr_offset;
    --int_;

    /* Function Body */
    *ierr = 0;
/*     .......... INITIALIZE EIGENVECTOR MATRIX .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            zr[i + j * zr_dim1] = 0.;
            zi[i + j * zi_dim1] = 0.;
            if (i == j) {
                zr[i + j * zr_dim1] = 1.;
            }
/* L100: */
        }
    }
/*     .......... FORM THE MATRIX OF ACCUMULATED TRANSFORMATIONS */
/*                FROM THE INFORMATION LEFT BY COMHES .......... */
    iend = *igh - *low - 1;
    if (iend <= 0) {
        goto L180;
    }
/*     .......... FOR I=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_2 = iend;
    for (ii = 1; ii <= i_2; ++ii) {
        i = *igh - ii;
        ip1 = i + 1;

        i_1 = *igh;
        for (k = ip1; k <= i_1; ++k) {
            zr[k + i * zr_dim1] = hr[k + (i - 1) * hr_dim1];
            zi[k + i * zi_dim1] = hi[k + (i - 1) * hi_dim1];
/* L120: */
        }

        j = int_[i];
        if (i == j) {
            goto L160;
        }

        i_1 = *igh;
        for (k = i; k <= i_1; ++k) {
            zr[i + k * zr_dim1] = zr[j + k * zr_dim1];
            zi[i + k * zi_dim1] = zi[j + k * zi_dim1];
            zr[j + k * zr_dim1] = 0.;
            zi[j + k * zi_dim1] = 0.;
/* L140: */
        }

        zr[j + i * zr_dim1] = 1.;
L160:
        ;
    }
/*     .......... STORE ROOTS ISOLATED BY CBAL .......... */
L180:
    i_2 = *n;
    for (i = 1; i <= i_2; ++i) {
        if (i >= *low && i <= *igh) {
            goto L200;
        }
        wr[i] = hr[i + i * hr_dim1];
        wi[i] = hi[i + i * hi_dim1];
L200:
        ;
    }

    en = *igh;
    tr = 0.;
    ti = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUE .......... */
L220:
    if (en < *low) {
        goto L680;
    }
    its = 0;
    enm1 = en - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW DO -- .......... */
L240:
    i_2 = en;
    for (ll = *low; ll <= i_2; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L300;
        }
        tst1 = (d_1 = hr[l - 1 + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[
                l - 1 + (l - 1) * hi_dim1], abs(d_2)) + (d_3 = hr[l + l * 
                hr_dim1], abs(d_3)) + (d_4 = hi[l + l * hi_dim1], abs(d_4))
                ;
        tst2 = tst1 + (d_1 = hr[l + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = 
                hi[l + (l - 1) * hi_dim1], abs(d_2));
        if (tst2 == tst1) {
            goto L300;
        }
/* L260: */
    }
/*     .......... FORM SHIFT .......... */
L300:
    if (l == en) {
        goto L660;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its == 10 || its == 20) {
        goto L320;
    }
    sr = hr[en + en * hr_dim1];
    si = hi[en + en * hi_dim1];
    xr = hr[enm1 + en * hr_dim1] * hr[en + enm1 * hr_dim1] - hi[enm1 + en * 
            hi_dim1] * hi[en + enm1 * hi_dim1];
    xi = hr[enm1 + en * hr_dim1] * hi[en + enm1 * hi_dim1] + hi[enm1 + en * 
            hi_dim1] * hr[en + enm1 * hr_dim1];
    if (xr == 0. && xi == 0.) {
        goto L340;
    }
    yr = (hr[enm1 + enm1 * hr_dim1] - sr) / 2.;
    yi = (hi[enm1 + enm1 * hi_dim1] - si) / 2.;
/* Computing 2nd power */
    d_2 = yr;
/* Computing 2nd power */
    d_3 = yi;
    d_1 = d_2 * d_2 - d_3 * d_3 + xr;
    d_4 = yr * 2. * yi + xi;
    csroot_(&d_1, &d_4, &zzr, &zzi);
    if (yr * zzr + yi * zzi >= 0.) {
        goto L310;
    }
    zzr = -zzr;
    zzi = -zzi;
L310:
    d_1 = yr + zzr;
    d_2 = yi + zzi;
    cdiv_(&xr, &xi, &d_1, &d_2, &xr, &xi);
    sr -= xr;
    si -= xi;
    goto L340;
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
L320:
    sr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hr[enm1 + (en 
            - 2) * hr_dim1], abs(d_2));
    si = (d_1 = hi[en + enm1 * hi_dim1], abs(d_1)) + (d_2 = hi[enm1 + (en 
            - 2) * hi_dim1], abs(d_2));

L340:
    i_2 = en;
    for (i = *low; i <= i_2; ++i) {
        hr[i + i * hr_dim1] -= sr;
        hi[i + i * hi_dim1] -= si;
/* L360: */
    }

    tr += sr;
    ti += si;
    ++its;
    --itn;
/*     .......... LOOK FOR TWO CONSECUTIVE SMALL */
/*                SUB-DIAGONAL ELEMENTS .......... */
    xr = (d_1 = hr[enm1 + enm1 * hr_dim1], abs(d_1)) + (d_2 = hi[enm1 + 
            enm1 * hi_dim1], abs(d_2));
    yr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hi[en + enm1 * 
            hi_dim1], abs(d_2));
    zzr = (d_1 = hr[en + en * hr_dim1], abs(d_1)) + (d_2 = hi[en + en * 
            hi_dim1], abs(d_2));
/*     .......... FOR M=EN-1 STEP -1 UNTIL L DO -- .......... */
    i_2 = enm1;
    for (mm = l; mm <= i_2; ++mm) {
        m = enm1 + l - mm;
        if (m == l) {
            goto L420;
        }
        yi = yr;
        yr = (d_1 = hr[m + (m - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[m + (
                m - 1) * hi_dim1], abs(d_2));
        xi = zzr;
        zzr = xr;
        xr = (d_1 = hr[m - 1 + (m - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[m 
                - 1 + (m - 1) * hi_dim1], abs(d_2));
        tst1 = zzr / yi * (zzr + xr + xi);
        tst2 = tst1 + yr;
        if (tst2 == tst1) {
            goto L420;
        }
/* L380: */
    }
/*     .......... TRIANGULAR DECOMPOSITION H=L*R .......... */
L420:
    mp1 = m + 1;

    i_2 = en;
    for (i = mp1; i <= i_2; ++i) {
        im1 = i - 1;
        xr = hr[im1 + im1 * hr_dim1];
        xi = hi[im1 + im1 * hi_dim1];
        yr = hr[i + im1 * hr_dim1];
        yi = hi[i + im1 * hi_dim1];
        if (abs(xr) + abs(xi) >= abs(yr) + abs(yi)) {
            goto L460;
        }
/*     .......... INTERCHANGE ROWS OF HR AND HI .......... */
        i_1 = *n;
        for (j = im1; j <= i_1; ++j) {
            zzr = hr[im1 + j * hr_dim1];
            hr[im1 + j * hr_dim1] = hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = zzr;
            zzi = hi[im1 + j * hi_dim1];
            hi[im1 + j * hi_dim1] = hi[i + j * hi_dim1];
            hi[i + j * hi_dim1] = zzi;
/* L440: */
        }

        cdiv_(&xr, &xi, &yr, &yi, &zzr, &zzi);
        wr[i] = 1.;
        goto L480;
L460:
        cdiv_(&yr, &yi, &xr, &xi, &zzr, &zzi);
        wr[i] = -1.;
L480:
        hr[i + im1 * hr_dim1] = zzr;
        hi[i + im1 * hi_dim1] = zzi;

        i_1 = *n;
        for (j = i; j <= i_1; ++j) {
            hr[i + j * hr_dim1] = hr[i + j * hr_dim1] - zzr * hr[im1 + j * 
                    hr_dim1] + zzi * hi[im1 + j * hi_dim1];
            hi[i + j * hi_dim1] = hi[i + j * hi_dim1] - zzr * hi[im1 + j * 
                    hi_dim1] - zzi * hr[im1 + j * hr_dim1];
/* L500: */
        }

/* L520: */
    }
/*     .......... COMPOSITION R*L=H .......... */
    i_2 = en;
    for (j = mp1; j <= i_2; ++j) {
        xr = hr[j + (j - 1) * hr_dim1];
        xi = hi[j + (j - 1) * hi_dim1];
        hr[j + (j - 1) * hr_dim1] = 0.;
        hi[j + (j - 1) * hi_dim1] = 0.;
/*     .......... INTERCHANGE COLUMNS OF HR, HI, ZR, AND ZI, */
/*                IF NECESSARY .......... */
        if (wr[j] <= 0.) {
            goto L580;
        }

        i_1 = j;
        for (i = 1; i <= i_1; ++i) {
            zzr = hr[i + (j - 1) * hr_dim1];
            hr[i + (j - 1) * hr_dim1] = hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = zzr;
            zzi = hi[i + (j - 1) * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = hi[i + j * hi_dim1];
            hi[i + j * hi_dim1] = zzi;
/* L540: */
        }

        i_1 = *igh;
        for (i = *low; i <= i_1; ++i) {
            zzr = zr[i + (j - 1) * zr_dim1];
            zr[i + (j - 1) * zr_dim1] = zr[i + j * zr_dim1];
            zr[i + j * zr_dim1] = zzr;
            zzi = zi[i + (j - 1) * zi_dim1];
            zi[i + (j - 1) * zi_dim1] = zi[i + j * zi_dim1];
            zi[i + j * zi_dim1] = zzi;
/* L560: */
        }

L580:
        i_1 = j;
        for (i = 1; i <= i_1; ++i) {
            hr[i + (j - 1) * hr_dim1] = hr[i + (j - 1) * hr_dim1] + xr * hr[i 
                    + j * hr_dim1] - xi * hi[i + j * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = hi[i + (j - 1) * hi_dim1] + xr * hi[i 
                    + j * hi_dim1] + xi * hr[i + j * hr_dim1];
/* L600: */
        }
/*     .......... ACCUMULATE TRANSFORMATIONS .......... */
        i_1 = *igh;
        for (i = *low; i <= i_1; ++i) {
            zr[i + (j - 1) * zr_dim1] = zr[i + (j - 1) * zr_dim1] + xr * zr[i 
                    + j * zr_dim1] - xi * zi[i + j * zi_dim1];
            zi[i + (j - 1) * zi_dim1] = zi[i + (j - 1) * zi_dim1] + xr * zi[i 
                    + j * zi_dim1] + xi * zr[i + j * zr_dim1];
/* L620: */
        }

/* L640: */
    }

    goto L240;
/*     .......... A ROOT FOUND .......... */
L660:
    hr[en + en * hr_dim1] += tr;
    wr[en] = hr[en + en * hr_dim1];
    hi[en + en * hi_dim1] += ti;
    wi[en] = hi[en + en * hi_dim1];
    en = enm1;
    goto L220;
/*     .......... ALL ROOTS FOUND.  BACKSUBSTITUTE TO FIND */
/*                VECTORS OF UPPER TRIANGULAR FORM .......... */
L680:
    norm = 0.;

    i_2 = *n;
    for (i = 1; i <= i_2; ++i) {

        i_1 = *n;
        for (j = i; j <= i_1; ++j) {
            tr = (d_1 = hr[i + j * hr_dim1], abs(d_1)) + (d_2 = hi[i + j * 
                    hi_dim1], abs(d_2));
            if (tr > norm) {
                norm = tr;
            }
/* L720: */
        }
    }

    hr[hr_dim1 + 1] = norm;
    if (*n == 1 || norm == 0.) {
        goto L1001;
    }
/*     .......... FOR EN=N STEP -1 UNTIL 2 DO -- .......... */
    i_1 = *n;
    for (nn = 2; nn <= i_1; ++nn) {
        en = *n + 2 - nn;
        xr = wr[en];
        xi = wi[en];
        hr[en + en * hr_dim1] = 1.;
        hi[en + en * hi_dim1] = 0.;
        enm1 = en - 1;
/*     .......... FOR I=EN-1 STEP -1 UNTIL 1 DO -- .......... */
        i_2 = enm1;
        for (ii = 1; ii <= i_2; ++ii) {
            i = en - ii;
            zzr = 0.;
            zzi = 0.;
            ip1 = i + 1;

            i_3 = en;
            for (j = ip1; j <= i_3; ++j) {
                zzr = zzr + hr[i + j * hr_dim1] * hr[j + en * hr_dim1] - hi[i 
                        + j * hi_dim1] * hi[j + en * hi_dim1];
                zzi = zzi + hr[i + j * hr_dim1] * hi[j + en * hi_dim1] + hi[i 
                        + j * hi_dim1] * hr[j + en * hr_dim1];
/* L740: */
            }

            yr = xr - wr[i];
            yi = xi - wi[i];
            if (yr != 0. || yi != 0.) {
                goto L765;
            }
            tst1 = norm;
            yr = tst1;
L760:
            yr *= .01;
            tst2 = norm + yr;
            if (tst2 > tst1) {
                goto L760;
            }
L765:
            cdiv_(&zzr, &zzi, &yr, &yi, &hr[i + en * hr_dim1], &hi[i + en * 
                    hi_dim1]);
/*     .......... OVERFLOW CONTROL .......... */
            tr = (d_1 = hr[i + en * hr_dim1], abs(d_1)) + (d_2 = hi[i + en 
                    * hi_dim1], abs(d_2));
            if (tr == 0.) {
                goto L780;
            }
            tst1 = tr;
            tst2 = tst1 + 1. / tst1;
            if (tst2 > tst1) {
                goto L780;
            }
            i_3 = en;
            for (j = i; j <= i_3; ++j) {
                hr[j + en * hr_dim1] /= tr;
                hi[j + en * hi_dim1] /= tr;
/* L770: */
            }

L780:
            ;
        }

/* L800: */
    }
/*     .......... END BACKSUBSTITUTION .......... */
    enm1 = *n - 1;
/*     .......... VECTORS OF ISOLATED ROOTS .......... */
    i_1 = enm1;
    for (i = 1; i <= i_1; ++i) {
        if (i >= *low && i <= *igh) {
            goto L840;
        }
        ip1 = i + 1;

        i_2 = *n;
        for (j = ip1; j <= i_2; ++j) {
            zr[i + j * zr_dim1] = hr[i + j * hr_dim1];
            zi[i + j * zi_dim1] = hi[i + j * hi_dim1];
/* L820: */
        }

L840:
        ;
    }
/*     .......... MULTIPLY BY TRANSFORMATION MATRIX TO GIVE */
/*                VECTORS OF ORIGINAL FULL MATRIX. */
/*                FOR J=N STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = enm1;
    for (jj = *low; jj <= i_1; ++jj) {
        j = *n + *low - jj;
        m = min(j,*igh);

        i_2 = *igh;
        for (i = *low; i <= i_2; ++i) {
            zzr = 0.;
            zzi = 0.;

            i_3 = m;
            for (k = *low; k <= i_3; ++k) {
                zzr = zzr + zr[i + k * zr_dim1] * hr[k + j * hr_dim1] - zi[i 
                        + k * zi_dim1] * hi[k + j * hi_dim1];
                zzi = zzi + zr[i + k * zr_dim1] * hi[k + j * hi_dim1] + zi[i 
                        + k * zi_dim1] * hr[k + j * hr_dim1];
/* L860: */
            }

            zr[i + j * zr_dim1] = zzr;
            zi[i + j * zi_dim1] = zzi;
/* L880: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* comlr2_ */

/* Subroutine */ int comqr_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *hr, doublereal *hi, doublereal *wr, doublereal *wi, 
        integer *ierr)
{
    /* System generated locals */
    integer hr_dim1, hr_offset, hi_dim1, hi_offset, i_1, i_2;
    doublereal d_1, d_2, d_3, d_4;

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, l, en, ll;
    static doublereal si, ti, xi, yi, sr, tr, xr, yr;
    extern doublereal pythag_(doublereal *, doublereal *);
    extern /* Subroutine */ int csroot_(doublereal *, doublereal *, 
            doublereal *, doublereal *);
    static integer lp1, itn, its;
    static doublereal zzi, zzr;
    static integer enm1;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF A UNITARY ANALOGUE OF THE */
/*     ALGOL PROCEDURE  COMLR, NUM. MATH. 12, 369-376(1968) BY MARTIN */
/*     AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 396-403(1971). */
/*     THE UNITARY ANALOGUE SUBSTITUTES THE QR ALGORITHM OF FRANCIS */
/*     (COMP. JOUR. 4, 332-345(1962)) FOR THE LR ALGORITHM. */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A COMPLEX */
/*     UPPER HESSENBERG MATRIX BY THE QR METHOD. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        HR AND HI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX UPPER HESSENBERG MATRIX. */
/*          THEIR LOWER TRIANGLES BELOW THE SUBDIAGONAL CONTAIN */
/*          INFORMATION ABOUT THE UNITARY TRANSFORMATIONS USED IN */
/*          THE REDUCTION BY  CORTH, IF PERFORMED. */

/*     ON OUTPUT */

/*        THE UPPER HESSENBERG PORTIONS OF HR AND HI HAVE BEEN */
/*          DESTROYED.  THEREFORE, THEY MUST BE SAVED BEFORE */
/*          CALLING  COMQR  IF SUBSEQUENT CALCULATION OF */
/*          EIGENVECTORS IS TO BE PERFORMED. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  IF AN ERROR */
/*          EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS CSROOT FOR COMPLEX SQUARE ROOT. */
/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --wi;
    --wr;
    hi_dim1 = *nm;
    hi_offset = hi_dim1 + 1;
    hi -= hi_offset;
    hr_dim1 = *nm;
    hr_offset = hr_dim1 + 1;
    hr -= hr_offset;

    /* Function Body */
    *ierr = 0;
    if (*low == *igh) {
        goto L180;
    }
/*     .......... CREATE REAL SUBDIAGONAL ELEMENTS .......... */
    l = *low + 1;

    i_1 = *igh;
    for (i = l; i <= i_1; ++i) {
/* Computing MIN */
        i_2 = i + 1;
        ll = min(i_2,*igh);
        if (hi[i + (i - 1) * hi_dim1] == 0.) {
            goto L170;
        }
        norm = pythag_(&hr[i + (i - 1) * hr_dim1], &hi[i + (i - 1) * hi_dim1])
                ;
        yr = hr[i + (i - 1) * hr_dim1] / norm;
        yi = hi[i + (i - 1) * hi_dim1] / norm;
        hr[i + (i - 1) * hr_dim1] = norm;
        hi[i + (i - 1) * hi_dim1] = 0.;

        i_2 = *igh;
        for (j = i; j <= i_2; ++j) {
            si = yr * hi[i + j * hi_dim1] - yi * hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = yr * hr[i + j * hr_dim1] + yi * hi[i + j * 
                    hi_dim1];
            hi[i + j * hi_dim1] = si;
/* L155: */
        }

        i_2 = ll;
        for (j = *low; j <= i_2; ++j) {
            si = yr * hi[j + i * hi_dim1] + yi * hr[j + i * hr_dim1];
            hr[j + i * hr_dim1] = yr * hr[j + i * hr_dim1] - yi * hi[j + i * 
                    hi_dim1];
            hi[j + i * hi_dim1] = si;
/* L160: */
        }

L170:
        ;
    }
/*     .......... STORE ROOTS ISOLATED BY CBAL .......... */
L180:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i >= *low && i <= *igh) {
            goto L200;
        }
        wr[i] = hr[i + i * hr_dim1];
        wi[i] = hi[i + i * hi_dim1];
L200:
        ;
    }

    en = *igh;
    tr = 0.;
    ti = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUE .......... */
L220:
    if (en < *low) {
        goto L1001;
    }
    its = 0;
    enm1 = en - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW D0 -- .......... */
L240:
    i_1 = en;
    for (ll = *low; ll <= i_1; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L300;
        }
        tst1 = (d_1 = hr[l - 1 + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[
                l - 1 + (l - 1) * hi_dim1], abs(d_2)) + (d_3 = hr[l + l * 
                hr_dim1], abs(d_3)) + (d_4 = hi[l + l * hi_dim1], abs(d_4))
                ;
        tst2 = tst1 + (d_1 = hr[l + (l - 1) * hr_dim1], abs(d_1));
        if (tst2 == tst1) {
            goto L300;
        }
/* L260: */
    }
/*     .......... FORM SHIFT .......... */
L300:
    if (l == en) {
        goto L660;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its == 10 || its == 20) {
        goto L320;
    }
    sr = hr[en + en * hr_dim1];
    si = hi[en + en * hi_dim1];
    xr = hr[enm1 + en * hr_dim1] * hr[en + enm1 * hr_dim1];
    xi = hi[enm1 + en * hi_dim1] * hr[en + enm1 * hr_dim1];
    if (xr == 0. && xi == 0.) {
        goto L340;
    }
    yr = (hr[enm1 + enm1 * hr_dim1] - sr) / 2.;
    yi = (hi[enm1 + enm1 * hi_dim1] - si) / 2.;
/* Computing 2nd power */
    d_2 = yr;
/* Computing 2nd power */
    d_3 = yi;
    d_1 = d_2 * d_2 - d_3 * d_3 + xr;
    d_4 = yr * 2. * yi + xi;
    csroot_(&d_1, &d_4, &zzr, &zzi);
    if (yr * zzr + yi * zzi >= 0.) {
        goto L310;
    }
    zzr = -zzr;
    zzi = -zzi;
L310:
    d_1 = yr + zzr;
    d_2 = yi + zzi;
    cdiv_(&xr, &xi, &d_1, &d_2, &xr, &xi);
    sr -= xr;
    si -= xi;
    goto L340;
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
L320:
    sr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hr[enm1 + (en 
            - 2) * hr_dim1], abs(d_2));
    si = 0.;

L340:
    i_1 = en;
    for (i = *low; i <= i_1; ++i) {
        hr[i + i * hr_dim1] -= sr;
        hi[i + i * hi_dim1] -= si;
/* L360: */
    }

    tr += sr;
    ti += si;
    ++its;
    --itn;
/*     .......... REDUCE TO TRIANGLE (ROWS) .......... */
    lp1 = l + 1;

    i_1 = en;
    for (i = lp1; i <= i_1; ++i) {
        sr = hr[i + (i - 1) * hr_dim1];
        hr[i + (i - 1) * hr_dim1] = 0.;
        d_1 = pythag_(&hr[i - 1 + (i - 1) * hr_dim1], &hi[i - 1 + (i - 1) * 
                hi_dim1]);
        norm = pythag_(&d_1, &sr);
        xr = hr[i - 1 + (i - 1) * hr_dim1] / norm;
        wr[i - 1] = xr;
        xi = hi[i - 1 + (i - 1) * hi_dim1] / norm;
        wi[i - 1] = xi;
        hr[i - 1 + (i - 1) * hr_dim1] = norm;
        hi[i - 1 + (i - 1) * hi_dim1] = 0.;
        hi[i + (i - 1) * hi_dim1] = sr / norm;

        i_2 = en;
        for (j = i; j <= i_2; ++j) {
            yr = hr[i - 1 + j * hr_dim1];
            yi = hi[i - 1 + j * hi_dim1];
            zzr = hr[i + j * hr_dim1];
            zzi = hi[i + j * hi_dim1];
            hr[i - 1 + j * hr_dim1] = xr * yr + xi * yi + hi[i + (i - 1) * 
                    hi_dim1] * zzr;
            hi[i - 1 + j * hi_dim1] = xr * yi - xi * yr + hi[i + (i - 1) * 
                    hi_dim1] * zzi;
            hr[i + j * hr_dim1] = xr * zzr - xi * zzi - hi[i + (i - 1) * 
                    hi_dim1] * yr;
            hi[i + j * hi_dim1] = xr * zzi + xi * zzr - hi[i + (i - 1) * 
                    hi_dim1] * yi;
/* L490: */
        }

/* L500: */
    }

    si = hi[en + en * hi_dim1];
    if (si == 0.) {
        goto L540;
    }
    norm = pythag_(&hr[en + en * hr_dim1], &si);
    sr = hr[en + en * hr_dim1] / norm;
    si /= norm;
    hr[en + en * hr_dim1] = norm;
    hi[en + en * hi_dim1] = 0.;
/*     .......... INVERSE OPERATION (COLUMNS) .......... */
L540:
    i_1 = en;
    for (j = lp1; j <= i_1; ++j) {
        xr = wr[j - 1];
        xi = wi[j - 1];

        i_2 = j;
        for (i = l; i <= i_2; ++i) {
            yr = hr[i + (j - 1) * hr_dim1];
            yi = 0.;
            zzr = hr[i + j * hr_dim1];
            zzi = hi[i + j * hi_dim1];
            if (i == j) {
                goto L560;
            }
            yi = hi[i + (j - 1) * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = xr * yi + xi * yr + hi[j + (j - 1) * 
                    hi_dim1] * zzi;
L560:
            hr[i + (j - 1) * hr_dim1] = xr * yr - xi * yi + hi[j + (j - 1) * 
                    hi_dim1] * zzr;
            hr[i + j * hr_dim1] = xr * zzr + xi * zzi - hi[j + (j - 1) * 
                    hi_dim1] * yr;
            hi[i + j * hi_dim1] = xr * zzi - xi * zzr - hi[j + (j - 1) * 
                    hi_dim1] * yi;
/* L580: */
        }

/* L600: */
    }

    if (si == 0.) {
        goto L240;
    }

    i_1 = en;
    for (i = l; i <= i_1; ++i) {
        yr = hr[i + en * hr_dim1];
        yi = hi[i + en * hi_dim1];
        hr[i + en * hr_dim1] = sr * yr - si * yi;
        hi[i + en * hi_dim1] = sr * yi + si * yr;
/* L630: */
    }

    goto L240;
/*     .......... A ROOT FOUND .......... */
L660:
    wr[en] = hr[en + en * hr_dim1] + tr;
    wi[en] = hi[en + en * hi_dim1] + ti;
    en = enm1;
    goto L220;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* comqr_ */

/* Subroutine */ int comqr2_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *ortr, doublereal *orti, doublereal *hr, doublereal *
        hi, doublereal *wr, doublereal *wi, doublereal *zr, doublereal *zi, 
        integer *ierr)
{
    /* System generated locals */
    integer hr_dim1, hr_offset, hi_dim1, hi_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Local variables */
    static integer iend;
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, k, l, m, ii, en, jj, ll, nn;
    static doublereal si, ti, xi, yi, sr, tr, xr, yr;
    extern doublereal pythag_(doublereal *, doublereal *);
    extern /* Subroutine */ int csroot_(doublereal *, doublereal *, 
            doublereal *, doublereal *);
    static integer ip1, lp1, itn, its;
    static doublereal zzi, zzr;
    static integer enm1;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF A UNITARY ANALOGUE OF THE */
/*     ALGOL PROCEDURE  COMLR2, NUM. MATH. 16, 181-204(1970) BY PETERS */
/*     AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 372-395(1971). */
/*     THE UNITARY ANALOGUE SUBSTITUTES THE QR ALGORITHM OF FRANCIS */
/*     (COMP. JOUR. 4, 332-345(1962)) FOR THE LR ALGORITHM. */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES AND EIGENVECTORS */
/*     OF A COMPLEX UPPER HESSENBERG MATRIX BY THE QR */
/*     METHOD.  THE EIGENVECTORS OF A COMPLEX GENERAL MATRIX */
/*     CAN ALSO BE FOUND IF  CORTH  HAS BEEN USED TO REDUCE */
/*     THIS GENERAL MATRIX TO HESSENBERG FORM. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        ORTR AND ORTI CONTAIN INFORMATION ABOUT THE UNITARY TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  CORTH, IF PERFORMED. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED.  IF THE EIGENVECTORS 
*/
/*          OF THE HESSENBERG MATRIX ARE DESIRED, SET ORTR(J) AND */
/*          ORTI(J) TO 0.0D0 FOR THESE ELEMENTS. */

/*        HR AND HI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX UPPER HESSENBERG MATRIX. */
/*          THEIR LOWER TRIANGLES BELOW THE SUBDIAGONAL CONTAIN FURTHER */
/*          INFORMATION ABOUT THE TRANSFORMATIONS WHICH WERE USED IN THE 
*/
/*          REDUCTION BY  CORTH, IF PERFORMED.  IF THE EIGENVECTORS OF */
/*          THE HESSENBERG MATRIX ARE DESIRED, THESE ELEMENTS MAY BE */
/*          ARBITRARY. */

/*     ON OUTPUT */

/*        ORTR, ORTI, AND THE UPPER HESSENBERG PORTIONS OF HR AND HI */
/*          HAVE BEEN DESTROYED. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  IF AN ERROR */
/*          EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVECTORS.  THE EIGENVECTORS */
/*          ARE UNNORMALIZED.  IF AN ERROR EXIT IS MADE, NONE OF */
/*          THE EIGENVECTORS HAS BEEN FOUND. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS CSROOT FOR COMPLEX SQUARE ROOT. */
/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;
    --wi;
    --wr;
    hi_dim1 = *nm;
    hi_offset = hi_dim1 + 1;
    hi -= hi_offset;
    hr_dim1 = *nm;
    hr_offset = hr_dim1 + 1;
    hr -= hr_offset;
    --orti;
    --ortr;

    /* Function Body */
    *ierr = 0;
/*     .......... INITIALIZE EIGENVECTOR MATRIX .......... */
    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            zr[i + j * zr_dim1] = 0.;
            zi[i + j * zi_dim1] = 0.;
/* L100: */
        }
        zr[j + j * zr_dim1] = 1.;
/* L101: */
    }
/*     .......... FORM THE MATRIX OF ACCUMULATED TRANSFORMATIONS */
/*                FROM THE INFORMATION LEFT BY CORTH .......... */
    iend = *igh - *low - 1;
    if (iend < 0) {
        goto L180;
    } else if (iend == 0) {
        goto L150;
    } else {
        goto L105;
    }
/*     .......... FOR I=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
L105:
    i_1 = iend;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *igh - ii;
        if (ortr[i] == 0. && orti[i] == 0.) {
            goto L140;
        }
        if (hr[i + (i - 1) * hr_dim1] == 0. && hi[i + (i - 1) * hi_dim1] == 
                0.) {
            goto L140;
        }
/*     .......... NORM BELOW IS NEGATIVE OF H FORMED IN CORTH ........
.. */
        norm = hr[i + (i - 1) * hr_dim1] * ortr[i] + hi[i + (i - 1) * hi_dim1]
                 * orti[i];
        ip1 = i + 1;

        i_2 = *igh;
        for (k = ip1; k <= i_2; ++k) {
            ortr[k] = hr[k + (i - 1) * hr_dim1];
            orti[k] = hi[k + (i - 1) * hi_dim1];
/* L110: */
        }

        i_2 = *igh;
        for (j = i; j <= i_2; ++j) {
            sr = 0.;
            si = 0.;

            i_3 = *igh;
            for (k = i; k <= i_3; ++k) {
                sr = sr + ortr[k] * zr[k + j * zr_dim1] + orti[k] * zi[k + j *
                         zi_dim1];
                si = si + ortr[k] * zi[k + j * zi_dim1] - orti[k] * zr[k + j *
                         zr_dim1];
/* L115: */
            }

            sr /= norm;
            si /= norm;

            i_3 = *igh;
            for (k = i; k <= i_3; ++k) {
                zr[k + j * zr_dim1] = zr[k + j * zr_dim1] + sr * ortr[k] - si 
                        * orti[k];
                zi[k + j * zi_dim1] = zi[k + j * zi_dim1] + sr * orti[k] + si 
                        * ortr[k];
/* L120: */
            }

/* L130: */
        }

L140:
        ;
    }
/*     .......... CREATE REAL SUBDIAGONAL ELEMENTS .......... */
L150:
    l = *low + 1;

    i_1 = *igh;
    for (i = l; i <= i_1; ++i) {
/* Computing MIN */
        i_2 = i + 1;
        ll = min(i_2,*igh);
        if (hi[i + (i - 1) * hi_dim1] == 0.) {
            goto L170;
        }
        norm = pythag_(&hr[i + (i - 1) * hr_dim1], &hi[i + (i - 1) * hi_dim1])
                ;
        yr = hr[i + (i - 1) * hr_dim1] / norm;
        yi = hi[i + (i - 1) * hi_dim1] / norm;
        hr[i + (i - 1) * hr_dim1] = norm;
        hi[i + (i - 1) * hi_dim1] = 0.;

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            si = yr * hi[i + j * hi_dim1] - yi * hr[i + j * hr_dim1];
            hr[i + j * hr_dim1] = yr * hr[i + j * hr_dim1] + yi * hi[i + j * 
                    hi_dim1];
            hi[i + j * hi_dim1] = si;
/* L155: */
        }

        i_2 = ll;
        for (j = 1; j <= i_2; ++j) {
            si = yr * hi[j + i * hi_dim1] + yi * hr[j + i * hr_dim1];
            hr[j + i * hr_dim1] = yr * hr[j + i * hr_dim1] - yi * hi[j + i * 
                    hi_dim1];
            hi[j + i * hi_dim1] = si;
/* L160: */
        }

        i_2 = *igh;
        for (j = *low; j <= i_2; ++j) {
            si = yr * zi[j + i * zi_dim1] + yi * zr[j + i * zr_dim1];
            zr[j + i * zr_dim1] = yr * zr[j + i * zr_dim1] - yi * zi[j + i * 
                    zi_dim1];
            zi[j + i * zi_dim1] = si;
/* L165: */
        }

L170:
        ;
    }
/*     .......... STORE ROOTS ISOLATED BY CBAL .......... */
L180:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i >= *low && i <= *igh) {
            goto L200;
        }
        wr[i] = hr[i + i * hr_dim1];
        wi[i] = hi[i + i * hi_dim1];
L200:
        ;
    }

    en = *igh;
    tr = 0.;
    ti = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUE .......... */
L220:
    if (en < *low) {
        goto L680;
    }
    its = 0;
    enm1 = en - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW DO -- .......... */
L240:
    i_1 = en;
    for (ll = *low; ll <= i_1; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L300;
        }
        tst1 = (d_1 = hr[l - 1 + (l - 1) * hr_dim1], abs(d_1)) + (d_2 = hi[
                l - 1 + (l - 1) * hi_dim1], abs(d_2)) + (d_3 = hr[l + l * 
                hr_dim1], abs(d_3)) + (d_4 = hi[l + l * hi_dim1], abs(d_4))
                ;
        tst2 = tst1 + (d_1 = hr[l + (l - 1) * hr_dim1], abs(d_1));
        if (tst2 == tst1) {
            goto L300;
        }
/* L260: */
    }
/*     .......... FORM SHIFT .......... */
L300:
    if (l == en) {
        goto L660;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its == 10 || its == 20) {
        goto L320;
    }
    sr = hr[en + en * hr_dim1];
    si = hi[en + en * hi_dim1];
    xr = hr[enm1 + en * hr_dim1] * hr[en + enm1 * hr_dim1];
    xi = hi[enm1 + en * hi_dim1] * hr[en + enm1 * hr_dim1];
    if (xr == 0. && xi == 0.) {
        goto L340;
    }
    yr = (hr[enm1 + enm1 * hr_dim1] - sr) / 2.;
    yi = (hi[enm1 + enm1 * hi_dim1] - si) / 2.;
/* Computing 2nd power */
    d_2 = yr;
/* Computing 2nd power */
    d_3 = yi;
    d_1 = d_2 * d_2 - d_3 * d_3 + xr;
    d_4 = yr * 2. * yi + xi;
    csroot_(&d_1, &d_4, &zzr, &zzi);
    if (yr * zzr + yi * zzi >= 0.) {
        goto L310;
    }
    zzr = -zzr;
    zzi = -zzi;
L310:
    d_1 = yr + zzr;
    d_2 = yi + zzi;
    cdiv_(&xr, &xi, &d_1, &d_2, &xr, &xi);
    sr -= xr;
    si -= xi;
    goto L340;
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
L320:
    sr = (d_1 = hr[en + enm1 * hr_dim1], abs(d_1)) + (d_2 = hr[enm1 + (en 
            - 2) * hr_dim1], abs(d_2));
    si = 0.;

L340:
    i_1 = en;
    for (i = *low; i <= i_1; ++i) {
        hr[i + i * hr_dim1] -= sr;
        hi[i + i * hi_dim1] -= si;
/* L360: */
    }

    tr += sr;
    ti += si;
    ++its;
    --itn;
/*     .......... REDUCE TO TRIANGLE (ROWS) .......... */
    lp1 = l + 1;

    i_1 = en;
    for (i = lp1; i <= i_1; ++i) {
        sr = hr[i + (i - 1) * hr_dim1];
        hr[i + (i - 1) * hr_dim1] = 0.;
        d_1 = pythag_(&hr[i - 1 + (i - 1) * hr_dim1], &hi[i - 1 + (i - 1) * 
                hi_dim1]);
        norm = pythag_(&d_1, &sr);
        xr = hr[i - 1 + (i - 1) * hr_dim1] / norm;
        wr[i - 1] = xr;
        xi = hi[i - 1 + (i - 1) * hi_dim1] / norm;
        wi[i - 1] = xi;
        hr[i - 1 + (i - 1) * hr_dim1] = norm;
        hi[i - 1 + (i - 1) * hi_dim1] = 0.;
        hi[i + (i - 1) * hi_dim1] = sr / norm;

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            yr = hr[i - 1 + j * hr_dim1];
            yi = hi[i - 1 + j * hi_dim1];
            zzr = hr[i + j * hr_dim1];
            zzi = hi[i + j * hi_dim1];
            hr[i - 1 + j * hr_dim1] = xr * yr + xi * yi + hi[i + (i - 1) * 
                    hi_dim1] * zzr;
            hi[i - 1 + j * hi_dim1] = xr * yi - xi * yr + hi[i + (i - 1) * 
                    hi_dim1] * zzi;
            hr[i + j * hr_dim1] = xr * zzr - xi * zzi - hi[i + (i - 1) * 
                    hi_dim1] * yr;
            hi[i + j * hi_dim1] = xr * zzi + xi * zzr - hi[i + (i - 1) * 
                    hi_dim1] * yi;
/* L490: */
        }

/* L500: */
    }

    si = hi[en + en * hi_dim1];
    if (si == 0.) {
        goto L540;
    }
    norm = pythag_(&hr[en + en * hr_dim1], &si);
    sr = hr[en + en * hr_dim1] / norm;
    si /= norm;
    hr[en + en * hr_dim1] = norm;
    hi[en + en * hi_dim1] = 0.;
    if (en == *n) {
        goto L540;
    }
    ip1 = en + 1;

    i_1 = *n;
    for (j = ip1; j <= i_1; ++j) {
        yr = hr[en + j * hr_dim1];
        yi = hi[en + j * hi_dim1];
        hr[en + j * hr_dim1] = sr * yr + si * yi;
        hi[en + j * hi_dim1] = sr * yi - si * yr;
/* L520: */
    }
/*     .......... INVERSE OPERATION (COLUMNS) .......... */
L540:
    i_1 = en;
    for (j = lp1; j <= i_1; ++j) {
        xr = wr[j - 1];
        xi = wi[j - 1];

        i_2 = j;
        for (i = 1; i <= i_2; ++i) {
            yr = hr[i + (j - 1) * hr_dim1];
            yi = 0.;
            zzr = hr[i + j * hr_dim1];
            zzi = hi[i + j * hi_dim1];
            if (i == j) {
                goto L560;
            }
            yi = hi[i + (j - 1) * hi_dim1];
            hi[i + (j - 1) * hi_dim1] = xr * yi + xi * yr + hi[j + (j - 1) * 
                    hi_dim1] * zzi;
L560:
            hr[i + (j - 1) * hr_dim1] = xr * yr - xi * yi + hi[j + (j - 1) * 
                    hi_dim1] * zzr;
            hr[i + j * hr_dim1] = xr * zzr + xi * zzi - hi[j + (j - 1) * 
                    hi_dim1] * yr;
            hi[i + j * hi_dim1] = xr * zzi - xi * zzr - hi[j + (j - 1) * 
                    hi_dim1] * yi;
/* L580: */
        }

        i_2 = *igh;
        for (i = *low; i <= i_2; ++i) {
            yr = zr[i + (j - 1) * zr_dim1];
            yi = zi[i + (j - 1) * zi_dim1];
            zzr = zr[i + j * zr_dim1];
            zzi = zi[i + j * zi_dim1];
            zr[i + (j - 1) * zr_dim1] = xr * yr - xi * yi + hi[j + (j - 1) * 
                    hi_dim1] * zzr;
            zi[i + (j - 1) * zi_dim1] = xr * yi + xi * yr + hi[j + (j - 1) * 
                    hi_dim1] * zzi;
            zr[i + j * zr_dim1] = xr * zzr + xi * zzi - hi[j + (j - 1) * 
                    hi_dim1] * yr;
            zi[i + j * zi_dim1] = xr * zzi - xi * zzr - hi[j + (j - 1) * 
                    hi_dim1] * yi;
/* L590: */
        }

/* L600: */
    }

    if (si == 0.) {
        goto L240;
    }

    i_1 = en;
    for (i = 1; i <= i_1; ++i) {
        yr = hr[i + en * hr_dim1];
        yi = hi[i + en * hi_dim1];
        hr[i + en * hr_dim1] = sr * yr - si * yi;
        hi[i + en * hi_dim1] = sr * yi + si * yr;
/* L630: */
    }

    i_1 = *igh;
    for (i = *low; i <= i_1; ++i) {
        yr = zr[i + en * zr_dim1];
        yi = zi[i + en * zi_dim1];
        zr[i + en * zr_dim1] = sr * yr - si * yi;
        zi[i + en * zi_dim1] = sr * yi + si * yr;
/* L640: */
    }

    goto L240;
/*     .......... A ROOT FOUND .......... */
L660:
    hr[en + en * hr_dim1] += tr;
    wr[en] = hr[en + en * hr_dim1];
    hi[en + en * hi_dim1] += ti;
    wi[en] = hi[en + en * hi_dim1];
    en = enm1;
    goto L220;
/*     .......... ALL ROOTS FOUND.  BACKSUBSTITUTE TO FIND */
/*                VECTORS OF UPPER TRIANGULAR FORM .......... */
L680:
    norm = 0.;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            tr = (d_1 = hr[i + j * hr_dim1], abs(d_1)) + (d_2 = hi[i + j * 
                    hi_dim1], abs(d_2));
            if (tr > norm) {
                norm = tr;
            }
/* L720: */
        }
    }

    if (*n == 1 || norm == 0.) {
        goto L1001;
    }
/*     .......... FOR EN=N STEP -1 UNTIL 2 DO -- .......... */
    i_2 = *n;
    for (nn = 2; nn <= i_2; ++nn) {
        en = *n + 2 - nn;
        xr = wr[en];
        xi = wi[en];
        hr[en + en * hr_dim1] = 1.;
        hi[en + en * hi_dim1] = 0.;
        enm1 = en - 1;
/*     .......... FOR I=EN-1 STEP -1 UNTIL 1 DO -- .......... */
        i_1 = enm1;
        for (ii = 1; ii <= i_1; ++ii) {
            i = en - ii;
            zzr = 0.;
            zzi = 0.;
            ip1 = i + 1;

            i_3 = en;
            for (j = ip1; j <= i_3; ++j) {
                zzr = zzr + hr[i + j * hr_dim1] * hr[j + en * hr_dim1] - hi[i 
                        + j * hi_dim1] * hi[j + en * hi_dim1];
                zzi = zzi + hr[i + j * hr_dim1] * hi[j + en * hi_dim1] + hi[i 
                        + j * hi_dim1] * hr[j + en * hr_dim1];
/* L740: */
            }

            yr = xr - wr[i];
            yi = xi - wi[i];
            if (yr != 0. || yi != 0.) {
                goto L765;
            }
            tst1 = norm;
            yr = tst1;
L760:
            yr *= .01;
            tst2 = norm + yr;
            if (tst2 > tst1) {
                goto L760;
            }
L765:
            cdiv_(&zzr, &zzi, &yr, &yi, &hr[i + en * hr_dim1], &hi[i + en * 
                    hi_dim1]);
/*     .......... OVERFLOW CONTROL .......... */
            tr = (d_1 = hr[i + en * hr_dim1], abs(d_1)) + (d_2 = hi[i + en 
                    * hi_dim1], abs(d_2));
            if (tr == 0.) {
                goto L780;
            }
            tst1 = tr;
            tst2 = tst1 + 1. / tst1;
            if (tst2 > tst1) {
                goto L780;
            }
            i_3 = en;
            for (j = i; j <= i_3; ++j) {
                hr[j + en * hr_dim1] /= tr;
                hi[j + en * hi_dim1] /= tr;
/* L770: */
            }

L780:
            ;
        }

/* L800: */
    }
/*     .......... END BACKSUBSTITUTION .......... */
    enm1 = *n - 1;
/*     .......... VECTORS OF ISOLATED ROOTS .......... */
    i_2 = enm1;
    for (i = 1; i <= i_2; ++i) {
        if (i >= *low && i <= *igh) {
            goto L840;
        }
        ip1 = i + 1;

        i_1 = *n;
        for (j = ip1; j <= i_1; ++j) {
            zr[i + j * zr_dim1] = hr[i + j * hr_dim1];
            zi[i + j * zi_dim1] = hi[i + j * hi_dim1];
/* L820: */
        }

L840:
        ;
    }
/*     .......... MULTIPLY BY TRANSFORMATION MATRIX TO GIVE */
/*                VECTORS OF ORIGINAL FULL MATRIX. */
/*                FOR J=N STEP -1 UNTIL LOW+1 DO -- .......... */
    i_2 = enm1;
    for (jj = *low; jj <= i_2; ++jj) {
        j = *n + *low - jj;
        m = min(j,*igh);

        i_1 = *igh;
        for (i = *low; i <= i_1; ++i) {
            zzr = 0.;
            zzi = 0.;

            i_3 = m;
            for (k = *low; k <= i_3; ++k) {
                zzr = zzr + zr[i + k * zr_dim1] * hr[k + j * hr_dim1] - zi[i 
                        + k * zi_dim1] * hi[k + j * hi_dim1];
                zzi = zzi + zr[i + k * zr_dim1] * hi[k + j * hi_dim1] + zi[i 
                        + k * zi_dim1] * hr[k + j * hr_dim1];
/* L860: */
            }

            zr[i + j * zr_dim1] = zzr;
            zi[i + j * zi_dim1] = zzi;
/* L880: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* comqr2_ */

/* Subroutine */ int cortb_(integer *nm, integer *low, integer *igh, 
        doublereal *ar, doublereal *ai, doublereal *ortr, doublereal *orti, 
        integer *m, doublereal *zr, doublereal *zi)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2, i_3;

    /* Local variables */
    static doublereal h;
    static integer i, j, la;
    static doublereal gi, gr;
    static integer mm, mp, kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE ORTBAK, NUM. MATH. 12, 349-368(1968) */
/*     BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A COMPLEX GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     UPPER HESSENBERG MATRIX DETERMINED BY  CORTH. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1 AND IGH EQUAL TO THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN INFORMATION ABOUT THE UNITARY */
/*          TRANSFORMATIONS USED IN THE REDUCTION BY  CORTH */
/*          IN THEIR STRICT LOWER TRIANGLES. */

/*        ORTR AND ORTI CONTAIN FURTHER INFORMATION ABOUT THE */
/*          TRANSFORMATIONS USED IN THE REDUCTION BY  CORTH. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*        M IS THE NUMBER OF COLUMNS OF ZR AND ZI TO BE BACK TRANSFORMED. 
*/

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVECTORS TO BE */
/*          BACK TRANSFORMED IN THEIR FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE TRANSFORMED EIGENVECTORS */
/*          IN THEIR FIRST M COLUMNS. */

/*        ORTR AND ORTI HAVE BEEN ALTERED. */

/*     NOTE THAT CORTB PRESERVES VECTOR EUCLIDEAN NORMS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --orti;
    --ortr;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = la;
    for (mm = kp1; mm <= i_1; ++mm) {
        mp = *low + *igh - mm;
        if (ar[mp + (mp - 1) * ar_dim1] == 0. && ai[mp + (mp - 1) * ai_dim1] 
                == 0.) {
            goto L140;
        }
/*     .......... H BELOW IS NEGATIVE OF H FORMED IN CORTH .......... 
*/
        h = ar[mp + (mp - 1) * ar_dim1] * ortr[mp] + ai[mp + (mp - 1) * 
                ai_dim1] * orti[mp];
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
            ortr[i] = ar[i + (mp - 1) * ar_dim1];
            orti[i] = ai[i + (mp - 1) * ai_dim1];
/* L100: */
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            gr = 0.;
            gi = 0.;

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
                gr = gr + ortr[i] * zr[i + j * zr_dim1] + orti[i] * zi[i + j *
                         zi_dim1];
                gi = gi + ortr[i] * zi[i + j * zi_dim1] - orti[i] * zr[i + j *
                         zr_dim1];
/* L110: */
            }

            gr /= h;
            gi /= h;

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
                zr[i + j * zr_dim1] = zr[i + j * zr_dim1] + gr * ortr[i] - gi 
                        * orti[i];
                zi[i + j * zi_dim1] = zi[i + j * zi_dim1] + gr * orti[i] + gi 
                        * ortr[i];
/* L120: */
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* cortb_ */

/* Subroutine */ int corth_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *ar, doublereal *ai, doublereal *ortr, doublereal *
        orti)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, m;
    static doublereal scale;
    static integer la;
    static doublereal fi;
    static integer ii, jj;
    static doublereal fr;
    static integer mp;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer kp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE ORTHES, NUM. MATH. 12, 349-368(1968) */
/*     BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     GIVEN A COMPLEX GENERAL MATRIX, THIS SUBROUTINE */
/*     REDUCES A SUBMATRIX SITUATED IN ROWS AND COLUMNS */
/*     LOW THROUGH IGH TO UPPER HESSENBERG FORM BY */
/*     UNITARY SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  CBAL.  IF  CBAL  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX INPUT MATRIX. */

/*     ON OUTPUT */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE HESSENBERG MATRIX.  INFORMATION */
/*          ABOUT THE UNITARY TRANSFORMATIONS USED IN THE REDUCTION */
/*          IS STORED IN THE REMAINING TRIANGLES UNDER THE */
/*          HESSENBERG MATRIX. */

/*        ORTR AND ORTI CONTAIN FURTHER INFORMATION ABOUT THE */
/*          TRANSFORMATIONS.  ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    --orti;
    --ortr;

    /* Function Body */
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }

    i_1 = la;
    for (m = kp1; m <= i_1; ++m) {
        h = 0.;
        ortr[m] = 0.;
        orti[m] = 0.;
        scale = 0.;
/*     .......... SCALE COLUMN (ALGOL TOL THEN NOT NEEDED) .......... 
*/
        i_2 = *igh;
        for (i = m; i <= i_2; ++i) {
/* L90: */
            scale = scale + (d_1 = ar[i + (m - 1) * ar_dim1], abs(d_1)) + (
                    d_2 = ai[i + (m - 1) * ai_dim1], abs(d_2));
        }

        if (scale == 0.) {
            goto L180;
        }
        mp = m + *igh;
/*     .......... FOR I=IGH STEP -1 UNTIL M DO -- .......... */
        i_2 = *igh;
        for (ii = m; ii <= i_2; ++ii) {
            i = mp - ii;
            ortr[i] = ar[i + (m - 1) * ar_dim1] / scale;
            orti[i] = ai[i + (m - 1) * ai_dim1] / scale;
            h = h + ortr[i] * ortr[i] + orti[i] * orti[i];
/* L100: */
        }

        g = sqrt(h);
        f = pythag_(&ortr[m], &orti[m]);
        if (f == 0.) {
            goto L103;
        }
        h += f * g;
        g /= f;
        ortr[m] = (g + 1.) * ortr[m];
        orti[m] = (g + 1.) * orti[m];
        goto L105;

L103:
        ortr[m] = g;
        ar[m + (m - 1) * ar_dim1] = scale;
/*     .......... FORM (I-(U*UT)/H) * A .......... */
L105:
        i_2 = *n;
        for (j = m; j <= i_2; ++j) {
            fr = 0.;
            fi = 0.;
/*     .......... FOR I=IGH STEP -1 UNTIL M DO -- .......... */
            i_3 = *igh;
            for (ii = m; ii <= i_3; ++ii) {
                i = mp - ii;
                fr = fr + ortr[i] * ar[i + j * ar_dim1] + orti[i] * ai[i + j *
                         ai_dim1];
                fi = fi + ortr[i] * ai[i + j * ai_dim1] - orti[i] * ar[i + j *
                         ar_dim1];
/* L110: */
            }

            fr /= h;
            fi /= h;

            i_3 = *igh;
            for (i = m; i <= i_3; ++i) {
                ar[i + j * ar_dim1] = ar[i + j * ar_dim1] - fr * ortr[i] + fi 
                        * orti[i];
                ai[i + j * ai_dim1] = ai[i + j * ai_dim1] - fr * orti[i] - fi 
                        * ortr[i];
/* L120: */
            }

/* L130: */
        }
/*     .......... FORM (I-(U*UT)/H)*A*(I-(U*UT)/H) .......... */
        i_2 = *igh;
        for (i = 1; i <= i_2; ++i) {
            fr = 0.;
            fi = 0.;
/*     .......... FOR J=IGH STEP -1 UNTIL M DO -- .......... */
            i_3 = *igh;
            for (jj = m; jj <= i_3; ++jj) {
                j = mp - jj;
                fr = fr + ortr[j] * ar[i + j * ar_dim1] - orti[j] * ai[i + j *
                         ai_dim1];
                fi = fi + ortr[j] * ai[i + j * ai_dim1] + orti[j] * ar[i + j *
                         ar_dim1];
/* L140: */
            }

            fr /= h;
            fi /= h;

            i_3 = *igh;
            for (j = m; j <= i_3; ++j) {
                ar[i + j * ar_dim1] = ar[i + j * ar_dim1] - fr * ortr[j] - fi 
                        * orti[j];
                ai[i + j * ai_dim1] = ai[i + j * ai_dim1] + fr * orti[j] - fi 
                        * ortr[j];
/* L150: */
            }

/* L160: */
        }

        ortr[m] = scale * ortr[m];
        orti[m] = scale * orti[m];
        ar[m + (m - 1) * ar_dim1] = -g * ar[m + (m - 1) * ar_dim1];
        ai[m + (m - 1) * ai_dim1] = -g * ai[m + (m - 1) * ai_dim1];
L180:
        ;
    }

L200:
    return 0;
} /* corth_ */

/* Subroutine */ int elmbak_(integer *nm, integer *low, integer *igh, 
        doublereal *a, integer *int_, integer *m, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer i, j;
    static doublereal x;
    static integer la, mm, mp, kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ELMBAK, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A REAL GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     UPPER HESSENBERG MATRIX DETERMINED BY  ELMHES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1 AND IGH EQUAL TO THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE MULTIPLIERS WHICH WERE USED IN THE */
/*          REDUCTION BY  ELMHES  IN ITS LOWER TRIANGLE */
/*          BELOW THE SUBDIAGONAL. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS */
/*          INTERCHANGED IN THE REDUCTION BY  ELMHES. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*        M IS THE NUMBER OF COLUMNS OF Z TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGEN- */
/*          VECTORS TO BE BACK TRANSFORMED IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE */
/*          TRANSFORMED EIGENVECTORS IN ITS FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --int_;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = la;
    for (mm = kp1; mm <= i_1; ++mm) {
        mp = *low + *igh - mm;
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
            x = a[i + (mp - 1) * a_dim1];
            if (x == 0.) {
                goto L110;
            }

            i_3 = *m;
            for (j = 1; j <= i_3; ++j) {
/* L100: */
                z[i + j * z_dim1] += x * z[mp + j * z_dim1];
            }

L110:
            ;
        }

        i = int_[mp];
        if (i == mp) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            x = z[i + j * z_dim1];
            z[i + j * z_dim1] = z[mp + j * z_dim1];
            z[mp + j * z_dim1] = x;
/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* elmbak_ */

/* Subroutine */ int elmhes_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *a, integer *int_)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;
    doublereal d_1;

    /* Local variables */
    static integer i, j, m;
    static doublereal x, y;
    static integer la, mm1, kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ELMHES, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     GIVEN A REAL GENERAL MATRIX, THIS SUBROUTINE */
/*     REDUCES A SUBMATRIX SITUATED IN ROWS AND COLUMNS */
/*     LOW THROUGH IGH TO UPPER HESSENBERG FORM BY */
/*     STABILIZED ELEMENTARY SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        A CONTAINS THE INPUT MATRIX. */

/*     ON OUTPUT */

/*        A CONTAINS THE HESSENBERG MATRIX.  THE MULTIPLIERS */
/*          WHICH WERE USED IN THE REDUCTION ARE STORED IN THE */
/*          REMAINING TRIANGLE UNDER THE HESSENBERG MATRIX. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS */
/*          INTERCHANGED IN THE REDUCTION. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    --int_;

    /* Function Body */
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }

    i_1 = la;
    for (m = kp1; m <= i_1; ++m) {
        mm1 = m - 1;
        x = 0.;
        i = m;

        i_2 = *igh;
        for (j = m; j <= i_2; ++j) {
            if ((d_1 = a[j + mm1 * a_dim1], abs(d_1)) <= abs(x)) {
                goto L100;
            }
            x = a[j + mm1 * a_dim1];
            i = j;
L100:
            ;
        }

        int_[m] = i;
        if (i == m) {
            goto L130;
        }
/*     .......... INTERCHANGE ROWS AND COLUMNS OF A .......... */
        i_2 = *n;
        for (j = mm1; j <= i_2; ++j) {
            y = a[i + j * a_dim1];
            a[i + j * a_dim1] = a[m + j * a_dim1];
            a[m + j * a_dim1] = y;
/* L110: */
        }

        i_2 = *igh;
        for (j = 1; j <= i_2; ++j) {
            y = a[j + i * a_dim1];
            a[j + i * a_dim1] = a[j + m * a_dim1];
            a[j + m * a_dim1] = y;
/* L120: */
        }
/*     .......... END INTERCHANGE .......... */
L130:
        if (x == 0.) {
            goto L180;
        }
        mp1 = m + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
            y = a[i + mm1 * a_dim1];
            if (y == 0.) {
                goto L160;
            }
            y /= x;
            a[i + mm1 * a_dim1] = y;

            i_3 = *n;
            for (j = m; j <= i_3; ++j) {
/* L140: */
                a[i + j * a_dim1] -= y * a[m + j * a_dim1];
            }

            i_3 = *igh;
            for (j = 1; j <= i_3; ++j) {
/* L150: */
                a[j + m * a_dim1] += y * a[j + i * a_dim1];
            }

L160:
            ;
        }

L180:
        ;
    }

L200:
    return 0;
} /* elmhes_ */

/* Subroutine */ int eltran_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *a, integer *int_, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2;

    /* Local variables */
    static integer i, j, kl, mm, mp, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ELMTRANS, 
*/
/*     NUM. MATH. 16, 181-204(1970) BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 372-395(1971). */

/*     THIS SUBROUTINE ACCUMULATES THE STABILIZED ELEMENTARY */
/*     SIMILARITY TRANSFORMATIONS USED IN THE REDUCTION OF A */
/*     REAL GENERAL MATRIX TO UPPER HESSENBERG FORM BY  ELMHES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        A CONTAINS THE MULTIPLIERS WHICH WERE USED IN THE */
/*          REDUCTION BY  ELMHES  IN ITS LOWER TRIANGLE */
/*          BELOW THE SUBDIAGONAL. */

/*        INT CONTAINS INFORMATION ON THE ROWS AND COLUMNS */
/*          INTERCHANGED IN THE REDUCTION BY  ELMHES. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN THE */
/*          REDUCTION BY  ELMHES. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

/*     .......... INITIALIZE Z TO IDENTITY MATRIX .......... */
    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --int_;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L60: */
            z[i + j * z_dim1] = 0.;
        }

        z[j + j * z_dim1] = 1.;
/* L80: */
    }

    kl = *igh - *low - 1;
    if (kl < 1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = kl;
    for (mm = 1; mm <= i_1; ++mm) {
        mp = *igh - mm;
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
/* L100: */
            z[i + mp * z_dim1] = a[i + (mp - 1) * a_dim1];
        }

        i = int_[mp];
        if (i == mp) {
            goto L140;
        }

        i_2 = *igh;
        for (j = mp; j <= i_2; ++j) {
            z[mp + j * z_dim1] = z[i + j * z_dim1];
            z[i + j * z_dim1] = 0.;
/* L130: */
        }

        z[i + mp * z_dim1] = 1.;
L140:
        ;
    }

L200:
    return 0;
} /* eltran_ */

/* Subroutine */ int figi_(integer *nm, integer *n, doublereal *t, doublereal 
        *d, doublereal *e, doublereal *e2, integer *ierr)
{
    /* System generated locals */
    integer t_dim1, t_offset, i_1;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i;



/*     GIVEN A NONSYMMETRIC TRIDIAGONAL MATRIX SUCH THAT THE PRODUCTS */
/*     OF CORRESPONDING PAIRS OF OFF-DIAGONAL ELEMENTS ARE ALL */
/*     NON-NEGATIVE, THIS SUBROUTINE REDUCES IT TO A SYMMETRIC */
/*     TRIDIAGONAL MATRIX WITH THE SAME EIGENVALUES.  IF, FURTHER, */
/*     A ZERO PRODUCT ONLY OCCURS WHEN BOTH FACTORS ARE ZERO, */
/*     THE REDUCED MATRIX IS SIMILAR TO THE ORIGINAL MATRIX. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        T CONTAINS THE INPUT MATRIX.  ITS SUBDIAGONAL IS */
/*          STORED IN THE LAST N-1 POSITIONS OF THE FIRST COLUMN, */
/*          ITS DIAGONAL IN THE N POSITIONS OF THE SECOND COLUMN, */
/*          AND ITS SUPERDIAGONAL IN THE FIRST N-1 POSITIONS OF */
/*          THE THIRD COLUMN.  T(1,1) AND T(N,3) ARE ARBITRARY. */

/*     ON OUTPUT */

/*        T IS UNALTERED. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE SYMMETRIC MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE SYMMETRIC */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS NOT SET. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          N+I        IF T(I,1)*T(I-1,3) IS NEGATIVE, */
/*          -(3*N+I)   IF T(I,1)*T(I-1,3) IS ZERO WITH ONE FACTOR */
/*                     NON-ZERO.  IN THIS CASE, THE EIGENVECTORS OF */
/*                     THE SYMMETRIC MATRIX ARE NOT SIMPLY RELATED */
/*                     TO THOSE OF  T  AND SHOULD NOT BE SOUGHT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    t_dim1 = *nm;
    t_offset = t_dim1 + 1;
    t -= t_offset;
    --e2;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i == 1) {
            goto L90;
        }
        e2[i] = t[i + t_dim1] * t[i - 1 + t_dim1 * 3];
        if ((d_1 = e2[i]) < 0.) {
            goto L1000;
        } else if (d_1 == 0) {
            goto L60;
        } else {
            goto L80;
        }
L60:
        if (t[i + t_dim1] == 0. && t[i - 1 + t_dim1 * 3] == 0.) {
            goto L80;
        }
/*     .......... SET ERROR -- PRODUCT OF SOME PAIR OF OFF-DIAGONAL */
/*                ELEMENTS IS ZERO WITH ONE MEMBER NON-ZERO ..........
 */
        *ierr = -(*n * 3 + i);
L80:
        e[i] = sqrt(e2[i]);
L90:
        d[i] = t[i + (t_dim1 << 1)];
/* L100: */
    }

    goto L1001;
/*     .......... SET ERROR -- PRODUCT OF SOME PAIR OF OFF-DIAGONAL */
/*                ELEMENTS IS NEGATIVE .......... */
L1000:
    *ierr = *n + i;
L1001:
    return 0;
} /* figi_ */

/* Subroutine */ int figi2_(integer *nm, integer *n, doublereal *t, 
        doublereal *d, doublereal *e, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer t_dim1, t_offset, z_dim1, z_offset, i_1, i_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal h;
    static integer i, j;



/*     GIVEN A NONSYMMETRIC TRIDIAGONAL MATRIX SUCH THAT THE PRODUCTS */
/*     OF CORRESPONDING PAIRS OF OFF-DIAGONAL ELEMENTS ARE ALL */
/*     NON-NEGATIVE, AND ZERO ONLY WHEN BOTH FACTORS ARE ZERO, THIS */
/*     SUBROUTINE REDUCES IT TO A SYMMETRIC TRIDIAGONAL MATRIX */
/*     USING AND ACCUMULATING DIAGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        T CONTAINS THE INPUT MATRIX.  ITS SUBDIAGONAL IS */
/*          STORED IN THE LAST N-1 POSITIONS OF THE FIRST COLUMN, */
/*          ITS DIAGONAL IN THE N POSITIONS OF THE SECOND COLUMN, */
/*          AND ITS SUPERDIAGONAL IN THE FIRST N-1 POSITIONS OF */
/*          THE THIRD COLUMN.  T(1,1) AND T(N,3) ARE ARBITRARY. */

/*     ON OUTPUT */

/*        T IS UNALTERED. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE SYMMETRIC MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE SYMMETRIC */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS NOT SET. */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN */
/*          THE REDUCTION. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          N+I        IF T(I,1)*T(I-1,3) IS NEGATIVE, */
/*          2*N+I      IF T(I,1)*T(I-1,3) IS ZERO WITH */
/*                     ONE FACTOR NON-ZERO. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    t_dim1 = *nm;
    t_offset = t_dim1 + 1;
    t -= t_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
/* L50: */
            z[i + j * z_dim1] = 0.;
        }

        if (i == 1) {
            goto L70;
        }
        h = t[i + t_dim1] * t[i - 1 + t_dim1 * 3];
        if (h < 0.) {
            goto L900;
        } else if (h == 0) {
            goto L60;
        } else {
            goto L80;
        }
L60:
        if (t[i + t_dim1] != 0. || t[i - 1 + t_dim1 * 3] != 0.) {
            goto L1000;
        }
        e[i] = 0.;
L70:
        z[i + i * z_dim1] = 1.;
        goto L90;
L80:
        e[i] = sqrt(h);
        z[i + i * z_dim1] = z[i - 1 + (i - 1) * z_dim1] * e[i] / t[i - 1 + 
                t_dim1 * 3];
L90:
        d[i] = t[i + (t_dim1 << 1)];
/* L100: */
    }

    goto L1001;
/*     .......... SET ERROR -- PRODUCT OF SOME PAIR OF OFF-DIAGONAL */
/*                ELEMENTS IS NEGATIVE .......... */
L900:
    *ierr = *n + i;
    goto L1001;
/*     .......... SET ERROR -- PRODUCT OF SOME PAIR OF OFF-DIAGONAL */
/*                ELEMENTS IS ZERO WITH ONE MEMBER NON-ZERO .......... */
L1000:
    *ierr = (*n << 1) + i;
L1001:
    return 0;
} /* figi2_ */

/* Subroutine */ int hqr_(integer *nm, integer *n, integer *low, integer *igh,
         doublereal *h, doublereal *wr, doublereal *wi, integer *ierr)
{
    /* System generated locals */
    integer h_dim1, h_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal norm;
    static integer i, j, k, l, m;
    static doublereal p, q, r, s, t, w, x, y;
    static integer na, en, ll, mm;
    static doublereal zz;
    static logical notlas;
    static integer mp2, itn, its, enm2;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE HQR, */
/*     NUM. MATH. 14, 219-231(1970) BY MARTIN, PETERS, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 359-371(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A REAL */
/*     UPPER HESSENBERG MATRIX BY THE QR METHOD. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        H CONTAINS THE UPPER HESSENBERG MATRIX.  INFORMATION ABOUT */
/*          THE TRANSFORMATIONS USED IN THE REDUCTION TO HESSENBERG */
/*          FORM BY  ELMHES  OR  ORTHES, IF PERFORMED, IS STORED */
/*          IN THE REMAINING TRIANGLE UNDER THE HESSENBERG MATRIX. */

/*     ON OUTPUT */

/*        H HAS BEEN DESTROYED.  THEREFORE, IT MUST BE SAVED */
/*          BEFORE CALLING  HQR  IF SUBSEQUENT CALCULATION AND */
/*          BACK TRANSFORMATION OF EIGENVECTORS IS TO BE PERFORMED. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  THE EIGENVALUES */
/*          ARE UNORDERED EXCEPT THAT COMPLEX CONJUGATE PAIRS */
/*          OF VALUES APPEAR CONSECUTIVELY WITH THE EIGENVALUE */
/*          HAVING THE POSITIVE IMAGINARY PART FIRST.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --wi;
    --wr;
    h_dim1 = *nm;
    h_offset = h_dim1 + 1;
    h -= h_offset;

    /* Function Body */
    *ierr = 0;
    norm = 0.;
    k = 1;
/*     .......... STORE ROOTS ISOLATED BY BALANC */
/*                AND COMPUTE MATRIX NORM .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
/* L40: */
            norm += (d_1 = h[i + j * h_dim1], abs(d_1));
        }

        k = i;
        if (i >= *low && i <= *igh) {
            goto L50;
        }
        wr[i] = h[i + i * h_dim1];
        wi[i] = 0.;
L50:
        ;
    }

    en = *igh;
    t = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUES .......... */
L60:
    if (en < *low) {
        goto L1001;
    }
    its = 0;
    na = en - 1;
    enm2 = na - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW DO -- .......... */
L70:
    i_1 = en;
    for (ll = *low; ll <= i_1; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L100;
        }
        s = (d_1 = h[l - 1 + (l - 1) * h_dim1], abs(d_1)) + (d_2 = h[l + l 
                * h_dim1], abs(d_2));
        if (s == 0.) {
            s = norm;
        }
        tst1 = s;
        tst2 = tst1 + (d_1 = h[l + (l - 1) * h_dim1], abs(d_1));
        if (tst2 == tst1) {
            goto L100;
        }
/* L80: */
    }
/*     .......... FORM SHIFT .......... */
L100:
    x = h[en + en * h_dim1];
    if (l == en) {
        goto L270;
    }
    y = h[na + na * h_dim1];
    w = h[en + na * h_dim1] * h[na + en * h_dim1];
    if (l == na) {
        goto L280;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its != 10 && its != 20) {
        goto L130;
    }
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
    t += x;

    i_1 = en;
    for (i = *low; i <= i_1; ++i) {
/* L120: */
        h[i + i * h_dim1] -= x;
    }

    s = (d_1 = h[en + na * h_dim1], abs(d_1)) + (d_2 = h[na + enm2 * 
            h_dim1], abs(d_2));
    x = s * .75;
    y = x;
    w = s * -.4375 * s;
L130:
    ++its;
    --itn;
/*     .......... LOOK FOR TWO CONSECUTIVE SMALL */
/*                SUB-DIAGONAL ELEMENTS. */
/*                FOR M=EN-2 STEP -1 UNTIL L DO -- .......... */
    i_1 = enm2;
    for (mm = l; mm <= i_1; ++mm) {
        m = enm2 + l - mm;
        zz = h[m + m * h_dim1];
        r = x - zz;
        s = y - zz;
        p = (r * s - w) / h[m + 1 + m * h_dim1] + h[m + (m + 1) * h_dim1];
        q = h[m + 1 + (m + 1) * h_dim1] - zz - r - s;
        r = h[m + 2 + (m + 1) * h_dim1];
        s = abs(p) + abs(q) + abs(r);
        p /= s;
        q /= s;
        r /= s;
        if (m == l) {
            goto L150;
        }
        tst1 = abs(p) * ((d_1 = h[m - 1 + (m - 1) * h_dim1], abs(d_1)) + 
                abs(zz) + (d_2 = h[m + 1 + (m + 1) * h_dim1], abs(d_2)));
        tst2 = tst1 + (d_1 = h[m + (m - 1) * h_dim1], abs(d_1)) * (abs(q) + 
                abs(r));
        if (tst2 == tst1) {
            goto L150;
        }
/* L140: */
    }

L150:
    mp2 = m + 2;

    i_1 = en;
    for (i = mp2; i <= i_1; ++i) {
        h[i + (i - 2) * h_dim1] = 0.;
        if (i == mp2) {
            goto L160;
        }
        h[i + (i - 3) * h_dim1] = 0.;
L160:
        ;
    }
/*     .......... DOUBLE QR STEP INVOLVING ROWS L TO EN AND */
/*                COLUMNS M TO EN .......... */
    i_1 = na;
    for (k = m; k <= i_1; ++k) {
        notlas = k != na;
        if (k == m) {
            goto L170;
        }
        p = h[k + (k - 1) * h_dim1];
        q = h[k + 1 + (k - 1) * h_dim1];
        r = 0.;
        if (notlas) {
            r = h[k + 2 + (k - 1) * h_dim1];
        }
        x = abs(p) + abs(q) + abs(r);
        if (x == 0.) {
            goto L260;
        }
        p /= x;
        q /= x;
        r /= x;
L170:
        d_1 = sqrt(p * p + q * q + r * r);
        s = d_sign(&d_1, &p);
        if (k == m) {
            goto L180;
        }
        h[k + (k - 1) * h_dim1] = -s * x;
        goto L190;
L180:
        if (l != m) {
            h[k + (k - 1) * h_dim1] = -h[k + (k - 1) * h_dim1];
        }
L190:
        p += s;
        x = p / s;
        y = q / s;
        zz = r / s;
        q /= p;
        r /= p;
        if (notlas) {
            goto L225;
        }
/*     .......... ROW MODIFICATION .......... */
        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            p = h[k + j * h_dim1] + q * h[k + 1 + j * h_dim1];
            h[k + j * h_dim1] -= p * x;
            h[k + 1 + j * h_dim1] -= p * y;
/* L200: */
        }

/* Computing MIN */
        i_2 = en, i_3 = k + 3;
        j = min(i_2,i_3);
/*     .......... COLUMN MODIFICATION .......... */
        i_2 = j;
        for (i = 1; i <= i_2; ++i) {
            p = x * h[i + k * h_dim1] + y * h[i + (k + 1) * h_dim1];
            h[i + k * h_dim1] -= p;
            h[i + (k + 1) * h_dim1] -= p * q;
/* L210: */
        }
        goto L255;
L225:
/*     .......... ROW MODIFICATION .......... */
        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            p = h[k + j * h_dim1] + q * h[k + 1 + j * h_dim1] + r * h[k + 2 + 
                    j * h_dim1];
            h[k + j * h_dim1] -= p * x;
            h[k + 1 + j * h_dim1] -= p * y;
            h[k + 2 + j * h_dim1] -= p * zz;
/* L230: */
        }

/* Computing MIN */
        i_2 = en, i_3 = k + 3;
        j = min(i_2,i_3);
/*     .......... COLUMN MODIFICATION .......... */
        i_2 = j;
        for (i = 1; i <= i_2; ++i) {
            p = x * h[i + k * h_dim1] + y * h[i + (k + 1) * h_dim1] + zz * h[
                    i + (k + 2) * h_dim1];
            h[i + k * h_dim1] -= p;
            h[i + (k + 1) * h_dim1] -= p * q;
            h[i + (k + 2) * h_dim1] -= p * r;
/* L240: */
        }
L255:

L260:
        ;
    }

    goto L70;
/*     .......... ONE ROOT FOUND .......... */
L270:
    wr[en] = x + t;
    wi[en] = 0.;
    en = na;
    goto L60;
/*     .......... TWO ROOTS FOUND .......... */
L280:
    p = (y - x) / 2.;
    q = p * p + w;
    zz = sqrt((abs(q)));
    x += t;
    if (q < 0.) {
        goto L320;
    }
/*     .......... REAL PAIR .......... */
    zz = p + d_sign(&zz, &p);
    wr[na] = x + zz;
    wr[en] = wr[na];
    if (zz != 0.) {
        wr[en] = x - w / zz;
    }
    wi[na] = 0.;
    wi[en] = 0.;
    goto L330;
/*     .......... COMPLEX PAIR .......... */
L320:
    wr[na] = x + p;
    wr[en] = x + p;
    wi[na] = zz;
    wi[en] = -zz;
L330:
    en = enm2;
    goto L60;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* hqr_ */

/* Subroutine */ int hqr2_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *h, doublereal *wr, doublereal *wi, doublereal *z, 
        integer *ierr)
{
    /* System generated locals */
    integer h_dim1, h_offset, z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, k, l, m;
    static doublereal p, q, r, s, t, w, x, y;
    static integer na, ii, en, jj;
    static doublereal ra, sa;
    static integer ll, mm, nn;
    static doublereal vi, vr, zz;
    static logical notlas;
    static integer mp2, itn, its, enm2;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE HQR2, */
/*     NUM. MATH. 16, 181-204(1970) BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 372-395(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES AND EIGENVECTORS */
/*     OF A REAL UPPER HESSENBERG MATRIX BY THE QR METHOD.  THE */
/*     EIGENVECTORS OF A REAL GENERAL MATRIX CAN ALSO BE FOUND */
/*     IF  ELMHES  AND  ELTRAN  OR  ORTHES  AND  ORTRAN  HAVE */
/*     BEEN USED TO REDUCE THIS GENERAL MATRIX TO HESSENBERG FORM */
/*     AND TO ACCUMULATE THE SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        H CONTAINS THE UPPER HESSENBERG MATRIX. */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED BY  ELTRAN */
/*          AFTER THE REDUCTION BY  ELMHES, OR BY  ORTRAN  AFTER THE */
/*          REDUCTION BY  ORTHES, IF PERFORMED.  IF THE EIGENVECTORS */
/*          OF THE HESSENBERG MATRIX ARE DESIRED, Z MUST CONTAIN THE */
/*          IDENTITY MATRIX. */

/*     ON OUTPUT */

/*        H HAS BEEN DESTROYED. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE EIGENVALUES.  THE EIGENVALUES */
/*          ARE UNORDERED EXCEPT THAT COMPLEX CONJUGATE PAIRS */
/*          OF VALUES APPEAR CONSECUTIVELY WITH THE EIGENVALUE */
/*          HAVING THE POSITIVE IMAGINARY PART FIRST.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,...,N. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGENVECTORS. */
/*          IF THE I-TH EIGENVALUE IS REAL, THE I-TH COLUMN OF Z */
/*          CONTAINS ITS EIGENVECTOR.  IF THE I-TH EIGENVALUE IS COMPLEX 
*/
/*          WITH POSITIVE IMAGINARY PART, THE I-TH AND (I+1)-TH */
/*          COLUMNS OF Z CONTAIN THE REAL AND IMAGINARY PARTS OF ITS */
/*          EIGENVECTOR.  THE EIGENVECTORS ARE UNNORMALIZED.  IF AN */
/*          ERROR EXIT IS MADE, NONE OF THE EIGENVECTORS HAS BEEN FOUND. 
*/

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --wi;
    --wr;
    h_dim1 = *nm;
    h_offset = h_dim1 + 1;
    h -= h_offset;

    /* Function Body */
    *ierr = 0;
    norm = 0.;
    k = 1;
/*     .......... STORE ROOTS ISOLATED BY BALANC */
/*                AND COMPUTE MATRIX NORM .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
/* L40: */
            norm += (d_1 = h[i + j * h_dim1], abs(d_1));
        }

        k = i;
        if (i >= *low && i <= *igh) {
            goto L50;
        }
        wr[i] = h[i + i * h_dim1];
        wi[i] = 0.;
L50:
        ;
    }

    en = *igh;
    t = 0.;
    itn = *n * 30;
/*     .......... SEARCH FOR NEXT EIGENVALUES .......... */
L60:
    if (en < *low) {
        goto L340;
    }
    its = 0;
    na = en - 1;
    enm2 = na - 1;
/*     .......... LOOK FOR SINGLE SMALL SUB-DIAGONAL ELEMENT */
/*                FOR L=EN STEP -1 UNTIL LOW DO -- .......... */
L70:
    i_1 = en;
    for (ll = *low; ll <= i_1; ++ll) {
        l = en + *low - ll;
        if (l == *low) {
            goto L100;
        }
        s = (d_1 = h[l - 1 + (l - 1) * h_dim1], abs(d_1)) + (d_2 = h[l + l 
                * h_dim1], abs(d_2));
        if (s == 0.) {
            s = norm;
        }
        tst1 = s;
        tst2 = tst1 + (d_1 = h[l + (l - 1) * h_dim1], abs(d_1));
        if (tst2 == tst1) {
            goto L100;
        }
/* L80: */
    }
/*     .......... FORM SHIFT .......... */
L100:
    x = h[en + en * h_dim1];
    if (l == en) {
        goto L270;
    }
    y = h[na + na * h_dim1];
    w = h[en + na * h_dim1] * h[na + en * h_dim1];
    if (l == na) {
        goto L280;
    }
    if (itn == 0) {
        goto L1000;
    }
    if (its != 10 && its != 20) {
        goto L130;
    }
/*     .......... FORM EXCEPTIONAL SHIFT .......... */
    t += x;

    i_1 = en;
    for (i = *low; i <= i_1; ++i) {
/* L120: */
        h[i + i * h_dim1] -= x;
    }

    s = (d_1 = h[en + na * h_dim1], abs(d_1)) + (d_2 = h[na + enm2 * 
            h_dim1], abs(d_2));
    x = s * .75;
    y = x;
    w = s * -.4375 * s;
L130:
    ++its;
    --itn;
/*     .......... LOOK FOR TWO CONSECUTIVE SMALL */
/*                SUB-DIAGONAL ELEMENTS. */
/*                FOR M=EN-2 STEP -1 UNTIL L DO -- .......... */
    i_1 = enm2;
    for (mm = l; mm <= i_1; ++mm) {
        m = enm2 + l - mm;
        zz = h[m + m * h_dim1];
        r = x - zz;
        s = y - zz;
        p = (r * s - w) / h[m + 1 + m * h_dim1] + h[m + (m + 1) * h_dim1];
        q = h[m + 1 + (m + 1) * h_dim1] - zz - r - s;
        r = h[m + 2 + (m + 1) * h_dim1];
        s = abs(p) + abs(q) + abs(r);
        p /= s;
        q /= s;
        r /= s;
        if (m == l) {
            goto L150;
        }
        tst1 = abs(p) * ((d_1 = h[m - 1 + (m - 1) * h_dim1], abs(d_1)) + 
                abs(zz) + (d_2 = h[m + 1 + (m + 1) * h_dim1], abs(d_2)));
        tst2 = tst1 + (d_1 = h[m + (m - 1) * h_dim1], abs(d_1)) * (abs(q) + 
                abs(r));
        if (tst2 == tst1) {
            goto L150;
        }
/* L140: */
    }

L150:
    mp2 = m + 2;

    i_1 = en;
    for (i = mp2; i <= i_1; ++i) {
        h[i + (i - 2) * h_dim1] = 0.;
        if (i == mp2) {
            goto L160;
        }
        h[i + (i - 3) * h_dim1] = 0.;
L160:
        ;
    }
/*     .......... DOUBLE QR STEP INVOLVING ROWS L TO EN AND */
/*                COLUMNS M TO EN .......... */
    i_1 = na;
    for (k = m; k <= i_1; ++k) {
        notlas = k != na;
        if (k == m) {
            goto L170;
        }
        p = h[k + (k - 1) * h_dim1];
        q = h[k + 1 + (k - 1) * h_dim1];
        r = 0.;
        if (notlas) {
            r = h[k + 2 + (k - 1) * h_dim1];
        }
        x = abs(p) + abs(q) + abs(r);
        if (x == 0.) {
            goto L260;
        }
        p /= x;
        q /= x;
        r /= x;
L170:
        d_1 = sqrt(p * p + q * q + r * r);
        s = d_sign(&d_1, &p);
        if (k == m) {
            goto L180;
        }
        h[k + (k - 1) * h_dim1] = -s * x;
        goto L190;
L180:
        if (l != m) {
            h[k + (k - 1) * h_dim1] = -h[k + (k - 1) * h_dim1];
        }
L190:
        p += s;
        x = p / s;
        y = q / s;
        zz = r / s;
        q /= p;
        r /= p;
        if (notlas) {
            goto L225;
        }
/*     .......... ROW MODIFICATION .......... */
        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            p = h[k + j * h_dim1] + q * h[k + 1 + j * h_dim1];
            h[k + j * h_dim1] -= p * x;
            h[k + 1 + j * h_dim1] -= p * y;
/* L200: */
        }

/* Computing MIN */
        i_2 = en, i_3 = k + 3;
        j = min(i_2,i_3);
/*     .......... COLUMN MODIFICATION .......... */
        i_2 = j;
        for (i = 1; i <= i_2; ++i) {
            p = x * h[i + k * h_dim1] + y * h[i + (k + 1) * h_dim1];
            h[i + k * h_dim1] -= p;
            h[i + (k + 1) * h_dim1] -= p * q;
/* L210: */
        }
/*     .......... ACCUMULATE TRANSFORMATIONS .......... */
        i_2 = *igh;
        for (i = *low; i <= i_2; ++i) {
            p = x * z[i + k * z_dim1] + y * z[i + (k + 1) * z_dim1];
            z[i + k * z_dim1] -= p;
            z[i + (k + 1) * z_dim1] -= p * q;
/* L220: */
        }
        goto L255;
L225:
/*     .......... ROW MODIFICATION .......... */
        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            p = h[k + j * h_dim1] + q * h[k + 1 + j * h_dim1] + r * h[k + 2 + 
                    j * h_dim1];
            h[k + j * h_dim1] -= p * x;
            h[k + 1 + j * h_dim1] -= p * y;
            h[k + 2 + j * h_dim1] -= p * zz;
/* L230: */
        }

/* Computing MIN */
        i_2 = en, i_3 = k + 3;
        j = min(i_2,i_3);
/*     .......... COLUMN MODIFICATION .......... */
        i_2 = j;
        for (i = 1; i <= i_2; ++i) {
            p = x * h[i + k * h_dim1] + y * h[i + (k + 1) * h_dim1] + zz * h[
                    i + (k + 2) * h_dim1];
            h[i + k * h_dim1] -= p;
            h[i + (k + 1) * h_dim1] -= p * q;
            h[i + (k + 2) * h_dim1] -= p * r;
/* L240: */
        }
/*     .......... ACCUMULATE TRANSFORMATIONS .......... */
        i_2 = *igh;
        for (i = *low; i <= i_2; ++i) {
            p = x * z[i + k * z_dim1] + y * z[i + (k + 1) * z_dim1] + zz * z[
                    i + (k + 2) * z_dim1];
            z[i + k * z_dim1] -= p;
            z[i + (k + 1) * z_dim1] -= p * q;
            z[i + (k + 2) * z_dim1] -= p * r;
/* L250: */
        }
L255:

L260:
        ;
    }

    goto L70;
/*     .......... ONE ROOT FOUND .......... */
L270:
    h[en + en * h_dim1] = x + t;
    wr[en] = h[en + en * h_dim1];
    wi[en] = 0.;
    en = na;
    goto L60;
/*     .......... TWO ROOTS FOUND .......... */
L280:
    p = (y - x) / 2.;
    q = p * p + w;
    zz = sqrt((abs(q)));
    h[en + en * h_dim1] = x + t;
    x = h[en + en * h_dim1];
    h[na + na * h_dim1] = y + t;
    if (q < 0.) {
        goto L320;
    }
/*     .......... REAL PAIR .......... */
    zz = p + d_sign(&zz, &p);
    wr[na] = x + zz;
    wr[en] = wr[na];
    if (zz != 0.) {
        wr[en] = x - w / zz;
    }
    wi[na] = 0.;
    wi[en] = 0.;
    x = h[en + na * h_dim1];
    s = abs(x) + abs(zz);
    p = x / s;
    q = zz / s;
    r = sqrt(p * p + q * q);
    p /= r;
    q /= r;
/*     .......... ROW MODIFICATION .......... */
    i_1 = *n;
    for (j = na; j <= i_1; ++j) {
        zz = h[na + j * h_dim1];
        h[na + j * h_dim1] = q * zz + p * h[en + j * h_dim1];
        h[en + j * h_dim1] = q * h[en + j * h_dim1] - p * zz;
/* L290: */
    }
/*     .......... COLUMN MODIFICATION .......... */
    i_1 = en;
    for (i = 1; i <= i_1; ++i) {
        zz = h[i + na * h_dim1];
        h[i + na * h_dim1] = q * zz + p * h[i + en * h_dim1];
        h[i + en * h_dim1] = q * h[i + en * h_dim1] - p * zz;
/* L300: */
    }
/*     .......... ACCUMULATE TRANSFORMATIONS .......... */
    i_1 = *igh;
    for (i = *low; i <= i_1; ++i) {
        zz = z[i + na * z_dim1];
        z[i + na * z_dim1] = q * zz + p * z[i + en * z_dim1];
        z[i + en * z_dim1] = q * z[i + en * z_dim1] - p * zz;
/* L310: */
    }

    goto L330;
/*     .......... COMPLEX PAIR .......... */
L320:
    wr[na] = x + p;
    wr[en] = x + p;
    wi[na] = zz;
    wi[en] = -zz;
L330:
    en = enm2;
    goto L60;
/*     .......... ALL ROOTS FOUND.  BACKSUBSTITUTE TO FIND */
/*                VECTORS OF UPPER TRIANGULAR FORM .......... */
L340:
    if (norm == 0.) {
        goto L1001;
    }
/*     .......... FOR EN=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (nn = 1; nn <= i_1; ++nn) {
        en = *n + 1 - nn;
        p = wr[en];
        q = wi[en];
        na = en - 1;
        if (q < 0.) {
            goto L710;
        } else if (q == 0) {
            goto L600;
        } else {
            goto L800;
        }
/*     .......... REAL VECTOR .......... */
L600:
        m = en;
        h[en + en * h_dim1] = 1.;
        if (na == 0) {
            goto L800;
        }
/*     .......... FOR I=EN-1 STEP -1 UNTIL 1 DO -- .......... */
        i_2 = na;
        for (ii = 1; ii <= i_2; ++ii) {
            i = en - ii;
            w = h[i + i * h_dim1] - p;
            r = 0.;

            i_3 = en;
            for (j = m; j <= i_3; ++j) {
/* L610: */
                r += h[i + j * h_dim1] * h[j + en * h_dim1];
            }

            if (wi[i] >= 0.) {
                goto L630;
            }
            zz = w;
            s = r;
            goto L700;
L630:
            m = i;
            if (wi[i] != 0.) {
                goto L640;
            }
            t = w;
            if (t != 0.) {
                goto L635;
            }
            tst1 = norm;
            t = tst1;
L632:
            t *= .01;
            tst2 = norm + t;
            if (tst2 > tst1) {
                goto L632;
            }
L635:
            h[i + en * h_dim1] = -r / t;
            goto L680;
/*     .......... SOLVE REAL EQUATIONS .......... */
L640:
            x = h[i + (i + 1) * h_dim1];
            y = h[i + 1 + i * h_dim1];
            q = (wr[i] - p) * (wr[i] - p) + wi[i] * wi[i];
            t = (x * s - zz * r) / q;
            h[i + en * h_dim1] = t;
            if (abs(x) <= abs(zz)) {
                goto L650;
            }
            h[i + 1 + en * h_dim1] = (-r - w * t) / x;
            goto L680;
L650:
            h[i + 1 + en * h_dim1] = (-s - y * t) / zz;

/*     .......... OVERFLOW CONTROL .......... */
L680:
            t = (d_1 = h[i + en * h_dim1], abs(d_1));
            if (t == 0.) {
                goto L700;
            }
            tst1 = t;
            tst2 = tst1 + 1. / tst1;
            if (tst2 > tst1) {
                goto L700;
            }
            i_3 = en;
            for (j = i; j <= i_3; ++j) {
                h[j + en * h_dim1] /= t;
/* L690: */
            }

L700:
            ;
        }
/*     .......... END REAL VECTOR .......... */
        goto L800;
/*     .......... COMPLEX VECTOR .......... */
L710:
        m = na;
/*     .......... LAST VECTOR COMPONENT CHOSEN IMAGINARY SO THAT */
/*                EIGENVECTOR MATRIX IS TRIANGULAR .......... */
        if ((d_1 = h[en + na * h_dim1], abs(d_1)) <= (d_2 = h[na + en * 
                h_dim1], abs(d_2))) {
            goto L720;
        }
        h[na + na * h_dim1] = q / h[en + na * h_dim1];
        h[na + en * h_dim1] = -(h[en + en * h_dim1] - p) / h[en + na * h_dim1]
                ;
        goto L730;
L720:
        d_1 = -h[na + en * h_dim1];
        d_2 = h[na + na * h_dim1] - p;
        cdiv_(&c_b550, &d_1, &d_2, &q, &h[na + na * h_dim1], &h[na + en * 
                h_dim1]);
L730:
        h[en + na * h_dim1] = 0.;
        h[en + en * h_dim1] = 1.;
        enm2 = na - 1;
        if (enm2 == 0) {
            goto L800;
        }
/*     .......... FOR I=EN-2 STEP -1 UNTIL 1 DO -- .......... */
        i_2 = enm2;
        for (ii = 1; ii <= i_2; ++ii) {
            i = na - ii;
            w = h[i + i * h_dim1] - p;
            ra = 0.;
            sa = 0.;

            i_3 = en;
            for (j = m; j <= i_3; ++j) {
                ra += h[i + j * h_dim1] * h[j + na * h_dim1];
                sa += h[i + j * h_dim1] * h[j + en * h_dim1];
/* L760: */
            }

            if (wi[i] >= 0.) {
                goto L770;
            }
            zz = w;
            r = ra;
            s = sa;
            goto L795;
L770:
            m = i;
            if (wi[i] != 0.) {
                goto L780;
            }
            d_1 = -ra;
            d_2 = -sa;
            cdiv_(&d_1, &d_2, &w, &q, &h[i + na * h_dim1], &h[i + en * 
                    h_dim1]);
            goto L790;
/*     .......... SOLVE COMPLEX EQUATIONS .......... */
L780:
            x = h[i + (i + 1) * h_dim1];
            y = h[i + 1 + i * h_dim1];
            vr = (wr[i] - p) * (wr[i] - p) + wi[i] * wi[i] - q * q;
            vi = (wr[i] - p) * 2. * q;
            if (vr != 0. || vi != 0.) {
                goto L784;
            }
            tst1 = norm * (abs(w) + abs(q) + abs(x) + abs(y) + abs(zz));
            vr = tst1;
L783:
            vr *= .01;
            tst2 = tst1 + vr;
            if (tst2 > tst1) {
                goto L783;
            }
L784:
            d_1 = x * r - zz * ra + q * sa;
            d_2 = x * s - zz * sa - q * ra;
            cdiv_(&d_1, &d_2, &vr, &vi, &h[i + na * h_dim1], &h[i + en * 
                    h_dim1]);
            if (abs(x) <= abs(zz) + abs(q)) {
                goto L785;
            }
            h[i + 1 + na * h_dim1] = (-ra - w * h[i + na * h_dim1] + q * h[i 
                    + en * h_dim1]) / x;
            h[i + 1 + en * h_dim1] = (-sa - w * h[i + en * h_dim1] - q * h[i 
                    + na * h_dim1]) / x;
            goto L790;
L785:
            d_1 = -r - y * h[i + na * h_dim1];
            d_2 = -s - y * h[i + en * h_dim1];
            cdiv_(&d_1, &d_2, &zz, &q, &h[i + 1 + na * h_dim1], &h[i + 1 + 
                    en * h_dim1]);

/*     .......... OVERFLOW CONTROL .......... */
L790:
/* Computing MAX */
            d_3 = (d_1 = h[i + na * h_dim1], abs(d_1)), d_4 = (d_2 = h[i 
                    + en * h_dim1], abs(d_2));
            t = max(d_3,d_4);
            if (t == 0.) {
                goto L795;
            }
            tst1 = t;
            tst2 = tst1 + 1. / tst1;
            if (tst2 > tst1) {
                goto L795;
            }
            i_3 = en;
            for (j = i; j <= i_3; ++j) {
                h[j + na * h_dim1] /= t;
                h[j + en * h_dim1] /= t;
/* L792: */
            }

L795:
            ;
        }
/*     .......... END COMPLEX VECTOR .......... */
L800:
        ;
    }
/*     .......... END BACK SUBSTITUTION. */
/*                VECTORS OF ISOLATED ROOTS .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i >= *low && i <= *igh) {
            goto L840;
        }

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
/* L820: */
            z[i + j * z_dim1] = h[i + j * h_dim1];
        }

L840:
        ;
    }
/*     .......... MULTIPLY BY TRANSFORMATION MATRIX TO GIVE */
/*                VECTORS OF ORIGINAL FULL MATRIX. */
/*                FOR J=N STEP -1 UNTIL LOW DO -- .......... */
    i_1 = *n;
    for (jj = *low; jj <= i_1; ++jj) {
        j = *n + *low - jj;
        m = min(j,*igh);

        i_2 = *igh;
        for (i = *low; i <= i_2; ++i) {
            zz = 0.;

            i_3 = m;
            for (k = *low; k <= i_3; ++k) {
/* L860: */
                zz += z[i + k * z_dim1] * h[k + j * h_dim1];
            }

            z[i + j * z_dim1] = zz;
/* L880: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
L1001:
    return 0;
} /* hqr2_ */

/* Subroutine */ int htrib3_(integer *nm, integer *n, doublereal *a, 
        doublereal *tau, integer *m, doublereal *zr, doublereal *zi)
{
    /* System generated locals */
    integer a_dim1, a_offset, zr_dim1, zr_offset, zi_dim1, zi_offset, i_1, 
            i_2, i_3;

    /* Local variables */
    static doublereal h;
    static integer i, j, k, l;
    static doublereal s, si;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE TRBAK3, NUM. MATH. 11, 181-195(1968) */
/*     BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A COMPLEX HERMITIAN */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     REAL SYMMETRIC TRIDIAGONAL MATRIX DETERMINED BY  HTRID3. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS INFORMATION ABOUT THE UNITARY TRANSFORMATIONS */
/*          USED IN THE REDUCTION BY  HTRID3. */

/*        TAU CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATIONS. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        ZR CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE TRANSFORMED EIGENVECTORS */
/*          IN THEIR FIRST M COLUMNS. */

/*     NOTE THAT THE LAST COMPONENT OF EACH RETURNED VECTOR */
/*     IS REAL AND THAT VECTOR EUCLIDEAN NORMS ARE PRESERVED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    tau -= 3;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
/*     .......... TRANSFORM THE EIGENVECTORS OF THE REAL SYMMETRIC */
/*                TRIDIAGONAL MATRIX TO THOSE OF THE HERMITIAN */
/*                TRIDIAGONAL MATRIX. .......... */
    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            zi[k + j * zi_dim1] = -zr[k + j * zr_dim1] * tau[(k << 1) + 2];
            zr[k + j * zr_dim1] *= tau[(k << 1) + 1];
/* L50: */
        }
    }

    if (*n == 1) {
        goto L200;
    }
/*     .......... RECOVER AND APPLY THE HOUSEHOLDER MATRICES .......... */
    i_2 = *n;
    for (i = 2; i <= i_2; ++i) {
        l = i - 1;
        h = a[i + i * a_dim1];
        if (h == 0.) {
            goto L140;
        }

        i_1 = *m;
        for (j = 1; j <= i_1; ++j) {
            s = 0.;
            si = 0.;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                s = s + a[i + k * a_dim1] * zr[k + j * zr_dim1] - a[k + i * 
                        a_dim1] * zi[k + j * zi_dim1];
                si = si + a[i + k * a_dim1] * zi[k + j * zi_dim1] + a[k + i * 
                        a_dim1] * zr[k + j * zr_dim1];
/* L110: */
            }
/*     .......... DOUBLE DIVISIONS AVOID POSSIBLE UNDERFLOW ......
.... */
            s = s / h / h;
            si = si / h / h;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                zr[k + j * zr_dim1] = zr[k + j * zr_dim1] - s * a[i + k * 
                        a_dim1] - si * a[k + i * a_dim1];
                zi[k + j * zi_dim1] = zi[k + j * zi_dim1] - si * a[i + k * 
                        a_dim1] + s * a[k + i * a_dim1];
/* L120: */
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* htrib3_ */

/* Subroutine */ int htribk_(integer *nm, integer *n, doublereal *ar, 
        doublereal *ai, doublereal *tau, integer *m, doublereal *zr, 
        doublereal *zi)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, zr_dim1, zr_offset, 
            zi_dim1, zi_offset, i_1, i_2, i_3;

    /* Local variables */
    static doublereal h;
    static integer i, j, k, l;
    static doublereal s, si;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE TRBAK1, NUM. MATH. 11, 181-195(1968) */
/*     BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A COMPLEX HERMITIAN */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     REAL SYMMETRIC TRIDIAGONAL MATRIX DETERMINED BY  HTRIDI. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN INFORMATION ABOUT THE UNITARY TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  HTRIDI  IN THEIR */
/*          FULL LOWER TRIANGLES EXCEPT FOR THE DIAGONAL OF AR. */

/*        TAU CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATIONS. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        ZR CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        ZR AND ZI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE TRANSFORMED EIGENVECTORS */
/*          IN THEIR FIRST M COLUMNS. */

/*     NOTE THAT THE LAST COMPONENT OF EACH RETURNED VECTOR */
/*     IS REAL AND THAT VECTOR EUCLIDEAN NORMS ARE PRESERVED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    tau -= 3;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;
    zi_dim1 = *nm;
    zi_offset = zi_dim1 + 1;
    zi -= zi_offset;
    zr_dim1 = *nm;
    zr_offset = zr_dim1 + 1;
    zr -= zr_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
/*     .......... TRANSFORM THE EIGENVECTORS OF THE REAL SYMMETRIC */
/*                TRIDIAGONAL MATRIX TO THOSE OF THE HERMITIAN */
/*                TRIDIAGONAL MATRIX. .......... */
    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            zi[k + j * zi_dim1] = -zr[k + j * zr_dim1] * tau[(k << 1) + 2];
            zr[k + j * zr_dim1] *= tau[(k << 1) + 1];
/* L50: */
        }
    }

    if (*n == 1) {
        goto L200;
    }
/*     .......... RECOVER AND APPLY THE HOUSEHOLDER MATRICES .......... */
    i_2 = *n;
    for (i = 2; i <= i_2; ++i) {
        l = i - 1;
        h = ai[i + i * ai_dim1];
        if (h == 0.) {
            goto L140;
        }

        i_1 = *m;
        for (j = 1; j <= i_1; ++j) {
            s = 0.;
            si = 0.;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                s = s + ar[i + k * ar_dim1] * zr[k + j * zr_dim1] - ai[i + k *
                         ai_dim1] * zi[k + j * zi_dim1];
                si = si + ar[i + k * ar_dim1] * zi[k + j * zi_dim1] + ai[i + 
                        k * ai_dim1] * zr[k + j * zr_dim1];
/* L110: */
            }
/*     .......... DOUBLE DIVISIONS AVOID POSSIBLE UNDERFLOW ......
.... */
            s = s / h / h;
            si = si / h / h;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                zr[k + j * zr_dim1] = zr[k + j * zr_dim1] - s * ar[i + k * 
                        ar_dim1] - si * ai[i + k * ai_dim1];
                zi[k + j * zi_dim1] = zi[k + j * zi_dim1] - si * ar[i + k * 
                        ar_dim1] + s * ai[i + k * ai_dim1];
/* L120: */
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* htribk_ */

/* Subroutine */ int htrid3_(integer *nm, integer *n, doublereal *a, 
        doublereal *d, doublereal *e, doublereal *e2, doublereal *tau)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, k, l;
    static doublereal scale, fi, gi, hh;
    static integer ii;
    static doublereal si;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer jm1, jp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE TRED3, NUM. MATH. 11, 181-195(1968) */
/*     BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE REDUCES A COMPLEX HERMITIAN MATRIX, STORED AS */
/*     A SINGLE SQUARE ARRAY, TO A REAL SYMMETRIC TRIDIAGONAL MATRIX */
/*     USING UNITARY SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE LOWER TRIANGLE OF THE COMPLEX HERMITIAN INPUT */
/*          MATRIX.  THE REAL PARTS OF THE MATRIX ELEMENTS ARE STORED */
/*          IN THE FULL LOWER TRIANGLE OF A, AND THE IMAGINARY PARTS */
/*          ARE STORED IN THE TRANSPOSED POSITIONS OF THE STRICT UPPER */
/*          TRIANGLE OF A.  NO STORAGE IS REQUIRED FOR THE ZERO */
/*          IMAGINARY PARTS OF THE DIAGONAL ELEMENTS. */

/*     ON OUTPUT */

/*        A CONTAINS INFORMATION ABOUT THE UNITARY TRANSFORMATIONS */
/*          USED IN THE REDUCTION. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE THE TRIDIAGONAL MATRIX. 
*/

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*        TAU CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    tau -= 3;
    --e2;
    --e;
    --d;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    tau[(*n << 1) + 1] = 1.;
    tau[(*n << 1) + 2] = 0.;
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *n + 1 - ii;
        l = i - 1;
        h = 0.;
        scale = 0.;
        if (l < 1) {
            goto L130;
        }
/*     .......... SCALE ROW (ALGOL TOL THEN NOT NEEDED) .......... */
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
/* L120: */
            scale = scale + (d_1 = a[i + k * a_dim1], abs(d_1)) + (d_2 = a[
                    k + i * a_dim1], abs(d_2));
        }

        if (scale != 0.) {
            goto L140;
        }
        tau[(l << 1) + 1] = 1.;
        tau[(l << 1) + 2] = 0.;
L130:
        e[i] = 0.;
        e2[i] = 0.;
        goto L290;

L140:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            a[i + k * a_dim1] /= scale;
            a[k + i * a_dim1] /= scale;
            h = h + a[i + k * a_dim1] * a[i + k * a_dim1] + a[k + i * a_dim1] 
                    * a[k + i * a_dim1];
/* L150: */
        }

        e2[i] = scale * scale * h;
        g = sqrt(h);
        e[i] = scale * g;
        f = pythag_(&a[i + l * a_dim1], &a[l + i * a_dim1]);
/*     .......... FORM NEXT DIAGONAL ELEMENT OF MATRIX T .......... */
        if (f == 0.) {
            goto L160;
        }
        tau[(l << 1) + 1] = (a[l + i * a_dim1] * tau[(i << 1) + 2] - a[i + l *
                 a_dim1] * tau[(i << 1) + 1]) / f;
        si = (a[i + l * a_dim1] * tau[(i << 1) + 2] + a[l + i * a_dim1] * tau[
                (i << 1) + 1]) / f;
        h += f * g;
        g = g / f + 1.;
        a[i + l * a_dim1] = g * a[i + l * a_dim1];
        a[l + i * a_dim1] = g * a[l + i * a_dim1];
        if (l == 1) {
            goto L270;
        }
        goto L170;
L160:
        tau[(l << 1) + 1] = -tau[(i << 1) + 1];
        si = tau[(i << 1) + 2];
        a[i + l * a_dim1] = g;
L170:
        f = 0.;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            g = 0.;
            gi = 0.;
            if (j == 1) {
                goto L190;
            }
            jm1 = j - 1;
/*     .......... FORM ELEMENT OF A*U .......... */
            i_3 = jm1;
            for (k = 1; k <= i_3; ++k) {
                g = g + a[j + k * a_dim1] * a[i + k * a_dim1] + a[k + j * 
                        a_dim1] * a[k + i * a_dim1];
                gi = gi - a[j + k * a_dim1] * a[k + i * a_dim1] + a[k + j * 
                        a_dim1] * a[i + k * a_dim1];
/* L180: */
            }

L190:
            g += a[j + j * a_dim1] * a[i + j * a_dim1];
            gi -= a[j + j * a_dim1] * a[j + i * a_dim1];
            jp1 = j + 1;
            if (l < jp1) {
                goto L220;
            }

            i_3 = l;
            for (k = jp1; k <= i_3; ++k) {
                g = g + a[k + j * a_dim1] * a[i + k * a_dim1] - a[j + k * 
                        a_dim1] * a[k + i * a_dim1];
                gi = gi - a[k + j * a_dim1] * a[k + i * a_dim1] - a[j + k * 
                        a_dim1] * a[i + k * a_dim1];
/* L200: */
            }
/*     .......... FORM ELEMENT OF P .......... */
L220:
            e[j] = g / h;
            tau[(j << 1) + 2] = gi / h;
            f = f + e[j] * a[i + j * a_dim1] - tau[(j << 1) + 2] * a[j + i * 
                    a_dim1];
/* L240: */
        }

        hh = f / (h + h);
/*     .......... FORM REDUCED A .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = a[i + j * a_dim1];
            g = e[j] - hh * f;
            e[j] = g;
            fi = -a[j + i * a_dim1];
            gi = tau[(j << 1) + 2] - hh * fi;
            tau[(j << 1) + 2] = -gi;
            a[j + j * a_dim1] -= (f * g + fi * gi) * 2.;
            if (j == 1) {
                goto L260;
            }
            jm1 = j - 1;

            i_3 = jm1;
            for (k = 1; k <= i_3; ++k) {
                a[j + k * a_dim1] = a[j + k * a_dim1] - f * e[k] - g * a[i + 
                        k * a_dim1] + fi * tau[(k << 1) + 2] + gi * a[k + i * 
                        a_dim1];
                a[k + j * a_dim1] = a[k + j * a_dim1] - f * tau[(k << 1) + 2] 
                        - g * a[k + i * a_dim1] - fi * e[k] - gi * a[i + k * 
                        a_dim1];
/* L250: */
            }

L260:
            ;
        }

L270:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            a[i + k * a_dim1] = scale * a[i + k * a_dim1];
            a[k + i * a_dim1] = scale * a[k + i * a_dim1];
/* L280: */
        }

        tau[(l << 1) + 2] = -si;
L290:
        d[i] = a[i + i * a_dim1];
        a[i + i * a_dim1] = scale * sqrt(h);
/* L300: */
    }

    return 0;
} /* htrid3_ */

/* Subroutine */ int htridi_(integer *nm, integer *n, doublereal *ar, 
        doublereal *ai, doublereal *d, doublereal *e, doublereal *e2, 
        doublereal *tau)
{
    /* System generated locals */
    integer ar_dim1, ar_offset, ai_dim1, ai_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, k, l;
    static doublereal scale, fi, gi, hh;
    static integer ii;
    static doublereal si;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer jp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF A COMPLEX ANALOGUE OF */
/*     THE ALGOL PROCEDURE TRED1, NUM. MATH. 11, 181-195(1968) */
/*     BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE REDUCES A COMPLEX HERMITIAN MATRIX */
/*     TO A REAL SYMMETRIC TRIDIAGONAL MATRIX USING */
/*     UNITARY SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        AR AND AI CONTAIN THE REAL AND IMAGINARY PARTS, */
/*          RESPECTIVELY, OF THE COMPLEX HERMITIAN INPUT MATRIX. */
/*          ONLY THE LOWER TRIANGLE OF THE MATRIX NEED BE SUPPLIED. */

/*     ON OUTPUT */

/*        AR AND AI CONTAIN INFORMATION ABOUT THE UNITARY TRANS- */
/*          FORMATIONS USED IN THE REDUCTION IN THEIR FULL LOWER */
/*          TRIANGLES.  THEIR STRICT UPPER TRIANGLES AND THE */
/*          DIAGONAL OF AR ARE UNALTERED. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE THE TRIDIAGONAL MATRIX. 
*/

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*        TAU CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    tau -= 3;
    --e2;
    --e;
    --d;
    ai_dim1 = *nm;
    ai_offset = ai_dim1 + 1;
    ai -= ai_offset;
    ar_dim1 = *nm;
    ar_offset = ar_dim1 + 1;
    ar -= ar_offset;

    /* Function Body */
    tau[(*n << 1) + 1] = 1.;
    tau[(*n << 1) + 2] = 0.;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L100: */
        d[i] = ar[i + i * ar_dim1];
    }
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *n + 1 - ii;
        l = i - 1;
        h = 0.;
        scale = 0.;
        if (l < 1) {
            goto L130;
        }
/*     .......... SCALE ROW (ALGOL TOL THEN NOT NEEDED) .......... */
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
/* L120: */
            scale = scale + (d_1 = ar[i + k * ar_dim1], abs(d_1)) + (d_2 = 
                    ai[i + k * ai_dim1], abs(d_2));
        }

        if (scale != 0.) {
            goto L140;
        }
        tau[(l << 1) + 1] = 1.;
        tau[(l << 1) + 2] = 0.;
L130:
        e[i] = 0.;
        e2[i] = 0.;
        goto L290;

L140:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            ar[i + k * ar_dim1] /= scale;
            ai[i + k * ai_dim1] /= scale;
            h = h + ar[i + k * ar_dim1] * ar[i + k * ar_dim1] + ai[i + k * 
                    ai_dim1] * ai[i + k * ai_dim1];
/* L150: */
        }

        e2[i] = scale * scale * h;
        g = sqrt(h);
        e[i] = scale * g;
        f = pythag_(&ar[i + l * ar_dim1], &ai[i + l * ai_dim1]);
/*     .......... FORM NEXT DIAGONAL ELEMENT OF MATRIX T .......... */
        if (f == 0.) {
            goto L160;
        }
        tau[(l << 1) + 1] = (ai[i + l * ai_dim1] * tau[(i << 1) + 2] - ar[i + 
                l * ar_dim1] * tau[(i << 1) + 1]) / f;
        si = (ar[i + l * ar_dim1] * tau[(i << 1) + 2] + ai[i + l * ai_dim1] * 
                tau[(i << 1) + 1]) / f;
        h += f * g;
        g = g / f + 1.;
        ar[i + l * ar_dim1] = g * ar[i + l * ar_dim1];
        ai[i + l * ai_dim1] = g * ai[i + l * ai_dim1];
        if (l == 1) {
            goto L270;
        }
        goto L170;
L160:
        tau[(l << 1) + 1] = -tau[(i << 1) + 1];
        si = tau[(i << 1) + 2];
        ar[i + l * ar_dim1] = g;
L170:
        f = 0.;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            g = 0.;
            gi = 0.;
/*     .......... FORM ELEMENT OF A*U .......... */
            i_3 = j;
            for (k = 1; k <= i_3; ++k) {
                g = g + ar[j + k * ar_dim1] * ar[i + k * ar_dim1] + ai[j + k *
                         ai_dim1] * ai[i + k * ai_dim1];
                gi = gi - ar[j + k * ar_dim1] * ai[i + k * ai_dim1] + ai[j + 
                        k * ai_dim1] * ar[i + k * ar_dim1];
/* L180: */
            }

            jp1 = j + 1;
            if (l < jp1) {
                goto L220;
            }

            i_3 = l;
            for (k = jp1; k <= i_3; ++k) {
                g = g + ar[k + j * ar_dim1] * ar[i + k * ar_dim1] - ai[k + j *
                         ai_dim1] * ai[i + k * ai_dim1];
                gi = gi - ar[k + j * ar_dim1] * ai[i + k * ai_dim1] - ai[k + 
                        j * ai_dim1] * ar[i + k * ar_dim1];
/* L200: */
            }
/*     .......... FORM ELEMENT OF P .......... */
L220:
            e[j] = g / h;
            tau[(j << 1) + 2] = gi / h;
            f = f + e[j] * ar[i + j * ar_dim1] - tau[(j << 1) + 2] * ai[i + j 
                    * ai_dim1];
/* L240: */
        }

        hh = f / (h + h);
/*     .......... FORM REDUCED A .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = ar[i + j * ar_dim1];
            g = e[j] - hh * f;
            e[j] = g;
            fi = -ai[i + j * ai_dim1];
            gi = tau[(j << 1) + 2] - hh * fi;
            tau[(j << 1) + 2] = -gi;

            i_3 = j;
            for (k = 1; k <= i_3; ++k) {
                ar[j + k * ar_dim1] = ar[j + k * ar_dim1] - f * e[k] - g * ar[
                        i + k * ar_dim1] + fi * tau[(k << 1) + 2] + gi * ai[i 
                        + k * ai_dim1];
                ai[j + k * ai_dim1] = ai[j + k * ai_dim1] - f * tau[(k << 1) 
                        + 2] - g * ai[i + k * ai_dim1] - fi * e[k] - gi * ar[
                        i + k * ar_dim1];
/* L260: */
            }
        }

L270:
        i_3 = l;
        for (k = 1; k <= i_3; ++k) {
            ar[i + k * ar_dim1] = scale * ar[i + k * ar_dim1];
            ai[i + k * ai_dim1] = scale * ai[i + k * ai_dim1];
/* L280: */
        }

        tau[(l << 1) + 2] = -si;
L290:
        hh = d[i];
        d[i] = ar[i + i * ar_dim1];
        ar[i + i * ar_dim1] = hh;
        ai[i + i * ai_dim1] = scale * sqrt(h);
/* L300: */
    }

    return 0;
} /* htridi_ */

/* Subroutine */ int imtql1_(integer *n, doublereal *d, doublereal *e, 
        integer *ierr)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal b, c, f, g;
    static integer i, j, l, m;
    static doublereal p, r, s;
    static integer ii;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer mml;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE IMTQL1, */
/*     NUM. MATH. 12, 377-383(1968) BY MARTIN AND WILKINSON, */
/*     AS MODIFIED IN NUM. MATH. 15, 450(1970) BY DUBRULLE. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 241-248(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A SYMMETRIC */
/*     TRIDIAGONAL MATRIX BY THE IMPLICIT QL METHOD. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*      ON OUTPUT */

/*        D CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT AND */
/*          ORDERED FOR INDICES 1,2,...IERR-1, BUT MAY NOT BE */
/*          THE SMALLEST EIGENVALUES. */

/*        E HAS BEEN DESTROYED. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
/* L100: */
        e[i - 1] = e[i];
    }

    e[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ELEMENT .......... */
L105:
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            if (m == *n) {
                goto L120;
            }
            tst1 = (d_1 = d[m], abs(d_1)) + (d_2 = d[m + 1], abs(d_2));
            tst2 = tst1 + (d_1 = e[m], abs(d_1));
            if (tst2 == tst1) {
                goto L120;
            }
/* L110: */
        }

L120:
        p = d[l];
        if (m == l) {
            goto L215;
        }
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        g = (d[l + 1] - p) / (e[l] * 2.);
        r = pythag_(&g, &c_b141);
        g = d[m] - p + e[l] / (g + d_sign(&r, &g));
        s = 1.;
        c = 1.;
        p = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            i = m - ii;
            f = s * e[i];
            b = c * e[i];
            r = pythag_(&f, &g);
            e[i + 1] = r;
            if (r == 0.) {
                goto L210;
            }
            s = f / r;
            c = g / r;
            g = d[i + 1] - p;
            r = (d[i] - g) * s + c * 2. * b;
            p = s * r;
            d[i + 1] = g + p;
            g = c * r - b;
/* L200: */
        }

        d[l] -= p;
        e[l] = g;
        e[m] = 0.;
        goto L105;
/*     .......... RECOVER FROM UNDERFLOW .......... */
L210:
        d[i + 1] -= p;
        e[m] = 0.;
        goto L105;
/*     .......... ORDER EIGENVALUES .......... */
L215:
        if (l == 1) {
            goto L250;
        }
/*     .......... FOR I=L STEP -1 UNTIL 2 DO -- .......... */
        i_2 = l;
        for (ii = 2; ii <= i_2; ++ii) {
            i = l + 2 - ii;
            if (p >= d[i - 1]) {
                goto L270;
            }
            d[i] = d[i - 1];
/* L230: */
        }

L250:
        i = 1;
L270:
        d[i] = p;
/* L290: */
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* imtql1_ */

/* Subroutine */ int imtql2_(integer *nm, integer *n, doublereal *d, 
        doublereal *e, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal b, c, f, g;
    static integer i, j, k, l, m;
    static doublereal p, r, s;
    static integer ii;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer mml;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE IMTQL2, */
/*     NUM. MATH. 12, 377-383(1968) BY MARTIN AND WILKINSON, */
/*     AS MODIFIED IN NUM. MATH. 15, 450(1970) BY DUBRULLE. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 241-248(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES AND EIGENVECTORS */
/*     OF A SYMMETRIC TRIDIAGONAL MATRIX BY THE IMPLICIT QL METHOD. */
/*     THE EIGENVECTORS OF A FULL SYMMETRIC MATRIX CAN ALSO */
/*     BE FOUND IF  TRED2  HAS BEEN USED TO REDUCE THIS */
/*     FULL MATRIX TO TRIDIAGONAL FORM. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN THE */
/*          REDUCTION BY  TRED2, IF PERFORMED.  IF THE EIGENVECTORS */
/*          OF THE TRIDIAGONAL MATRIX ARE DESIRED, Z MUST CONTAIN */
/*          THE IDENTITY MATRIX. */

/*      ON OUTPUT */

/*        D CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT BUT */
/*          UNORDERED FOR INDICES 1,2,...,IERR-1. */

/*        E HAS BEEN DESTROYED. */

/*        Z CONTAINS ORTHONORMAL EIGENVECTORS OF THE SYMMETRIC */
/*          TRIDIAGONAL (OR FULL) MATRIX.  IF AN ERROR EXIT IS MADE, */
/*          Z CONTAINS THE EIGENVECTORS ASSOCIATED WITH THE STORED */
/*          EIGENVALUES. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
/* L100: */
        e[i - 1] = e[i];
    }

    e[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ELEMENT .......... */
L105:
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            if (m == *n) {
                goto L120;
            }
            tst1 = (d_1 = d[m], abs(d_1)) + (d_2 = d[m + 1], abs(d_2));
            tst2 = tst1 + (d_1 = e[m], abs(d_1));
            if (tst2 == tst1) {
                goto L120;
            }
/* L110: */
        }

L120:
        p = d[l];
        if (m == l) {
            goto L240;
        }
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        g = (d[l + 1] - p) / (e[l] * 2.);
        r = pythag_(&g, &c_b141);
        g = d[m] - p + e[l] / (g + d_sign(&r, &g));
        s = 1.;
        c = 1.;
        p = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            i = m - ii;
            f = s * e[i];
            b = c * e[i];
            r = pythag_(&f, &g);
            e[i + 1] = r;
            if (r == 0.) {
                goto L210;
            }
            s = f / r;
            c = g / r;
            g = d[i + 1] - p;
            r = (d[i] - g) * s + c * 2. * b;
            p = s * r;
            d[i + 1] = g + p;
            g = c * r - b;
/*     .......... FORM VECTOR .......... */
            i_3 = *n;
            for (k = 1; k <= i_3; ++k) {
                f = z[k + (i + 1) * z_dim1];
                z[k + (i + 1) * z_dim1] = s * z[k + i * z_dim1] + c * f;
                z[k + i * z_dim1] = c * z[k + i * z_dim1] - s * f;
/* L180: */
            }

/* L200: */
        }

        d[l] -= p;
        e[l] = g;
        e[m] = 0.;
        goto L105;
/*     .......... RECOVER FROM UNDERFLOW .......... */
L210:
        d[i + 1] -= p;
        e[m] = 0.;
        goto L105;
L240:
        ;
    }
/*     .......... ORDER EIGENVALUES AND EIGENVECTORS .......... */
    i_1 = *n;
    for (ii = 2; ii <= i_1; ++ii) {
        i = ii - 1;
        k = i;
        p = d[i];

        i_2 = *n;
        for (j = ii; j <= i_2; ++j) {
            if (d[j] >= p) {
                goto L260;
            }
            k = j;
            p = d[j];
L260:
            ;
        }

        if (k == i) {
            goto L300;
        }
        d[k] = d[i];
        d[i] = p;

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            p = z[j + i * z_dim1];
            z[j + i * z_dim1] = z[j + k * z_dim1];
            z[j + k * z_dim1] = p;
/* L280: */
        }

L300:
        ;
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* imtql2_ */

/* Subroutine */ int imtqlv_(integer *n, doublereal *d, doublereal *e, 
        doublereal *e2, doublereal *w, integer *ind, integer *ierr, 
        doublereal *rv1)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal b, c, f, g;
    static integer i, j, k, l, m;
    static doublereal p, r, s;
    static integer ii;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer tag, mml;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A VARIANT OF  IMTQL1  WHICH IS A TRANSLATION OF 
*/
/*     ALGOL PROCEDURE IMTQL1, NUM. MATH. 12, 377-383(1968) BY MARTIN AND 
*/
/*     WILKINSON, AS MODIFIED IN NUM. MATH. 15, 450(1970) BY DUBRULLE. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 241-248(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A SYMMETRIC TRIDIAGONAL */
/*     MATRIX BY THE IMPLICIT QL METHOD AND ASSOCIATES WITH THEM */
/*     THEIR CORRESPONDING SUBMATRIX INDICES. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2(1) IS ARBITRARY. */

/*     ON OUTPUT */

/*        D AND E ARE UNALTERED. */

/*        ELEMENTS OF E2, CORRESPONDING TO ELEMENTS OF E REGARDED */
/*          AS NEGLIGIBLE, HAVE BEEN REPLACED BY ZERO CAUSING THE */
/*          MATRIX TO SPLIT INTO A DIRECT SUM OF SUBMATRICES. */
/*          E2(1) IS ALSO SET TO ZERO. */

/*        W CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT AND */
/*          ORDERED FOR INDICES 1,2,...IERR-1, BUT MAY NOT BE */
/*          THE SMALLEST EIGENVALUES. */

/*        IND CONTAINS THE SUBMATRIX INDICES ASSOCIATED WITH THE */
/*          CORRESPONDING EIGENVALUES IN W -- 1 FOR EIGENVALUES */
/*          BELONGING TO THE FIRST SUBMATRIX FROM THE TOP, */
/*          2 FOR THOSE BELONGING TO THE SECOND SUBMATRIX, ETC.. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*        RV1 IS A TEMPORARY STORAGE ARRAY. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv1;
    --ind;
    --w;
    --e2;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    k = 0;
    tag = 0;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        w[i] = d[i];
        if (i != 1) {
            rv1[i - 1] = e[i];
        }
/* L100: */
    }

    e2[1] = 0.;
    rv1[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ELEMENT .......... */
L105:
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            if (m == *n) {
                goto L120;
            }
            tst1 = (d_1 = w[m], abs(d_1)) + (d_2 = w[m + 1], abs(d_2));
            tst2 = tst1 + (d_1 = rv1[m], abs(d_1));
            if (tst2 == tst1) {
                goto L120;
            }
/*     .......... GUARD AGAINST UNDERFLOWED ELEMENT OF E2 ........
.. */
            if (e2[m + 1] == 0.) {
                goto L125;
            }
/* L110: */
        }

L120:
        if (m <= k) {
            goto L130;
        }
        if (m != *n) {
            e2[m + 1] = 0.;
        }
L125:
        k = m;
        ++tag;
L130:
        p = w[l];
        if (m == l) {
            goto L215;
        }
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        g = (w[l + 1] - p) / (rv1[l] * 2.);
        r = pythag_(&g, &c_b141);
        g = w[m] - p + rv1[l] / (g + d_sign(&r, &g));
        s = 1.;
        c = 1.;
        p = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            i = m - ii;
            f = s * rv1[i];
            b = c * rv1[i];
            r = pythag_(&f, &g);
            rv1[i + 1] = r;
            if (r == 0.) {
                goto L210;
            }
            s = f / r;
            c = g / r;
            g = w[i + 1] - p;
            r = (w[i] - g) * s + c * 2. * b;
            p = s * r;
            w[i + 1] = g + p;
            g = c * r - b;
/* L200: */
        }

        w[l] -= p;
        rv1[l] = g;
        rv1[m] = 0.;
        goto L105;
/*     .......... RECOVER FROM UNDERFLOW .......... */
L210:
        w[i + 1] -= p;
        rv1[m] = 0.;
        goto L105;
/*     .......... ORDER EIGENVALUES .......... */
L215:
        if (l == 1) {
            goto L250;
        }
/*     .......... FOR I=L STEP -1 UNTIL 2 DO -- .......... */
        i_2 = l;
        for (ii = 2; ii <= i_2; ++ii) {
            i = l + 2 - ii;
            if (p >= w[i - 1]) {
                goto L270;
            }
            w[i] = w[i - 1];
            ind[i] = ind[i - 1];
/* L230: */
        }

L250:
        i = 1;
L270:
        w[i] = p;
        ind[i] = tag;
/* L290: */
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* imtqlv_ */

/* Subroutine */ int invit_(integer *nm, integer *n, doublereal *a, 
        doublereal *wr, doublereal *wi, logical *select, integer *mm, integer 
        *m, doublereal *z, integer *ierr, doublereal *rm1, doublereal *rv1, 
        doublereal *rv2)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, rm1_dim1, rm1_offset, i_1, 
            i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    extern /* Subroutine */ int cdiv_(doublereal *, doublereal *, doublereal *
            , doublereal *, doublereal *, doublereal *);
    static doublereal norm;
    static integer i, j, k, l, s;
    static doublereal t, w, x, y;
    static integer n1;
    static doublereal normv;
    static integer ii;
    static doublereal ilambd;
    static integer ip, mp, ns, uk;
    static doublereal rlambd;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer km1, ip1;
    static doublereal growto, ukroot;
    static integer its;
    static doublereal eps3;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE INVIT */
/*     BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 418-439(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVECTORS OF A REAL UPPER */
/*     HESSENBERG MATRIX CORRESPONDING TO SPECIFIED EIGENVALUES, */
/*     USING INVERSE ITERATION. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE HESSENBERG MATRIX. */

/*        WR AND WI CONTAIN THE REAL AND IMAGINARY PARTS, RESPECTIVELY, */
/*          OF THE EIGENVALUES OF THE MATRIX.  THE EIGENVALUES MUST BE */
/*          STORED IN A MANNER IDENTICAL TO THAT OF SUBROUTINE  HQR, */
/*          WHICH RECOGNIZES POSSIBLE SPLITTING OF THE MATRIX. */

/*        SELECT SPECIFIES THE EIGENVECTORS TO BE FOUND. THE */
/*          EIGENVECTOR CORRESPONDING TO THE J-TH EIGENVALUE IS */
/*          SPECIFIED BY SETTING SELECT(J) TO .TRUE.. */

/*        MM SHOULD BE SET TO AN UPPER BOUND FOR THE NUMBER OF */
/*          COLUMNS REQUIRED TO STORE THE EIGENVECTORS TO BE FOUND. */
/*          NOTE THAT TWO COLUMNS ARE REQUIRED TO STORE THE */
/*          EIGENVECTOR CORRESPONDING TO A COMPLEX EIGENVALUE. */

/*     ON OUTPUT */

/*        A AND WI ARE UNALTERED. */

/*        WR MAY HAVE BEEN ALTERED SINCE CLOSE EIGENVALUES ARE PERTURBED 
*/
/*          SLIGHTLY IN SEARCHING FOR INDEPENDENT EIGENVECTORS. */

/*        SELECT MAY HAVE BEEN ALTERED.  IF THE ELEMENTS CORRESPONDING */
/*          TO A PAIR OF CONJUGATE COMPLEX EIGENVALUES WERE EACH */
/*          INITIALLY SET TO .TRUE., THE PROGRAM RESETS THE SECOND OF */
/*          THE TWO ELEMENTS TO .FALSE.. */

/*        M IS THE NUMBER OF COLUMNS ACTUALLY USED TO STORE */
/*          THE EIGENVECTORS. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGENVECTORS. */
/*          IF THE NEXT SELECTED EIGENVALUE IS REAL, THE NEXT COLUMN */
/*          OF Z CONTAINS ITS EIGENVECTOR.  IF THE EIGENVALUE IS */
/*          COMPLEX, THE NEXT TWO COLUMNS OF Z CONTAIN THE REAL AND */
/*          IMAGINARY PARTS OF ITS EIGENVECTOR.  THE EIGENVECTORS ARE */
/*          NORMALIZED SO THAT THE COMPONENT OF LARGEST MAGNITUDE IS 1. */
/*          ANY VECTOR WHICH FAILS THE ACCEPTANCE TEST IS SET TO ZERO. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          -(2*N+1)   IF MORE THAN MM COLUMNS OF Z ARE NECESSARY */
/*                     TO STORE THE EIGENVECTORS CORRESPONDING TO */
/*                     THE SPECIFIED EIGENVALUES. */
/*          -K         IF THE ITERATION CORRESPONDING TO THE K-TH */
/*                     VALUE FAILS, */
/*          -(N+K)     IF BOTH ERROR SITUATIONS OCCUR. */

/*        RM1, RV1, AND RV2 ARE TEMPORARY STORAGE ARRAYS.  NOTE THAT RM1 
*/
/*          IS SQUARE OF DIMENSION N BY N AND, AUGMENTED BY TWO COLUMNS */
/*          OF Z, IS THE TRANSPOSE OF THE CORRESPONDING ALGOL B ARRAY. */

/*     THE ALGOL PROCEDURE GUESSVEC APPEARS IN INVIT IN LINE. */

/*     CALLS CDIV FOR COMPLEX DIVISION. */
/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv2;
    --rv1;
    rm1_dim1 = *n;
    rm1_offset = rm1_dim1 + 1;
    rm1 -= rm1_offset;
    --select;
    --wi;
    --wr;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    *ierr = 0;
    uk = 0;
    s = 1;
/*     .......... IP = 0, REAL EIGENVALUE */
/*                     1, FIRST OF CONJUGATE COMPLEX PAIR */
/*                    -1, SECOND OF CONJUGATE COMPLEX PAIR .......... */
    ip = 0;
    n1 = *n - 1;

    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {
        if (wi[k] == 0. || ip < 0) {
            goto L100;
        }
        ip = 1;
        if (select[k] && select[k + 1]) {
            select[k + 1] = FALSE_;
        }
L100:
        if (! select[k]) {
            goto L960;
        }
        if (wi[k] != 0.) {
            ++s;
        }
        if (s > *mm) {
            goto L1000;
        }
        if (uk >= k) {
            goto L200;
        }
/*     .......... CHECK FOR POSSIBLE SPLITTING .......... */
        i_2 = *n;
        for (uk = k; uk <= i_2; ++uk) {
            if (uk == *n) {
                goto L140;
            }
            if (a[uk + 1 + uk * a_dim1] == 0.) {
                goto L140;
            }
/* L120: */
        }
/*     .......... COMPUTE INFINITY NORM OF LEADING UK BY UK */
/*                (HESSENBERG) MATRIX .......... */
L140:
        norm = 0.;
        mp = 1;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            x = 0.;

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
/* L160: */
                x += (d_1 = a[i + j * a_dim1], abs(d_1));
            }

            if (x > norm) {
                norm = x;
            }
            mp = i;
/* L180: */
        }
/*     .......... EPS3 REPLACES ZERO PIVOT IN DECOMPOSITION */
/*                AND CLOSE ROOTS ARE MODIFIED BY EPS3 .......... */
        if (norm == 0.) {
            norm = 1.;
        }
        eps3 = epslon_(&norm);
/*     .......... GROWTO IS THE CRITERION FOR THE GROWTH .......... */
        ukroot = (doublereal) uk;
        ukroot = sqrt(ukroot);
        growto = .1 / ukroot;
L200:
        rlambd = wr[k];
        ilambd = wi[k];
        if (k == 1) {
            goto L280;
        }
        km1 = k - 1;
        goto L240;
/*     .......... PERTURB EIGENVALUE IF IT IS CLOSE */
/*                TO ANY PREVIOUS EIGENVALUE .......... */
L220:
        rlambd += eps3;
/*     .......... FOR I=K-1 STEP -1 UNTIL 1 DO -- .......... */
L240:
        i_2 = km1;
        for (ii = 1; ii <= i_2; ++ii) {
            i = k - ii;
            if (select[i] && (d_1 = wr[i] - rlambd, abs(d_1)) < eps3 && (
                    d_2 = wi[i] - ilambd, abs(d_2)) < eps3) {
                goto L220;
            }
/* L260: */
        }

        wr[k] = rlambd;
/*     .......... PERTURB CONJUGATE EIGENVALUE TO MATCH .......... */
        ip1 = k + ip;
        wr[ip1] = rlambd;
/*     .......... FORM UPPER HESSENBERG A-RLAMBD*I (TRANSPOSED) */
/*                AND INITIAL REAL VECTOR .......... */
L280:
        mp = 1;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
/* L300: */
                rm1[j + i * rm1_dim1] = a[i + j * a_dim1];
            }

            rm1[i + i * rm1_dim1] -= rlambd;
            mp = i;
            rv1[i] = eps3;
/* L320: */
        }

        its = 0;
        if (ilambd != 0.) {
            goto L520;
        }
/*     .......... REAL EIGENVALUE. */
/*                TRIANGULAR DECOMPOSITION WITH INTERCHANGES, */
/*                REPLACING ZERO PIVOTS BY EPS3 .......... */
        if (uk == 1) {
            goto L420;
        }

        i_2 = uk;
        for (i = 2; i <= i_2; ++i) {
            mp = i - 1;
            if ((d_1 = rm1[mp + i * rm1_dim1], abs(d_1)) <= (d_2 = rm1[mp 
                    + mp * rm1_dim1], abs(d_2))) {
                goto L360;
            }

            i_3 = uk;
            for (j = mp; j <= i_3; ++j) {
                y = rm1[j + i * rm1_dim1];
                rm1[j + i * rm1_dim1] = rm1[j + mp * rm1_dim1];
                rm1[j + mp * rm1_dim1] = y;
/* L340: */
            }

L360:
            if (rm1[mp + mp * rm1_dim1] == 0.) {
                rm1[mp + mp * rm1_dim1] = eps3;
            }
            x = rm1[mp + i * rm1_dim1] / rm1[mp + mp * rm1_dim1];
            if (x == 0.) {
                goto L400;
            }

            i_3 = uk;
            for (j = i; j <= i_3; ++j) {
/* L380: */
                rm1[j + i * rm1_dim1] -= x * rm1[j + mp * rm1_dim1];
            }

L400:
            ;
        }

L420:
        if (rm1[uk + uk * rm1_dim1] == 0.) {
            rm1[uk + uk * rm1_dim1] = eps3;
        }
/*     .......... BACK SUBSTITUTION FOR REAL VECTOR */
/*                FOR I=UK STEP -1 UNTIL 1 DO -- .......... */
L440:
        i_2 = uk;
        for (ii = 1; ii <= i_2; ++ii) {
            i = uk + 1 - ii;
            y = rv1[i];
            if (i == uk) {
                goto L480;
            }
            ip1 = i + 1;

            i_3 = uk;
            for (j = ip1; j <= i_3; ++j) {
/* L460: */
                y -= rm1[j + i * rm1_dim1] * rv1[j];
            }

L480:
            rv1[i] = y / rm1[i + i * rm1_dim1];
/* L500: */
        }

        goto L740;
/*     .......... COMPLEX EIGENVALUE. */
/*                TRIANGULAR DECOMPOSITION WITH INTERCHANGES, */
/*                REPLACING ZERO PIVOTS BY EPS3.  STORE IMAGINARY */
/*                PARTS IN UPPER TRIANGLE STARTING AT (1,3) ..........
 */
L520:
        ns = *n - s;
        z[(s - 1) * z_dim1 + 1] = -ilambd;
        z[s * z_dim1 + 1] = 0.;
        if (*n == 2) {
            goto L550;
        }
        rm1[rm1_dim1 * 3 + 1] = -ilambd;
        z[(s - 1) * z_dim1 + 1] = 0.;
        if (*n == 3) {
            goto L550;
        }

        i_2 = *n;
        for (i = 4; i <= i_2; ++i) {
/* L540: */
            rm1[i * rm1_dim1 + 1] = 0.;
        }

L550:
        i_2 = uk;
        for (i = 2; i <= i_2; ++i) {
            mp = i - 1;
            w = rm1[mp + i * rm1_dim1];
            if (i < *n) {
                t = rm1[mp + (i + 1) * rm1_dim1];
            }
            if (i == *n) {
                t = z[mp + (s - 1) * z_dim1];
            }
            x = rm1[mp + mp * rm1_dim1] * rm1[mp + mp * rm1_dim1] + t * t;
            if (w * w <= x) {
                goto L580;
            }
            x = rm1[mp + mp * rm1_dim1] / w;
            y = t / w;
            rm1[mp + mp * rm1_dim1] = w;
            if (i < *n) {
                rm1[mp + (i + 1) * rm1_dim1] = 0.;
            }
            if (i == *n) {
                z[mp + (s - 1) * z_dim1] = 0.;
            }

            i_3 = uk;
            for (j = i; j <= i_3; ++j) {
                w = rm1[j + i * rm1_dim1];
                rm1[j + i * rm1_dim1] = rm1[j + mp * rm1_dim1] - x * w;
                rm1[j + mp * rm1_dim1] = w;
                if (j < n1) {
                    goto L555;
                }
                l = j - ns;
                z[i + l * z_dim1] = z[mp + l * z_dim1] - y * w;
                z[mp + l * z_dim1] = 0.;
                goto L560;
L555:
                rm1[i + (j + 2) * rm1_dim1] = rm1[mp + (j + 2) * rm1_dim1] - 
                        y * w;
                rm1[mp + (j + 2) * rm1_dim1] = 0.;
L560:
                ;
            }

            rm1[i + i * rm1_dim1] -= y * ilambd;
            if (i < n1) {
                goto L570;
            }
            l = i - ns;
            z[mp + l * z_dim1] = -ilambd;
            z[i + l * z_dim1] += x * ilambd;
            goto L640;
L570:
            rm1[mp + (i + 2) * rm1_dim1] = -ilambd;
            rm1[i + (i + 2) * rm1_dim1] += x * ilambd;
            goto L640;
L580:
            if (x != 0.) {
                goto L600;
            }
            rm1[mp + mp * rm1_dim1] = eps3;
            if (i < *n) {
                rm1[mp + (i + 1) * rm1_dim1] = 0.;
            }
            if (i == *n) {
                z[mp + (s - 1) * z_dim1] = 0.;
            }
            t = 0.;
            x = eps3 * eps3;
L600:
            w /= x;
            x = rm1[mp + mp * rm1_dim1] * w;
            y = -t * w;

            i_3 = uk;
            for (j = i; j <= i_3; ++j) {
                if (j < n1) {
                    goto L610;
                }
                l = j - ns;
                t = z[mp + l * z_dim1];
                z[i + l * z_dim1] = -x * t - y * rm1[j + mp * rm1_dim1];
                goto L615;
L610:
                t = rm1[mp + (j + 2) * rm1_dim1];
                rm1[i + (j + 2) * rm1_dim1] = -x * t - y * rm1[j + mp * 
                        rm1_dim1];
L615:
                rm1[j + i * rm1_dim1] = rm1[j + i * rm1_dim1] - x * rm1[j + 
                        mp * rm1_dim1] + y * t;
/* L620: */
            }

            if (i < n1) {
                goto L630;
            }
            l = i - ns;
            z[i + l * z_dim1] -= ilambd;
            goto L640;
L630:
            rm1[i + (i + 2) * rm1_dim1] -= ilambd;
L640:
            ;
        }

        if (uk < n1) {
            goto L650;
        }
        l = uk - ns;
        t = z[uk + l * z_dim1];
        goto L655;
L650:
        t = rm1[uk + (uk + 2) * rm1_dim1];
L655:
        if (rm1[uk + uk * rm1_dim1] == 0. && t == 0.) {
            rm1[uk + uk * rm1_dim1] = eps3;
        }
/*     .......... BACK SUBSTITUTION FOR COMPLEX VECTOR */
/*                FOR I=UK STEP -1 UNTIL 1 DO -- .......... */
L660:
        i_2 = uk;
        for (ii = 1; ii <= i_2; ++ii) {
            i = uk + 1 - ii;
            x = rv1[i];
            y = 0.;
            if (i == uk) {
                goto L700;
            }
            ip1 = i + 1;

            i_3 = uk;
            for (j = ip1; j <= i_3; ++j) {
                if (j < n1) {
                    goto L670;
                }
                l = j - ns;
                t = z[i + l * z_dim1];
                goto L675;
L670:
                t = rm1[i + (j + 2) * rm1_dim1];
L675:
                x = x - rm1[j + i * rm1_dim1] * rv1[j] + t * rv2[j];
                y = y - rm1[j + i * rm1_dim1] * rv2[j] - t * rv1[j];
/* L680: */
            }

L700:
            if (i < n1) {
                goto L710;
            }
            l = i - ns;
            t = z[i + l * z_dim1];
            goto L715;
L710:
            t = rm1[i + (i + 2) * rm1_dim1];
L715:
            cdiv_(&x, &y, &rm1[i + i * rm1_dim1], &t, &rv1[i], &rv2[i]);
/* L720: */
        }
/*     .......... ACCEPTANCE TEST FOR REAL OR COMPLEX */
/*                EIGENVECTOR AND NORMALIZATION .......... */
L740:
        ++its;
        norm = 0.;
        normv = 0.;

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            if (ilambd == 0.) {
                x = (d_1 = rv1[i], abs(d_1));
            }
            if (ilambd != 0.) {
                x = pythag_(&rv1[i], &rv2[i]);
            }
            if (normv >= x) {
                goto L760;
            }
            normv = x;
            j = i;
L760:
            norm += x;
/* L780: */
        }

        if (norm < growto) {
            goto L840;
        }
/*     .......... ACCEPT VECTOR .......... */
        x = rv1[j];
        if (ilambd == 0.) {
            x = 1. / x;
        }
        if (ilambd != 0.) {
            y = rv2[j];
        }

        i_2 = uk;
        for (i = 1; i <= i_2; ++i) {
            if (ilambd != 0.) {
                goto L800;
            }
            z[i + s * z_dim1] = rv1[i] * x;
            goto L820;
L800:
            cdiv_(&rv1[i], &rv2[i], &x, &y, &z[i + (s - 1) * z_dim1], &z[i + 
                    s * z_dim1]);
L820:
            ;
        }

        if (uk == *n) {
            goto L940;
        }
        j = uk + 1;
        goto L900;
/*     .......... IN-LINE PROCEDURE FOR CHOOSING */
/*                A NEW STARTING VECTOR .......... */
L840:
        if (its >= uk) {
            goto L880;
        }
        x = ukroot;
        y = eps3 / (x + 1.);
        rv1[1] = eps3;

        i_2 = uk;
        for (i = 2; i <= i_2; ++i) {
/* L860: */
            rv1[i] = y;
        }

        j = uk - its + 1;
        rv1[j] -= eps3 * x;
        if (ilambd == 0.) {
            goto L440;
        }
        goto L660;
/*     .......... SET ERROR -- UNACCEPTED EIGENVECTOR .......... */
L880:
        j = 1;
        *ierr = -k;
/*     .......... SET REMAINING VECTOR COMPONENTS TO ZERO .......... 
*/
L900:
        i_2 = *n;
        for (i = j; i <= i_2; ++i) {
            z[i + s * z_dim1] = 0.;
            if (ilambd != 0.) {
                z[i + (s - 1) * z_dim1] = 0.;
            }
/* L920: */
        }

L940:
        ++s;
L960:
        if (ip == -1) {
            ip = 0;
        }
        if (ip == 1) {
            ip = -1;
        }
/* L980: */
    }

    goto L1001;
/*     .......... SET ERROR -- UNDERESTIMATE OF EIGENVECTOR */
/*                SPACE REQUIRED .......... */
L1000:
    if (*ierr != 0) {
        *ierr -= *n;
    }
    if (*ierr == 0) {
        *ierr = -((*n << 1) + 1);
    }
L1001:
    *m = s - 1 - abs(ip);
    return 0;
} /* invit_ */

/* Subroutine */ int minfit_(integer *nm, integer *m, integer *n, doublereal *
        a, doublereal *w, integer *ip, doublereal *b, integer *ierr, 
        doublereal *rv1)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal c, f, g, h;
    static integer i, j, k, l;
    static doublereal s, x, y, z, scale;
    static integer i1, k1, l1, m1, ii, kk, ll;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer its;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE MINFIT, */
/*     NUM. MATH. 14, 403-420(1970) BY GOLUB AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL II-LINEAR ALGEBRA, 134-151(1971). */

/*     THIS SUBROUTINE DETERMINES, TOWARDS THE SOLUTION OF THE LINEAR */
/*                                                        T */
/*     SYSTEM AX=B, THE SINGULAR VALUE DECOMPOSITION A=USV  OF A REAL */
/*                                         T */
/*     M BY N RECTANGULAR MATRIX, FORMING U B RATHER THAN U.  HOUSEHOLDER 
*/
/*     BIDIAGONALIZATION AND A VARIANT OF THE QR ALGORITHM ARE USED. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT.  NOTE THAT NM MUST BE AT LEAST */
/*          AS LARGE AS THE MAXIMUM OF M AND N. */

/*        M IS THE NUMBER OF ROWS OF A AND B. */

/*        N IS THE NUMBER OF COLUMNS OF A AND THE ORDER OF V. */

/*        A CONTAINS THE RECTANGULAR COEFFICIENT MATRIX OF THE SYSTEM. */

/*        IP IS THE NUMBER OF COLUMNS OF B.  IP CAN BE ZERO. */

/*        B CONTAINS THE CONSTANT COLUMN MATRIX OF THE SYSTEM */
/*          IF IP IS NOT ZERO.  OTHERWISE B IS NOT REFERENCED. */

/*     ON OUTPUT */

/*        A HAS BEEN OVERWRITTEN BY THE MATRIX V (ORTHOGONAL) OF THE */
/*          DECOMPOSITION IN ITS FIRST N ROWS AND COLUMNS.  IF AN */
/*          ERROR EXIT IS MADE, THE COLUMNS OF V CORRESPONDING TO */
/*          INDICES OF CORRECT SINGULAR VALUES SHOULD BE CORRECT. */

/*        W CONTAINS THE N (NON-NEGATIVE) SINGULAR VALUES OF A (THE */
/*          DIAGONAL ELEMENTS OF S).  THEY ARE UNORDERED.  IF AN */
/*          ERROR EXIT IS MADE, THE SINGULAR VALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,IERR+2,...,N. */

/*                                   T */
/*        B HAS BEEN OVERWRITTEN BY U B.  IF AN ERROR EXIT IS MADE, */
/*                       T */
/*          THE ROWS OF U B CORRESPONDING TO INDICES OF CORRECT */
/*          SINGULAR VALUES SHOULD BE CORRECT. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          K          IF THE K-TH SINGULAR VALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*        RV1 IS A TEMPORARY STORAGE ARRAY. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv1;
    --w;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;

    /* Function Body */
    *ierr = 0;
/*     .......... HOUSEHOLDER REDUCTION TO BIDIAGONAL FORM .......... */
    g = 0.;
    scale = 0.;
    x = 0.;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        l = i + 1;
        rv1[i] = scale * g;
        g = 0.;
        s = 0.;
        scale = 0.;
        if (i > *m) {
            goto L210;
        }

        i_2 = *m;
        for (k = i; k <= i_2; ++k) {
/* L120: */
            scale += (d_1 = a[k + i * a_dim1], abs(d_1));
        }

        if (scale == 0.) {
            goto L210;
        }

        i_2 = *m;
        for (k = i; k <= i_2; ++k) {
            a[k + i * a_dim1] /= scale;
/* Computing 2nd power */
            d_1 = a[k + i * a_dim1];
            s += d_1 * d_1;
/* L130: */
        }

        f = a[i + i * a_dim1];
        d_1 = sqrt(s);
        g = -d_sign(&d_1, &f);
        h = f * g - s;
        a[i + i * a_dim1] = f - g;
        if (i == *n) {
            goto L160;
        }

        i_2 = *n;
        for (j = l; j <= i_2; ++j) {
            s = 0.;

            i_3 = *m;
            for (k = i; k <= i_3; ++k) {
/* L140: */
                s += a[k + i * a_dim1] * a[k + j * a_dim1];
            }

            f = s / h;

            i_3 = *m;
            for (k = i; k <= i_3; ++k) {
                a[k + j * a_dim1] += f * a[k + i * a_dim1];
/* L150: */
            }
        }

L160:
        if (*ip == 0) {
            goto L190;
        }

        i_3 = *ip;
        for (j = 1; j <= i_3; ++j) {
            s = 0.;

            i_2 = *m;
            for (k = i; k <= i_2; ++k) {
/* L170: */
                s += a[k + i * a_dim1] * b[k + j * b_dim1];
            }

            f = s / h;

            i_2 = *m;
            for (k = i; k <= i_2; ++k) {
                b[k + j * b_dim1] += f * a[k + i * a_dim1];
/* L180: */
            }
        }

L190:
        i_2 = *m;
        for (k = i; k <= i_2; ++k) {
/* L200: */
            a[k + i * a_dim1] = scale * a[k + i * a_dim1];
        }

L210:
        w[i] = scale * g;
        g = 0.;
        s = 0.;
        scale = 0.;
        if (i > *m || i == *n) {
            goto L290;
        }

        i_2 = *n;
        for (k = l; k <= i_2; ++k) {
/* L220: */
            scale += (d_1 = a[i + k * a_dim1], abs(d_1));
        }

        if (scale == 0.) {
            goto L290;
        }

        i_2 = *n;
        for (k = l; k <= i_2; ++k) {
            a[i + k * a_dim1] /= scale;
/* Computing 2nd power */
            d_1 = a[i + k * a_dim1];
            s += d_1 * d_1;
/* L230: */
        }

        f = a[i + l * a_dim1];
        d_1 = sqrt(s);
        g = -d_sign(&d_1, &f);
        h = f * g - s;
        a[i + l * a_dim1] = f - g;

        i_2 = *n;
        for (k = l; k <= i_2; ++k) {
/* L240: */
            rv1[k] = a[i + k * a_dim1] / h;
        }

        if (i == *m) {
            goto L270;
        }

        i_2 = *m;
        for (j = l; j <= i_2; ++j) {
            s = 0.;

            i_3 = *n;
            for (k = l; k <= i_3; ++k) {
/* L250: */
                s += a[j + k * a_dim1] * a[i + k * a_dim1];
            }

            i_3 = *n;
            for (k = l; k <= i_3; ++k) {
                a[j + k * a_dim1] += s * rv1[k];
/* L260: */
            }
        }

L270:
        i_3 = *n;
        for (k = l; k <= i_3; ++k) {
/* L280: */
            a[i + k * a_dim1] = scale * a[i + k * a_dim1];
        }

L290:
/* Computing MAX */
        d_3 = x, d_4 = (d_1 = w[i], abs(d_1)) + (d_2 = rv1[i], abs(d_2))
                ;
        x = max(d_3,d_4);
/* L300: */
    }
/*     .......... ACCUMULATION OF RIGHT-HAND TRANSFORMATIONS. */
/*                FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *n + 1 - ii;
        if (i == *n) {
            goto L390;
        }
        if (g == 0.) {
            goto L360;
        }

        i_3 = *n;
        for (j = l; j <= i_3; ++j) {
/*     .......... DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
/* L320: */
            a[j + i * a_dim1] = a[i + j * a_dim1] / a[i + l * a_dim1] / g;
        }

        i_3 = *n;
        for (j = l; j <= i_3; ++j) {
            s = 0.;

            i_2 = *n;
            for (k = l; k <= i_2; ++k) {
/* L340: */
                s += a[i + k * a_dim1] * a[k + j * a_dim1];
            }

            i_2 = *n;
            for (k = l; k <= i_2; ++k) {
                a[k + j * a_dim1] += s * a[k + i * a_dim1];
/* L350: */
            }
        }

L360:
        i_2 = *n;
        for (j = l; j <= i_2; ++j) {
            a[i + j * a_dim1] = 0.;
            a[j + i * a_dim1] = 0.;
/* L380: */
        }

L390:
        a[i + i * a_dim1] = 1.;
        g = rv1[i];
        l = i;
/* L400: */
    }

    if (*m >= *n || *ip == 0) {
        goto L510;
    }
    m1 = *m + 1;

    i_1 = *n;
    for (i = m1; i <= i_1; ++i) {

        i_2 = *ip;
        for (j = 1; j <= i_2; ++j) {
            b[i + j * b_dim1] = 0.;
/* L500: */
        }
    }
/*     .......... DIAGONALIZATION OF THE BIDIAGONAL FORM .......... */
L510:
    tst1 = x;
/*     .......... FOR K=N STEP -1 UNTIL 1 DO -- .......... */
    i_2 = *n;
    for (kk = 1; kk <= i_2; ++kk) {
        k1 = *n - kk;
        k = k1 + 1;
        its = 0;
/*     .......... TEST FOR SPLITTING. */
/*                FOR L=K STEP -1 UNTIL 1 DO -- .......... */
L520:
        i_1 = k;
        for (ll = 1; ll <= i_1; ++ll) {
            l1 = k - ll;
            l = l1 + 1;
            tst2 = tst1 + (d_1 = rv1[l], abs(d_1));
            if (tst2 == tst1) {
                goto L565;
            }
/*     .......... RV1(1) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP .......... */
            tst2 = tst1 + (d_1 = w[l1], abs(d_1));
            if (tst2 == tst1) {
                goto L540;
            }
/* L530: */
        }
/*     .......... CANCELLATION OF RV1(L) IF L GREATER THAN 1 .........
. */
L540:
        c = 0.;
        s = 1.;

        i_1 = k;
        for (i = l; i <= i_1; ++i) {
            f = s * rv1[i];
            rv1[i] = c * rv1[i];
            tst2 = tst1 + abs(f);
            if (tst2 == tst1) {
                goto L565;
            }
            g = w[i];
            h = pythag_(&f, &g);
            w[i] = h;
            c = g / h;
            s = -f / h;
            if (*ip == 0) {
                goto L560;
            }

            i_3 = *ip;
            for (j = 1; j <= i_3; ++j) {
                y = b[l1 + j * b_dim1];
                z = b[i + j * b_dim1];
                b[l1 + j * b_dim1] = y * c + z * s;
                b[i + j * b_dim1] = -y * s + z * c;
/* L550: */
            }

L560:
            ;
        }
/*     .......... TEST FOR CONVERGENCE .......... */
L565:
        z = w[k];
        if (l == k) {
            goto L650;
        }
/*     .......... SHIFT FROM BOTTOM 2 BY 2 MINOR .......... */
        if (its == 30) {
            goto L1000;
        }
        ++its;
        x = w[l];
        y = w[k1];
        g = rv1[k1];
        h = rv1[k];
        f = ((g + z) / h * ((g - z) / y) + y / h - h / y) * .5;
        g = pythag_(&f, &c_b141);
        f = x - z / x * z + h / x * (y / (f + d_sign(&g, &f)) - h);
/*     .......... NEXT QR TRANSFORMATION .......... */
        c = 1.;
        s = 1.;

        i_1 = k1;
        for (i1 = l; i1 <= i_1; ++i1) {
            i = i1 + 1;
            g = rv1[i];
            y = w[i];
            h = s * g;
            g = c * g;
            z = pythag_(&f, &h);
            rv1[i1] = z;
            c = f / z;
            s = h / z;
            f = x * c + g * s;
            g = -x * s + g * c;
            h = y * s;
            y *= c;

            i_3 = *n;
            for (j = 1; j <= i_3; ++j) {
                x = a[j + i1 * a_dim1];
                z = a[j + i * a_dim1];
                a[j + i1 * a_dim1] = x * c + z * s;
                a[j + i * a_dim1] = -x * s + z * c;
/* L570: */
            }

            z = pythag_(&f, &h);
            w[i1] = z;
/*     .......... ROTATION CAN BE ARBITRARY IF Z IS ZERO .........
. */
            if (z == 0.) {
                goto L580;
            }
            c = f / z;
            s = h / z;
L580:
            f = c * g + s * y;
            x = -s * g + c * y;
            if (*ip == 0) {
                goto L600;
            }

            i_3 = *ip;
            for (j = 1; j <= i_3; ++j) {
                y = b[i1 + j * b_dim1];
                z = b[i + j * b_dim1];
                b[i1 + j * b_dim1] = y * c + z * s;
                b[i + j * b_dim1] = -y * s + z * c;
/* L590: */
            }

L600:
            ;
        }

        rv1[l] = 0.;
        rv1[k] = f;
        w[k] = x;
        goto L520;
/*     .......... CONVERGENCE .......... */
L650:
        if (z >= 0.) {
            goto L700;
        }
/*     .......... W(K) IS MADE NON-NEGATIVE .......... */
        w[k] = -z;

        i_1 = *n;
        for (j = 1; j <= i_1; ++j) {
/* L690: */
            a[j + k * a_dim1] = -a[j + k * a_dim1];
        }

L700:
        ;
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO A */
/*                SINGULAR VALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = k;
L1001:
    return 0;
} /* minfit_ */

/* Subroutine */ int ortbak_(integer *nm, integer *low, integer *igh, 
        doublereal *a, doublereal *ort, integer *m, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static doublereal g;
    static integer i, j, la, mm, mp, kp1, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ORTBAK, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A REAL GENERAL */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     UPPER HESSENBERG MATRIX DETERMINED BY  ORTHES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1 AND IGH EQUAL TO THE ORDER OF THE MATRIX. */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  ORTHES */
/*          IN ITS STRICT LOWER TRIANGLE. */

/*        ORT CONTAINS FURTHER INFORMATION ABOUT THE TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  ORTHES. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*        M IS THE NUMBER OF COLUMNS OF Z TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGEN- */
/*          VECTORS TO BE BACK TRANSFORMED IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE */
/*          TRANSFORMED EIGENVECTORS IN ITS FIRST M COLUMNS. */

/*        ORT HAS BEEN ALTERED. */

/*     NOTE THAT ORTBAK PRESERVES VECTOR EUCLIDEAN NORMS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --ort;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = la;
    for (mm = kp1; mm <= i_1; ++mm) {
        mp = *low + *igh - mm;
        if (a[mp + (mp - 1) * a_dim1] == 0.) {
            goto L140;
        }
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
/* L100: */
            ort[i] = a[i + (mp - 1) * a_dim1];
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            g = 0.;

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
/* L110: */
                g += ort[i] * z[i + j * z_dim1];
            }
/*     .......... DIVISOR BELOW IS NEGATIVE OF H FORMED IN ORTHES.
 */
/*                DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
            g = g / ort[mp] / a[mp + (mp - 1) * a_dim1];

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
/* L120: */
                z[i + j * z_dim1] += g * ort[i];
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* ortbak_ */

/* Subroutine */ int orthes_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *a, doublereal *ort)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, m;
    static doublereal scale;
    static integer la, ii, jj, mp, kp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ORTHES, */
/*     NUM. MATH. 12, 349-368(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 339-358(1971). */

/*     GIVEN A REAL GENERAL MATRIX, THIS SUBROUTINE */
/*     REDUCES A SUBMATRIX SITUATED IN ROWS AND COLUMNS */
/*     LOW THROUGH IGH TO UPPER HESSENBERG FORM BY */
/*     ORTHOGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        A CONTAINS THE INPUT MATRIX. */

/*     ON OUTPUT */

/*        A CONTAINS THE HESSENBERG MATRIX.  INFORMATION ABOUT */
/*          THE ORTHOGONAL TRANSFORMATIONS USED IN THE REDUCTION */
/*          IS STORED IN THE REMAINING TRIANGLE UNDER THE */
/*          HESSENBERG MATRIX. */

/*        ORT CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATIONS. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    --ort;

    /* Function Body */
    la = *igh - 1;
    kp1 = *low + 1;
    if (la < kp1) {
        goto L200;
    }

    i_1 = la;
    for (m = kp1; m <= i_1; ++m) {
        h = 0.;
        ort[m] = 0.;
        scale = 0.;
/*     .......... SCALE COLUMN (ALGOL TOL THEN NOT NEEDED) .......... 
*/
        i_2 = *igh;
        for (i = m; i <= i_2; ++i) {
/* L90: */
            scale += (d_1 = a[i + (m - 1) * a_dim1], abs(d_1));
        }

        if (scale == 0.) {
            goto L180;
        }
        mp = m + *igh;
/*     .......... FOR I=IGH STEP -1 UNTIL M DO -- .......... */
        i_2 = *igh;
        for (ii = m; ii <= i_2; ++ii) {
            i = mp - ii;
            ort[i] = a[i + (m - 1) * a_dim1] / scale;
            h += ort[i] * ort[i];
/* L100: */
        }

        d_1 = sqrt(h);
        g = -d_sign(&d_1, &ort[m]);
        h -= ort[m] * g;
        ort[m] -= g;
/*     .......... FORM (I-(U*UT)/H) * A .......... */
        i_2 = *n;
        for (j = m; j <= i_2; ++j) {
            f = 0.;
/*     .......... FOR I=IGH STEP -1 UNTIL M DO -- .......... */
            i_3 = *igh;
            for (ii = m; ii <= i_3; ++ii) {
                i = mp - ii;
                f += ort[i] * a[i + j * a_dim1];
/* L110: */
            }

            f /= h;

            i_3 = *igh;
            for (i = m; i <= i_3; ++i) {
/* L120: */
                a[i + j * a_dim1] -= f * ort[i];
            }

/* L130: */
        }
/*     .......... FORM (I-(U*UT)/H)*A*(I-(U*UT)/H) .......... */
        i_2 = *igh;
        for (i = 1; i <= i_2; ++i) {
            f = 0.;
/*     .......... FOR J=IGH STEP -1 UNTIL M DO -- .......... */
            i_3 = *igh;
            for (jj = m; jj <= i_3; ++jj) {
                j = mp - jj;
                f += ort[j] * a[i + j * a_dim1];
/* L140: */
            }

            f /= h;

            i_3 = *igh;
            for (j = m; j <= i_3; ++j) {
/* L150: */
                a[i + j * a_dim1] -= f * ort[j];
            }

/* L160: */
        }

        ort[m] = scale * ort[m];
        a[m + (m - 1) * a_dim1] = scale * g;
L180:
        ;
    }

L200:
    return 0;
} /* orthes_ */

/* Subroutine */ int ortran_(integer *nm, integer *n, integer *low, integer *
        igh, doublereal *a, doublereal *ort, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static doublereal g;
    static integer i, j, kl, mm, mp, mp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE ORTRANS, */
/*     NUM. MATH. 16, 181-204(1970) BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 372-395(1971). */

/*     THIS SUBROUTINE ACCUMULATES THE ORTHOGONAL SIMILARITY */
/*     TRANSFORMATIONS USED IN THE REDUCTION OF A REAL GENERAL */
/*     MATRIX TO UPPER HESSENBERG FORM BY  ORTHES. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        LOW AND IGH ARE INTEGERS DETERMINED BY THE BALANCING */
/*          SUBROUTINE  BALANC.  IF  BALANC  HAS NOT BEEN USED, */
/*          SET LOW=1, IGH=N. */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  ORTHES */
/*          IN ITS STRICT LOWER TRIANGLE. */

/*        ORT CONTAINS FURTHER INFORMATION ABOUT THE TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  ORTHES. */
/*          ONLY ELEMENTS LOW THROUGH IGH ARE USED. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN THE */
/*          REDUCTION BY  ORTHES. */

/*        ORT HAS BEEN ALTERED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

/*     .......... INITIALIZE Z TO IDENTITY MATRIX .......... */
    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --ort;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L60: */
            z[i + j * z_dim1] = 0.;
        }

        z[j + j * z_dim1] = 1.;
/* L80: */
    }

    kl = *igh - *low - 1;
    if (kl < 1) {
        goto L200;
    }
/*     .......... FOR MP=IGH-1 STEP -1 UNTIL LOW+1 DO -- .......... */
    i_1 = kl;
    for (mm = 1; mm <= i_1; ++mm) {
        mp = *igh - mm;
        if (a[mp + (mp - 1) * a_dim1] == 0.) {
            goto L140;
        }
        mp1 = mp + 1;

        i_2 = *igh;
        for (i = mp1; i <= i_2; ++i) {
/* L100: */
            ort[i] = a[i + (mp - 1) * a_dim1];
        }

        i_2 = *igh;
        for (j = mp; j <= i_2; ++j) {
            g = 0.;

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
/* L110: */
                g += ort[i] * z[i + j * z_dim1];
            }
/*     .......... DIVISOR BELOW IS NEGATIVE OF H FORMED IN ORTHES.
 */
/*                DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
            g = g / ort[mp] / a[mp + (mp - 1) * a_dim1];

            i_3 = *igh;
            for (i = mp; i <= i_3; ++i) {
/* L120: */
                z[i + j * z_dim1] += g * ort[i];
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* ortran_ */

/* Subroutine */ int qzhes_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, logical *matz, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset, i_1, i_2, 
            i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static integer i, j, k, l;
    static doublereal r, s, t;
    static integer l1;
    static doublereal u1, u2, v1, v2;
    static integer lb, nk1, nm1, nm2;
    static doublereal rho;



/*     THIS SUBROUTINE IS THE FIRST STEP OF THE QZ ALGORITHM */
/*     FOR SOLVING GENERALIZED MATRIX EIGENVALUE PROBLEMS, */
/*     SIAM J. NUMER. ANAL. 10, 241-256(1973) BY MOLER AND STEWART. */

/*     THIS SUBROUTINE ACCEPTS A PAIR OF REAL GENERAL MATRICES AND */
/*     REDUCES ONE OF THEM TO UPPER HESSENBERG FORM AND THE OTHER */
/*     TO UPPER TRIANGULAR FORM USING ORTHOGONAL TRANSFORMATIONS. */
/*     IT IS USUALLY FOLLOWED BY  QZIT,  QZVAL  AND, POSSIBLY,  QZVEC. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES. */

/*        A CONTAINS A REAL GENERAL MATRIX. */

/*        B CONTAINS A REAL GENERAL MATRIX. */

/*        MATZ SHOULD BE SET TO .TRUE. IF THE RIGHT HAND TRANSFORMATIONS 
*/
/*          ARE TO BE ACCUMULATED FOR LATER USE IN COMPUTING */
/*          EIGENVECTORS, AND TO .FALSE. OTHERWISE. */

/*     ON OUTPUT */

/*        A HAS BEEN REDUCED TO UPPER HESSENBERG FORM.  THE ELEMENTS */
/*          BELOW THE FIRST SUBDIAGONAL HAVE BEEN SET TO ZERO. */

/*        B HAS BEEN REDUCED TO UPPER TRIANGULAR FORM.  THE ELEMENTS */
/*          BELOW THE MAIN DIAGONAL HAVE BEEN SET TO ZERO. */

/*        Z CONTAINS THE PRODUCT OF THE RIGHT HAND TRANSFORMATIONS IF */
/*          MATZ HAS BEEN SET TO .TRUE.  OTHERWISE, Z IS NOT REFERENCED. 
*/

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

/*     .......... INITIALIZE Z .......... */
    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (! (*matz)) {
        goto L10;
    }

    i_1 = *n;
    for (j = 1; j <= i_1; ++j) {

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            z[i + j * z_dim1] = 0.;
/* L2: */
        }

        z[j + j * z_dim1] = 1.;
/* L3: */
    }
/*     .......... REDUCE B TO UPPER TRIANGULAR FORM .......... */
L10:
    if (*n <= 1) {
        goto L170;
    }
    nm1 = *n - 1;

    i_1 = nm1;
    for (l = 1; l <= i_1; ++l) {
        l1 = l + 1;
        s = 0.;

        i_2 = *n;
        for (i = l1; i <= i_2; ++i) {
            s += (d_1 = b[i + l * b_dim1], abs(d_1));
/* L20: */
        }

        if (s == 0.) {
            goto L100;
        }
        s += (d_1 = b[l + l * b_dim1], abs(d_1));
        r = 0.;

        i_2 = *n;
        for (i = l; i <= i_2; ++i) {
            b[i + l * b_dim1] /= s;
/* Computing 2nd power */
            d_1 = b[i + l * b_dim1];
            r += d_1 * d_1;
/* L25: */
        }

        d_1 = sqrt(r);
        r = d_sign(&d_1, &b[l + l * b_dim1]);
        b[l + l * b_dim1] += r;
        rho = r * b[l + l * b_dim1];

        i_2 = *n;
        for (j = l1; j <= i_2; ++j) {
            t = 0.;

            i_3 = *n;
            for (i = l; i <= i_3; ++i) {
                t += b[i + l * b_dim1] * b[i + j * b_dim1];
/* L30: */
            }

            t = -t / rho;

            i_3 = *n;
            for (i = l; i <= i_3; ++i) {
                b[i + j * b_dim1] += t * b[i + l * b_dim1];
/* L40: */
            }

/* L50: */
        }

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            t = 0.;

            i_3 = *n;
            for (i = l; i <= i_3; ++i) {
                t += b[i + l * b_dim1] * a[i + j * a_dim1];
/* L60: */
            }

            t = -t / rho;

            i_3 = *n;
            for (i = l; i <= i_3; ++i) {
                a[i + j * a_dim1] += t * b[i + l * b_dim1];
/* L70: */
            }

/* L80: */
        }

        b[l + l * b_dim1] = -s * r;

        i_2 = *n;
        for (i = l1; i <= i_2; ++i) {
            b[i + l * b_dim1] = 0.;
/* L90: */
        }

L100:
        ;
    }
/*     .......... REDUCE A TO UPPER HESSENBERG FORM, WHILE */
/*                KEEPING B TRIANGULAR .......... */
    if (*n == 2) {
        goto L170;
    }
    nm2 = *n - 2;

    i_1 = nm2;
    for (k = 1; k <= i_1; ++k) {
        nk1 = nm1 - k;
/*     .......... FOR L=N-1 STEP -1 UNTIL K+1 DO -- .......... */
        i_2 = nk1;
        for (lb = 1; lb <= i_2; ++lb) {
            l = *n - lb;
            l1 = l + 1;
/*     .......... ZERO A(L+1,K) .......... */
            s = (d_1 = a[l + k * a_dim1], abs(d_1)) + (d_2 = a[l1 + k * 
                    a_dim1], abs(d_2));
            if (s == 0.) {
                goto L150;
            }
            u1 = a[l + k * a_dim1] / s;
            u2 = a[l1 + k * a_dim1] / s;
            d_1 = sqrt(u1 * u1 + u2 * u2);
            r = d_sign(&d_1, &u1);
            v1 = -(u1 + r) / r;
            v2 = -u2 / r;
            u2 = v2 / v1;

            i_3 = *n;
            for (j = k; j <= i_3; ++j) {
                t = a[l + j * a_dim1] + u2 * a[l1 + j * a_dim1];
                a[l + j * a_dim1] += t * v1;
                a[l1 + j * a_dim1] += t * v2;
/* L110: */
            }

            a[l1 + k * a_dim1] = 0.;

            i_3 = *n;
            for (j = l; j <= i_3; ++j) {
                t = b[l + j * b_dim1] + u2 * b[l1 + j * b_dim1];
                b[l + j * b_dim1] += t * v1;
                b[l1 + j * b_dim1] += t * v2;
/* L120: */
            }
/*     .......... ZERO B(L+1,L) .......... */
            s = (d_1 = b[l1 + l1 * b_dim1], abs(d_1)) + (d_2 = b[l1 + l * 
                    b_dim1], abs(d_2));
            if (s == 0.) {
                goto L150;
            }
            u1 = b[l1 + l1 * b_dim1] / s;
            u2 = b[l1 + l * b_dim1] / s;
            d_1 = sqrt(u1 * u1 + u2 * u2);
            r = d_sign(&d_1, &u1);
            v1 = -(u1 + r) / r;
            v2 = -u2 / r;
            u2 = v2 / v1;

            i_3 = l1;
            for (i = 1; i <= i_3; ++i) {
                t = b[i + l1 * b_dim1] + u2 * b[i + l * b_dim1];
                b[i + l1 * b_dim1] += t * v1;
                b[i + l * b_dim1] += t * v2;
/* L130: */
            }

            b[l1 + l * b_dim1] = 0.;

            i_3 = *n;
            for (i = 1; i <= i_3; ++i) {
                t = a[i + l1 * a_dim1] + u2 * a[i + l * a_dim1];
                a[i + l1 * a_dim1] += t * v1;
                a[i + l * a_dim1] += t * v2;
/* L140: */
            }

            if (! (*matz)) {
                goto L150;
            }

            i_3 = *n;
            for (i = 1; i <= i_3; ++i) {
                t = z[i + l1 * z_dim1] + u2 * z[i + l * z_dim1];
                z[i + l1 * z_dim1] += t * v1;
                z[i + l * z_dim1] += t * v2;
/* L145: */
            }

L150:
            ;
        }

/* L160: */
    }

L170:
    return 0;
} /* qzhes_ */

/* Subroutine */ int qzit_(integer *nm, integer *n, doublereal *a, doublereal 
        *b, doublereal *eps1, logical *matz, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset, i_1, i_2, 
            i_3;
    doublereal d_1, d_2, d_3;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal epsa, epsb;
    static integer i, j, k, l;
    static doublereal r, s, t, anorm, bnorm;
    static integer enorn;
    static doublereal a1, a2, a3;
    static integer k1, k2, l1;
    static doublereal u1, u2, u3, v1, v2, v3, a11, a12, a21, a22, a33, a34, 
            a43, a44, b11, b12, b22, b33;
    static integer na, ld;
    static doublereal b34, b44;
    static integer en;
    static doublereal ep;
    static integer ll;
    static doublereal sh;
    extern doublereal epslon_(doublereal *);
    static logical notlas;
    static integer km1, lm1;
    static doublereal ani, bni;
    static integer ish, itn, its, enm2, lor1;



/*     THIS SUBROUTINE IS THE SECOND STEP OF THE QZ ALGORITHM */
/*     FOR SOLVING GENERALIZED MATRIX EIGENVALUE PROBLEMS, */
/*     SIAM J. NUMER. ANAL. 10, 241-256(1973) BY MOLER AND STEWART, */
/*     AS MODIFIED IN TECHNICAL NOTE NASA TN D-7305(1973) BY WARD. */

/*     THIS SUBROUTINE ACCEPTS A PAIR OF REAL MATRICES, ONE OF THEM */
/*     IN UPPER HESSENBERG FORM AND THE OTHER IN UPPER TRIANGULAR FORM. */
/*     IT REDUCES THE HESSENBERG MATRIX TO QUASI-TRIANGULAR FORM USING */
/*     ORTHOGONAL TRANSFORMATIONS WHILE MAINTAINING THE TRIANGULAR FORM */
/*     OF THE OTHER MATRIX.  IT IS USUALLY PRECEDED BY  QZHES  AND */
/*     FOLLOWED BY  QZVAL  AND, POSSIBLY,  QZVEC. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES. */

/*        A CONTAINS A REAL UPPER HESSENBERG MATRIX. */

/*        B CONTAINS A REAL UPPER TRIANGULAR MATRIX. */

/*        EPS1 IS A TOLERANCE USED TO DETERMINE NEGLIGIBLE ELEMENTS. */
/*          EPS1 = 0.0 (OR NEGATIVE) MAY BE INPUT, IN WHICH CASE AN */
/*          ELEMENT WILL BE NEGLECTED ONLY IF IT IS LESS THAN ROUNDOFF */
/*          ERROR TIMES THE NORM OF ITS MATRIX.  IF THE INPUT EPS1 IS */
/*          POSITIVE, THEN AN ELEMENT WILL BE CONSIDERED NEGLIGIBLE */
/*          IF IT IS LESS THAN EPS1 TIMES THE NORM OF ITS MATRIX.  A */
/*          POSITIVE VALUE OF EPS1 MAY RESULT IN FASTER EXECUTION, */
/*          BUT LESS ACCURATE RESULTS. */

/*        MATZ SHOULD BE SET TO .TRUE. IF THE RIGHT HAND TRANSFORMATIONS 
*/
/*          ARE TO BE ACCUMULATED FOR LATER USE IN COMPUTING */
/*          EIGENVECTORS, AND TO .FALSE. OTHERWISE. */

/*        Z CONTAINS, IF MATZ HAS BEEN SET TO .TRUE., THE */
/*          TRANSFORMATION MATRIX PRODUCED IN THE REDUCTION */
/*          BY  QZHES, IF PERFORMED, OR ELSE THE IDENTITY MATRIX. */
/*          IF MATZ HAS BEEN SET TO .FALSE., Z IS NOT REFERENCED. */

/*     ON OUTPUT */

/*        A HAS BEEN REDUCED TO QUASI-TRIANGULAR FORM.  THE ELEMENTS */
/*          BELOW THE FIRST SUBDIAGONAL ARE STILL ZERO AND NO TWO */
/*          CONSECUTIVE SUBDIAGONAL ELEMENTS ARE NONZERO. */

/*        B IS STILL IN UPPER TRIANGULAR FORM, ALTHOUGH ITS ELEMENTS */
/*          HAVE BEEN ALTERED.  THE LOCATION B(N,1) IS USED TO STORE */
/*          EPS1 TIMES THE NORM OF B FOR LATER USE BY  QZVAL  AND  QZVEC. 
*/

/*        Z CONTAINS THE PRODUCT OF THE RIGHT HAND TRANSFORMATIONS */
/*          (FOR BOTH STEPS) IF MATZ HAS BEEN SET TO .TRUE.. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE LIMIT OF 30*N ITERATIONS IS EXHAUSTED */
/*                     WHILE THE J-TH EIGENVALUE IS BEING SOUGHT. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *ierr = 0;
/*     .......... COMPUTE EPSA,EPSB .......... */
    anorm = 0.;
    bnorm = 0.;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        ani = 0.;
        if (i != 1) {
            ani = (d_1 = a[i + (i - 1) * a_dim1], abs(d_1));
        }
        bni = 0.;

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            ani += (d_1 = a[i + j * a_dim1], abs(d_1));
            bni += (d_1 = b[i + j * b_dim1], abs(d_1));
/* L20: */
        }

        if (ani > anorm) {
            anorm = ani;
        }
        if (bni > bnorm) {
            bnorm = bni;
        }
/* L30: */
    }

    if (anorm == 0.) {
        anorm = 1.;
    }
    if (bnorm == 0.) {
        bnorm = 1.;
    }
    ep = *eps1;
    if (ep > 0.) {
        goto L50;
    }
/*     .......... USE ROUNDOFF LEVEL IF EPS1 IS ZERO .......... */
    ep = epslon_(&c_b141);
L50:
    epsa = ep * anorm;
    epsb = ep * bnorm;
/*     .......... REDUCE A TO QUASI-TRIANGULAR FORM, WHILE */
/*                KEEPING B TRIANGULAR .......... */
    lor1 = 1;
    enorn = *n;
    en = *n;
    itn = *n * 30;
/*     .......... BEGIN QZ STEP .......... */
L60:
    if (en <= 2) {
        goto L1001;
    }
    if (! (*matz)) {
        enorn = en;
    }
    its = 0;
    na = en - 1;
    enm2 = na - 1;
L70:
    ish = 2;
/*     .......... CHECK FOR CONVERGENCE OR REDUCIBILITY. */
/*                FOR L=EN STEP -1 UNTIL 1 DO -- .......... */
    i_1 = en;
    for (ll = 1; ll <= i_1; ++ll) {
        lm1 = en - ll;
        l = lm1 + 1;
        if (l == 1) {
            goto L95;
        }
        if ((d_1 = a[l + lm1 * a_dim1], abs(d_1)) <= epsa) {
            goto L90;
        }
/* L80: */
    }

L90:
    a[l + lm1 * a_dim1] = 0.;
    if (l < na) {
        goto L95;
    }
/*     .......... 1-BY-1 OR 2-BY-2 BLOCK ISOLATED .......... */
    en = lm1;
    goto L60;
/*     .......... CHECK FOR SMALL TOP OF B .......... */
L95:
    ld = l;
L100:
    l1 = l + 1;
    b11 = b[l + l * b_dim1];
    if (abs(b11) > epsb) {
        goto L120;
    }
    b[l + l * b_dim1] = 0.;
    s = (d_1 = a[l + l * a_dim1], abs(d_1)) + (d_2 = a[l1 + l * a_dim1], 
            abs(d_2));
    u1 = a[l + l * a_dim1] / s;
    u2 = a[l1 + l * a_dim1] / s;
    d_1 = sqrt(u1 * u1 + u2 * u2);
    r = d_sign(&d_1, &u1);
    v1 = -(u1 + r) / r;
    v2 = -u2 / r;
    u2 = v2 / v1;

    i_1 = enorn;
    for (j = l; j <= i_1; ++j) {
        t = a[l + j * a_dim1] + u2 * a[l1 + j * a_dim1];
        a[l + j * a_dim1] += t * v1;
        a[l1 + j * a_dim1] += t * v2;
        t = b[l + j * b_dim1] + u2 * b[l1 + j * b_dim1];
        b[l + j * b_dim1] += t * v1;
        b[l1 + j * b_dim1] += t * v2;
/* L110: */
    }

    if (l != 1) {
        a[l + lm1 * a_dim1] = -a[l + lm1 * a_dim1];
    }
    lm1 = l;
    l = l1;
    goto L90;
L120:
    a11 = a[l + l * a_dim1] / b11;
    a21 = a[l1 + l * a_dim1] / b11;
    if (ish == 1) {
        goto L140;
    }
/*     .......... ITERATION STRATEGY .......... */
    if (itn == 0) {
        goto L1000;
    }
    if (its == 10) {
        goto L155;
    }
/*     .......... DETERMINE TYPE OF SHIFT .......... */
    b22 = b[l1 + l1 * b_dim1];
    if (abs(b22) < epsb) {
        b22 = epsb;
    }
    b33 = b[na + na * b_dim1];
    if (abs(b33) < epsb) {
        b33 = epsb;
    }
    b44 = b[en + en * b_dim1];
    if (abs(b44) < epsb) {
        b44 = epsb;
    }
    a33 = a[na + na * a_dim1] / b33;
    a34 = a[na + en * a_dim1] / b44;
    a43 = a[en + na * a_dim1] / b33;
    a44 = a[en + en * a_dim1] / b44;
    b34 = b[na + en * b_dim1] / b44;
    t = (a43 * b34 - a33 - a44) * .5;
    r = t * t + a34 * a43 - a33 * a44;
    if (r < 0.) {
        goto L150;
    }
/*     .......... DETERMINE SINGLE SHIFT ZEROTH COLUMN OF A .......... */
    ish = 1;
    r = sqrt(r);
    sh = -t + r;
    s = -t - r;
    if ((d_1 = s - a44, abs(d_1)) < (d_2 = sh - a44, abs(d_2))) {
        sh = s;
    }
/*     .......... LOOK FOR TWO CONSECUTIVE SMALL */
/*                SUB-DIAGONAL ELEMENTS OF A. */
/*                FOR L=EN-2 STEP -1 UNTIL LD DO -- .......... */
    i_1 = enm2;
    for (ll = ld; ll <= i_1; ++ll) {
        l = enm2 + ld - ll;
        if (l == ld) {
            goto L140;
        }
        lm1 = l - 1;
        l1 = l + 1;
        t = a[l + l * a_dim1];
        if ((d_1 = b[l + l * b_dim1], abs(d_1)) > epsb) {
            t -= sh * b[l + l * b_dim1];
        }
        if ((d_1 = a[l + lm1 * a_dim1], abs(d_1)) <= (d_2 = t / a[l1 + l * 
                a_dim1], abs(d_2)) * epsa) {
            goto L100;
        }
/* L130: */
    }

L140:
    a1 = a11 - sh;
    a2 = a21;
    if (l != ld) {
        a[l + lm1 * a_dim1] = -a[l + lm1 * a_dim1];
    }
    goto L160;
/*     .......... DETERMINE DOUBLE SHIFT ZEROTH COLUMN OF A .......... */
L150:
    a12 = a[l + l1 * a_dim1] / b22;
    a22 = a[l1 + l1 * a_dim1] / b22;
    b12 = b[l + l1 * b_dim1] / b22;
    a1 = ((a33 - a11) * (a44 - a11) - a34 * a43 + a43 * b34 * a11) / a21 + 
            a12 - a11 * b12;
    a2 = a22 - a11 - a21 * b12 - (a33 - a11) - (a44 - a11) + a43 * b34;
    a3 = a[l1 + 1 + l1 * a_dim1] / b22;
    goto L160;
/*     .......... AD HOC SHIFT .......... */
L155:
    a1 = 0.;
    a2 = 1.;
    a3 = 1.1605;
L160:
    ++its;
    --itn;
    if (! (*matz)) {
        lor1 = ld;
    }
/*     .......... MAIN LOOP .......... */
    i_1 = na;
    for (k = l; k <= i_1; ++k) {
        notlas = k != na && ish == 2;
        k1 = k + 1;
        k2 = k + 2;
/* Computing MAX */
        i_2 = k - 1;
        km1 = max(i_2,l);
/* Computing MIN */
        i_2 = en, i_3 = k1 + ish;
        ll = min(i_2,i_3);
        if (notlas) {
            goto L190;
        }
/*     .......... ZERO A(K+1,K-1) .......... */
        if (k == l) {
            goto L170;
        }
        a1 = a[k + km1 * a_dim1];
        a2 = a[k1 + km1 * a_dim1];
L170:
        s = abs(a1) + abs(a2);
        if (s == 0.) {
            goto L70;
        }
        u1 = a1 / s;
        u2 = a2 / s;
        d_1 = sqrt(u1 * u1 + u2 * u2);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        u2 = v2 / v1;

        i_2 = enorn;
        for (j = km1; j <= i_2; ++j) {
            t = a[k + j * a_dim1] + u2 * a[k1 + j * a_dim1];
            a[k + j * a_dim1] += t * v1;
            a[k1 + j * a_dim1] += t * v2;
            t = b[k + j * b_dim1] + u2 * b[k1 + j * b_dim1];
            b[k + j * b_dim1] += t * v1;
            b[k1 + j * b_dim1] += t * v2;
/* L180: */
        }

        if (k != l) {
            a[k1 + km1 * a_dim1] = 0.;
        }
        goto L240;
/*     .......... ZERO A(K+1,K-1) AND A(K+2,K-1) .......... */
L190:
        if (k == l) {
            goto L200;
        }
        a1 = a[k + km1 * a_dim1];
        a2 = a[k1 + km1 * a_dim1];
        a3 = a[k2 + km1 * a_dim1];
L200:
        s = abs(a1) + abs(a2) + abs(a3);
        if (s == 0.) {
            goto L260;
        }
        u1 = a1 / s;
        u2 = a2 / s;
        u3 = a3 / s;
        d_1 = sqrt(u1 * u1 + u2 * u2 + u3 * u3);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        v3 = -u3 / r;
        u2 = v2 / v1;
        u3 = v3 / v1;

        i_2 = enorn;
        for (j = km1; j <= i_2; ++j) {
            t = a[k + j * a_dim1] + u2 * a[k1 + j * a_dim1] + u3 * a[k2 + j * 
                    a_dim1];
            a[k + j * a_dim1] += t * v1;
            a[k1 + j * a_dim1] += t * v2;
            a[k2 + j * a_dim1] += t * v3;
            t = b[k + j * b_dim1] + u2 * b[k1 + j * b_dim1] + u3 * b[k2 + j * 
                    b_dim1];
            b[k + j * b_dim1] += t * v1;
            b[k1 + j * b_dim1] += t * v2;
            b[k2 + j * b_dim1] += t * v3;
/* L210: */
        }

        if (k == l) {
            goto L220;
        }
        a[k1 + km1 * a_dim1] = 0.;
        a[k2 + km1 * a_dim1] = 0.;
/*     .......... ZERO B(K+2,K+1) AND B(K+2,K) .......... */
L220:
        s = (d_1 = b[k2 + k2 * b_dim1], abs(d_1)) + (d_2 = b[k2 + k1 * 
                b_dim1], abs(d_2)) + (d_3 = b[k2 + k * b_dim1], abs(d_3));
        if (s == 0.) {
            goto L240;
        }
        u1 = b[k2 + k2 * b_dim1] / s;
        u2 = b[k2 + k1 * b_dim1] / s;
        u3 = b[k2 + k * b_dim1] / s;
        d_1 = sqrt(u1 * u1 + u2 * u2 + u3 * u3);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        v3 = -u3 / r;
        u2 = v2 / v1;
        u3 = v3 / v1;

        i_2 = ll;
        for (i = lor1; i <= i_2; ++i) {
            t = a[i + k2 * a_dim1] + u2 * a[i + k1 * a_dim1] + u3 * a[i + k * 
                    a_dim1];
            a[i + k2 * a_dim1] += t * v1;
            a[i + k1 * a_dim1] += t * v2;
            a[i + k * a_dim1] += t * v3;
            t = b[i + k2 * b_dim1] + u2 * b[i + k1 * b_dim1] + u3 * b[i + k * 
                    b_dim1];
            b[i + k2 * b_dim1] += t * v1;
            b[i + k1 * b_dim1] += t * v2;
            b[i + k * b_dim1] += t * v3;
/* L230: */
        }

        b[k2 + k * b_dim1] = 0.;
        b[k2 + k1 * b_dim1] = 0.;
        if (! (*matz)) {
            goto L240;
        }

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            t = z[i + k2 * z_dim1] + u2 * z[i + k1 * z_dim1] + u3 * z[i + k * 
                    z_dim1];
            z[i + k2 * z_dim1] += t * v1;
            z[i + k1 * z_dim1] += t * v2;
            z[i + k * z_dim1] += t * v3;
/* L235: */
        }
/*     .......... ZERO B(K+1,K) .......... */
L240:
        s = (d_1 = b[k1 + k1 * b_dim1], abs(d_1)) + (d_2 = b[k1 + k * 
                b_dim1], abs(d_2));
        if (s == 0.) {
            goto L260;
        }
        u1 = b[k1 + k1 * b_dim1] / s;
        u2 = b[k1 + k * b_dim1] / s;
        d_1 = sqrt(u1 * u1 + u2 * u2);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        u2 = v2 / v1;

        i_2 = ll;
        for (i = lor1; i <= i_2; ++i) {
            t = a[i + k1 * a_dim1] + u2 * a[i + k * a_dim1];
            a[i + k1 * a_dim1] += t * v1;
            a[i + k * a_dim1] += t * v2;
            t = b[i + k1 * b_dim1] + u2 * b[i + k * b_dim1];
            b[i + k1 * b_dim1] += t * v1;
            b[i + k * b_dim1] += t * v2;
/* L250: */
        }

        b[k1 + k * b_dim1] = 0.;
        if (! (*matz)) {
            goto L260;
        }

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            t = z[i + k1 * z_dim1] + u2 * z[i + k * z_dim1];
            z[i + k1 * z_dim1] += t * v1;
            z[i + k * z_dim1] += t * v2;
/* L255: */
        }

L260:
        ;
    }
/*     .......... END QZ STEP .......... */
    goto L70;
/*     .......... SET ERROR -- ALL EIGENVALUES HAVE NOT */
/*                CONVERGED AFTER 30*N ITERATIONS .......... */
L1000:
    *ierr = en;
/*     .......... SAVE EPSB FOR USE BY QZVAL AND QZVEC .......... */
L1001:
    if (*n > 1) {
        b[*n + b_dim1] = epsb;
    }
    return 0;
} /* qzit_ */

/* Subroutine */ int qzval_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *alfr, doublereal *alfi, doublereal *beta, 
        logical *matz, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset, i_1, i_2;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal epsb, c, d, e;
    static integer i, j;
    static doublereal r, s, t, a1, a2, u1, u2, v1, v2, a11, a12, a21, a22, 
            b11, b12, b22, di, ei;
    static integer na;
    static doublereal an, bn;
    static integer en;
    static doublereal cq, dr;
    static integer nn;
    static doublereal cz, ti, tr, a1i, a2i, a11i, a12i, a22i, a11r, a12r, 
            a22r, sqi, ssi;
    static integer isw;
    static doublereal sqr, szi, ssr, szr;



/*     THIS SUBROUTINE IS THE THIRD STEP OF THE QZ ALGORITHM */
/*     FOR SOLVING GENERALIZED MATRIX EIGENVALUE PROBLEMS, */
/*     SIAM J. NUMER. ANAL. 10, 241-256(1973) BY MOLER AND STEWART. */

/*     THIS SUBROUTINE ACCEPTS A PAIR OF REAL MATRICES, ONE OF THEM */
/*     IN QUASI-TRIANGULAR FORM AND THE OTHER IN UPPER TRIANGULAR FORM. */
/*     IT REDUCES THE QUASI-TRIANGULAR MATRIX FURTHER, SO THAT ANY */
/*     REMAINING 2-BY-2 BLOCKS CORRESPOND TO PAIRS OF COMPLEX */
/*     EIGENVALUES, AND RETURNS QUANTITIES WHOSE RATIOS GIVE THE */
/*     GENERALIZED EIGENVALUES.  IT IS USUALLY PRECEDED BY  QZHES */
/*     AND  QZIT  AND MAY BE FOLLOWED BY  QZVEC. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES. */

/*        A CONTAINS A REAL UPPER QUASI-TRIANGULAR MATRIX. */

/*        B CONTAINS A REAL UPPER TRIANGULAR MATRIX.  IN ADDITION, */
/*          LOCATION B(N,1) CONTAINS THE TOLERANCE QUANTITY (EPSB) */
/*          COMPUTED AND SAVED IN  QZIT. */

/*        MATZ SHOULD BE SET TO .TRUE. IF THE RIGHT HAND TRANSFORMATIONS 
*/
/*          ARE TO BE ACCUMULATED FOR LATER USE IN COMPUTING */
/*          EIGENVECTORS, AND TO .FALSE. OTHERWISE. */

/*        Z CONTAINS, IF MATZ HAS BEEN SET TO .TRUE., THE */
/*          TRANSFORMATION MATRIX PRODUCED IN THE REDUCTIONS BY QZHES */
/*          AND QZIT, IF PERFORMED, OR ELSE THE IDENTITY MATRIX. */
/*          IF MATZ HAS BEEN SET TO .FALSE., Z IS NOT REFERENCED. */

/*     ON OUTPUT */

/*        A HAS BEEN REDUCED FURTHER TO A QUASI-TRIANGULAR MATRIX */
/*          IN WHICH ALL NONZERO SUBDIAGONAL ELEMENTS CORRESPOND TO */
/*          PAIRS OF COMPLEX EIGENVALUES. */

/*        B IS STILL IN UPPER TRIANGULAR FORM, ALTHOUGH ITS ELEMENTS */
/*          HAVE BEEN ALTERED.  B(N,1) IS UNALTERED. */

/*        ALFR AND ALFI CONTAIN THE REAL AND IMAGINARY PARTS OF THE */
/*          DIAGONAL ELEMENTS OF THE TRIANGULAR MATRIX THAT WOULD BE */
/*          OBTAINED IF A WERE REDUCED COMPLETELY TO TRIANGULAR FORM */
/*          BY UNITARY TRANSFORMATIONS.  NON-ZERO VALUES OF ALFI OCCUR */
/*          IN PAIRS, THE FIRST MEMBER POSITIVE AND THE SECOND NEGATIVE. 
*/

/*        BETA CONTAINS THE DIAGONAL ELEMENTS OF THE CORRESPONDING B, */
/*          NORMALIZED TO BE REAL AND NON-NEGATIVE.  THE GENERALIZED */
/*          EIGENVALUES ARE THEN THE RATIOS ((ALFR+I*ALFI)/BETA). */

/*        Z CONTAINS THE PRODUCT OF THE RIGHT HAND TRANSFORMATIONS */
/*          (FOR ALL THREE STEPS) IF MATZ HAS BEEN SET TO .TRUE. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --beta;
    --alfi;
    --alfr;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    epsb = b[*n + b_dim1];
    isw = 1;
/*     .......... FIND EIGENVALUES OF QUASI-TRIANGULAR MATRICES. */
/*                FOR EN=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (nn = 1; nn <= i_1; ++nn) {
        en = *n + 1 - nn;
        na = en - 1;
        if (isw == 2) {
            goto L505;
        }
        if (en == 1) {
            goto L410;
        }
        if (a[en + na * a_dim1] != 0.) {
            goto L420;
        }
/*     .......... 1-BY-1 BLOCK, ONE REAL ROOT .......... */
L410:
        alfr[en] = a[en + en * a_dim1];
        if (b[en + en * b_dim1] < 0.) {
            alfr[en] = -alfr[en];
        }
        beta[en] = (d_1 = b[en + en * b_dim1], abs(d_1));
        alfi[en] = 0.;
        goto L510;
/*     .......... 2-BY-2 BLOCK .......... */
L420:
        if ((d_1 = b[na + na * b_dim1], abs(d_1)) <= epsb) {
            goto L455;
        }
        if ((d_1 = b[en + en * b_dim1], abs(d_1)) > epsb) {
            goto L430;
        }
        a1 = a[en + en * a_dim1];
        a2 = a[en + na * a_dim1];
        bn = 0.;
        goto L435;
L430:
        an = (d_1 = a[na + na * a_dim1], abs(d_1)) + (d_2 = a[na + en * 
                a_dim1], abs(d_2)) + (d_3 = a[en + na * a_dim1], abs(d_3)) 
                + (d_4 = a[en + en * a_dim1], abs(d_4));
        bn = (d_1 = b[na + na * b_dim1], abs(d_1)) + (d_2 = b[na + en * 
                b_dim1], abs(d_2)) + (d_3 = b[en + en * b_dim1], abs(d_3));
        a11 = a[na + na * a_dim1] / an;
        a12 = a[na + en * a_dim1] / an;
        a21 = a[en + na * a_dim1] / an;
        a22 = a[en + en * a_dim1] / an;
        b11 = b[na + na * b_dim1] / bn;
        b12 = b[na + en * b_dim1] / bn;
        b22 = b[en + en * b_dim1] / bn;
        e = a11 / b11;
        ei = a22 / b22;
        s = a21 / (b11 * b22);
        t = (a22 - e * b22) / b22;
        if (abs(e) <= abs(ei)) {
            goto L431;
        }
        e = ei;
        t = (a11 - e * b11) / b11;
L431:
        c = (t - s * b12) * .5;
        d = c * c + s * (a12 - e * b12);
        if (d < 0.) {
            goto L480;
        }
/*     .......... TWO REAL ROOTS. */
/*                ZERO BOTH A(EN,NA) AND B(EN,NA) .......... */
        d_1 = sqrt(d);
        e += c + d_sign(&d_1, &c);
        a11 -= e * b11;
        a12 -= e * b12;
        a22 -= e * b22;
        if (abs(a11) + abs(a12) < abs(a21) + abs(a22)) {
            goto L432;
        }
        a1 = a12;
        a2 = a11;
        goto L435;
L432:
        a1 = a22;
        a2 = a21;
/*     .......... CHOOSE AND APPLY REAL Z .......... */
L435:
        s = abs(a1) + abs(a2);
        u1 = a1 / s;
        u2 = a2 / s;
        d_1 = sqrt(u1 * u1 + u2 * u2);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        u2 = v2 / v1;

        i_2 = en;
        for (i = 1; i <= i_2; ++i) {
            t = a[i + en * a_dim1] + u2 * a[i + na * a_dim1];
            a[i + en * a_dim1] += t * v1;
            a[i + na * a_dim1] += t * v2;
            t = b[i + en * b_dim1] + u2 * b[i + na * b_dim1];
            b[i + en * b_dim1] += t * v1;
            b[i + na * b_dim1] += t * v2;
/* L440: */
        }

        if (! (*matz)) {
            goto L450;
        }

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            t = z[i + en * z_dim1] + u2 * z[i + na * z_dim1];
            z[i + en * z_dim1] += t * v1;
            z[i + na * z_dim1] += t * v2;
/* L445: */
        }

L450:
        if (bn == 0.) {
            goto L475;
        }
        if (an < abs(e) * bn) {
            goto L455;
        }
        a1 = b[na + na * b_dim1];
        a2 = b[en + na * b_dim1];
        goto L460;
L455:
        a1 = a[na + na * a_dim1];
        a2 = a[en + na * a_dim1];
/*     .......... CHOOSE AND APPLY REAL Q .......... */
L460:
        s = abs(a1) + abs(a2);
        if (s == 0.) {
            goto L475;
        }
        u1 = a1 / s;
        u2 = a2 / s;
        d_1 = sqrt(u1 * u1 + u2 * u2);
        r = d_sign(&d_1, &u1);
        v1 = -(u1 + r) / r;
        v2 = -u2 / r;
        u2 = v2 / v1;

        i_2 = *n;
        for (j = na; j <= i_2; ++j) {
            t = a[na + j * a_dim1] + u2 * a[en + j * a_dim1];
            a[na + j * a_dim1] += t * v1;
            a[en + j * a_dim1] += t * v2;
            t = b[na + j * b_dim1] + u2 * b[en + j * b_dim1];
            b[na + j * b_dim1] += t * v1;
            b[en + j * b_dim1] += t * v2;
/* L470: */
        }

L475:
        a[en + na * a_dim1] = 0.;
        b[en + na * b_dim1] = 0.;
        alfr[na] = a[na + na * a_dim1];
        alfr[en] = a[en + en * a_dim1];
        if (b[na + na * b_dim1] < 0.) {
            alfr[na] = -alfr[na];
        }
        if (b[en + en * b_dim1] < 0.) {
            alfr[en] = -alfr[en];
        }
        beta[na] = (d_1 = b[na + na * b_dim1], abs(d_1));
        beta[en] = (d_1 = b[en + en * b_dim1], abs(d_1));
        alfi[en] = 0.;
        alfi[na] = 0.;
        goto L505;
/*     .......... TWO COMPLEX ROOTS .......... */
L480:
        e += c;
        ei = sqrt(-d);
        a11r = a11 - e * b11;
        a11i = ei * b11;
        a12r = a12 - e * b12;
        a12i = ei * b12;
        a22r = a22 - e * b22;
        a22i = ei * b22;
        if (abs(a11r) + abs(a11i) + abs(a12r) + abs(a12i) < abs(a21) + abs(
                a22r) + abs(a22i)) {
            goto L482;
        }
        a1 = a12r;
        a1i = a12i;
        a2 = -a11r;
        a2i = -a11i;
        goto L485;
L482:
        a1 = a22r;
        a1i = a22i;
        a2 = -a21;
        a2i = 0.;
/*     .......... CHOOSE COMPLEX Z .......... */
L485:
        cz = sqrt(a1 * a1 + a1i * a1i);
        if (cz == 0.) {
            goto L487;
        }
        szr = (a1 * a2 + a1i * a2i) / cz;
        szi = (a1 * a2i - a1i * a2) / cz;
        r = sqrt(cz * cz + szr * szr + szi * szi);
        cz /= r;
        szr /= r;
        szi /= r;
        goto L490;
L487:
        szr = 1.;
        szi = 0.;
L490:
        if (an < (abs(e) + ei) * bn) {
            goto L492;
        }
        a1 = cz * b11 + szr * b12;
        a1i = szi * b12;
        a2 = szr * b22;
        a2i = szi * b22;
        goto L495;
L492:
        a1 = cz * a11 + szr * a12;
        a1i = szi * a12;
        a2 = cz * a21 + szr * a22;
        a2i = szi * a22;
/*     .......... CHOOSE COMPLEX Q .......... */
L495:
        cq = sqrt(a1 * a1 + a1i * a1i);
        if (cq == 0.) {
            goto L497;
        }
        sqr = (a1 * a2 + a1i * a2i) / cq;
        sqi = (a1 * a2i - a1i * a2) / cq;
        r = sqrt(cq * cq + sqr * sqr + sqi * sqi);
        cq /= r;
        sqr /= r;
        sqi /= r;
        goto L500;
L497:
        sqr = 1.;
        sqi = 0.;
/*     .......... COMPUTE DIAGONAL ELEMENTS THAT WOULD RESULT */
/*                IF TRANSFORMATIONS WERE APPLIED .......... */
L500:
        ssr = sqr * szr + sqi * szi;
        ssi = sqr * szi - sqi * szr;
        i = 1;
        tr = cq * cz * a11 + cq * szr * a12 + sqr * cz * a21 + ssr * a22;
        ti = cq * szi * a12 - sqi * cz * a21 + ssi * a22;
        dr = cq * cz * b11 + cq * szr * b12 + ssr * b22;
        di = cq * szi * b12 + ssi * b22;
        goto L503;
L502:
        i = 2;
        tr = ssr * a11 - sqr * cz * a12 - cq * szr * a21 + cq * cz * a22;
        ti = -ssi * a11 - sqi * cz * a12 + cq * szi * a21;
        dr = ssr * b11 - sqr * cz * b12 + cq * cz * b22;
        di = -ssi * b11 - sqi * cz * b12;
L503:
        t = ti * dr - tr * di;
        j = na;
        if (t < 0.) {
            j = en;
        }
        r = sqrt(dr * dr + di * di);
        beta[j] = bn * r;
        alfr[j] = an * (tr * dr + ti * di) / r;
        alfi[j] = an * t / r;
        if (i == 1) {
            goto L502;
        }
L505:
        isw = 3 - isw;
L510:
        ;
    }
    b[*n + b_dim1] = epsb;

    return 0;
} /* qzval_ */

/* Subroutine */ int qzvec_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *alfr, doublereal *alfi, doublereal *beta, 
        doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset, i_1, i_2, 
            i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal alfm, almi, betm, epsb, almr, d;
    static integer i, j, k, m;
    static doublereal q, r, s, t, w, x, y, t1, t2, w1, x1, z1, di;
    static integer na, ii, en, jj;
    static doublereal ra, dr, sa;
    static integer nn;
    static doublereal ti, rr, tr, zz;
    static integer isw, enm2;



/*     THIS SUBROUTINE IS THE OPTIONAL FOURTH STEP OF THE QZ ALGORITHM */
/*     FOR SOLVING GENERALIZED MATRIX EIGENVALUE PROBLEMS, */
/*     SIAM J. NUMER. ANAL. 10, 241-256(1973) BY MOLER AND STEWART. */

/*     THIS SUBROUTINE ACCEPTS A PAIR OF REAL MATRICES, ONE OF THEM IN */
/*     QUASI-TRIANGULAR FORM (IN WHICH EACH 2-BY-2 BLOCK CORRESPONDS TO */
/*     A PAIR OF COMPLEX EIGENVALUES) AND THE OTHER IN UPPER TRIANGULAR */
/*     FORM.  IT COMPUTES THE EIGENVECTORS OF THE TRIANGULAR PROBLEM AND 
*/
/*     TRANSFORMS THE RESULTS BACK TO THE ORIGINAL COORDINATE SYSTEM. */
/*     IT IS USUALLY PRECEDED BY  QZHES,  QZIT, AND  QZVAL. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES. */

/*        A CONTAINS A REAL UPPER QUASI-TRIANGULAR MATRIX. */

/*        B CONTAINS A REAL UPPER TRIANGULAR MATRIX.  IN ADDITION, */
/*          LOCATION B(N,1) CONTAINS THE TOLERANCE QUANTITY (EPSB) */
/*          COMPUTED AND SAVED IN  QZIT. */

/*        ALFR, ALFI, AND BETA  ARE VECTORS WITH COMPONENTS WHOSE */
/*          RATIOS ((ALFR+I*ALFI)/BETA) ARE THE GENERALIZED */
/*          EIGENVALUES.  THEY ARE USUALLY OBTAINED FROM  QZVAL. */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN THE */
/*          REDUCTIONS BY  QZHES,  QZIT, AND  QZVAL, IF PERFORMED. */
/*          IF THE EIGENVECTORS OF THE TRIANGULAR PROBLEM ARE */
/*          DESIRED, Z MUST CONTAIN THE IDENTITY MATRIX. */

/*     ON OUTPUT */

/*        A IS UNALTERED.  ITS SUBDIAGONAL ELEMENTS PROVIDE INFORMATION */
/*           ABOUT THE STORAGE OF THE COMPLEX EIGENVECTORS. */

/*        B HAS BEEN DESTROYED. */

/*        ALFR, ALFI, AND BETA ARE UNALTERED. */

/*        Z CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGENVECTORS. */
/*          IF ALFI(I) .EQ. 0.0, THE I-TH EIGENVALUE IS REAL AND */
/*            THE I-TH COLUMN OF Z CONTAINS ITS EIGENVECTOR. */
/*          IF ALFI(I) .NE. 0.0, THE I-TH EIGENVALUE IS COMPLEX. */
/*            IF ALFI(I) .GT. 0.0, THE EIGENVALUE IS THE FIRST OF */
/*              A COMPLEX PAIR AND THE I-TH AND (I+1)-TH COLUMNS */
/*              OF Z CONTAIN ITS EIGENVECTOR. */
/*            IF ALFI(I) .LT. 0.0, THE EIGENVALUE IS THE SECOND OF */
/*              A COMPLEX PAIR AND THE (I-1)-TH AND I-TH COLUMNS */
/*              OF Z CONTAIN THE CONJUGATE OF ITS EIGENVECTOR. */
/*          EACH EIGENVECTOR IS NORMALIZED SO THAT THE MODULUS */
/*          OF ITS LARGEST COMPONENT IS 1.0 . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --beta;
    --alfi;
    --alfr;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    epsb = b[*n + b_dim1];
    isw = 1;
/*     .......... FOR EN=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (nn = 1; nn <= i_1; ++nn) {
        en = *n + 1 - nn;
        na = en - 1;
        if (isw == 2) {
            goto L795;
        }
        if (alfi[en] != 0.) {
            goto L710;
        }
/*     .......... REAL VECTOR .......... */
        m = en;
        b[en + en * b_dim1] = 1.;
        if (na == 0) {
            goto L800;
        }
        alfm = alfr[m];
        betm = beta[m];
/*     .......... FOR I=EN-1 STEP -1 UNTIL 1 DO -- .......... */
        i_2 = na;
        for (ii = 1; ii <= i_2; ++ii) {
            i = en - ii;
            w = betm * a[i + i * a_dim1] - alfm * b[i + i * b_dim1];
            r = 0.;

            i_3 = en;
            for (j = m; j <= i_3; ++j) {
/* L610: */
                r += (betm * a[i + j * a_dim1] - alfm * b[i + j * b_dim1]) * 
                        b[j + en * b_dim1];
            }

            if (i == 1 || isw == 2) {
                goto L630;
            }
            if (betm * a[i + (i - 1) * a_dim1] == 0.) {
                goto L630;
            }
            zz = w;
            s = r;
            goto L690;
L630:
            m = i;
            if (isw == 2) {
                goto L640;
            }
/*     .......... REAL 1-BY-1 BLOCK .......... */
            t = w;
            if (w == 0.) {
                t = epsb;
            }
            b[i + en * b_dim1] = -r / t;
            goto L700;
/*     .......... REAL 2-BY-2 BLOCK .......... */
L640:
            x = betm * a[i + (i + 1) * a_dim1] - alfm * b[i + (i + 1) * 
                    b_dim1];
            y = betm * a[i + 1 + i * a_dim1];
            q = w * zz - x * y;
            t = (x * s - zz * r) / q;
            b[i + en * b_dim1] = t;
            if (abs(x) <= abs(zz)) {
                goto L650;
            }
            b[i + 1 + en * b_dim1] = (-r - w * t) / x;
            goto L690;
L650:
            b[i + 1 + en * b_dim1] = (-s - y * t) / zz;
L690:
            isw = 3 - isw;
L700:
            ;
        }
/*     .......... END REAL VECTOR .......... */
        goto L800;
/*     .......... COMPLEX VECTOR .......... */
L710:
        m = na;
        almr = alfr[m];
        almi = alfi[m];
        betm = beta[m];
/*     .......... LAST VECTOR COMPONENT CHOSEN IMAGINARY SO THAT */
/*                EIGENVECTOR MATRIX IS TRIANGULAR .......... */
        y = betm * a[en + na * a_dim1];
        b[na + na * b_dim1] = -almi * b[en + en * b_dim1] / y;
        b[na + en * b_dim1] = (almr * b[en + en * b_dim1] - betm * a[en + en *
                 a_dim1]) / y;
        b[en + na * b_dim1] = 0.;
        b[en + en * b_dim1] = 1.;
        enm2 = na - 1;
        if (enm2 == 0) {
            goto L795;
        }
/*     .......... FOR I=EN-2 STEP -1 UNTIL 1 DO -- .......... */
        i_2 = enm2;
        for (ii = 1; ii <= i_2; ++ii) {
            i = na - ii;
            w = betm * a[i + i * a_dim1] - almr * b[i + i * b_dim1];
            w1 = -almi * b[i + i * b_dim1];
            ra = 0.;
            sa = 0.;

            i_3 = en;
            for (j = m; j <= i_3; ++j) {
                x = betm * a[i + j * a_dim1] - almr * b[i + j * b_dim1];
                x1 = -almi * b[i + j * b_dim1];
                ra = ra + x * b[j + na * b_dim1] - x1 * b[j + en * b_dim1];
                sa = sa + x * b[j + en * b_dim1] + x1 * b[j + na * b_dim1];
/* L760: */
            }

            if (i == 1 || isw == 2) {
                goto L770;
            }
            if (betm * a[i + (i - 1) * a_dim1] == 0.) {
                goto L770;
            }
            zz = w;
            z1 = w1;
            r = ra;
            s = sa;
            isw = 2;
            goto L790;
L770:
            m = i;
            if (isw == 2) {
                goto L780;
            }
/*     .......... COMPLEX 1-BY-1 BLOCK .......... */
            tr = -ra;
            ti = -sa;
L773:
            dr = w;
            di = w1;
/*     .......... COMPLEX DIVIDE (T1,T2) = (TR,TI) / (DR,DI) .....
..... */
L775:
            if (abs(di) > abs(dr)) {
                goto L777;
            }
            rr = di / dr;
            d = dr + di * rr;
            t1 = (tr + ti * rr) / d;
            t2 = (ti - tr * rr) / d;
            switch (isw) {
                case 1:  goto L787;
                case 2:  goto L782;
            }
L777:
            rr = dr / di;
            d = dr * rr + di;
            t1 = (tr * rr + ti) / d;
            t2 = (ti * rr - tr) / d;
            switch (isw) {
                case 1:  goto L787;
                case 2:  goto L782;
            }
/*     .......... COMPLEX 2-BY-2 BLOCK .......... */
L780:
            x = betm * a[i + (i + 1) * a_dim1] - almr * b[i + (i + 1) * 
                    b_dim1];
            x1 = -almi * b[i + (i + 1) * b_dim1];
            y = betm * a[i + 1 + i * a_dim1];
            tr = y * ra - w * r + w1 * s;
            ti = y * sa - w * s - w1 * r;
            dr = w * zz - w1 * z1 - x * y;
            di = w * z1 + w1 * zz - x1 * y;
            if (dr == 0. && di == 0.) {
                dr = epsb;
            }
            goto L775;
L782:
            b[i + 1 + na * b_dim1] = t1;
            b[i + 1 + en * b_dim1] = t2;
            isw = 1;
            if (abs(y) > abs(w) + abs(w1)) {
                goto L785;
            }
            tr = -ra - x * b[i + 1 + na * b_dim1] + x1 * b[i + 1 + en * 
                    b_dim1];
            ti = -sa - x * b[i + 1 + en * b_dim1] - x1 * b[i + 1 + na * 
                    b_dim1];
            goto L773;
L785:
            t1 = (-r - zz * b[i + 1 + na * b_dim1] + z1 * b[i + 1 + en * 
                    b_dim1]) / y;
            t2 = (-s - zz * b[i + 1 + en * b_dim1] - z1 * b[i + 1 + na * 
                    b_dim1]) / y;
L787:
            b[i + na * b_dim1] = t1;
            b[i + en * b_dim1] = t2;
L790:
            ;
        }
/*     .......... END COMPLEX VECTOR .......... */
L795:
        isw = 3 - isw;
L800:
        ;
    }
/*     .......... END BACK SUBSTITUTION. */
/*                TRANSFORM TO ORIGINAL COORDINATE SYSTEM. */
/*                FOR J=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (jj = 1; jj <= i_1; ++jj) {
        j = *n + 1 - jj;

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
            zz = 0.;

            i_3 = j;
            for (k = 1; k <= i_3; ++k) {
/* L860: */
                zz += z[i + k * z_dim1] * b[k + j * b_dim1];
            }

            z[i + j * z_dim1] = zz;
/* L880: */
        }
    }
/*     .......... NORMALIZE SO THAT MODULUS OF LARGEST */
/*                COMPONENT OF EACH VECTOR IS 1. */
/*                (ISW IS 1 INITIALLY FROM BEFORE) .......... */
    i_2 = *n;
    for (j = 1; j <= i_2; ++j) {
        d = 0.;
        if (isw == 2) {
            goto L920;
        }
        if (alfi[j] != 0.) {
            goto L945;
        }

        i_1 = *n;
        for (i = 1; i <= i_1; ++i) {
            if ((d_1 = z[i + j * z_dim1], abs(d_1)) > d) {
                d = (d_2 = z[i + j * z_dim1], abs(d_2));
            }
/* L890: */
        }

        i_1 = *n;
        for (i = 1; i <= i_1; ++i) {
/* L900: */
            z[i + j * z_dim1] /= d;
        }

        goto L950;

L920:
        i_1 = *n;
        for (i = 1; i <= i_1; ++i) {
            r = (d_1 = z[i + (j - 1) * z_dim1], abs(d_1)) + (d_2 = z[i + j 
                    * z_dim1], abs(d_2));
            if (r != 0.) {
/* Computing 2nd power */
                d_1 = z[i + (j - 1) * z_dim1] / r;
/* Computing 2nd power */
                d_2 = z[i + j * z_dim1] / r;
                r *= sqrt(d_1 * d_1 + d_2 * d_2);
            }
            if (r > d) {
                d = r;
            }
/* L930: */
        }

        i_1 = *n;
        for (i = 1; i <= i_1; ++i) {
            z[i + (j - 1) * z_dim1] /= d;
            z[i + j * z_dim1] /= d;
/* L940: */
        }

L945:
        isw = 3 - isw;
L950:
        ;
    }

    return 0;
} /* qzvec_ */

/* Subroutine */ int ratqr_(integer *n, doublereal *eps1, doublereal *d, 
        doublereal *e, doublereal *e2, integer *m, doublereal *w, integer *
        ind, doublereal *bd, logical *type, integer *idef, integer *ierr)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2, d_3;

    /* Local variables */
    static integer jdef;
    static doublereal f;
    static integer i, j, k;
    static doublereal p, q, r, s, delta;
    static integer k1, ii, jj;
    static doublereal ep, qp;
    extern doublereal epslon_(doublereal *);
    static doublereal err, tot;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE RATQR, */
/*     NUM. MATH. 11, 264-272(1968) BY REINSCH AND BAUER. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 257-265(1971). */

/*     THIS SUBROUTINE FINDS THE ALGEBRAICALLY SMALLEST OR LARGEST */
/*     EIGENVALUES OF A SYMMETRIC TRIDIAGONAL MATRIX BY THE */
/*     RATIONAL QR METHOD WITH NEWTON CORRECTIONS. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        EPS1 IS A THEORETICAL ABSOLUTE ERROR TOLERANCE FOR THE */
/*          COMPUTED EIGENVALUES.  IF THE INPUT EPS1 IS NON-POSITIVE, */
/*          OR INDEED SMALLER THAN ITS DEFAULT VALUE, IT IS RESET */
/*          AT EACH ITERATION TO THE RESPECTIVE DEFAULT VALUE, */
/*          NAMELY, THE PRODUCT OF THE RELATIVE MACHINE PRECISION */
/*          AND THE MAGNITUDE OF THE CURRENT EIGENVALUE ITERATE. */
/*          THE THEORETICAL ABSOLUTE ERROR IN THE K-TH EIGENVALUE */
/*          IS USUALLY NOT GREATER THAN K TIMES EPS1. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2(1) IS ARBITRARY. */

/*        M IS THE NUMBER OF EIGENVALUES TO BE FOUND. */

/*        IDEF SHOULD BE SET TO 1 IF THE INPUT MATRIX IS KNOWN TO BE */
/*          POSITIVE DEFINITE, TO -1 IF THE INPUT MATRIX IS KNOWN TO */
/*          BE NEGATIVE DEFINITE, AND TO 0 OTHERWISE. */

/*        TYPE SHOULD BE SET TO .TRUE. IF THE SMALLEST EIGENVALUES */
/*          ARE TO BE FOUND, AND TO .FALSE. IF THE LARGEST EIGENVALUES */
/*          ARE TO BE FOUND. */

/*     ON OUTPUT */

/*        EPS1 IS UNALTERED UNLESS IT HAS BEEN RESET TO ITS */
/*          (LAST) DEFAULT VALUE. */

/*        D AND E ARE UNALTERED (UNLESS W OVERWRITES D). */

/*        ELEMENTS OF E2, CORRESPONDING TO ELEMENTS OF E REGARDED */
/*          AS NEGLIGIBLE, HAVE BEEN REPLACED BY ZERO CAUSING THE */
/*          MATRIX TO SPLIT INTO A DIRECT SUM OF SUBMATRICES. */
/*          E2(1) IS SET TO 0.0D0 IF THE SMALLEST EIGENVALUES HAVE BEEN */
/*          FOUND, AND TO 2.0D0 IF THE LARGEST EIGENVALUES HAVE BEEN */
/*          FOUND.  E2 IS OTHERWISE UNALTERED (UNLESS OVERWRITTEN BY BD). 
*/

/*        W CONTAINS THE M ALGEBRAICALLY SMALLEST EIGENVALUES IN */
/*          ASCENDING ORDER, OR THE M LARGEST EIGENVALUES IN */
/*          DESCENDING ORDER.  IF AN ERROR EXIT IS MADE BECAUSE OF */
/*          AN INCORRECT SPECIFICATION OF IDEF, NO EIGENVALUES */
/*          ARE FOUND.  IF THE NEWTON ITERATES FOR A PARTICULAR */
/*          EIGENVALUE ARE NOT MONOTONE, THE BEST ESTIMATE OBTAINED */
/*          IS RETURNED AND IERR IS SET.  W MAY COINCIDE WITH D. */

/*        IND CONTAINS IN ITS FIRST M POSITIONS THE SUBMATRIX INDICES */
/*          ASSOCIATED WITH THE CORRESPONDING EIGENVALUES IN W -- */
/*          1 FOR EIGENVALUES BELONGING TO THE FIRST SUBMATRIX FROM */
/*          THE TOP, 2 FOR THOSE BELONGING TO THE SECOND SUBMATRIX, ETC.. 
*/

/*        BD CONTAINS REFINED BOUNDS FOR THE THEORETICAL ERRORS OF THE */
/*          CORRESPONDING EIGENVALUES IN W.  THESE BOUNDS ARE USUALLY */
/*          WITHIN THE TOLERANCE SPECIFIED BY EPS1.  BD MAY COINCIDE */
/*          WITH E2. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          6*N+1      IF  IDEF  IS SET TO 1 AND  TYPE  TO .TRUE. */
/*                     WHEN THE MATRIX IS NOT POSITIVE DEFINITE, OR */
/*                     IF  IDEF  IS SET TO -1 AND  TYPE  TO .FALSE. */
/*                     WHEN THE MATRIX IS NOT NEGATIVE DEFINITE, */
/*          5*N+K      IF SUCCESSIVE ITERATES TO THE K-TH EIGENVALUE */
/*                     ARE NOT MONOTONE INCREASING, WHERE K REFERS */
/*                     TO THE LAST SUCH OCCURRENCE. */

/*     NOTE THAT SUBROUTINE TRIDIB IS GENERALLY FASTER AND MORE */
/*     ACCURATE THAN RATQR IF THE EIGENVALUES ARE CLUSTERED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --bd;
    --ind;
    --w;
    --e2;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    jdef = *idef;
/*     .......... COPY D ARRAY INTO W .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L20: */
        w[i] = d[i];
    }

    if (*type) {
        goto L40;
    }
    j = 1;
    goto L400;
L40:
    err = 0.;
    s = 0.;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ENTRIES AND DEFINE */
/*                INITIAL SHIFT FROM LOWER GERSCHGORIN BOUND. */
/*                COPY E2 ARRAY INTO BD .......... */
    tot = w[1];
    q = 0.;
    j = 0;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        p = q;
        if (i == 1) {
            goto L60;
        }
        d_3 = (d_1 = d[i], abs(d_1)) + (d_2 = d[i - 1], abs(d_2));
        if (p > epslon_(&d_3)) {
            goto L80;
        }
L60:
        e2[i] = 0.;
L80:
        bd[i] = e2[i];
/*     .......... COUNT ALSO IF ELEMENT OF E2 HAS UNDERFLOWED ........
.. */
        if (e2[i] == 0.) {
            ++j;
        }
        ind[i] = j;
        q = 0.;
        if (i != *n) {
            q = (d_1 = e[i + 1], abs(d_1));
        }
/* Computing MIN */
        d_1 = w[i] - p - q;
        tot = min(d_1,tot);
/* L100: */
    }

    if (jdef == 1 && tot < 0.) {
        goto L140;
    }

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L110: */
        w[i] -= tot;
    }

    goto L160;
L140:
    tot = 0.;

L160:
    i_1 = *m;
    for (k = 1; k <= i_1; ++k) {
/*     .......... NEXT QR TRANSFORMATION .......... */
L180:
        tot += s;
        delta = w[*n] - s;
        i = *n;
        f = (d_1 = epslon_(&tot), abs(d_1));
        if (*eps1 < f) {
            *eps1 = f;
        }
        if (delta > *eps1) {
            goto L190;
        }
        if (delta < -(*eps1)) {
            goto L1000;
        }
        goto L300;
/*     .......... REPLACE SMALL SUB-DIAGONAL SQUARES BY ZERO */
/*                TO REDUCE THE INCIDENCE OF UNDERFLOWS .......... */
L190:
        if (k == *n) {
            goto L210;
        }
        k1 = k + 1;
        i_2 = *n;
        for (j = k1; j <= i_2; ++j) {
            d_2 = w[j] + w[j - 1];
/* Computing 2nd power */
            d_1 = epslon_(&d_2);
            if (bd[j] <= d_1 * d_1) {
                bd[j] = 0.;
            }
/* L200: */
        }

L210:
        f = bd[*n] / delta;
        qp = delta + f;
        p = 1.;
        if (k == *n) {
            goto L260;
        }
        k1 = *n - k;
/*     .......... FOR I=N-1 STEP -1 UNTIL K DO -- .......... */
        i_2 = k1;
        for (ii = 1; ii <= i_2; ++ii) {
            i = *n - ii;
            q = w[i] - s - f;
            r = q / qp;
            p = p * r + 1.;
            ep = f * r;
            w[i + 1] = qp + ep;
            delta = q - ep;
            if (delta > *eps1) {
                goto L220;
            }
            if (delta < -(*eps1)) {
                goto L1000;
            }
            goto L300;
L220:
            f = bd[i] / q;
            qp = delta + f;
            bd[i + 1] = qp * ep;
/* L240: */
        }

L260:
        w[k] = qp;
        s = qp / p;
        if (tot + s > tot) {
            goto L180;
        }
/*     .......... SET ERROR -- IRREGULAR END OF ITERATION. */
/*                DEFLATE MINIMUM DIAGONAL ELEMENT .......... */
        *ierr = *n * 5 + k;
        s = 0.;
        delta = qp;

        i_2 = *n;
        for (j = k; j <= i_2; ++j) {
            if (w[j] > delta) {
                goto L280;
            }
            i = j;
            delta = w[j];
L280:
            ;
        }
/*     .......... CONVERGENCE .......... */
L300:
        if (i < *n) {
            bd[i + 1] = bd[i] * f / qp;
        }
        ii = ind[i];
        if (i == k) {
            goto L340;
        }
        k1 = i - k;
/*     .......... FOR J=I-1 STEP -1 UNTIL K DO -- .......... */
        i_2 = k1;
        for (jj = 1; jj <= i_2; ++jj) {
            j = i - jj;
            w[j + 1] = w[j] - s;
            bd[j + 1] = bd[j];
            ind[j + 1] = ind[j];
/* L320: */
        }

L340:
        w[k] = tot;
        err += abs(delta);
        bd[k] = err;
        ind[k] = ii;
/* L360: */
    }

    if (*type) {
        goto L1001;
    }
    f = bd[1];
    e2[1] = 2.;
    bd[1] = f;
    j = 2;
/*     .......... NEGATE ELEMENTS OF W FOR LARGEST VALUES .......... */
L400:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L500: */
        w[i] = -w[i];
    }

    jdef = -jdef;
    switch (j) {
        case 1:  goto L40;
        case 2:  goto L1001;
    }
/*     .......... SET ERROR -- IDEF SPECIFIED INCORRECTLY .......... */
L1000:
    *ierr = *n * 6 + 1;
L1001:
    return 0;
} /* ratqr_ */

/* Subroutine */ int rebak_(integer *nm, integer *n, doublereal *b, 
        doublereal *dl, integer *m, doublereal *z)
{
    /* System generated locals */
    integer b_dim1, b_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer i, j, k;
    static doublereal x;
    static integer i1, ii;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE REBAKA, */
/*     NUM. MATH. 11, 99-110(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 303-314(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A GENERALIZED */
/*     SYMMETRIC EIGENSYSTEM BY BACK TRANSFORMING THOSE OF THE */
/*     DERIVED SYMMETRIC MATRIX DETERMINED BY  REDUC. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX SYSTEM. */

/*        B CONTAINS INFORMATION ABOUT THE SIMILARITY TRANSFORMATION */
/*          (CHOLESKY DECOMPOSITION) USED IN THE REDUCTION BY  REDUC */
/*          IN ITS STRICT LOWER TRIANGLE. */

/*        DL CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATION. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMED EIGENVECTORS */
/*          IN ITS FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --dl;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }

    i_1 = *m;
    for (j = 1; j <= i_1; ++j) {
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
        i_2 = *n;
        for (ii = 1; ii <= i_2; ++ii) {
            i = *n + 1 - ii;
            i1 = i + 1;
            x = z[i + j * z_dim1];
            if (i == *n) {
                goto L80;
            }

            i_3 = *n;
            for (k = i1; k <= i_3; ++k) {
/* L60: */
                x -= b[k + i * b_dim1] * z[k + j * z_dim1];
            }

L80:
            z[i + j * z_dim1] = x / dl[i];
/* L100: */
        }
    }

L200:
    return 0;
} /* rebak_ */

/* Subroutine */ int rebakb_(integer *nm, integer *n, doublereal *b, 
        doublereal *dl, integer *m, doublereal *z)
{
    /* System generated locals */
    integer b_dim1, b_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer i, j, k;
    static doublereal x;
    static integer i1, ii;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE REBAKB, */
/*     NUM. MATH. 11, 99-110(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 303-314(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A GENERALIZED */
/*     SYMMETRIC EIGENSYSTEM BY BACK TRANSFORMING THOSE OF THE */
/*     DERIVED SYMMETRIC MATRIX DETERMINED BY  REDUC2. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX SYSTEM. */

/*        B CONTAINS INFORMATION ABOUT THE SIMILARITY TRANSFORMATION */
/*          (CHOLESKY DECOMPOSITION) USED IN THE REDUCTION BY  REDUC2 */
/*          IN ITS STRICT LOWER TRIANGLE. */

/*        DL CONTAINS FURTHER INFORMATION ABOUT THE TRANSFORMATION. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMED EIGENVECTORS */
/*          IN ITS FIRST M COLUMNS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --dl;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }

    i_1 = *m;
    for (j = 1; j <= i_1; ++j) {
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
        i_2 = *n;
        for (ii = 1; ii <= i_2; ++ii) {
            i1 = *n - ii;
            i = i1 + 1;
            x = dl[i] * z[i + j * z_dim1];
            if (i == 1) {
                goto L80;
            }

            i_3 = i1;
            for (k = 1; k <= i_3; ++k) {
/* L60: */
                x += b[i + k * b_dim1] * z[k + j * z_dim1];
            }

L80:
            z[i + j * z_dim1] = x;
/* L100: */
        }
    }

L200:
    return 0;
} /* rebakb_ */

/* Subroutine */ int reduc_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *dl, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, i_1, i_2, i_3;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i, j, k;
    static doublereal x, y;
    static integer i1, j1, nn;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE REDUC1, */
/*     NUM. MATH. 11, 99-110(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 303-314(1971). */

/*     THIS SUBROUTINE REDUCES THE GENERALIZED SYMMETRIC EIGENPROBLEM */
/*     AX=(LAMBDA)BX, WHERE B IS POSITIVE DEFINITE, TO THE STANDARD */
/*     SYMMETRIC EIGENPROBLEM USING THE CHOLESKY FACTORIZATION OF B. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES A AND B.  IF THE CHOLESKY */
/*          FACTOR L OF B IS ALREADY AVAILABLE, N SHOULD BE PREFIXED */
/*          WITH A MINUS SIGN. */

/*        A AND B CONTAIN THE REAL SYMMETRIC INPUT MATRICES.  ONLY THE */
/*          FULL UPPER TRIANGLES OF THE MATRICES NEED BE SUPPLIED.  IF */
/*          N IS NEGATIVE, THE STRICT LOWER TRIANGLE OF B CONTAINS, */
/*          INSTEAD, THE STRICT LOWER TRIANGLE OF ITS CHOLESKY FACTOR L. 
*/

/*        DL CONTAINS, IF N IS NEGATIVE, THE DIAGONAL ELEMENTS OF L. */

/*     ON OUTPUT */

/*        A CONTAINS IN ITS FULL LOWER TRIANGLE THE FULL LOWER TRIANGLE */
/*          OF THE SYMMETRIC MATRIX DERIVED FROM THE REDUCTION TO THE */
/*          STANDARD FORM.  THE STRICT UPPER TRIANGLE OF A IS UNALTERED. 
*/

/*        B CONTAINS IN ITS STRICT LOWER TRIANGLE THE STRICT LOWER */
/*          TRIANGLE OF ITS CHOLESKY FACTOR L.  THE FULL UPPER */
/*          TRIANGLE OF B IS UNALTERED. */

/*        DL CONTAINS THE DIAGONAL ELEMENTS OF L. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          7*N+1      IF B IS NOT POSITIVE DEFINITE. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --dl;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *ierr = 0;
    nn = abs(*n);
    if (*n < 0) {
        goto L100;
    }
/*     .......... FORM L IN THE ARRAYS B AND DL .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        i1 = i - 1;

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            x = b[i + j * b_dim1];
            if (i == 1) {
                goto L40;
            }

            i_3 = i1;
            for (k = 1; k <= i_3; ++k) {
/* L20: */
                x -= b[i + k * b_dim1] * b[j + k * b_dim1];
            }

L40:
            if (j != i) {
                goto L60;
            }
            if (x <= 0.) {
                goto L1000;
            }
            y = sqrt(x);
            dl[i] = y;
            goto L80;
L60:
            b[j + i * b_dim1] = x / y;
L80:
            ;
        }
    }
/*     .......... FORM THE TRANSPOSE OF THE UPPER TRIANGLE OF INV(L)*A */
/*                IN THE LOWER TRIANGLE OF THE ARRAY A .......... */
L100:
    i_2 = nn;
    for (i = 1; i <= i_2; ++i) {
        i1 = i - 1;
        y = dl[i];

        i_1 = nn;
        for (j = i; j <= i_1; ++j) {
            x = a[i + j * a_dim1];
            if (i == 1) {
                goto L180;
            }

            i_3 = i1;
            for (k = 1; k <= i_3; ++k) {
/* L160: */
                x -= b[i + k * b_dim1] * a[j + k * a_dim1];
            }

L180:
            a[j + i * a_dim1] = x / y;
/* L200: */
        }
    }
/*     .......... PRE-MULTIPLY BY INV(L) AND OVERWRITE .......... */
    i_1 = nn;
    for (j = 1; j <= i_1; ++j) {
        j1 = j - 1;

        i_2 = nn;
        for (i = j; i <= i_2; ++i) {
            x = a[i + j * a_dim1];
            if (i == j) {
                goto L240;
            }
            i1 = i - 1;

            i_3 = i1;
            for (k = j; k <= i_3; ++k) {
/* L220: */
                x -= a[k + j * a_dim1] * b[i + k * b_dim1];
            }

L240:
            if (j == 1) {
                goto L280;
            }

            i_3 = j1;
            for (k = 1; k <= i_3; ++k) {
/* L260: */
                x -= a[j + k * a_dim1] * b[i + k * b_dim1];
            }

L280:
            a[i + j * a_dim1] = x / dl[i];
/* L300: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- B IS NOT POSITIVE DEFINITE .......... */
L1000:
    *ierr = *n * 7 + 1;
L1001:
    return 0;
} /* reduc_ */

/* Subroutine */ int reduc2_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *dl, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, i_1, i_2, i_3;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i, j, k;
    static doublereal x, y;
    static integer i1, j1, nn;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE REDUC2, */
/*     NUM. MATH. 11, 99-110(1968) BY MARTIN AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 303-314(1971). */

/*     THIS SUBROUTINE REDUCES THE GENERALIZED SYMMETRIC EIGENPROBLEMS */
/*     ABX=(LAMBDA)X OR BAY=(LAMBDA)Y, WHERE B IS POSITIVE DEFINITE, */
/*     TO THE STANDARD SYMMETRIC EIGENPROBLEM USING THE CHOLESKY */
/*     FACTORIZATION OF B. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRICES A AND B.  IF THE CHOLESKY */
/*          FACTOR L OF B IS ALREADY AVAILABLE, N SHOULD BE PREFIXED */
/*          WITH A MINUS SIGN. */

/*        A AND B CONTAIN THE REAL SYMMETRIC INPUT MATRICES.  ONLY THE */
/*          FULL UPPER TRIANGLES OF THE MATRICES NEED BE SUPPLIED.  IF */
/*          N IS NEGATIVE, THE STRICT LOWER TRIANGLE OF B CONTAINS, */
/*          INSTEAD, THE STRICT LOWER TRIANGLE OF ITS CHOLESKY FACTOR L. 
*/

/*        DL CONTAINS, IF N IS NEGATIVE, THE DIAGONAL ELEMENTS OF L. */

/*     ON OUTPUT */

/*        A CONTAINS IN ITS FULL LOWER TRIANGLE THE FULL LOWER TRIANGLE */
/*          OF THE SYMMETRIC MATRIX DERIVED FROM THE REDUCTION TO THE */
/*          STANDARD FORM.  THE STRICT UPPER TRIANGLE OF A IS UNALTERED. 
*/

/*        B CONTAINS IN ITS STRICT LOWER TRIANGLE THE STRICT LOWER */
/*          TRIANGLE OF ITS CHOLESKY FACTOR L.  THE FULL UPPER */
/*          TRIANGLE OF B IS UNALTERED. */

/*        DL CONTAINS THE DIAGONAL ELEMENTS OF L. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          7*N+1      IF B IS NOT POSITIVE DEFINITE. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --dl;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *ierr = 0;
    nn = abs(*n);
    if (*n < 0) {
        goto L100;
    }
/*     .......... FORM L IN THE ARRAYS B AND DL .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        i1 = i - 1;

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
            x = b[i + j * b_dim1];
            if (i == 1) {
                goto L40;
            }

            i_3 = i1;
            for (k = 1; k <= i_3; ++k) {
/* L20: */
                x -= b[i + k * b_dim1] * b[j + k * b_dim1];
            }

L40:
            if (j != i) {
                goto L60;
            }
            if (x <= 0.) {
                goto L1000;
            }
            y = sqrt(x);
            dl[i] = y;
            goto L80;
L60:
            b[j + i * b_dim1] = x / y;
L80:
            ;
        }
    }
/*     .......... FORM THE LOWER TRIANGLE OF A*L */
/*                IN THE LOWER TRIANGLE OF THE ARRAY A .......... */
L100:
    i_2 = nn;
    for (i = 1; i <= i_2; ++i) {
        i1 = i + 1;

        i_1 = i;
        for (j = 1; j <= i_1; ++j) {
            x = a[j + i * a_dim1] * dl[j];
            if (j == i) {
                goto L140;
            }
            j1 = j + 1;

            i_3 = i;
            for (k = j1; k <= i_3; ++k) {
/* L120: */
                x += a[k + i * a_dim1] * b[k + j * b_dim1];
            }

L140:
            if (i == nn) {
                goto L180;
            }

            i_3 = nn;
            for (k = i1; k <= i_3; ++k) {
/* L160: */
                x += a[i + k * a_dim1] * b[k + j * b_dim1];
            }

L180:
            a[i + j * a_dim1] = x;
/* L200: */
        }
    }
/*     .......... PRE-MULTIPLY BY TRANSPOSE(L) AND OVERWRITE .......... */
    i_1 = nn;
    for (i = 1; i <= i_1; ++i) {
        i1 = i + 1;
        y = dl[i];

        i_2 = i;
        for (j = 1; j <= i_2; ++j) {
            x = y * a[i + j * a_dim1];
            if (i == nn) {
                goto L280;
            }

            i_3 = nn;
            for (k = i1; k <= i_3; ++k) {
/* L260: */
                x += a[k + j * a_dim1] * b[k + i * b_dim1];
            }

L280:
            a[i + j * a_dim1] = x;
/* L300: */
        }
    }

    goto L1001;
/*     .......... SET ERROR -- B IS NOT POSITIVE DEFINITE .......... */
L1000:
    *ierr = *n * 7 + 1;
L1001:
    return 0;
} /* reduc2_ */

/* Subroutine */ int rg_(integer *nm, integer *n, doublereal *a, doublereal *
        wr, doublereal *wi, integer *matz, doublereal *z, integer *iv1, 
        doublereal *fv1, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int balbak_(integer *, integer *, integer *, 
            integer *, doublereal *, integer *, doublereal *), balanc_(
            integer *, integer *, doublereal *, integer *, integer *, 
            doublereal *), elmhes_(integer *, integer *, integer *, integer *,
             doublereal *, integer *), eltran_(integer *, integer *, integer *
            , integer *, doublereal *, integer *, doublereal *);
    static integer is1, is2;
    extern /* Subroutine */ int hqr_(integer *, integer *, integer *, integer 
            *, doublereal *, doublereal *, doublereal *, integer *), hqr2_(
            integer *, integer *, integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A REAL GENERAL MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        A  CONTAINS THE REAL GENERAL MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        WR  AND  WI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE EIGENVALUES.  COMPLEX CONJUGATE */
/*        PAIRS OF EIGENVALUES APPEAR CONSECUTIVELY WITH THE */
/*        EIGENVALUE HAVING THE POSITIVE IMAGINARY PART FIRST. */

/*        Z  CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGENVECTORS */
/*        IF MATZ IS NOT ZERO.  IF THE J-TH EIGENVALUE IS REAL, THE */
/*        J-TH COLUMN OF  Z  CONTAINS ITS EIGENVECTOR.  IF THE J-TH */
/*        EIGENVALUE IS COMPLEX WITH POSITIVE IMAGINARY PART, THE */
/*        J-TH AND (J+1)-TH COLUMNS OF  Z  CONTAIN THE REAL AND */
/*        IMAGINARY PARTS OF ITS EIGENVECTOR.  THE CONJUGATE OF THIS */
/*        VECTOR IS THE EIGENVECTOR FOR THE CONJUGATE EIGENVALUE. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR HQR */
/*           AND HQR2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        IV1  AND  FV1  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv1;
    --iv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --wi;
    --wr;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    balanc_(nm, n, &a[a_offset], &is1, &is2, &fv1[1]);
    elmhes_(nm, n, &is1, &is2, &a[a_offset], &iv1[1]);
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    hqr_(nm, n, &is1, &is2, &a[a_offset], &wr[1], &wi[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    eltran_(nm, n, &is1, &is2, &a[a_offset], &iv1[1], &z[z_offset]);
    hqr2_(nm, n, &is1, &is2, &a[a_offset], &wr[1], &wi[1], &z[z_offset], ierr)
            ;
    if (*ierr != 0) {
        goto L50;
    }
    balbak_(nm, n, &is1, &is2, &fv1[1], n, &z[z_offset]);
L50:
    return 0;
} /* rg_ */

/* Subroutine */ int rgg_(integer *nm, integer *n, doublereal *a, doublereal *
        b, doublereal *alfr, doublereal *alfi, doublereal *beta, integer *
        matz, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int qzit_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, logical *, doublereal *, integer *), 
            qzvec_(integer *, integer *, doublereal *, doublereal *, 
            doublereal *, doublereal *, doublereal *, doublereal *), qzhes_(
            integer *, integer *, doublereal *, doublereal *, logical *, 
            doublereal *), qzval_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *, doublereal *, logical *,
             doublereal *);
    static logical tf;



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     FOR THE REAL GENERAL GENERALIZED EIGENPROBLEM  AX = (LAMBDA)BX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRICES  A  AND  B. */

/*        A  CONTAINS A REAL GENERAL MATRIX. */

/*        B  CONTAINS A REAL GENERAL MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        ALFR  AND  ALFI  CONTAIN THE REAL AND IMAGINARY PARTS, */
/*        RESPECTIVELY, OF THE NUMERATORS OF THE EIGENVALUES. */

/*        BETA  CONTAINS THE DENOMINATORS OF THE EIGENVALUES, */
/*        WHICH ARE THUS GIVEN BY THE RATIOS  (ALFR+I*ALFI)/BETA. */
/*        COMPLEX CONJUGATE PAIRS OF EIGENVALUES APPEAR CONSECUTIVELY */
/*        WITH THE EIGENVALUE HAVING THE POSITIVE IMAGINARY PART FIRST. */

/*        Z  CONTAINS THE REAL AND IMAGINARY PARTS OF THE EIGENVECTORS */
/*        IF MATZ IS NOT ZERO.  IF THE J-TH EIGENVALUE IS REAL, THE */
/*        J-TH COLUMN OF  Z  CONTAINS ITS EIGENVECTOR.  IF THE J-TH */
/*        EIGENVALUE IS COMPLEX WITH POSITIVE IMAGINARY PART, THE */
/*        J-TH AND (J+1)-TH COLUMNS OF  Z  CONTAIN THE REAL AND */
/*        IMAGINARY PARTS OF ITS EIGENVECTOR.  THE CONJUGATE OF THIS */
/*        VECTOR IS THE EIGENVECTOR FOR THE CONJUGATE EIGENVALUE. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR QZIT. */
/*           THE NORMAL COMPLETION CODE IS ZERO. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --beta;
    --alfi;
    --alfr;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tf = FALSE_;
    qzhes_(nm, n, &a[a_offset], &b[b_offset], &tf, &z[z_offset]);
    qzit_(nm, n, &a[a_offset], &b[b_offset], &c_b550, &tf, &z[z_offset], ierr)
            ;
    qzval_(nm, n, &a[a_offset], &b[b_offset], &alfr[1], &alfi[1], &beta[1], &
            tf, &z[z_offset]);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tf = TRUE_;
    qzhes_(nm, n, &a[a_offset], &b[b_offset], &tf, &z[z_offset]);
    qzit_(nm, n, &a[a_offset], &b[b_offset], &c_b550, &tf, &z[z_offset], ierr)
            ;
    qzval_(nm, n, &a[a_offset], &b[b_offset], &alfr[1], &alfi[1], &beta[1], &
            tf, &z[z_offset]);
    if (*ierr != 0) {
        goto L50;
    }
    qzvec_(nm, n, &a[a_offset], &b[b_offset], &alfr[1], &alfi[1], &beta[1], &
            z[z_offset]);
L50:
    return 0;
} /* rgg_ */

/* Subroutine */ int rs_(integer *nm, integer *n, doublereal *a, doublereal *
        w, integer *matz, doublereal *z, doublereal *fv1, doublereal *fv2, 
        integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int tred1_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *), tred2_(integer *, 
            integer *, doublereal *, doublereal *, doublereal *, doublereal *)
            , tqlrat_(integer *, doublereal *, doublereal *, integer *), 
            tql2_(integer *, integer *, doublereal *, doublereal *, 
            doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A REAL SYMMETRIC MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        A  CONTAINS THE REAL SYMMETRIC MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tred1_(nm, n, &a[a_offset], &w[1], &fv1[1], &fv2[1]);
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tred2_(nm, n, &a[a_offset], &w[1], &fv1[1], &z[z_offset]);
    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
L50:
    return 0;
} /* rs_ */

/* Subroutine */ int rsb_(integer *nm, integer *n, integer *mb, doublereal *a,
         doublereal *w, integer *matz, doublereal *z, doublereal *fv1, 
        doublereal *fv2, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int bandr_(integer *, integer *, integer *, 
            doublereal *, doublereal *, doublereal *, doublereal *, logical *,
             doublereal *);
    static logical tf;
    extern /* Subroutine */ int tqlrat_(integer *, doublereal *, doublereal *,
             integer *), tql2_(integer *, integer *, doublereal *, doublereal 
            *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A REAL SYMMETRIC BAND MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        MB  IS THE HALF BAND WIDTH OF THE MATRIX, DEFINED AS THE */
/*        NUMBER OF ADJACENT DIAGONALS, INCLUDING THE PRINCIPAL */
/*        DIAGONAL, REQUIRED TO SPECIFY THE NON-ZERO PORTION OF THE */
/*        LOWER TRIANGLE OF THE MATRIX. */

/*        A  CONTAINS THE LOWER TRIANGLE OF THE REAL SYMMETRIC */
/*        BAND MATRIX.  ITS LOWEST SUBDIAGONAL IS STORED IN THE */
/*        LAST  N+1-MB  POSITIONS OF THE FIRST COLUMN, ITS NEXT */
/*        SUBDIAGONAL IN THE LAST  N+2-MB  POSITIONS OF THE */
/*        SECOND COLUMN, FURTHER SUBDIAGONALS SIMILARLY, AND */
/*        FINALLY ITS PRINCIPAL DIAGONAL IN THE  N  POSITIONS */
/*        OF THE LAST COLUMN.  CONTENTS OF STORAGES NOT PART */
/*        OF THE MATRIX ARE ARBITRARY. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L5;
    }
    *ierr = *n * 10;
    goto L50;
L5:
    if (*mb > 0) {
        goto L10;
    }
    *ierr = *n * 12;
    goto L50;
L10:
    if (*mb <= *n) {
        goto L15;
    }
    *ierr = *n * 12;
    goto L50;

L15:
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tf = FALSE_;
    bandr_(nm, n, mb, &a[a_offset], &w[1], &fv1[1], &fv2[1], &tf, &z[z_offset]
            );
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tf = TRUE_;
    bandr_(nm, n, mb, &a[a_offset], &w[1], &fv1[1], &fv1[1], &tf, &z[z_offset]
            );
    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
L50:
    return 0;
} /* rsb_ */

/* Subroutine */ int rsg_(integer *nm, integer *n, doublereal *a, doublereal *
        b, doublereal *w, integer *matz, doublereal *z, doublereal *fv1, 
        doublereal *fv2, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int tred1_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *), tred2_(integer *, 
            integer *, doublereal *, doublereal *, doublereal *, doublereal *)
            , rebak_(integer *, integer *, doublereal *, doublereal *, 
            integer *, doublereal *), reduc_(integer *, integer *, doublereal 
            *, doublereal *, doublereal *, integer *), tqlrat_(integer *, 
            doublereal *, doublereal *, integer *), tql2_(integer *, integer *
            , doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     FOR THE REAL SYMMETRIC GENERALIZED EIGENPROBLEM  AX = (LAMBDA)BX. 
*/

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRICES  A  AND  B. */

/*        A  CONTAINS A REAL SYMMETRIC MATRIX. */

/*        B  CONTAINS A POSITIVE DEFINITE REAL SYMMETRIC MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    reduc_(nm, n, &a[a_offset], &b[b_offset], &fv2[1], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tred1_(nm, n, &a[a_offset], &w[1], &fv1[1], &fv2[1]);
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tred2_(nm, n, &a[a_offset], &w[1], &fv1[1], &z[z_offset]);
    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    rebak_(nm, n, &b[b_offset], &fv2[1], n, &z[z_offset]);
L50:
    return 0;
} /* rsg_ */

/* Subroutine */ int rsgab_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *w, integer *matz, doublereal *z, 
        doublereal *fv1, doublereal *fv2, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int tred1_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *), tred2_(integer *, 
            integer *, doublereal *, doublereal *, doublereal *, doublereal *)
            , rebak_(integer *, integer *, doublereal *, doublereal *, 
            integer *, doublereal *), reduc2_(integer *, integer *, 
            doublereal *, doublereal *, doublereal *, integer *), tqlrat_(
            integer *, doublereal *, doublereal *, integer *), tql2_(integer *
            , integer *, doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     FOR THE REAL SYMMETRIC GENERALIZED EIGENPROBLEM  ABX = (LAMBDA)X. 
*/

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRICES  A  AND  B. */

/*        A  CONTAINS A REAL SYMMETRIC MATRIX. */

/*        B  CONTAINS A POSITIVE DEFINITE REAL SYMMETRIC MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    reduc2_(nm, n, &a[a_offset], &b[b_offset], &fv2[1], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tred1_(nm, n, &a[a_offset], &w[1], &fv1[1], &fv2[1]);
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tred2_(nm, n, &a[a_offset], &w[1], &fv1[1], &z[z_offset]);
    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    rebak_(nm, n, &b[b_offset], &fv2[1], n, &z[z_offset]);
L50:
    return 0;
} /* rsgab_ */

/* Subroutine */ int rsgba_(integer *nm, integer *n, doublereal *a, 
        doublereal *b, doublereal *w, integer *matz, doublereal *z, 
        doublereal *fv1, doublereal *fv2, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, b_dim1, b_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int tred1_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *), tred2_(integer *, 
            integer *, doublereal *, doublereal *, doublereal *, doublereal *)
            , reduc2_(integer *, integer *, doublereal *, doublereal *, 
            doublereal *, integer *), rebakb_(integer *, integer *, 
            doublereal *, doublereal *, integer *, doublereal *), tqlrat_(
            integer *, doublereal *, doublereal *, integer *), tql2_(integer *
            , integer *, doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     FOR THE REAL SYMMETRIC GENERALIZED EIGENPROBLEM  BAX = (LAMBDA)X. 
*/

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRICES  A  AND  B. */

/*        A  CONTAINS A REAL SYMMETRIC MATRIX. */

/*        B  CONTAINS A POSITIVE DEFINITE REAL SYMMETRIC MATRIX. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    b_dim1 = *nm;
    b_offset = b_dim1 + 1;
    b -= b_offset;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    reduc2_(nm, n, &a[a_offset], &b[b_offset], &fv2[1], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tred1_(nm, n, &a[a_offset], &w[1], &fv1[1], &fv2[1]);
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    tred2_(nm, n, &a[a_offset], &w[1], &fv1[1], &z[z_offset]);
    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    rebakb_(nm, n, &b[b_offset], &fv2[1], n, &z[z_offset]);
L50:
    return 0;
} /* rsgba_ */

/* Subroutine */ int rsm_(integer *nm, integer *n, doublereal *a, doublereal *
        w, integer *m, doublereal *z, doublereal *fwork, integer *iwork, 
        integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int tred1_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *);
    static integer k1, k2, k3, k4, k5, k6, k7, k8;
    extern /* Subroutine */ int trbak1_(integer *, integer *, doublereal *, 
            doublereal *, integer *, doublereal *), tqlrat_(integer *, 
            doublereal *, doublereal *, integer *), imtqlv_(integer *, 
            doublereal *, doublereal *, doublereal *, doublereal *, integer *,
             integer *, doublereal *), tinvit_(integer *, integer *, 
            doublereal *, doublereal *, doublereal *, integer *, doublereal *,
             integer *, doublereal *, integer *, doublereal *, doublereal *, 
            doublereal *, doublereal *, doublereal *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND ALL OF THE EIGENVALUES AND SOME OF THE EIGENVECTORS */
/*     OF A REAL SYMMETRIC MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        A  CONTAINS THE REAL SYMMETRIC MATRIX. */

/*        M  THE EIGENVECTORS CORRESPONDING TO THE FIRST M EIGENVALUES */
/*           ARE TO BE COMPUTED. */
/*           IF M = 0 THEN NO EIGENVECTORS ARE COMPUTED. */
/*           IF M = N THEN ALL OF THE EIGENVECTORS ARE COMPUTED. */

/*     ON OUTPUT */

/*        W  CONTAINS ALL N EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE ORTHONORMAL EIGENVECTORS ASSOCIATED WITH */
/*           THE FIRST M EIGENVALUES. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT, */
/*           IMTQLV AND TINVIT.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FWORK  IS A TEMPORARY STORAGE ARRAY OF DIMENSION 8*N. */

/*        IWORK  IS AN INTEGER TEMPORARY STORAGE ARRAY OF DIMENSION N. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --iwork;
    --w;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --fwork;

    /* Function Body */
    *ierr = *n * 10;
    if (*n > *nm || *m > *nm) {
        goto L50;
    }
    k1 = 1;
    k2 = k1 + *n;
    k3 = k2 + *n;
    k4 = k3 + *n;
    k5 = k4 + *n;
    k6 = k5 + *n;
    k7 = k6 + *n;
    k8 = k7 + *n;
    if (*m > 0) {
        goto L10;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tred1_(nm, n, &a[a_offset], &w[1], &fwork[k1], &fwork[k2]);
    tqlrat_(n, &w[1], &fwork[k2], ierr);
    goto L50;
/*     .......... FIND ALL EIGENVALUES AND M EIGENVECTORS .......... */
L10:
    tred1_(nm, n, &a[a_offset], &fwork[k1], &fwork[k2], &fwork[k3]);
    imtqlv_(n, &fwork[k1], &fwork[k2], &fwork[k3], &w[1], &iwork[1], ierr, &
            fwork[k4]);
    tinvit_(nm, n, &fwork[k1], &fwork[k2], &fwork[k3], m, &w[1], &iwork[1], &
            z[z_offset], ierr, &fwork[k4], &fwork[k5], &fwork[k6], &fwork[k7],
             &fwork[k8]);
    trbak1_(nm, n, &a[a_offset], &fwork[k2], m, &z[z_offset]);
L50:
    return 0;
} /* rsm_ */

/* Subroutine */ int rsp_(integer *nm, integer *n, integer *nv, doublereal *a,
         doublereal *w, integer *matz, doublereal *z, doublereal *fv1, 
        doublereal *fv2, integer *ierr)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2;

    /* Local variables */
    extern /* Subroutine */ int tred3_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *);
    static integer i, j;
    extern /* Subroutine */ int trbak3_(integer *, integer *, integer *, 
            doublereal *, integer *, doublereal *), tqlrat_(integer *, 
            doublereal *, doublereal *, integer *), tql2_(integer *, integer *
            , doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A REAL SYMMETRIC PACKED MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        NV  IS AN INTEGER VARIABLE SET EQUAL TO THE */
/*        DIMENSION OF THE ARRAY  A  AS SPECIFIED FOR */
/*        A  IN THE CALLING PROGRAM.  NV  MUST NOT BE */
/*        LESS THAN  N*(N+1)/2. */

/*        A  CONTAINS THE LOWER TRIANGLE OF THE REAL SYMMETRIC */
/*        PACKED MATRIX STORED ROW-WISE. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR TQLRAT */
/*           AND TQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  AND  FV2  ARE TEMPORARY STORAGE ARRAYS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --fv2;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;
    --a;

    /* Function Body */
    if (*n <= *nm) {
        goto L5;
    }
    *ierr = *n * 10;
    goto L50;
L5:
    if (*nv >= *n * (*n + 1) / 2) {
        goto L10;
    }
    *ierr = *n * 20;
    goto L50;

L10:
    tred3_(n, nv, &a[1], &w[1], &fv1[1], &fv2[1]);
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    tqlrat_(n, &w[1], &fv2[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            z[j + i * z_dim1] = 0.;
/* L30: */
        }

        z[i + i * z_dim1] = 1.;
/* L40: */
    }

    tql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    trbak3_(nm, n, nv, &a[1], n, &z[z_offset]);
L50:
    return 0;
} /* rsp_ */

/* Subroutine */ int rst_(integer *nm, integer *n, doublereal *w, doublereal *
        e, integer *matz, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2;

    /* Local variables */
    static integer i, j;
    extern /* Subroutine */ int imtql1_(integer *, doublereal *, doublereal *,
             integer *), imtql2_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A REAL SYMMETRIC TRIDIAGONAL MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX. */

/*        W  CONTAINS THE DIAGONAL ELEMENTS OF THE REAL */
/*        SYMMETRIC TRIDIAGONAL MATRIX. */

/*        E  CONTAINS THE SUBDIAGONAL ELEMENTS OF THE MATRIX IN */
/*        ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR IMTQL1 */
/*           AND IMTQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e;
    --w;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    imtql1_(n, &w[1], &e[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            z[j + i * z_dim1] = 0.;
/* L30: */
        }

        z[i + i * z_dim1] = 1.;
/* L40: */
    }

    imtql2_(nm, n, &w[1], &e[1], &z[z_offset], ierr);
L50:
    return 0;
} /* rst_ */

/* Subroutine */ int rt_(integer *nm, integer *n, doublereal *a, doublereal *
        w, integer *matz, doublereal *z, doublereal *fv1, integer *ierr)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int figi_(integer *, integer *, doublereal *, 
            doublereal *, doublereal *, doublereal *, integer *), figi2_(
            integer *, integer *, doublereal *, doublereal *, doublereal *, 
            doublereal *, integer *), imtql1_(integer *, doublereal *, 
            doublereal *, integer *), imtql2_(integer *, integer *, 
            doublereal *, doublereal *, doublereal *, integer *);



/*     THIS SUBROUTINE CALLS THE RECOMMENDED SEQUENCE OF */
/*     SUBROUTINES FROM THE EIGENSYSTEM SUBROUTINE PACKAGE (EISPACK) */
/*     TO FIND THE EIGENVALUES AND EIGENVECTORS (IF DESIRED) */
/*     OF A SPECIAL REAL TRIDIAGONAL MATRIX. */

/*     ON INPUT */

/*        NM  MUST BE SET TO THE ROW DIMENSION OF THE TWO-DIMENSIONAL */
/*        ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*        DIMENSION STATEMENT. */

/*        N  IS THE ORDER OF THE MATRIX  A. */

/*        A  CONTAINS THE SPECIAL REAL TRIDIAGONAL MATRIX IN ITS */
/*        FIRST THREE COLUMNS.  THE SUBDIAGONAL ELEMENTS ARE STORED */
/*        IN THE LAST  N-1  POSITIONS OF THE FIRST COLUMN, THE */
/*        DIAGONAL ELEMENTS IN THE SECOND COLUMN, AND THE SUPERDIAGONAL */
/*        ELEMENTS IN THE FIRST  N-1  POSITIONS OF THE THIRD COLUMN. */
/*        ELEMENTS  A(1,1)  AND  A(N,3)  ARE ARBITRARY. */

/*        MATZ  IS AN INTEGER VARIABLE SET EQUAL TO ZERO IF */
/*        ONLY EIGENVALUES ARE DESIRED.  OTHERWISE IT IS SET TO */
/*        ANY NON-ZERO INTEGER FOR BOTH EIGENVALUES AND EIGENVECTORS. */

/*     ON OUTPUT */

/*        W  CONTAINS THE EIGENVALUES IN ASCENDING ORDER. */

/*        Z  CONTAINS THE EIGENVECTORS IF MATZ IS NOT ZERO. */

/*        IERR  IS AN INTEGER OUTPUT VARIABLE SET EQUAL TO AN ERROR */
/*           COMPLETION CODE DESCRIBED IN THE DOCUMENTATION FOR IMTQL1 */
/*           AND IMTQL2.  THE NORMAL COMPLETION CODE IS ZERO. */

/*        FV1  IS A TEMPORARY STORAGE ARRAY. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    --fv1;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;

    /* Function Body */
    if (*n <= *nm) {
        goto L10;
    }
    *ierr = *n * 10;
    goto L50;

L10:
    if (*matz != 0) {
        goto L20;
    }
/*     .......... FIND EIGENVALUES ONLY .......... */
    figi_(nm, n, &a[a_offset], &w[1], &fv1[1], &fv1[1], ierr);
    if (*ierr > 0) {
        goto L50;
    }
    imtql1_(n, &w[1], &fv1[1], ierr);
    goto L50;
/*     .......... FIND BOTH EIGENVALUES AND EIGENVECTORS .......... */
L20:
    figi2_(nm, n, &a[a_offset], &w[1], &fv1[1], &z[z_offset], ierr);
    if (*ierr != 0) {
        goto L50;
    }
    imtql2_(nm, n, &w[1], &fv1[1], &z[z_offset], ierr);
L50:
    return 0;
} /* rt_ */

/* Subroutine */ int svd_(integer *nm, integer *m, integer *n, doublereal *a, 
        doublereal *w, logical *matu, doublereal *u, logical *matv, 
        doublereal *v, integer *ierr, doublereal *rv1)
{
    /* System generated locals */
    integer a_dim1, a_offset, u_dim1, u_offset, v_dim1, v_offset, i_1, i_2, 
            i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal c, f, g, h;
    static integer i, j, k, l;
    static doublereal s, x, y, z, scale;
    static integer i1, k1, l1, ii, kk, ll, mn;
    extern doublereal pythag_(doublereal *, doublereal *);
    static integer its;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE SVD, */
/*     NUM. MATH. 14, 403-420(1970) BY GOLUB AND REINSCH. */
/*     HANDBOOK FOR AUTO. COMP., VOL II-LINEAR ALGEBRA, 134-151(1971). */

/*     THIS SUBROUTINE DETERMINES THE SINGULAR VALUE DECOMPOSITION */
/*          T */
/*     A=USV  OF A REAL M BY N RECTANGULAR MATRIX.  HOUSEHOLDER */
/*     BIDIAGONALIZATION AND A VARIANT OF THE QR ALGORITHM ARE USED. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT.  NOTE THAT NM MUST BE AT LEAST */
/*          AS LARGE AS THE MAXIMUM OF M AND N. */

/*        M IS THE NUMBER OF ROWS OF A (AND U). */

/*        N IS THE NUMBER OF COLUMNS OF A (AND U) AND THE ORDER OF V. */

/*        A CONTAINS THE RECTANGULAR INPUT MATRIX TO BE DECOMPOSED. */

/*        MATU SHOULD BE SET TO .TRUE. IF THE U MATRIX IN THE */
/*          DECOMPOSITION IS DESIRED, AND TO .FALSE. OTHERWISE. */

/*        MATV SHOULD BE SET TO .TRUE. IF THE V MATRIX IN THE */
/*          DECOMPOSITION IS DESIRED, AND TO .FALSE. OTHERWISE. */

/*     ON OUTPUT */

/*        A IS UNALTERED (UNLESS OVERWRITTEN BY U OR V). */

/*        W CONTAINS THE N (NON-NEGATIVE) SINGULAR VALUES OF A (THE */
/*          DIAGONAL ELEMENTS OF S).  THEY ARE UNORDERED.  IF AN */
/*          ERROR EXIT IS MADE, THE SINGULAR VALUES SHOULD BE CORRECT */
/*          FOR INDICES IERR+1,IERR+2,...,N. */

/*        U CONTAINS THE MATRIX U (ORTHOGONAL COLUMN VECTORS) OF THE */
/*          DECOMPOSITION IF MATU HAS BEEN SET TO .TRUE.  OTHERWISE */
/*          U IS USED AS A TEMPORARY ARRAY.  U MAY COINCIDE WITH A. */
/*          IF AN ERROR EXIT IS MADE, THE COLUMNS OF U CORRESPONDING */
/*          TO INDICES OF CORRECT SINGULAR VALUES SHOULD BE CORRECT. */

/*        V CONTAINS THE MATRIX V (ORTHOGONAL) OF THE DECOMPOSITION IF */
/*          MATV HAS BEEN SET TO .TRUE.  OTHERWISE V IS NOT REFERENCED. */
/*          V MAY ALSO COINCIDE WITH A IF U IS NOT NEEDED.  IF AN ERROR */
/*          EXIT IS MADE, THE COLUMNS OF V CORRESPONDING TO INDICES OF */
/*          CORRECT SINGULAR VALUES SHOULD BE CORRECT. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          K          IF THE K-TH SINGULAR VALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*        RV1 IS A TEMPORARY STORAGE ARRAY. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv1;
    v_dim1 = *nm;
    v_offset = v_dim1 + 1;
    v -= v_offset;
    u_dim1 = *nm;
    u_offset = u_dim1 + 1;
    u -= u_offset;
    --w;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *ierr = 0;

    i_1 = *m;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            u[i + j * u_dim1] = a[i + j * a_dim1];
/* L100: */
        }
    }
/*     .......... HOUSEHOLDER REDUCTION TO BIDIAGONAL FORM .......... */
    g = 0.;
    scale = 0.;
    x = 0.;

    i_2 = *n;
    for (i = 1; i <= i_2; ++i) {
        l = i + 1;
        rv1[i] = scale * g;
        g = 0.;
        s = 0.;
        scale = 0.;
        if (i > *m) {
            goto L210;
        }

        i_1 = *m;
        for (k = i; k <= i_1; ++k) {
/* L120: */
            scale += (d_1 = u[k + i * u_dim1], abs(d_1));
        }

        if (scale == 0.) {
            goto L210;
        }

        i_1 = *m;
        for (k = i; k <= i_1; ++k) {
            u[k + i * u_dim1] /= scale;
/* Computing 2nd power */
            d_1 = u[k + i * u_dim1];
            s += d_1 * d_1;
/* L130: */
        }

        f = u[i + i * u_dim1];
        d_1 = sqrt(s);
        g = -d_sign(&d_1, &f);
        h = f * g - s;
        u[i + i * u_dim1] = f - g;
        if (i == *n) {
            goto L190;
        }

        i_1 = *n;
        for (j = l; j <= i_1; ++j) {
            s = 0.;

            i_3 = *m;
            for (k = i; k <= i_3; ++k) {
/* L140: */
                s += u[k + i * u_dim1] * u[k + j * u_dim1];
            }

            f = s / h;

            i_3 = *m;
            for (k = i; k <= i_3; ++k) {
                u[k + j * u_dim1] += f * u[k + i * u_dim1];
/* L150: */
            }
        }

L190:
        i_3 = *m;
        for (k = i; k <= i_3; ++k) {
/* L200: */
            u[k + i * u_dim1] = scale * u[k + i * u_dim1];
        }

L210:
        w[i] = scale * g;
        g = 0.;
        s = 0.;
        scale = 0.;
        if (i > *m || i == *n) {
            goto L290;
        }

        i_3 = *n;
        for (k = l; k <= i_3; ++k) {
/* L220: */
            scale += (d_1 = u[i + k * u_dim1], abs(d_1));
        }

        if (scale == 0.) {
            goto L290;
        }

        i_3 = *n;
        for (k = l; k <= i_3; ++k) {
            u[i + k * u_dim1] /= scale;
/* Computing 2nd power */
            d_1 = u[i + k * u_dim1];
            s += d_1 * d_1;
/* L230: */
        }

        f = u[i + l * u_dim1];
        d_1 = sqrt(s);
        g = -d_sign(&d_1, &f);
        h = f * g - s;
        u[i + l * u_dim1] = f - g;

        i_3 = *n;
        for (k = l; k <= i_3; ++k) {
/* L240: */
            rv1[k] = u[i + k * u_dim1] / h;
        }

        if (i == *m) {
            goto L270;
        }

        i_3 = *m;
        for (j = l; j <= i_3; ++j) {
            s = 0.;

            i_1 = *n;
            for (k = l; k <= i_1; ++k) {
/* L250: */
                s += u[j + k * u_dim1] * u[i + k * u_dim1];
            }

            i_1 = *n;
            for (k = l; k <= i_1; ++k) {
                u[j + k * u_dim1] += s * rv1[k];
/* L260: */
            }
        }

L270:
        i_1 = *n;
        for (k = l; k <= i_1; ++k) {
/* L280: */
            u[i + k * u_dim1] = scale * u[i + k * u_dim1];
        }

L290:
/* Computing MAX */
        d_3 = x, d_4 = (d_1 = w[i], abs(d_1)) + (d_2 = rv1[i], abs(d_2))
                ;
        x = max(d_3,d_4);
/* L300: */
    }
/*     .......... ACCUMULATION OF RIGHT-HAND TRANSFORMATIONS .......... */
    if (! (*matv)) {
        goto L410;
    }
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    i_2 = *n;
    for (ii = 1; ii <= i_2; ++ii) {
        i = *n + 1 - ii;
        if (i == *n) {
            goto L390;
        }
        if (g == 0.) {
            goto L360;
        }

        i_1 = *n;
        for (j = l; j <= i_1; ++j) {
/*     .......... DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
/* L320: */
            v[j + i * v_dim1] = u[i + j * u_dim1] / u[i + l * u_dim1] / g;
        }

        i_1 = *n;
        for (j = l; j <= i_1; ++j) {
            s = 0.;

            i_3 = *n;
            for (k = l; k <= i_3; ++k) {
/* L340: */
                s += u[i + k * u_dim1] * v[k + j * v_dim1];
            }

            i_3 = *n;
            for (k = l; k <= i_3; ++k) {
                v[k + j * v_dim1] += s * v[k + i * v_dim1];
/* L350: */
            }
        }

L360:
        i_3 = *n;
        for (j = l; j <= i_3; ++j) {
            v[i + j * v_dim1] = 0.;
            v[j + i * v_dim1] = 0.;
/* L380: */
        }

L390:
        v[i + i * v_dim1] = 1.;
        g = rv1[i];
        l = i;
/* L400: */
    }
/*     .......... ACCUMULATION OF LEFT-HAND TRANSFORMATIONS .......... */
L410:
    if (! (*matu)) {
        goto L510;
    }
/*     ..........FOR I=MIN(M,N) STEP -1 UNTIL 1 DO -- .......... */
    mn = *n;
    if (*m < *n) {
        mn = *m;
    }

    i_2 = mn;
    for (ii = 1; ii <= i_2; ++ii) {
        i = mn + 1 - ii;
        l = i + 1;
        g = w[i];
        if (i == *n) {
            goto L430;
        }

        i_3 = *n;
        for (j = l; j <= i_3; ++j) {
/* L420: */
            u[i + j * u_dim1] = 0.;
        }

L430:
        if (g == 0.) {
            goto L475;
        }
        if (i == mn) {
            goto L460;
        }

        i_3 = *n;
        for (j = l; j <= i_3; ++j) {
            s = 0.;

            i_1 = *m;
            for (k = l; k <= i_1; ++k) {
/* L440: */
                s += u[k + i * u_dim1] * u[k + j * u_dim1];
            }
/*     .......... DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
            f = s / u[i + i * u_dim1] / g;

            i_1 = *m;
            for (k = i; k <= i_1; ++k) {
                u[k + j * u_dim1] += f * u[k + i * u_dim1];
/* L450: */
            }
        }

L460:
        i_1 = *m;
        for (j = i; j <= i_1; ++j) {
/* L470: */
            u[j + i * u_dim1] /= g;
        }

        goto L490;

L475:
        i_1 = *m;
        for (j = i; j <= i_1; ++j) {
/* L480: */
            u[j + i * u_dim1] = 0.;
        }

L490:
        u[i + i * u_dim1] += 1.;
/* L500: */
    }
/*     .......... DIAGONALIZATION OF THE BIDIAGONAL FORM .......... */
L510:
    tst1 = x;
/*     .......... FOR K=N STEP -1 UNTIL 1 DO -- .......... */
    i_2 = *n;
    for (kk = 1; kk <= i_2; ++kk) {
        k1 = *n - kk;
        k = k1 + 1;
        its = 0;
/*     .......... TEST FOR SPLITTING. */
/*                FOR L=K STEP -1 UNTIL 1 DO -- .......... */
L520:
        i_1 = k;
        for (ll = 1; ll <= i_1; ++ll) {
            l1 = k - ll;
            l = l1 + 1;
            tst2 = tst1 + (d_1 = rv1[l], abs(d_1));
            if (tst2 == tst1) {
                goto L565;
            }
/*     .......... RV1(1) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP .......... */
            tst2 = tst1 + (d_1 = w[l1], abs(d_1));
            if (tst2 == tst1) {
                goto L540;
            }
/* L530: */
        }
/*     .......... CANCELLATION OF RV1(L) IF L GREATER THAN 1 .........
. */
L540:
        c = 0.;
        s = 1.;

        i_1 = k;
        for (i = l; i <= i_1; ++i) {
            f = s * rv1[i];
            rv1[i] = c * rv1[i];
            tst2 = tst1 + abs(f);
            if (tst2 == tst1) {
                goto L565;
            }
            g = w[i];
            h = pythag_(&f, &g);
            w[i] = h;
            c = g / h;
            s = -f / h;
            if (! (*matu)) {
                goto L560;
            }

            i_3 = *m;
            for (j = 1; j <= i_3; ++j) {
                y = u[j + l1 * u_dim1];
                z = u[j + i * u_dim1];
                u[j + l1 * u_dim1] = y * c + z * s;
                u[j + i * u_dim1] = -y * s + z * c;
/* L550: */
            }

L560:
            ;
        }
/*     .......... TEST FOR CONVERGENCE .......... */
L565:
        z = w[k];
        if (l == k) {
            goto L650;
        }
/*     .......... SHIFT FROM BOTTOM 2 BY 2 MINOR .......... */
        if (its == 30) {
            goto L1000;
        }
        ++its;
        x = w[l];
        y = w[k1];
        g = rv1[k1];
        h = rv1[k];
        f = ((g + z) / h * ((g - z) / y) + y / h - h / y) * .5;
        g = pythag_(&f, &c_b141);
        f = x - z / x * z + h / x * (y / (f + d_sign(&g, &f)) - h);
/*     .......... NEXT QR TRANSFORMATION .......... */
        c = 1.;
        s = 1.;

        i_1 = k1;
        for (i1 = l; i1 <= i_1; ++i1) {
            i = i1 + 1;
            g = rv1[i];
            y = w[i];
            h = s * g;
            g = c * g;
            z = pythag_(&f, &h);
            rv1[i1] = z;
            c = f / z;
            s = h / z;
            f = x * c + g * s;
            g = -x * s + g * c;
            h = y * s;
            y *= c;
            if (! (*matv)) {
                goto L575;
            }

            i_3 = *n;
            for (j = 1; j <= i_3; ++j) {
                x = v[j + i1 * v_dim1];
                z = v[j + i * v_dim1];
                v[j + i1 * v_dim1] = x * c + z * s;
                v[j + i * v_dim1] = -x * s + z * c;
/* L570: */
            }

L575:
            z = pythag_(&f, &h);
            w[i1] = z;
/*     .......... ROTATION CAN BE ARBITRARY IF Z IS ZERO .........
. */
            if (z == 0.) {
                goto L580;
            }
            c = f / z;
            s = h / z;
L580:
            f = c * g + s * y;
            x = -s * g + c * y;
            if (! (*matu)) {
                goto L600;
            }

            i_3 = *m;
            for (j = 1; j <= i_3; ++j) {
                y = u[j + i1 * u_dim1];
                z = u[j + i * u_dim1];
                u[j + i1 * u_dim1] = y * c + z * s;
                u[j + i * u_dim1] = -y * s + z * c;
/* L590: */
            }

L600:
            ;
        }

        rv1[l] = 0.;
        rv1[k] = f;
        w[k] = x;
        goto L520;
/*     .......... CONVERGENCE .......... */
L650:
        if (z >= 0.) {
            goto L700;
        }
/*     .......... W(K) IS MADE NON-NEGATIVE .......... */
        w[k] = -z;
        if (! (*matv)) {
            goto L700;
        }

        i_1 = *n;
        for (j = 1; j <= i_1; ++j) {
/* L690: */
            v[j + k * v_dim1] = -v[j + k * v_dim1];
        }

L700:
        ;
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO A */
/*                SINGULAR VALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = k;
L1001:
    return 0;
} /* svd_ */

/* Subroutine */ int tinvit_(integer *nm, integer *n, doublereal *d, 
        doublereal *e, doublereal *e2, integer *m, doublereal *w, integer *
        ind, doublereal *z, integer *ierr, doublereal *rv1, doublereal *rv2, 
        doublereal *rv3, doublereal *rv4, doublereal *rv6)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal norm;
    static integer i, j, p, q, r, s;
    static doublereal u, v, order;
    static integer group;
    static doublereal x0, x1;
    static integer ii, jj, ip;
    static doublereal uk, xu;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer tag, its;
    static doublereal eps2, eps3, eps4;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE INVERSE ITERATION TECH- */
/*     NIQUE IN THE ALGOL PROCEDURE TRISTURM BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 418-439(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVECTORS OF A TRIDIAGONAL */
/*     SYMMETRIC MATRIX CORRESPONDING TO SPECIFIED EIGENVALUES, */
/*     USING INVERSE ITERATION. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E, */
/*          WITH ZEROS CORRESPONDING TO NEGLIGIBLE ELEMENTS OF E. */
/*          E(I) IS CONSIDERED NEGLIGIBLE IF IT IS NOT LARGER THAN */
/*          THE PRODUCT OF THE RELATIVE MACHINE PRECISION AND THE SUM */
/*          OF THE MAGNITUDES OF D(I) AND D(I-1).  E2(1) MUST CONTAIN */
/*          0.0D0 IF THE EIGENVALUES ARE IN ASCENDING ORDER, OR 2.0D0 */
/*          IF THE EIGENVALUES ARE IN DESCENDING ORDER.  IF  BISECT, */
/*          TRIDIB, OR  IMTQLV  HAS BEEN USED TO FIND THE EIGENVALUES, */
/*          THEIR OUTPUT E2 ARRAY IS EXACTLY WHAT IS EXPECTED HERE. */

/*        M IS THE NUMBER OF SPECIFIED EIGENVALUES. */

/*        W CONTAINS THE M EIGENVALUES IN ASCENDING OR DESCENDING ORDER. 
*/

/*        IND CONTAINS IN ITS FIRST M POSITIONS THE SUBMATRIX INDICES */
/*          ASSOCIATED WITH THE CORRESPONDING EIGENVALUES IN W -- */
/*          1 FOR EIGENVALUES BELONGING TO THE FIRST SUBMATRIX FROM */
/*          THE TOP, 2 FOR THOSE BELONGING TO THE SECOND SUBMATRIX, ETC. 
*/

/*     ON OUTPUT */

/*        ALL INPUT ARRAYS ARE UNALTERED. */

/*        Z CONTAINS THE ASSOCIATED SET OF ORTHONORMAL EIGENVECTORS. */
/*          ANY VECTOR WHICH FAILS TO CONVERGE IS SET TO ZERO. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          -R         IF THE EIGENVECTOR CORRESPONDING TO THE R-TH */
/*                     EIGENVALUE FAILS TO CONVERGE IN 5 ITERATIONS. */

/*        RV1, RV2, RV3, RV4, AND RV6 ARE TEMPORARY STORAGE ARRAYS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv6;
    --rv4;
    --rv3;
    --rv2;
    --rv1;
    --e2;
    --e;
    --d;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --ind;
    --w;

    /* Function Body */
    *ierr = 0;
    if (*m == 0) {
        goto L1001;
    }
    tag = 0;
    order = 1. - e2[1];
    q = 0;
/*     .......... ESTABLISH AND PROCESS NEXT SUBMATRIX .......... */
L100:
    p = q + 1;

    i_1 = *n;
    for (q = p; q <= i_1; ++q) {
        if (q == *n) {
            goto L140;
        }
        if (e2[q + 1] == 0.) {
            goto L140;
        }
/* L120: */
    }
/*     .......... FIND VECTORS BY INVERSE ITERATION .......... */
L140:
    ++tag;
    s = 0;

    i_1 = *m;
    for (r = 1; r <= i_1; ++r) {
        if (ind[r] != tag) {
            goto L920;
        }
        its = 1;
        x1 = w[r];
        if (s != 0) {
            goto L510;
        }
/*     .......... CHECK FOR ISOLATED ROOT .......... */
        xu = 1.;
        if (p != q) {
            goto L490;
        }
        rv6[p] = 1.;
        goto L870;
L490:
        norm = (d_1 = d[p], abs(d_1));
        ip = p + 1;

        i_2 = q;
        for (i = ip; i <= i_2; ++i) {
/* L500: */
/* Computing MAX */
            d_3 = norm, d_4 = (d_1 = d[i], abs(d_1)) + (d_2 = e[i], abs(
                    d_2));
            norm = max(d_3,d_4);
        }
/*     .......... EPS2 IS THE CRITERION FOR GROUPING, */
/*                EPS3 REPLACES ZERO PIVOTS AND EQUAL */
/*                ROOTS ARE MODIFIED BY EPS3, */
/*                EPS4 IS TAKEN VERY SMALL TO AVOID OVERFLOW .........
. */
        eps2 = norm * .001;
        eps3 = epslon_(&norm);
        uk = (doublereal) (q - p + 1);
        eps4 = uk * eps3;
        uk = eps4 / sqrt(uk);
        s = p;
L505:
        group = 0;
        goto L520;
/*     .......... LOOK FOR CLOSE OR COINCIDENT ROOTS .......... */
L510:
        if ((d_1 = x1 - x0, abs(d_1)) >= eps2) {
            goto L505;
        }
        ++group;
        if (order * (x1 - x0) <= 0.) {
            x1 = x0 + order * eps3;
        }
/*     .......... ELIMINATION WITH INTERCHANGES AND */
/*                INITIALIZATION OF VECTOR .......... */
L520:
        v = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
            rv6[i] = uk;
            if (i == p) {
                goto L560;
            }
            if ((d_1 = e[i], abs(d_1)) < abs(u)) {
                goto L540;
            }
/*     .......... WARNING -- A DIVIDE CHECK MAY OCCUR HERE IF */
/*                E2 ARRAY HAS NOT BEEN SPECIFIED CORRECTLY ......
.... */
            xu = u / e[i];
            rv4[i] = xu;
            rv1[i - 1] = e[i];
            rv2[i - 1] = d[i] - x1;
            rv3[i - 1] = 0.;
            if (i != q) {
                rv3[i - 1] = e[i + 1];
            }
            u = v - xu * rv2[i - 1];
            v = -xu * rv3[i - 1];
            goto L580;
L540:
            xu = e[i] / u;
            rv4[i] = xu;
            rv1[i - 1] = u;
            rv2[i - 1] = v;
            rv3[i - 1] = 0.;
L560:
            u = d[i] - x1 - xu * v;
            if (i != q) {
                v = e[i + 1];
            }
L580:
            ;
        }

        if (u == 0.) {
            u = eps3;
        }
        rv1[q] = u;
        rv2[q] = 0.;
        rv3[q] = 0.;
/*     .......... BACK SUBSTITUTION */
/*                FOR I=Q STEP -1 UNTIL P DO -- .......... */
L600:
        i_2 = q;
        for (ii = p; ii <= i_2; ++ii) {
            i = p + q - ii;
            rv6[i] = (rv6[i] - u * rv2[i] - v * rv3[i]) / rv1[i];
            v = u;
            u = rv6[i];
/* L620: */
        }
/*     .......... ORTHOGONALIZE WITH RESPECT TO PREVIOUS */
/*                MEMBERS OF GROUP .......... */
        if (group == 0) {
            goto L700;
        }
        j = r;

        i_2 = group;
        for (jj = 1; jj <= i_2; ++jj) {
L630:
            --j;
            if (ind[j] != tag) {
                goto L630;
            }
            xu = 0.;

            i_3 = q;
            for (i = p; i <= i_3; ++i) {
/* L640: */
                xu += rv6[i] * z[i + j * z_dim1];
            }

            i_3 = q;
            for (i = p; i <= i_3; ++i) {
/* L660: */
                rv6[i] -= xu * z[i + j * z_dim1];
            }

/* L680: */
        }

L700:
        norm = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L720: */
            norm += (d_1 = rv6[i], abs(d_1));
        }

        if (norm >= 1.) {
            goto L840;
        }
/*     .......... FORWARD SUBSTITUTION .......... */
        if (its == 5) {
            goto L830;
        }
        if (norm != 0.) {
            goto L740;
        }
        rv6[s] = eps4;
        ++s;
        if (s > q) {
            s = p;
        }
        goto L780;
L740:
        xu = eps4 / norm;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L760: */
            rv6[i] *= xu;
        }
/*     .......... ELIMINATION OPERATIONS ON NEXT VECTOR */
/*                ITERATE .......... */
L780:
        i_2 = q;
        for (i = ip; i <= i_2; ++i) {
            u = rv6[i];
/*     .......... IF RV1(I-1) .EQ. E(I), A ROW INTERCHANGE */
/*                WAS PERFORMED EARLIER IN THE */
/*                TRIANGULARIZATION PROCESS .......... */
            if (rv1[i - 1] != e[i]) {
                goto L800;
            }
            u = rv6[i - 1];
            rv6[i - 1] = rv6[i];
L800:
            rv6[i] = u - rv4[i] * rv6[i - 1];
/* L820: */
        }

        ++its;
        goto L600;
/*     .......... SET ERROR -- NON-CONVERGED EIGENVECTOR .......... */
L830:
        *ierr = -r;
        xu = 0.;
        goto L870;
/*     .......... NORMALIZE SO THAT SUM OF SQUARES IS */
/*                1 AND EXPAND TO FULL ORDER .......... */
L840:
        u = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L860: */
            u = pythag_(&u, &rv6[i]);
        }

        xu = 1. / u;

L870:
        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L880: */
            z[i + r * z_dim1] = 0.;
        }

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L900: */
            z[i + r * z_dim1] = rv6[i] * xu;
        }

        x0 = x1;
L920:
        ;
    }

    if (q < *n) {
        goto L100;
    }
L1001:
    return 0;
} /* tinvit_ */

/* Subroutine */ int tql1_(integer *n, doublereal *d, doublereal *e, integer *
        ierr)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal c, f, g, h;
    static integer i, j, l, m;
    static doublereal p, r, s, c2, c3;
    static integer l1, l2;
    static doublereal s2;
    static integer ii;
    extern doublereal pythag_(doublereal *, doublereal *);
    static doublereal dl1, el1;
    static integer mml;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TQL1, */
/*     NUM. MATH. 11, 293-306(1968) BY BOWDLER, MARTIN, REINSCH, AND */
/*     WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 227-240(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A SYMMETRIC */
/*     TRIDIAGONAL MATRIX BY THE QL METHOD. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*      ON OUTPUT */

/*        D CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT AND */
/*          ORDERED FOR INDICES 1,2,...IERR-1, BUT MAY NOT BE */
/*          THE SMALLEST EIGENVALUES. */

/*        E HAS BEEN DESTROYED. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
/* L100: */
        e[i - 1] = e[i];
    }

    f = 0.;
    tst1 = 0.;
    e[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
        h = (d_1 = d[l], abs(d_1)) + (d_2 = e[l], abs(d_2));
        if (tst1 < h) {
            tst1 = h;
        }
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ELEMENT .......... */
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            tst2 = tst1 + (d_1 = e[m], abs(d_1));
            if (tst2 == tst1) {
                goto L120;
            }
/*     .......... E(N) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP .......... */
/* L110: */
        }

L120:
        if (m == l) {
            goto L210;
        }
L130:
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        l1 = l + 1;
        l2 = l1 + 1;
        g = d[l];
        p = (d[l1] - g) / (e[l] * 2.);
        r = pythag_(&p, &c_b141);
        d[l] = e[l] / (p + d_sign(&r, &p));
        d[l1] = e[l] * (p + d_sign(&r, &p));
        dl1 = d[l1];
        h = g - d[l];
        if (l2 > *n) {
            goto L145;
        }

        i_2 = *n;
        for (i = l2; i <= i_2; ++i) {
/* L140: */
            d[i] -= h;
        }

L145:
        f += h;
/*     .......... QL TRANSFORMATION .......... */
        p = d[m];
        c = 1.;
        c2 = c;
        el1 = e[l1];
        s = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            c3 = c2;
            c2 = c;
            s2 = s;
            i = m - ii;
            g = c * e[i];
            h = c * p;
            r = pythag_(&p, &e[i]);
            e[i + 1] = s * r;
            s = e[i] / r;
            c = p / r;
            p = c * d[i] - s * g;
            d[i + 1] = h + s * (c * g + s * d[i]);
/* L200: */
        }

        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p;
        d[l] = c * p;
        tst2 = tst1 + (d_1 = e[l], abs(d_1));
        if (tst2 > tst1) {
            goto L130;
        }
L210:
        p = d[l] + f;
/*     .......... ORDER EIGENVALUES .......... */
        if (l == 1) {
            goto L250;
        }
/*     .......... FOR I=L STEP -1 UNTIL 2 DO -- .......... */
        i_2 = l;
        for (ii = 2; ii <= i_2; ++ii) {
            i = l + 2 - ii;
            if (p >= d[i - 1]) {
                goto L270;
            }
            d[i] = d[i - 1];
/* L230: */
        }

L250:
        i = 1;
L270:
        d[i] = p;
/* L290: */
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* tql1_ */

/* Subroutine */ int tql2_(integer *nm, integer *n, doublereal *d, doublereal 
        *e, doublereal *z, integer *ierr)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1, d_2;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal c, f, g, h;
    static integer i, j, k, l, m;
    static doublereal p, r, s, c2, c3;
    static integer l1, l2;
    static doublereal s2;
    static integer ii;
    extern doublereal pythag_(doublereal *, doublereal *);
    static doublereal dl1, el1;
    static integer mml;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TQL2, */
/*     NUM. MATH. 11, 293-306(1968) BY BOWDLER, MARTIN, REINSCH, AND */
/*     WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 227-240(1971). */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES AND EIGENVECTORS */
/*     OF A SYMMETRIC TRIDIAGONAL MATRIX BY THE QL METHOD. */
/*     THE EIGENVECTORS OF A FULL SYMMETRIC MATRIX CAN ALSO */
/*     BE FOUND IF  TRED2  HAS BEEN USED TO REDUCE THIS */
/*     FULL MATRIX TO TRIDIAGONAL FORM. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        Z CONTAINS THE TRANSFORMATION MATRIX PRODUCED IN THE */
/*          REDUCTION BY  TRED2, IF PERFORMED.  IF THE EIGENVECTORS */
/*          OF THE TRIDIAGONAL MATRIX ARE DESIRED, Z MUST CONTAIN */
/*          THE IDENTITY MATRIX. */

/*      ON OUTPUT */

/*        D CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT BUT */
/*          UNORDERED FOR INDICES 1,2,...,IERR-1. */

/*        E HAS BEEN DESTROYED. */

/*        Z CONTAINS ORTHONORMAL EIGENVECTORS OF THE SYMMETRIC */
/*          TRIDIAGONAL (OR FULL) MATRIX.  IF AN ERROR EXIT IS MADE, */
/*          Z CONTAINS THE EIGENVECTORS ASSOCIATED WITH THE STORED */
/*          EIGENVALUES. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e;
    --d;

    /* Function Body */
    *ierr = 0;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
/* L100: */
        e[i - 1] = e[i];
    }

    f = 0.;
    tst1 = 0.;
    e[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
        h = (d_1 = d[l], abs(d_1)) + (d_2 = e[l], abs(d_2));
        if (tst1 < h) {
            tst1 = h;
        }
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ELEMENT .......... */
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            tst2 = tst1 + (d_1 = e[m], abs(d_1));
            if (tst2 == tst1) {
                goto L120;
            }
/*     .......... E(N) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP .......... */
/* L110: */
        }

L120:
        if (m == l) {
            goto L220;
        }
L130:
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        l1 = l + 1;
        l2 = l1 + 1;
        g = d[l];
        p = (d[l1] - g) / (e[l] * 2.);
        r = pythag_(&p, &c_b141);
        d[l] = e[l] / (p + d_sign(&r, &p));
        d[l1] = e[l] * (p + d_sign(&r, &p));
        dl1 = d[l1];
        h = g - d[l];
        if (l2 > *n) {
            goto L145;
        }

        i_2 = *n;
        for (i = l2; i <= i_2; ++i) {
/* L140: */
            d[i] -= h;
        }

L145:
        f += h;
/*     .......... QL TRANSFORMATION .......... */
        p = d[m];
        c = 1.;
        c2 = c;
        el1 = e[l1];
        s = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            c3 = c2;
            c2 = c;
            s2 = s;
            i = m - ii;
            g = c * e[i];
            h = c * p;
            r = pythag_(&p, &e[i]);
            e[i + 1] = s * r;
            s = e[i] / r;
            c = p / r;
            p = c * d[i] - s * g;
            d[i + 1] = h + s * (c * g + s * d[i]);
/*     .......... FORM VECTOR .......... */
            i_3 = *n;
            for (k = 1; k <= i_3; ++k) {
                h = z[k + (i + 1) * z_dim1];
                z[k + (i + 1) * z_dim1] = s * z[k + i * z_dim1] + c * h;
                z[k + i * z_dim1] = c * z[k + i * z_dim1] - s * h;
/* L180: */
            }

/* L200: */
        }

        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p;
        d[l] = c * p;
        tst2 = tst1 + (d_1 = e[l], abs(d_1));
        if (tst2 > tst1) {
            goto L130;
        }
L220:
        d[l] += f;
/* L240: */
    }
/*     .......... ORDER EIGENVALUES AND EIGENVECTORS .......... */
    i_1 = *n;
    for (ii = 2; ii <= i_1; ++ii) {
        i = ii - 1;
        k = i;
        p = d[i];

        i_2 = *n;
        for (j = ii; j <= i_2; ++j) {
            if (d[j] >= p) {
                goto L260;
            }
            k = j;
            p = d[j];
L260:
            ;
        }

        if (k == i) {
            goto L300;
        }
        d[k] = d[i];
        d[i] = p;

        i_2 = *n;
        for (j = 1; j <= i_2; ++j) {
            p = z[j + i * z_dim1];
            z[j + i * z_dim1] = z[j + k * z_dim1];
            z[j + k * z_dim1] = p;
/* L280: */
        }

L300:
        ;
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* tql2_ */

/* Subroutine */ int tqlrat_(integer *n, doublereal *d, doublereal *e2, 
        integer *ierr)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal b, c, f, g, h;
    static integer i, j, l, m;
    static doublereal p, r, s, t;
    static integer l1, ii;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer mml;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TQLRAT, */
/*     ALGORITHM 464, COMM. ACM 16, 689(1973) BY REINSCH. */

/*     THIS SUBROUTINE FINDS THE EIGENVALUES OF A SYMMETRIC */
/*     TRIDIAGONAL MATRIX BY THE RATIONAL QL METHOD. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E2 CONTAINS THE SQUARES OF THE SUBDIAGONAL ELEMENTS OF THE */
/*          INPUT MATRIX IN ITS LAST N-1 POSITIONS.  E2(1) IS ARBITRARY. 
*/

/*      ON OUTPUT */

/*        D CONTAINS THE EIGENVALUES IN ASCENDING ORDER.  IF AN */
/*          ERROR EXIT IS MADE, THE EIGENVALUES ARE CORRECT AND */
/*          ORDERED FOR INDICES 1,2,...IERR-1, BUT MAY NOT BE */
/*          THE SMALLEST EIGENVALUES. */

/*        E2 HAS BEEN DESTROYED. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          J          IF THE J-TH EIGENVALUE HAS NOT BEEN */
/*                     DETERMINED AFTER 30 ITERATIONS. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --e2;
    --d;

    /* Function Body */
    *ierr = 0;
    if (*n == 1) {
        goto L1001;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
/* L100: */
        e2[i - 1] = e2[i];
    }

    f = 0.;
    t = 0.;
    e2[*n] = 0.;

    i_1 = *n;
    for (l = 1; l <= i_1; ++l) {
        j = 0;
        h = (d_1 = d[l], abs(d_1)) + sqrt(e2[l]);
        if (t > h) {
            goto L105;
        }
        t = h;
        b = epslon_(&t);
        c = b * b;
/*     .......... LOOK FOR SMALL SQUARED SUB-DIAGONAL ELEMENT ........
.. */
L105:
        i_2 = *n;
        for (m = l; m <= i_2; ++m) {
            if (e2[m] <= c) {
                goto L120;
            }
/*     .......... E2(N) IS ALWAYS ZERO, SO THERE IS NO EXIT */
/*                THROUGH THE BOTTOM OF THE LOOP .......... */
/* L110: */
        }

L120:
        if (m == l) {
            goto L210;
        }
L130:
        if (j == 30) {
            goto L1000;
        }
        ++j;
/*     .......... FORM SHIFT .......... */
        l1 = l + 1;
        s = sqrt(e2[l]);
        g = d[l];
        p = (d[l1] - g) / (s * 2.);
        r = pythag_(&p, &c_b141);
        d[l] = s / (p + d_sign(&r, &p));
        h = g - d[l];

        i_2 = *n;
        for (i = l1; i <= i_2; ++i) {
/* L140: */
            d[i] -= h;
        }

        f += h;
/*     .......... RATIONAL QL TRANSFORMATION .......... */
        g = d[m];
        if (g == 0.) {
            g = b;
        }
        h = g;
        s = 0.;
        mml = m - l;
/*     .......... FOR I=M-1 STEP -1 UNTIL L DO -- .......... */
        i_2 = mml;
        for (ii = 1; ii <= i_2; ++ii) {
            i = m - ii;
            p = g * h;
            r = p + e2[i];
            e2[i + 1] = s * r;
            s = e2[i] / r;
            d[i + 1] = h + s * (h + d[i]);
            g = d[i] - e2[i] / g;
            if (g == 0.) {
                g = b;
            }
            h = g * p / r;
/* L200: */
        }

        e2[l] = s * g;
        d[l] = h;
/*     .......... GUARD AGAINST UNDERFLOW IN CONVERGENCE TEST ........
.. */
        if (h == 0.) {
            goto L210;
        }
        if ((d_1 = e2[l], abs(d_1)) <= (d_2 = c / h, abs(d_2))) {
            goto L210;
        }
        e2[l] = h * e2[l];
        if (e2[l] != 0.) {
            goto L130;
        }
L210:
        p = d[l] + f;
/*     .......... ORDER EIGENVALUES .......... */
        if (l == 1) {
            goto L250;
        }
/*     .......... FOR I=L STEP -1 UNTIL 2 DO -- .......... */
        i_2 = l;
        for (ii = 2; ii <= i_2; ++ii) {
            i = l + 2 - ii;
            if (p >= d[i - 1]) {
                goto L270;
            }
            d[i] = d[i - 1];
/* L230: */
        }

L250:
        i = 1;
L270:
        d[i] = p;
/* L290: */
    }

    goto L1001;
/*     .......... SET ERROR -- NO CONVERGENCE TO AN */
/*                EIGENVALUE AFTER 30 ITERATIONS .......... */
L1000:
    *ierr = l;
L1001:
    return 0;
} /* tqlrat_ */

/* Subroutine */ int trbak1_(integer *nm, integer *n, doublereal *a, 
        doublereal *e, integer *m, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer i, j, k, l;
    static doublereal s;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRBAK1, */
/*     NUM. MATH. 11, 181-195(1968) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A REAL SYMMETRIC */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     SYMMETRIC TRIDIAGONAL MATRIX DETERMINED BY  TRED1. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL TRANS- */
/*          FORMATIONS USED IN THE REDUCTION BY  TRED1 */
/*          IN ITS STRICT LOWER TRIANGLE. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMED EIGENVECTORS */
/*          IN ITS FIRST M COLUMNS. */

/*     NOTE THAT TRBAK1 PRESERVES VECTOR EUCLIDEAN NORMS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --e;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    if (*n == 1) {
        goto L200;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
        l = i - 1;
        if (e[i] == 0.) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            s = 0.;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
/* L110: */
                s += a[i + k * a_dim1] * z[k + j * z_dim1];
            }
/*     .......... DIVISOR BELOW IS NEGATIVE OF H FORMED IN TRED1. 
*/
/*                DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
            s = s / a[i + l * a_dim1] / e[i];

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
/* L120: */
                z[k + j * z_dim1] += s * a[i + k * a_dim1];
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* trbak1_ */

/* Subroutine */ int trbak3_(integer *nm, integer *n, integer */*nv*/, doublereal 
        *a, integer *m, doublereal *z)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2, i_3;

    /* Local variables */
    static doublereal h;
    static integer i, j, k, l;
    static doublereal s;
    static integer ik, iz;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRBAK3, */
/*     NUM. MATH. 11, 181-195(1968) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE FORMS THE EIGENVECTORS OF A REAL SYMMETRIC */
/*     MATRIX BY BACK TRANSFORMING THOSE OF THE CORRESPONDING */
/*     SYMMETRIC TRIDIAGONAL MATRIX DETERMINED BY  TRED3. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        NV MUST BE SET TO THE DIMENSION OF THE ARRAY PARAMETER A */
/*          AS DECLARED IN THE CALLING PROGRAM DIMENSION STATEMENT. */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL TRANSFORMATIONS */
/*          USED IN THE REDUCTION BY  TRED3  IN ITS FIRST */
/*          N*(N+1)/2 POSITIONS. */

/*        M IS THE NUMBER OF EIGENVECTORS TO BE BACK TRANSFORMED. */

/*        Z CONTAINS THE EIGENVECTORS TO BE BACK TRANSFORMED */
/*          IN ITS FIRST M COLUMNS. */

/*     ON OUTPUT */

/*        Z CONTAINS THE TRANSFORMED EIGENVECTORS */
/*          IN ITS FIRST M COLUMNS. */

/*     NOTE THAT TRBAK3 PRESERVES VECTOR EUCLIDEAN NORMS. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --a;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;

    /* Function Body */
    if (*m == 0) {
        goto L200;
    }
    if (*n == 1) {
        goto L200;
    }

    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
        l = i - 1;
        iz = i * l / 2;
        ik = iz + i;
        h = a[ik];
        if (h == 0.) {
            goto L140;
        }

        i_2 = *m;
        for (j = 1; j <= i_2; ++j) {
            s = 0.;
            ik = iz;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                ++ik;
                s += a[ik] * z[k + j * z_dim1];
/* L110: */
            }
/*     .......... DOUBLE DIVISION AVOIDS POSSIBLE UNDERFLOW ......
.... */
            s = s / h / h;
            ik = iz;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                ++ik;
                z[k + j * z_dim1] -= s * a[ik];
/* L120: */
            }

/* L130: */
        }

L140:
        ;
    }

L200:
    return 0;
} /* trbak3_ */

/* Subroutine */ int tred1_(integer *nm, integer *n, doublereal *a, 
        doublereal *d, doublereal *e, doublereal *e2)
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, k, l;
    static doublereal scale;
    static integer ii, jp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRED1, */
/*     NUM. MATH. 11, 181-195(1968) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE REDUCES A REAL SYMMETRIC MATRIX */
/*     TO A SYMMETRIC TRIDIAGONAL MATRIX USING */
/*     ORTHOGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE REAL SYMMETRIC INPUT MATRIX.  ONLY THE */
/*          LOWER TRIANGLE OF THE MATRIX NEED BE SUPPLIED. */

/*     ON OUTPUT */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL TRANS- */
/*          FORMATIONS USED IN THE REDUCTION IN ITS STRICT LOWER */
/*          TRIANGLE.  THE FULL UPPER TRIANGLE OF A IS UNALTERED. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE TRIDIAGONAL MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --e2;
    --e;
    --d;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        d[i] = a[*n + i * a_dim1];
        a[*n + i * a_dim1] = a[i + i * a_dim1];
/* L100: */
    }
/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *n + 1 - ii;
        l = i - 1;
        h = 0.;
        scale = 0.;
        if (l < 1) {
            goto L130;
        }
/*     .......... SCALE ROW (ALGOL TOL THEN NOT NEEDED) .......... */
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
/* L120: */
            scale += (d_1 = d[k], abs(d_1));
        }

        if (scale != 0.) {
            goto L140;
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            d[j] = a[l + j * a_dim1];
            a[l + j * a_dim1] = a[i + j * a_dim1];
            a[i + j * a_dim1] = 0.;
/* L125: */
        }

L130:
        e[i] = 0.;
        e2[i] = 0.;
        goto L300;

L140:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            d[k] /= scale;
            h += d[k] * d[k];
/* L150: */
        }

        e2[i] = scale * scale * h;
        f = d[l];
        d_1 = sqrt(h);
        g = -d_sign(&d_1, &f);
        e[i] = scale * g;
        h -= f * g;
        d[l] = f - g;
        if (l == 1) {
            goto L285;
        }
/*     .......... FORM A*U .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L170: */
            e[j] = 0.;
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            g = e[j] + a[j + j * a_dim1] * f;
            jp1 = j + 1;
            if (l < jp1) {
                goto L220;
            }

            i_3 = l;
            for (k = jp1; k <= i_3; ++k) {
                g += a[k + j * a_dim1] * d[k];
                e[k] += a[k + j * a_dim1] * f;
/* L200: */
            }

L220:
            e[j] = g;
/* L240: */
        }
/*     .......... FORM P .......... */
        f = 0.;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            e[j] /= h;
            f += e[j] * d[j];
/* L245: */
        }

        h = f / (h + h);
/*     .......... FORM Q .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L250: */
            e[j] -= h * d[j];
        }
/*     .......... FORM REDUCED A .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            g = e[j];

            i_3 = l;
            for (k = j; k <= i_3; ++k) {
/* L260: */
                a[k + j * a_dim1] = a[k + j * a_dim1] - f * e[k] - g * d[k];
            }

/* L280: */
        }

L285:
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            d[j] = a[l + j * a_dim1];
            a[l + j * a_dim1] = a[i + j * a_dim1];
            a[i + j * a_dim1] = f * scale;
/* L290: */
        }

L300:
        ;
    }

    return 0;
} /* tred1_ */

/* Subroutine */ int tred2_(integer *nm, integer *n, doublereal *a, 
        doublereal *d, doublereal *e, doublereal *z)
{
    /* System generated locals */
    integer a_dim1, a_offset, z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, k, l;
    static doublereal scale, hh;
    static integer ii, jp1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRED2, */
/*     NUM. MATH. 11, 181-195(1968) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE REDUCES A REAL SYMMETRIC MATRIX TO A */
/*     SYMMETRIC TRIDIAGONAL MATRIX USING AND ACCUMULATING */
/*     ORTHOGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        A CONTAINS THE REAL SYMMETRIC INPUT MATRIX.  ONLY THE */
/*          LOWER TRIANGLE OF THE MATRIX NEED BE SUPPLIED. */

/*     ON OUTPUT */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE TRIDIAGONAL MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        Z CONTAINS THE ORTHOGONAL TRANSFORMATION MATRIX */
/*          PRODUCED IN THE REDUCTION. */

/*        A AND Z MAY COINCIDE.  IF DISTINCT, A IS UNALTERED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --e;
    --d;
    a_dim1 = *nm;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {

        i_2 = *n;
        for (j = i; j <= i_2; ++j) {
/* L80: */
            z[j + i * z_dim1] = a[j + i * a_dim1];
        }

        d[i] = a[*n + i * a_dim1];
/* L100: */
    }

    if (*n == 1) {
        goto L510;
    }
/*     .......... FOR I=N STEP -1 UNTIL 2 DO -- .......... */
    i_1 = *n;
    for (ii = 2; ii <= i_1; ++ii) {
        i = *n + 2 - ii;
        l = i - 1;
        h = 0.;
        scale = 0.;
        if (l < 2) {
            goto L130;
        }
/*     .......... SCALE ROW (ALGOL TOL THEN NOT NEEDED) .......... */
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
/* L120: */
            scale += (d_1 = d[k], abs(d_1));
        }

        if (scale != 0.) {
            goto L140;
        }
L130:
        e[i] = d[l];

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            d[j] = z[l + j * z_dim1];
            z[i + j * z_dim1] = 0.;
            z[j + i * z_dim1] = 0.;
/* L135: */
        }

        goto L290;

L140:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            d[k] /= scale;
            h += d[k] * d[k];
/* L150: */
        }

        f = d[l];
        d_1 = sqrt(h);
        g = -d_sign(&d_1, &f);
        e[i] = scale * g;
        h -= f * g;
        d[l] = f - g;
/*     .......... FORM A*U .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L170: */
            e[j] = 0.;
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            z[j + i * z_dim1] = f;
            g = e[j] + z[j + j * z_dim1] * f;
            jp1 = j + 1;
            if (l < jp1) {
                goto L220;
            }

            i_3 = l;
            for (k = jp1; k <= i_3; ++k) {
                g += z[k + j * z_dim1] * d[k];
                e[k] += z[k + j * z_dim1] * f;
/* L200: */
            }

L220:
            e[j] = g;
/* L240: */
        }
/*     .......... FORM P .......... */
        f = 0.;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            e[j] /= h;
            f += e[j] * d[j];
/* L245: */
        }

        hh = f / (h + h);
/*     .......... FORM Q .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L250: */
            e[j] -= hh * d[j];
        }
/*     .......... FORM REDUCED A .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            g = e[j];

            i_3 = l;
            for (k = j; k <= i_3; ++k) {
/* L260: */
                z[k + j * z_dim1] = z[k + j * z_dim1] - f * e[k] - g * d[k];
            }

            d[j] = z[l + j * z_dim1];
            z[i + j * z_dim1] = 0.;
/* L280: */
        }

L290:
        d[i] = h;
/* L300: */
    }
/*     .......... ACCUMULATION OF TRANSFORMATION MATRICES .......... */
    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
        l = i - 1;
        z[*n + l * z_dim1] = z[l + l * z_dim1];
        z[l + l * z_dim1] = 1.;
        h = d[i];
        if (h == 0.) {
            goto L380;
        }

        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
/* L330: */
            d[k] = z[k + i * z_dim1] / h;
        }

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            g = 0.;

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
/* L340: */
                g += z[k + i * z_dim1] * z[k + j * z_dim1];
            }

            i_3 = l;
            for (k = 1; k <= i_3; ++k) {
                z[k + j * z_dim1] -= g * d[k];
/* L360: */
            }
        }

L380:
        i_3 = l;
        for (k = 1; k <= i_3; ++k) {
/* L400: */
            z[k + i * z_dim1] = 0.;
        }

/* L500: */
    }

L510:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        d[i] = z[*n + i * z_dim1];
        z[*n + i * z_dim1] = 0.;
/* L520: */
    }

    z[*n + *n * z_dim1] = 1.;
    e[1] = 0.;
    return 0;
} /* tred2_ */

/* Subroutine */ int tred3_(integer *n, integer */*nv*/, doublereal *a, 
        doublereal *d, doublereal *e, doublereal *e2)
{
    /* System generated locals */
    integer i_1, i_2, i_3;
    doublereal d_1;

    /* Builtin functions */
    double sqrt(doublereal), d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal f, g, h;
    static integer i, j, k, l;
    static doublereal scale, hh;
    static integer ii, jk, iz, jm1;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRED3, */
/*     NUM. MATH. 11, 181-195(1968) BY MARTIN, REINSCH, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 212-226(1971). */

/*     THIS SUBROUTINE REDUCES A REAL SYMMETRIC MATRIX, STORED AS */
/*     A ONE-DIMENSIONAL ARRAY, TO A SYMMETRIC TRIDIAGONAL MATRIX */
/*     USING ORTHOGONAL SIMILARITY TRANSFORMATIONS. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        NV MUST BE SET TO THE DIMENSION OF THE ARRAY PARAMETER A */
/*          AS DECLARED IN THE CALLING PROGRAM DIMENSION STATEMENT. */

/*        A CONTAINS THE LOWER TRIANGLE OF THE REAL SYMMETRIC */
/*          INPUT MATRIX, STORED ROW-WISE AS A ONE-DIMENSIONAL */
/*          ARRAY, IN ITS FIRST N*(N+1)/2 POSITIONS. */

/*     ON OUTPUT */

/*        A CONTAINS INFORMATION ABOUT THE ORTHOGONAL */
/*          TRANSFORMATIONS USED IN THE REDUCTION. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE TRIDIAGONAL MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE TRIDIAGONAL */
/*          MATRIX IN ITS LAST N-1 POSITIONS.  E(1) IS SET TO ZERO. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2 MAY COINCIDE WITH E IF THE SQUARES ARE NOT NEEDED. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

/*     .......... FOR I=N STEP -1 UNTIL 1 DO -- .......... */
    /* Parameter adjustments */
    --e2;
    --e;
    --d;
    --a;

    /* Function Body */
    i_1 = *n;
    for (ii = 1; ii <= i_1; ++ii) {
        i = *n + 1 - ii;
        l = i - 1;
        iz = i * l / 2;
        h = 0.;
        scale = 0.;
        if (l < 1) {
            goto L130;
        }
/*     .......... SCALE ROW (ALGOL TOL THEN NOT NEEDED) .......... */
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            ++iz;
            d[k] = a[iz];
            scale += (d_1 = d[k], abs(d_1));
/* L120: */
        }

        if (scale != 0.) {
            goto L140;
        }
L130:
        e[i] = 0.;
        e2[i] = 0.;
        goto L290;

L140:
        i_2 = l;
        for (k = 1; k <= i_2; ++k) {
            d[k] /= scale;
            h += d[k] * d[k];
/* L150: */
        }

        e2[i] = scale * scale * h;
        f = d[l];
        d_1 = sqrt(h);
        g = -d_sign(&d_1, &f);
        e[i] = scale * g;
        h -= f * g;
        d[l] = f - g;
        a[iz] = scale * d[l];
        if (l == 1) {
            goto L290;
        }
        jk = 1;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            g = 0.;
            jm1 = j - 1;
            if (jm1 < 1) {
                goto L220;
            }

            i_3 = jm1;
            for (k = 1; k <= i_3; ++k) {
                g += a[jk] * d[k];
                e[k] += a[jk] * f;
                ++jk;
/* L200: */
            }

L220:
            e[j] = g + a[jk] * f;
            ++jk;
/* L240: */
        }
/*     .......... FORM P .......... */
        f = 0.;

        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            e[j] /= h;
            f += e[j] * d[j];
/* L245: */
        }

        hh = f / (h + h);
/*     .......... FORM Q .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
/* L250: */
            e[j] -= hh * d[j];
        }

        jk = 1;
/*     .......... FORM REDUCED A .......... */
        i_2 = l;
        for (j = 1; j <= i_2; ++j) {
            f = d[j];
            g = e[j];

            i_3 = j;
            for (k = 1; k <= i_3; ++k) {
                a[jk] = a[jk] - f * e[k] - g * d[k];
                ++jk;
/* L260: */
            }

/* L280: */
        }

L290:
        d[i] = a[iz + 1];
        a[iz + 1] = scale * sqrt(h);
/* L300: */
    }

    return 0;
} /* tred3_ */

/* Subroutine */ int tridib_(integer *n, doublereal *eps1, doublereal *d, 
        doublereal *e, doublereal *e2, doublereal *lb, doublereal *ub, 
        integer *m11, integer *m, doublereal *w, integer *ind, integer *ierr, 
        doublereal *rv4, doublereal *rv5)
{
    /* System generated locals */
    integer i_1, i_2;
    doublereal d_1, d_2, d_3;

    /* Local variables */
    static integer i, j, k, l, p, q, r, s;
    static doublereal u, v;
    static integer m1, m2;
    static doublereal t1, t2, x0, x1;
    static integer m22, ii;
    static doublereal xu;
    extern doublereal epslon_(doublereal *);
    static integer isturm, tag;
    static doublereal tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE BISECT, */
/*     NUM. MATH. 9, 386-393(1967) BY BARTH, MARTIN, AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 249-256(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVALUES OF A TRIDIAGONAL */
/*     SYMMETRIC MATRIX BETWEEN SPECIFIED BOUNDARY INDICES, */
/*     USING BISECTION. */

/*     ON INPUT */

/*        N IS THE ORDER OF THE MATRIX. */

/*        EPS1 IS AN ABSOLUTE ERROR TOLERANCE FOR THE COMPUTED */
/*          EIGENVALUES.  IF THE INPUT EPS1 IS NON-POSITIVE, */
/*          IT IS RESET FOR EACH SUBMATRIX TO A DEFAULT VALUE, */
/*          NAMELY, MINUS THE PRODUCT OF THE RELATIVE MACHINE */
/*          PRECISION AND THE 1-NORM OF THE SUBMATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2(1) IS ARBITRARY. */

/*        M11 SPECIFIES THE LOWER BOUNDARY INDEX FOR THE DESIRED */
/*          EIGENVALUES. */

/*        M SPECIFIES THE NUMBER OF EIGENVALUES DESIRED.  THE UPPER */
/*          BOUNDARY INDEX M22 IS THEN OBTAINED AS M22=M11+M-1. */

/*     ON OUTPUT */

/*        EPS1 IS UNALTERED UNLESS IT HAS BEEN RESET TO ITS */
/*          (LAST) DEFAULT VALUE. */

/*        D AND E ARE UNALTERED. */

/*        ELEMENTS OF E2, CORRESPONDING TO ELEMENTS OF E REGARDED */
/*          AS NEGLIGIBLE, HAVE BEEN REPLACED BY ZERO CAUSING THE */
/*          MATRIX TO SPLIT INTO A DIRECT SUM OF SUBMATRICES. */
/*          E2(1) IS ALSO SET TO ZERO. */

/*        LB AND UB DEFINE AN INTERVAL CONTAINING EXACTLY THE DESIRED */
/*          EIGENVALUES. */

/*        W CONTAINS, IN ITS FIRST M POSITIONS, THE EIGENVALUES */
/*          BETWEEN INDICES M11 AND M22 IN ASCENDING ORDER. */

/*        IND CONTAINS IN ITS FIRST M POSITIONS THE SUBMATRIX INDICES */
/*          ASSOCIATED WITH THE CORRESPONDING EIGENVALUES IN W -- */
/*          1 FOR EIGENVALUES BELONGING TO THE FIRST SUBMATRIX FROM */
/*          THE TOP, 2 FOR THOSE BELONGING TO THE SECOND SUBMATRIX, ETC.. 
*/

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          3*N+1      IF MULTIPLE EIGENVALUES AT INDEX M11 MAKE */
/*                     UNIQUE SELECTION IMPOSSIBLE, */
/*          3*N+2      IF MULTIPLE EIGENVALUES AT INDEX M22 MAKE */
/*                     UNIQUE SELECTION IMPOSSIBLE. */

/*        RV4 AND RV5 ARE TEMPORARY STORAGE ARRAYS. */

/*     NOTE THAT SUBROUTINE TQL1, IMTQL1, OR TQLRAT IS GENERALLY FASTER */
/*     THAN TRIDIB, IF MORE THAN N/4 EIGENVALUES ARE TO BE FOUND. */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv5;
    --rv4;
    --e2;
    --e;
    --d;
    --ind;
    --w;

    /* Function Body */
    *ierr = 0;
    tag = 0;
    xu = d[1];
    x0 = d[1];
    u = 0.;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ENTRIES AND DETERMINE AN */
/*                INTERVAL CONTAINING ALL THE EIGENVALUES .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        x1 = u;
        u = 0.;
        if (i != *n) {
            u = (d_1 = e[i + 1], abs(d_1));
        }
/* Computing MIN */
        d_1 = d[i] - (x1 + u);
        xu = min(d_1,xu);
/* Computing MAX */
        d_1 = d[i] + (x1 + u);
        x0 = max(d_1,x0);
        if (i == 1) {
            goto L20;
        }
        tst1 = (d_1 = d[i], abs(d_1)) + (d_2 = d[i - 1], abs(d_2));
        tst2 = tst1 + (d_1 = e[i], abs(d_1));
        if (tst2 > tst1) {
            goto L40;
        }
L20:
        e2[i] = 0.;
L40:
        ;
    }

    x1 = (doublereal) (*n);
/* Computing MAX */
    d_2 = abs(xu), d_3 = abs(x0);
    d_1 = max(d_2,d_3);
    x1 *= epslon_(&d_1);
    xu -= x1;
    t1 = xu;
    x0 += x1;
    t2 = x0;
/*     .......... DETERMINE AN INTERVAL CONTAINING EXACTLY */
/*                THE DESIRED EIGENVALUES .......... */
    p = 1;
    q = *n;
    m1 = *m11 - 1;
    if (m1 == 0) {
        goto L75;
    }
    isturm = 1;
L50:
    v = x1;
    x1 = xu + (x0 - xu) * .5;
    if (x1 == v) {
        goto L980;
    }
    goto L320;
L60:
    if ((i_1 = s - m1) < 0) {
        goto L65;
    } else if (i_1 == 0) {
        goto L73;
    } else {
        goto L70;
    }
L65:
    xu = x1;
    goto L50;
L70:
    x0 = x1;
    goto L50;
L73:
    xu = x1;
    t1 = x1;
L75:
    m22 = m1 + *m;
    if (m22 == *n) {
        goto L90;
    }
    x0 = t2;
    isturm = 2;
    goto L50;
L80:
    if ((i_1 = s - m22) < 0) {
        goto L65;
    } else if (i_1 == 0) {
        goto L85;
    } else {
        goto L70;
    }
L85:
    t2 = x1;
L90:
    q = 0;
    r = 0;
/*     .......... ESTABLISH AND PROCESS NEXT SUBMATRIX, REFINING */
/*                INTERVAL BY THE GERSCHGORIN BOUNDS .......... */
L100:
    if (r == *m) {
        goto L1001;
    }
    ++tag;
    p = q + 1;
    xu = d[p];
    x0 = d[p];
    u = 0.;

    i_1 = *n;
    for (q = p; q <= i_1; ++q) {
        x1 = u;
        u = 0.;
        v = 0.;
        if (q == *n) {
            goto L110;
        }
        u = (d_1 = e[q + 1], abs(d_1));
        v = e2[q + 1];
L110:
/* Computing MIN */
        d_1 = d[q] - (x1 + u);
        xu = min(d_1,xu);
/* Computing MAX */
        d_1 = d[q] + (x1 + u);
        x0 = max(d_1,x0);
        if (v == 0.) {
            goto L140;
        }
/* L120: */
    }

L140:
/* Computing MAX */
    d_2 = abs(xu), d_3 = abs(x0);
    d_1 = max(d_2,d_3);
    x1 = epslon_(&d_1);
    if (*eps1 <= 0.) {
        *eps1 = -x1;
    }
    if (p != q) {
        goto L180;
    }
/*     .......... CHECK FOR ISOLATED ROOT WITHIN INTERVAL .......... */
    if (t1 > d[p] || d[p] >= t2) {
        goto L940;
    }
    m1 = p;
    m2 = p;
    rv5[p] = d[p];
    goto L900;
L180:
    x1 *= q - p + 1;
/* Computing MAX */
    d_1 = t1, d_2 = xu - x1;
    *lb = max(d_1,d_2);
/* Computing MIN */
    d_1 = t2, d_2 = x0 + x1;
    *ub = min(d_1,d_2);
    x1 = *lb;
    isturm = 3;
    goto L320;
L200:
    m1 = s + 1;
    x1 = *ub;
    isturm = 4;
    goto L320;
L220:
    m2 = s;
    if (m1 > m2) {
        goto L940;
    }
/*     .......... FIND ROOTS BY BISECTION .......... */
    x0 = *ub;
    isturm = 5;

    i_1 = m2;
    for (i = m1; i <= i_1; ++i) {
        rv5[i] = *ub;
        rv4[i] = *lb;
/* L240: */
    }
/*     .......... LOOP FOR K-TH EIGENVALUE */
/*                FOR K=M2 STEP -1 UNTIL M1 DO -- */
/*                (-DO- NOT USED TO LEGALIZE -COMPUTED GO TO-) .......... 
*/
    k = m2;
L250:
    xu = *lb;
/*     .......... FOR I=K STEP -1 UNTIL M1 DO -- .......... */
    i_1 = k;
    for (ii = m1; ii <= i_1; ++ii) {
        i = m1 + k - ii;
        if (xu >= rv4[i]) {
            goto L260;
        }
        xu = rv4[i];
        goto L280;
L260:
        ;
    }

L280:
    if (x0 > rv5[k]) {
        x0 = rv5[k];
    }
/*     .......... NEXT BISECTION STEP .......... */
L300:
    x1 = (xu + x0) * .5;
    if (x0 - xu <= abs(*eps1)) {
        goto L420;
    }
    tst1 = (abs(xu) + abs(x0)) * 2.;
    tst2 = tst1 + (x0 - xu);
    if (tst2 == tst1) {
        goto L420;
    }
/*     .......... IN-LINE PROCEDURE FOR STURM SEQUENCE .......... */
L320:
    s = p - 1;
    u = 1.;

    i_1 = q;
    for (i = p; i <= i_1; ++i) {
        if (u != 0.) {
            goto L325;
        }
        v = (d_1 = e[i], abs(d_1)) / epslon_(&c_b141);
        if (e2[i] == 0.) {
            v = 0.;
        }
        goto L330;
L325:
        v = e2[i] / u;
L330:
        u = d[i] - x1 - v;
        if (u < 0.) {
            ++s;
        }
/* L340: */
    }

    switch (isturm) {
        case 1:  goto L60;
        case 2:  goto L80;
        case 3:  goto L200;
        case 4:  goto L220;
        case 5:  goto L360;
    }
/*     .......... REFINE INTERVALS .......... */
L360:
    if (s >= k) {
        goto L400;
    }
    xu = x1;
    if (s >= m1) {
        goto L380;
    }
    rv4[m1] = x1;
    goto L300;
L380:
    rv4[s + 1] = x1;
    if (rv5[s] > x1) {
        rv5[s] = x1;
    }
    goto L300;
L400:
    x0 = x1;
    goto L300;
/*     .......... K-TH EIGENVALUE FOUND .......... */
L420:
    rv5[k] = x1;
    --k;
    if (k >= m1) {
        goto L250;
    }
/*     .......... ORDER EIGENVALUES TAGGED WITH THEIR */
/*                SUBMATRIX ASSOCIATIONS .......... */
L900:
    s = r;
    r = r + m2 - m1 + 1;
    j = 1;
    k = m1;

    i_1 = r;
    for (l = 1; l <= i_1; ++l) {
        if (j > s) {
            goto L910;
        }
        if (k > m2) {
            goto L940;
        }
        if (rv5[k] >= w[l]) {
            goto L915;
        }

        i_2 = s;
        for (ii = j; ii <= i_2; ++ii) {
            i = l + s - ii;
            w[i + 1] = w[i];
            ind[i + 1] = ind[i];
/* L905: */
        }

L910:
        w[l] = rv5[k];
        ind[l] = tag;
        ++k;
        goto L920;
L915:
        ++j;
L920:
        ;
    }

L940:
    if (q < *n) {
        goto L100;
    }
    goto L1001;
/*     .......... SET ERROR -- INTERVAL CANNOT BE FOUND CONTAINING */
/*                EXACTLY THE DESIRED EIGENVALUES .......... */
L980:
    *ierr = *n * 3 + isturm;
L1001:
    *lb = t1;
    *ub = t2;
    return 0;
} /* tridib_ */

/* Subroutine */ int tsturm_(integer *nm, integer *n, doublereal *eps1, 
        doublereal *d, doublereal *e, doublereal *e2, doublereal *lb, 
        doublereal *ub, integer *mm, integer *m, doublereal *w, doublereal *z,
         integer *ierr, doublereal *rv1, doublereal *rv2, doublereal *rv3, 
        doublereal *rv4, doublereal *rv5, doublereal *rv6)
{
    /* System generated locals */
    integer z_dim1, z_offset, i_1, i_2, i_3;
    doublereal d_1, d_2, d_3, d_4;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal norm;
    static integer i, j, k, p, q, r, s;
    static doublereal u, v;
    static integer group, m1, m2;
    static doublereal t1, t2, x0, x1;
    static integer ii, jj, ip;
    static doublereal uk, xu;
    extern doublereal pythag_(doublereal *, doublereal *), epslon_(doublereal 
            *);
    static integer isturm, its;
    static doublereal eps2, eps3, eps4, tst1, tst2;



/*     THIS SUBROUTINE IS A TRANSLATION OF THE ALGOL PROCEDURE TRISTURM */
/*     BY PETERS AND WILKINSON. */
/*     HANDBOOK FOR AUTO. COMP., VOL.II-LINEAR ALGEBRA, 418-439(1971). */

/*     THIS SUBROUTINE FINDS THOSE EIGENVALUES OF A TRIDIAGONAL */
/*     SYMMETRIC MATRIX WHICH LIE IN A SPECIFIED INTERVAL AND THEIR */
/*     ASSOCIATED EIGENVECTORS, USING BISECTION AND INVERSE ITERATION. */

/*     ON INPUT */

/*        NM MUST BE SET TO THE ROW DIMENSION OF TWO-DIMENSIONAL */
/*          ARRAY PARAMETERS AS DECLARED IN THE CALLING PROGRAM */
/*          DIMENSION STATEMENT. */

/*        N IS THE ORDER OF THE MATRIX. */

/*        EPS1 IS AN ABSOLUTE ERROR TOLERANCE FOR THE COMPUTED */
/*          EIGENVALUES.  IT SHOULD BE CHOSEN COMMENSURATE WITH */
/*          RELATIVE PERTURBATIONS IN THE MATRIX ELEMENTS OF THE */
/*          ORDER OF THE RELATIVE MACHINE PRECISION.  IF THE */
/*          INPUT EPS1 IS NON-POSITIVE, IT IS RESET FOR EACH */
/*          SUBMATRIX TO A DEFAULT VALUE, NAMELY, MINUS THE */
/*          PRODUCT OF THE RELATIVE MACHINE PRECISION AND THE */
/*          1-NORM OF THE SUBMATRIX. */

/*        D CONTAINS THE DIAGONAL ELEMENTS OF THE INPUT MATRIX. */

/*        E CONTAINS THE SUBDIAGONAL ELEMENTS OF THE INPUT MATRIX */
/*          IN ITS LAST N-1 POSITIONS.  E(1) IS ARBITRARY. */

/*        E2 CONTAINS THE SQUARES OF THE CORRESPONDING ELEMENTS OF E. */
/*          E2(1) IS ARBITRARY. */

/*        LB AND UB DEFINE THE INTERVAL TO BE SEARCHED FOR EIGENVALUES. */
/*          IF LB IS NOT LESS THAN UB, NO EIGENVALUES WILL BE FOUND. */

/*        MM SHOULD BE SET TO AN UPPER BOUND FOR THE NUMBER OF */
/*          EIGENVALUES IN THE INTERVAL.  WARNING. IF MORE THAN */
/*          MM EIGENVALUES ARE DETERMINED TO LIE IN THE INTERVAL, */
/*          AN ERROR RETURN IS MADE WITH NO VALUES OR VECTORS FOUND. */

/*     ON OUTPUT */

/*        EPS1 IS UNALTERED UNLESS IT HAS BEEN RESET TO ITS */
/*          (LAST) DEFAULT VALUE. */

/*        D AND E ARE UNALTERED. */

/*        ELEMENTS OF E2, CORRESPONDING TO ELEMENTS OF E REGARDED */
/*          AS NEGLIGIBLE, HAVE BEEN REPLACED BY ZERO CAUSING THE */
/*          MATRIX TO SPLIT INTO A DIRECT SUM OF SUBMATRICES. */
/*          E2(1) IS ALSO SET TO ZERO. */

/*        M IS THE NUMBER OF EIGENVALUES DETERMINED TO LIE IN (LB,UB). */

/*        W CONTAINS THE M EIGENVALUES IN ASCENDING ORDER IF THE MATRIX */
/*          DOES NOT SPLIT.  IF THE MATRIX SPLITS, THE EIGENVALUES ARE */
/*          IN ASCENDING ORDER FOR EACH SUBMATRIX.  IF A VECTOR ERROR */
/*          EXIT IS MADE, W CONTAINS THOSE VALUES ALREADY FOUND. */

/*        Z CONTAINS THE ASSOCIATED SET OF ORTHONORMAL EIGENVECTORS. */
/*          IF AN ERROR EXIT IS MADE, Z CONTAINS THOSE VECTORS */
/*          ALREADY FOUND. */

/*        IERR IS SET TO */
/*          ZERO       FOR NORMAL RETURN, */
/*          3*N+1      IF M EXCEEDS MM. */
/*          4*N+R      IF THE EIGENVECTOR CORRESPONDING TO THE R-TH */
/*                     EIGENVALUE FAILS TO CONVERGE IN 5 ITERATIONS. */

/*        RV1, RV2, RV3, RV4, RV5, AND RV6 ARE TEMPORARY STORAGE ARRAYS. 
*/

/*     THE ALGOL PROCEDURE STURMCNT CONTAINED IN TRISTURM */
/*     APPEARS IN TSTURM IN-LINE. */

/*     CALLS PYTHAG FOR  DSQRT(A*A + B*B) . */

/*     QUESTIONS AND COMMENTS SHOULD BE DIRECTED TO BURTON S. GARBOW, */
/*     MATHEMATICS AND COMPUTER SCIENCE DIV, ARGONNE NATIONAL LABORATORY 
*/

/*     THIS VERSION DATED AUGUST 1983. */

/*     ------------------------------------------------------------------ 
*/

    /* Parameter adjustments */
    --rv6;
    --rv5;
    --rv4;
    --rv3;
    --rv2;
    --rv1;
    --e2;
    --e;
    --d;
    z_dim1 = *nm;
    z_offset = z_dim1 + 1;
    z -= z_offset;
    --w;

    /* Function Body */
    *ierr = 0;
    t1 = *lb;
    t2 = *ub;
/*     .......... LOOK FOR SMALL SUB-DIAGONAL ENTRIES .......... */
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
        if (i == 1) {
            goto L20;
        }
        tst1 = (d_1 = d[i], abs(d_1)) + (d_2 = d[i - 1], abs(d_2));
        tst2 = tst1 + (d_1 = e[i], abs(d_1));
        if (tst2 > tst1) {
            goto L40;
        }
L20:
        e2[i] = 0.;
L40:
        ;
    }
/*     .......... DETERMINE THE NUMBER OF EIGENVALUES */
/*                IN THE INTERVAL .......... */
    p = 1;
    q = *n;
    x1 = *ub;
    isturm = 1;
    goto L320;
L60:
    *m = s;
    x1 = *lb;
    isturm = 2;
    goto L320;
L80:
    *m -= s;
    if (*m > *mm) {
        goto L980;
    }
    q = 0;
    r = 0;
/*     .......... ESTABLISH AND PROCESS NEXT SUBMATRIX, REFINING */
/*                INTERVAL BY THE GERSCHGORIN BOUNDS .......... */
L100:
    if (r == *m) {
        goto L1001;
    }
    p = q + 1;
    xu = d[p];
    x0 = d[p];
    u = 0.;

    i_1 = *n;
    for (q = p; q <= i_1; ++q) {
        x1 = u;
        u = 0.;
        v = 0.;
        if (q == *n) {
            goto L110;
        }
        u = (d_1 = e[q + 1], abs(d_1));
        v = e2[q + 1];
L110:
/* Computing MIN */
        d_1 = d[q] - (x1 + u);
        xu = min(d_1,xu);
/* Computing MAX */
        d_1 = d[q] + (x1 + u);
        x0 = max(d_1,x0);
        if (v == 0.) {
            goto L140;
        }
/* L120: */
    }

L140:
/* Computing MAX */
    d_2 = abs(xu), d_3 = abs(x0);
    d_1 = max(d_2,d_3);
    x1 = epslon_(&d_1);
    if (*eps1 <= 0.) {
        *eps1 = -x1;
    }
    if (p != q) {
        goto L180;
    }
/*     .......... CHECK FOR ISOLATED ROOT WITHIN INTERVAL .......... */
    if (t1 > d[p] || d[p] >= t2) {
        goto L940;
    }
    ++r;

    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L160: */
        z[i + r * z_dim1] = 0.;
    }

    w[r] = d[p];
    z[p + r * z_dim1] = 1.;
    goto L940;
L180:
    u = (doublereal) (q - p + 1);
    x1 = u * x1;
/* Computing MAX */
    d_1 = t1, d_2 = xu - x1;
    *lb = max(d_1,d_2);
/* Computing MIN */
    d_1 = t2, d_2 = x0 + x1;
    *ub = min(d_1,d_2);
    x1 = *lb;
    isturm = 3;
    goto L320;
L200:
    m1 = s + 1;
    x1 = *ub;
    isturm = 4;
    goto L320;
L220:
    m2 = s;
    if (m1 > m2) {
        goto L940;
    }
/*     .......... FIND ROOTS BY BISECTION .......... */
    x0 = *ub;
    isturm = 5;

    i_1 = m2;
    for (i = m1; i <= i_1; ++i) {
        rv5[i] = *ub;
        rv4[i] = *lb;
/* L240: */
    }
/*     .......... LOOP FOR K-TH EIGENVALUE */
/*                FOR K=M2 STEP -1 UNTIL M1 DO -- */
/*                (-DO- NOT USED TO LEGALIZE -COMPUTED GO TO-) .......... 
*/
    k = m2;
L250:
    xu = *lb;
/*     .......... FOR I=K STEP -1 UNTIL M1 DO -- .......... */
    i_1 = k;
    for (ii = m1; ii <= i_1; ++ii) {
        i = m1 + k - ii;
        if (xu >= rv4[i]) {
            goto L260;
        }
        xu = rv4[i];
        goto L280;
L260:
        ;
    }

L280:
    if (x0 > rv5[k]) {
        x0 = rv5[k];
    }
/*     .......... NEXT BISECTION STEP .......... */
L300:
    x1 = (xu + x0) * .5;
    if (x0 - xu <= abs(*eps1)) {
        goto L420;
    }
    tst1 = (abs(xu) + abs(x0)) * 2.;
    tst2 = tst1 + (x0 - xu);
    if (tst2 == tst1) {
        goto L420;
    }
/*     .......... IN-LINE PROCEDURE FOR STURM SEQUENCE .......... */
L320:
    s = p - 1;
    u = 1.;

    i_1 = q;
    for (i = p; i <= i_1; ++i) {
        if (u != 0.) {
            goto L325;
        }
        v = (d_1 = e[i], abs(d_1)) / epslon_(&c_b141);
        if (e2[i] == 0.) {
            v = 0.;
        }
        goto L330;
L325:
        v = e2[i] / u;
L330:
        u = d[i] - x1 - v;
        if (u < 0.) {
            ++s;
        }
/* L340: */
    }

    switch (isturm) {
        case 1:  goto L60;
        case 2:  goto L80;
        case 3:  goto L200;
        case 4:  goto L220;
        case 5:  goto L360;
    }
/*     .......... REFINE INTERVALS .......... */
L360:
    if (s >= k) {
        goto L400;
    }
    xu = x1;
    if (s >= m1) {
        goto L380;
    }
    rv4[m1] = x1;
    goto L300;
L380:
    rv4[s + 1] = x1;
    if (rv5[s] > x1) {
        rv5[s] = x1;
    }
    goto L300;
L400:
    x0 = x1;
    goto L300;
/*     .......... K-TH EIGENVALUE FOUND .......... */
L420:
    rv5[k] = x1;
    --k;
    if (k >= m1) {
        goto L250;
    }
/*     .......... FIND VECTORS BY INVERSE ITERATION .......... */
    norm = (d_1 = d[p], abs(d_1));
    ip = p + 1;

    i_1 = q;
    for (i = ip; i <= i_1; ++i) {
/* L500: */
/* Computing MAX */
        d_3 = norm, d_4 = (d_1 = d[i], abs(d_1)) + (d_2 = e[i], abs(d_2)
                );
        norm = max(d_3,d_4);
    }
/*     .......... EPS2 IS THE CRITERION FOR GROUPING, */
/*                EPS3 REPLACES ZERO PIVOTS AND EQUAL */
/*                ROOTS ARE MODIFIED BY EPS3, */
/*                EPS4 IS TAKEN VERY SMALL TO AVOID OVERFLOW .......... */
    eps2 = norm * .001;
    eps3 = epslon_(&norm);
    uk = (doublereal) (q - p + 1);
    eps4 = uk * eps3;
    uk = eps4 / sqrt(uk);
    group = 0;
    s = p;

    i_1 = m2;
    for (k = m1; k <= i_1; ++k) {
        ++r;
        its = 1;
        w[r] = rv5[k];
        x1 = rv5[k];
/*     .......... LOOK FOR CLOSE OR COINCIDENT ROOTS .......... */
        if (k == m1) {
            goto L520;
        }
        if (x1 - x0 >= eps2) {
            group = -1;
        }
        ++group;
        if (x1 <= x0) {
            x1 = x0 + eps3;
        }
/*     .......... ELIMINATION WITH INTERCHANGES AND */
/*                INITIALIZATION OF VECTOR .......... */
L520:
        v = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
            rv6[i] = uk;
            if (i == p) {
                goto L560;
            }
            if ((d_1 = e[i], abs(d_1)) < abs(u)) {
                goto L540;
            }
            xu = u / e[i];
            rv4[i] = xu;
            rv1[i - 1] = e[i];
            rv2[i - 1] = d[i] - x1;
            rv3[i - 1] = 0.;
            if (i != q) {
                rv3[i - 1] = e[i + 1];
            }
            u = v - xu * rv2[i - 1];
            v = -xu * rv3[i - 1];
            goto L580;
L540:
            xu = e[i] / u;
            rv4[i] = xu;
            rv1[i - 1] = u;
            rv2[i - 1] = v;
            rv3[i - 1] = 0.;
L560:
            u = d[i] - x1 - xu * v;
            if (i != q) {
                v = e[i + 1];
            }
L580:
            ;
        }

        if (u == 0.) {
            u = eps3;
        }
        rv1[q] = u;
        rv2[q] = 0.;
        rv3[q] = 0.;
/*     .......... BACK SUBSTITUTION */
/*                FOR I=Q STEP -1 UNTIL P DO -- .......... */
L600:
        i_2 = q;
        for (ii = p; ii <= i_2; ++ii) {
            i = p + q - ii;
            rv6[i] = (rv6[i] - u * rv2[i] - v * rv3[i]) / rv1[i];
            v = u;
            u = rv6[i];
/* L620: */
        }
/*     .......... ORTHOGONALIZE WITH RESPECT TO PREVIOUS */
/*                MEMBERS OF GROUP .......... */
        if (group == 0) {
            goto L700;
        }

        i_2 = group;
        for (jj = 1; jj <= i_2; ++jj) {
            j = r - group - 1 + jj;
            xu = 0.;

            i_3 = q;
            for (i = p; i <= i_3; ++i) {
/* L640: */
                xu += rv6[i] * z[i + j * z_dim1];
            }

            i_3 = q;
            for (i = p; i <= i_3; ++i) {
/* L660: */
                rv6[i] -= xu * z[i + j * z_dim1];
            }

/* L680: */
        }

L700:
        norm = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L720: */
            norm += (d_1 = rv6[i], abs(d_1));
        }

        if (norm >= 1.) {
            goto L840;
        }
/*     .......... FORWARD SUBSTITUTION .......... */
        if (its == 5) {
            goto L960;
        }
        if (norm != 0.) {
            goto L740;
        }
        rv6[s] = eps4;
        ++s;
        if (s > q) {
            s = p;
        }
        goto L780;
L740:
        xu = eps4 / norm;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L760: */
            rv6[i] *= xu;
        }
/*     .......... ELIMINATION OPERATIONS ON NEXT VECTOR */
/*                ITERATE .......... */
L780:
        i_2 = q;
        for (i = ip; i <= i_2; ++i) {
            u = rv6[i];
/*     .......... IF RV1(I-1) .EQ. E(I), A ROW INTERCHANGE */
/*                WAS PERFORMED EARLIER IN THE */
/*                TRIANGULARIZATION PROCESS .......... */
            if (rv1[i - 1] != e[i]) {
                goto L800;
            }
            u = rv6[i - 1];
            rv6[i - 1] = rv6[i];
L800:
            rv6[i] = u - rv4[i] * rv6[i - 1];
/* L820: */
        }

        ++its;
        goto L600;
/*     .......... NORMALIZE SO THAT SUM OF SQUARES IS */
/*                1 AND EXPAND TO FULL ORDER .......... */
L840:
        u = 0.;

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L860: */
            u = pythag_(&u, &rv6[i]);
        }

        xu = 1. / u;

        i_2 = *n;
        for (i = 1; i <= i_2; ++i) {
/* L880: */
            z[i + r * z_dim1] = 0.;
        }

        i_2 = q;
        for (i = p; i <= i_2; ++i) {
/* L900: */
            z[i + r * z_dim1] = rv6[i] * xu;
        }

        x0 = x1;
/* L920: */
    }

L940:
    if (q < *n) {
        goto L100;
    }
    goto L1001;
/*     .......... SET ERROR -- NON-CONVERGED EIGENVECTOR .......... */
L960:
    *ierr = (*n << 2) + r;
    goto L1001;
/*     .......... SET ERROR -- UNDERESTIMATE OF NUMBER OF */
/*                EIGENVALUES IN INTERVAL .......... */
L980:
    *ierr = *n * 3 + 1;
L1001:
    *lb = t1;
    *ub = t2;
    return 0;
} /* tsturm_ */

#ifdef __cplusplus
        }
#endif
