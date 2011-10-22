/*
 * tranprob.c   Adachi, J.   1995.10.19
 * Copyright (C) 1992-1995 J. Adachi & M. Hasegawa; All rights reserved.
 */

#define SH 1 /* suggested by K.Strimmer & A.v.Haeseler */

#define TPMWRITE 0

#include "protml.h"


void 
elmhes(a, ordr, n)
	dmattpmty a;
	int *ordr;
	int n;
{
	int m, j, i;
	double y, x;

	for (i = 0; i < n; i++)
		ordr[i] = 0;
	for (m = 2; m < n; m++) {
		x = 0.0;
		i = m;
		for (j = m; j <= n; j++) {
			if (fabs(a[j - 1][m - 2]) > fabs(x)) {
				x = a[j - 1][m - 2];
				i = j;
			}
		}
		ordr[m - 1] = i;      /* vector */
		if (i != m) {
			for (j = m - 2; j < n; j++) {
				y = a[i - 1][j];
				a[i - 1][j] = a[m - 1][j];
				a[m - 1][j] = y;
			}
			for (j = 0; j < n; j++) {
				y = a[j][i - 1];
				a[j][i - 1] = a[j][m - 1];
				a[j][m - 1] = y;
			}
		}
		if (x != 0.0) {
			for (i = m; i < n; i++) {
				y = a[i][m - 2];
				if (y != 0.0) {
					y /= x;
					a[i][m - 2] = y;
					for (j = m - 1; j < n; j++)
						a[i][j] -= y * a[m - 1][j];
					for (j = 0; j < n; j++)
						a[j][m - 1] += y * a[j][i];
				}
			}
		}
	}
} /*_ elmhes */


void 
eltran(a, zz, ordr, n)
	dmattpmty a, zz;
	int *ordr;
	int n;
{
	int i, j, m;

	for (i = 0; i < n; i++) {
		for (j = i + 1; j < n; j++) {
			zz[i][j] = 0.0;
			zz[j][i] = 0.0;
		}
		zz[i][i] = 1.0;
	}
	if (n <= 2)
		return;
	for (m = n - 1; m >= 2; m--) {
		for (i = m; i < n; i++)
			zz[i][m - 1] = a[i][m - 2];
		i = ordr[m - 1];
		if (i != m) {
			for (j = m - 1; j < n; j++) {
				zz[m - 1][j] = zz[i - 1][j];
				zz[i - 1][j] = 0.0;
			}
			zz[i - 1][m - 1] = 1.0;
		}
	}
} /*_ eltran */


static void 
cdiv(ar, ai, br, bi, cr, ci)
	double ar, ai, br, bi, *cr, *ci;
{
	double s, ars, ais, brs, bis;

	s = fabs(br) + fabs(bi);
	ars = ar / s;
	ais = ai / s;
	brs = br / s;
	bis = bi / s;
	s = brs * brs + bis * bis;
	*cr = (ars * brs + ais * bis) / s;
	*ci = (ais * brs - ars * bis) / s;
} /*_ cdiv */


void 
hqr2(n, low, hgh, err, h, zz, wr, wi)
	int n, low, hgh, *err;
	dmattpmty h, zz;
	double *wr, *wi;
{
	int i, j, k, l, m, en, na, itn, its;
	double p, q, r, s, t, w, x, y, ra, sa, vi, vr, z, norm, tst1, tst2;
	boolean notlas;

	*err = 0;
	norm = 0.0;
	k = 1;
	/* store roots isolated by balanc and compute matrix norm */
	for (i = 0; i < n; i++) {
		for (j = k - 1; j < n; j++)
			norm += fabs(h[i][j]);
		k = i + 1;
		if (i + 1 < low || i + 1 > hgh) {
			wr[i] = h[i][i];
			wi[i] = 0.0;
		}
	}
	en = hgh;
	t = 0.0;
	itn = n * 30;

	while (en >= low) {	       /* search for next eigenvalues */
		its = 0;
		na = en - 1;

		while (en >= 1) {      /* infinietr loop */
			/* look for single small sub-diagonal element */
			for (l = en; l > low; l--) {
				s = fabs(h[l - 2][l - 2]) + fabs(h[l - 1][l - 1]);
				if (s == 0.0)
					s = norm;
				tst1 = s;
				tst2 = tst1 + fabs(h[l - 1][l - 2]);
				if (tst2 == tst1)
					goto L100;
			}
			l = low;
	L100:
			x = h[en - 1][en - 1];	/* form shift */
			if (l == en || l == na)
				break;
			if (itn == 0) { /* all eigenvalues have not converged */
				*err = en;
				goto Lerror;
			}
			y = h[na - 1][na - 1];
			w = h[en - 1][na - 1] * h[na - 1][en - 1];
			/* form exceptional shift */
			if (its == 10 || its == 20) {
				t += x;
				for (i = low - 1; i < en; i++)
					h[i][i] -= x;
				s = fabs(h[en - 1][na - 1]) + fabs(h[na - 1][en - 3]);
				x = 0.75 * s;
				y = x;
				w = -0.4375 * s * s;
			}
			its++;
			itn--;
			/* look for two consecutive small sub-diagonal elements */
			for (m = en - 2; m >= l; m--) {
				z = h[m - 1][m - 1];
				r = x - z;
				s = y - z;
				p = (r * s - w) / h[m][m - 1] + h[m - 1][m];
				q = h[m][m] - z - r - s;
				r = h[m + 1][m];
				s = fabs(p) + fabs(q) + fabs(r);
				p /= s;
				q /= s;
				r /= s;
				if (m == l)
					break;
				tst1 = fabs(p) * (fabs(h[m-2][m-2]) + fabs(z) + fabs(h[m][m]));
				tst2 = tst1 + fabs(h[m - 1][m - 2]) * (fabs(q) + fabs(r));
				if (tst2 == tst1)
					break;
			}

			for (i = m + 2; i <= en; i++) {
				h[i - 1][i - 3] = 0.0;
				if (i != m + 2)
					h[i - 1][i - 4] = 0.0;
			}

			/* double qr step involving rows l to en and columns m to en */
			for (k = m; k <= na; k++) {
				notlas = (k != na);
				if (k != m) {
					p = h[k - 1][k - 2];
					q = h[k][k - 2];
					r = 0.0;
					if (notlas)
						r = h[k + 1][k - 2];
					x = fabs(p) + fabs(q) + fabs(r);
					if (x != 0.0) {
						p /= x;
						q /= x;
						r /= x;
					}
				}
				if (x != 0.0) {
					if (p < 0.0) /* sign */
						s = - sqrt(p * p + q * q + r * r);
					else
						s = sqrt(p * p + q * q + r * r);
					if (k != m)
						h[k - 1][k - 2] = -s * x;
					else {
						if (l != m)
							h[k - 1][k - 2] = -h[k - 1][k - 2];
					}
					p += s;
					x = p / s;
					y = q / s;
					z = r / s;
					q /= p;
					r /= p;
					if (!notlas) {
						for (j = k - 1; j < n; j++) {	/* row modification */
							p = h[k - 1][j] + q * h[k][j];
							h[k - 1][j] -= p * x;
							h[k][j] -= p * y;
						}
						j = (en < (k + 3)) ? en : (k + 3); /* min */
						for (i = 0; i < j; i++) {	/* column modification */
							p = x * h[i][k - 1] + y * h[i][k];
							h[i][k - 1] -= p;
							h[i][k] -= p * q;
						}
						/* accumulate transformations */
						for (i = low - 1; i < hgh; i++) {
							p = x * zz[i][k - 1] + y * zz[i][k];
							zz[i][k - 1] -= p;
							zz[i][k] -= p * q;
						}
					} else {
						for (j = k - 1; j < n; j++) {	/* row modification */
							p = h[k - 1][j] + q * h[k][j] + r * h[k + 1][j];
							h[k - 1][j] -= p * x;
							h[k][j] -= p * y;
							h[k + 1][j] -= p * z;
						}
						j = (en < (k + 3)) ? en : (k + 3); /* min */
						for (i = 0; i < j; i++) {	/* column modification */
							p = x * h[i][k - 1] + y * h[i][k] + z * h[i][k + 1];
							h[i][k - 1] -= p;
							h[i][k] -= p * q;
							h[i][k + 1] -= p * r;
						}
						/* accumulate transformations */
						for (i = low - 1; i < hgh; i++) {
							p = x * zz[i][k-1] + y * zz[i][k] + z * zz[i][k+1];
							zz[i][k - 1] -= p;
							zz[i][k] -= p * q;
							zz[i][k + 1] -= p * r;
						}
					}
				}
			}	       /* for k */
		}		       /* while infinite loop */


		if (l == en) {	       /* one root found */
			h[en - 1][en - 1] = x + t;
			wr[en - 1] = h[en - 1][en - 1];
			wi[en - 1] = 0.0;
			en = na;
			continue;
		}
		y = h[na - 1][na - 1];
		w = h[en - 1][na - 1] * h[na - 1][en - 1];
		p = (y - x) / 2.0;
		q = p * p + w;
		z = sqrt(fabs(q));
		h[en - 1][en - 1] = x + t;
		x = h[en - 1][en - 1];
		h[na - 1][na - 1] = y + t;
		if (q >= 0.0) {	       /* real pair */
			if (p < 0.0) /* sign */
				z = p - fabs(z);
			else
				z = p + fabs(z);
			wr[na - 1] = x + z;
			wr[en - 1] = wr[na - 1];
			if (z != 0.0)
				wr[en - 1] = x - w / z;
			wi[na - 1] = 0.0;
			wi[en - 1] = 0.0;
			x = h[en - 1][na - 1];
			s = fabs(x) + fabs(z);
			p = x / s;
			q = z / s;
			r = sqrt(p * p + q * q);
			p /= r;
			q /= r;
			for (j = na - 1; j < n; j++) {	/* row modification */
				z = h[na - 1][j];
				h[na - 1][j] = q * z + p * h[en - 1][j];
				h[en - 1][j] = q * h[en - 1][j] - p * z;
			}
			for (i = 0; i < en; i++) {	/* column modification */
				z = h[i][na - 1];
				h[i][na - 1] = q * z + p * h[i][en - 1];
				h[i][en - 1] = q * h[i][en - 1] - p * z;
			}
			/* accumulate transformations */
			for (i = low - 1; i < hgh; i++) {
				z = zz[i][na - 1];
				zz[i][na - 1] = q * z + p * zz[i][en - 1];
				zz[i][en - 1] = q * zz[i][en - 1] - p * z;
			}
		} else {	       /* complex pair */
			wr[na - 1] = x + p;
			wr[en - 1] = x + p;
			wi[na - 1] = z;
			wi[en - 1] = -z;
		}
		en -= 2;
	}			       /* while en >= low */

	/* backsubstitute to find vectors of upper triangular form */
	if (norm != 0.0) {
		for (en = n; en >= 1; en--) {
			p = wr[en - 1];
			q = wi[en - 1];
			na = en - 1;
			if (q == 0.0) {/* real vector */
				m = en;
				h[en - 1][en - 1] = 1.0;
				if (na != 0) {
					for (i = en - 2; i >= 0; i--) {
						w = h[i][i] - p;
						r = 0.0;
						for (j = m - 1; j < en; j++)
							r += h[i][j] * h[j][en - 1];
						if (wi[i] < 0.0) {
							z = w;
							s = r;
						} else {
							m = i + 1;
							if (wi[i] == 0.0) {
								t = w;
								if (t == 0.0) {
									tst1 = norm;
									t = tst1;
									do {
										t = 0.01 * t;
										tst2 = norm + t;
									} while (tst2 > tst1);
								}
								h[i][en - 1] = -(r / t);
							} else {	/* solve real equations */
								x = h[i][i + 1];
								y = h[i + 1][i];
								q = (wr[i] - p) * (wr[i] - p) + wi[i] * wi[i];
								t = (x * s - z * r) / q;
								h[i][en - 1] = t;
								if (fabs(x) > fabs(z))
									h[i + 1][en - 1] = (-r - w * t) / x;
								else
									h[i + 1][en - 1] = (-s - y * t) / z;
							}
							/* overflow control */
							t = fabs(h[i][en - 1]);
							if (t != 0.0) {
								tst1 = t;
								tst2 = tst1 + 1.0 / tst1;
								if (tst2 <= tst1) {
									for (j = i; j < en; j++)
										h[j][en - 1] /= t;
								}
							}
						}
					}
				}
			} else if (q > 0.0) {
				m = na;
				if (fabs(h[en - 1][na - 1]) > fabs(h[na - 1][en - 1])) {
					h[na - 1][na - 1] = q / h[en - 1][na - 1];
					h[na - 1][en - 1] = (p - h[en-1][en-1]) / h[en-1][na-1];
				} else
					cdiv(0.0, -h[na-1][en-1], h[na-1][na-1] - p, q, &h[na-1]
					     [na-1], &h[na-1][en-1]);
				h[en - 1][na - 1] = 0.0;
				h[en - 1][en - 1] = 1.0;
				if (en != 2) {
					for (i = en - 3; i >= 0; i--) {
						w = h[i][i] - p;
						ra = 0.0;
						sa = 0.0;
						for (j = m - 1; j < en; j++) {
							ra += h[i][j] * h[j][na - 1];
							sa += h[i][j] * h[j][en - 1];
						}
						if (wi[i] < 0.0) {
							z = w;
							r = ra;
							s = sa;
						} else {
							m = i + 1;
							if (wi[i] == 0.0)
								cdiv(-ra, -sa, w, q, &h[i][na-1], &h[i][en-1]);
							else {	/* solve complex equations */
								x = h[i][i + 1];
								y = h[i + 1][i];
								vr = (wr[i]-p) * (wr[i]-p) + wi[i]*wi[i] - q*q;
								vi = (wr[i] - p) * 2.0 * q;
								if (vr == 0.0 && vi == 0.0) {
									tst1 = norm * (fabs(w) + fabs(q) + fabs(x)
										+ fabs(y) + fabs(z));
									vr = tst1;
									do {
										vr = 0.01 * vr;
										tst2 = tst1 + vr;
									} while (tst2 > tst1);
								}
								cdiv(x * r - z * ra + q * sa,
									x * s - z * sa - q * ra, vr, vi,
								     &h[i][na - 1], &h[i][en - 1]);
								if (fabs(x) > fabs(z) + fabs(q)) {
									h[i + 1][na - 1] = (q * h[i][en - 1]
										- w * h[i][na - 1] - ra) / x;
									h[i + 1][en - 1] = (-sa - w * h[i][en - 1]
										- q * h[i][na - 1]) / x;
								} else
									cdiv(-r - y * h[i][na - 1],
										-s - y * h[i][en - 1], z, q,
									     &h[i + 1][na - 1], &h[i + 1][en - 1]);
							}
							/* overflow control */
							t = (fabs(h[i][na-1]) > fabs(h[i][en-1])) ?
								 fabs(h[i][na-1]) : fabs(h[i][en-1]); /* max */
							if (t != 0.0) {
								tst1 = t;
								tst2 = tst1 + 1.0 / tst1;
								if (tst2 <= tst1) {
									for (j = i; j < en; j++) {
										h[j][na - 1] /= t;
										h[j][en - 1] /= t;
									}
								}
							}
						}
					}
				}
			}
		}		       /* for nn */

		/* end back substitution. vectors of isolated roots */
		for (i = 0; i < n; i++) {
			if (i + 1 < low || i + 1 > hgh) {
				for (j = i; j < n; j++)
					zz[i][j] = h[i][j];
			}
		}
		/* multiply by transformation matrix to give vectors of
		 * original full matrix. */
		for (j = n - 1; j >= low - 1; j--) {
			m = ((j + 1) < hgh) ? (j + 1) : hgh; /* min */
			for (i = low - 1; i < hgh; i++) {
				z = 0.0;
				for (k = low - 1; k < m; k++)
					z += zz[i][k] * h[k][j];
				zz[i][j] = z;
			}
		}
	}
	return;

Lerror: /* two roots found and complex vector */
	fprintf(stderr, "ERROR in hqr2. two roots found or complex vector");
	exit(1);

} /*_ hqr2 */


void
readrsrf(r, f, n)
dmattpmty r;
dvectpmty f;
int n;
{
	int i, j;
	double x;

	/* puts("READ 1 Relative Transition Frequencies"); */
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			fscanf(Rtfifp, "%lf", &x);
			if (Debug_optn) printf("%8.1f", x);
			r[i][j] = x;
		}
		if (Debug_optn) putchar('\n');
	}
	if (Debug_optn) putchar('\n');
	/* puts("READ 2 Relative Transition Frequencies"); */
	for (i = 0; i < n; i++) {
		fscanf(Rtfifp, "%lf", &x);
		if (Debug_optn) printf("%8.1f", x);
		f[i] = x;
	}
	if (Debug_optn) putchar('\n');
	/* puts("READ # Relative Transition Frequencies"); */

#if 0
	for (i = 1, x = 0.0; i < Tpmradix; i++) {
		for (j = 0; j < i; j++) {
			printf(" r[%2d][%2d]=%.13e;", i, j, r[i][j]);
			if ((j % 2 == 1) || j == (i-1)) putchar('\n');
			x += r[i][j];
		}
	}
	for (i = 0; i < Tpmradix; i++) {
		printf(" f[%2d]=%5.3f;", i, f[i]);
		if (i % 5 == 4) putchar('\n');
	}
	printf("%f\n", x);
#endif

} /* readrsrf */


void
tpmonepam(a, f)
dmattpmty a;
double *f;
{
	int i, j, k, tpmhalf;
	double delta, temp, sum;
	dvectpmty m;

	for (i = 0, sum = 0.0; i < Tpmradix; i++) {
		for (j = 0, temp = 0.0; j < Tpmradix; j++)
			temp += a[i][j] * f[j];
		m[i] = temp;
		sum += temp * f[i];
	}
	delta = 0.01 / sum; /* 1 PAM */
	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) {
			if (i != j)
				a[i][j] = delta * a[i][j] * f[j];
			else
#if SH
				a[i][j] = - delta * m[i];
#else
				a[i][j] = 1.0 - delta * m[i];
#endif
		}
	}

#ifndef TPM
	if (Write_optn && !Topting) { /* Write_optn */
		printf("\nTransition Probability Matrix (x1.0e7)  1PAM\n");
		if (Tpmradix == NUMAMI) {
			tpmhalf = Tpmradix / 2;
			for (i = 0; i < Tpmradix; i++) {
				printf("%7.0f", a[i][0] * 1.0e7);
				for (j = 1; j < tpmhalf; j++)
					printf("%8.0f", a[i][j] * 1.0e7);
				putchar('\n');
			}
			putchar('\n');
			for (i = 0; i < Tpmradix; i++) {
				printf("%7.0f", a[i][10] * 1.0e7);
				for (j = tpmhalf + 1; j < Tpmradix; j++)
					printf("%8.0f", a[i][j] * 1.0e7);
				putchar('\n');
			}
		} else {
			for (i = 0; i < Tpmradix; i++) {
				for (j = 0; j < Tpmradix; j++) {
					if (i == j) {
						for (k = 0, temp = 0; k < Tpmradix; k++) {
							if (i != k) temp += a[i][k];
						}
						printf("%8.0f", (1.0 - temp) * 1.0e7);
					} else {
						printf("%8.0f", a[i][j]* 1.0e7);
					}
				}
				putchar('\n');
			}
		}
	}

	if (Write_optn && Info_optn && !Topting) { /* Write_optn */
		printf("\nTransition Probability Matrix (x1.0e5)  1PAM\n");
		printf("%3s", "");
		for (j = 0; j < Tpmradix; j++) printf("%6s", Cacid3[j]); putchar('\n');
		for (i = 0; i < Tpmradix; i++) {
			printf("%3s", Cacid3[i]);
			for (j = 0; j < Tpmradix; j++)
				if (i == j) {
					for (k = 0, temp = 0; k < Tpmradix; k++) {
						if (i != k) temp += a[i][k];
					}
					printf("%6.0f", (1.0 - temp) * 1.0e5);
				} else {
					printf("%6.0f", a[i][j]* 1.0e5);
				}
			putchar('\n');
		}
		printf("%3s", "Pai");
		for (j = 0; j < Tpmradix; j++) printf("%6.3f", f[j]); putchar('\n');
	}

	if (Write_optn && !Topting) { /* Write_optn */
		if (Tpmradix != NUMAMI) {
			/*
			printf("\nSubstitution Matrix (x1.0e7)  1PAM\n");
			for (i = 0; i < Tpmradix; i++) {
				for (j = 0; j < Tpmradix; j++)
					printf("%8.0f", a[i][j] * f[i] * 1.0e7);
				putchar('\n');
			}
			*/
			printf("\nTS/TV: %.3f\n",
				( a[0][1]*f[0] + a[2][3]*f[2] )
				/ ( a[0][2]*f[0] + a[0][3]*f[0]
				  + a[1][2]*f[1] + a[1][3]*f[1] ) );
		}
	}
#endif /* TPM */
} /* tpmonepam */


void 
luinverse(omtrx, imtrx, size)
	dmattpmty omtrx, imtrx;
	int size;
{
	/* INVERSION OF MATRIX ON LU DECOMPOSITION */
    double eps = 1.0e-20; /* ! */
	int i, j, k, l, maxi, idx, ix, jx;
	double sum, tmp, maxb, aw;
	ivectpmty index;
	double *wk;

	wk = (double *) malloc((unsigned)size * sizeof(double));
	aw = 1.0;
	for (i = 0; i < size; i++) {
		maxb = 0.0;
		for (j = 0; j < size; j++) {
			if (fabs(omtrx[i][j]) > maxb)
				maxb = fabs(omtrx[i][j]);
		}
		if (maxb == 0.0) {
			fprintf(stderr, "luinverse: singular matrix\n");
			exit(1);
		}
		wk[i] = 1.0 / maxb;
	}
	for (j = 0; j < size; j++) {
		for (i = 0; i < j; i++) {
			sum = omtrx[i][j];
			for (k = 0; k < i; k++)
				sum -= omtrx[i][k] * omtrx[k][j];
			omtrx[i][j] = sum;
		}
		maxb = 0.0;
		for (i = j; i < size; i++) {
			sum = omtrx[i][j];
			for (k = 0; k < j; k++)
				sum -= omtrx[i][k] * omtrx[k][j];
			omtrx[i][j] = sum;
			tmp = wk[i] * fabs(sum);
			if (tmp >= maxb) {
				maxb = tmp;
				maxi = i;
			}
		}
		if (j != maxi) {
			for (k = 0; k < size; k++) {
				tmp = omtrx[maxi][k];
				omtrx[maxi][k] = omtrx[j][k];
				omtrx[j][k] = tmp;
			}
			aw = -aw;
			wk[maxi] = wk[j];
		}
		index[j] = maxi;
		if (omtrx[j][j] == 0.0)
			omtrx[j][j] = eps;
		if (j != size - 1) {
			tmp = 1.0 / omtrx[j][j];
			for (i = j + 1; i < size; i++)
				omtrx[i][j] *= tmp;
		}
	}
	for (jx = 0; jx < size; jx++) {
		for (ix = 0; ix < size; ix++)
			wk[ix] = 0.0;
		wk[jx] = 1.0;
		l = -1;
		for (i = 0; i < size; i++) {
			idx = index[i];
			sum = wk[idx];
			wk[idx] = wk[i];
			if (l != -1) {
				for (j = l; j < i; j++)
					sum -= omtrx[i][j] * wk[j];
			} else if (sum != 0.0)
				l = i;
			wk[i] = sum;
		}
		for (i = size - 1; i >= 0; i--) {
			sum = wk[i];
			for (j = i + 1; j < size; j++)
				sum -= omtrx[i][j] * wk[j];
			wk[i] = sum / omtrx[i][i];
		}
		for (ix = 0; ix < size; ix++)
			imtrx[ix][jx] = wk[ix];
	}
	free((char *)wk);
	wk = NULL;
} /*_ luinverse */


#ifdef DEBUG
void 
mproduct(am, bm, cm, na, nb, nc)
	dmattpmty am, bm, cm;
	int na, nb, nc;
{
	int ia, ib, ic;
	double sum;

	for (ia = 0; ia < na; ia++) {
		for (ic = 0; ic < nc; ic++) {
			sum = 0.0;
			for (ib = 0; ib < nb; ib++)
				sum += am[ia][ib] * bm[ib][ic];
			cm[ia][ic] = sum;
		}
	}
} /*_ mproduct */


void 
preigen()
{
	int nam1, nam2;

	printf(" EIGEN VECTOR\n");
	for (nam1 = 0; nam1 < Tpmradix; nam1++) {
		for (nam2 = 1; nam2 <= Tpmradix; nam2++) {
			printf("%7.3f", Evec[nam1][nam2 - 1]);
			if (nam2 == 10 || nam2 == 20)
				putchar('\n');
		}
		putchar('\n');
	}
	printf(" EIGEN VALUE\n");
	for (nam1 = 1; nam1 <= Tpmradix; nam1++) {
		printf("%7.3f", Eval[nam1 - 1]);
		if (nam1 == 10 || nam1 == 20)
			putchar('\n');
	}
} /*_ preigen */


void 
checkevector(imtrx, nn)
	dmattpmty imtrx;
	int nn;
{
	int i, j;

	for (i = 0; i < nn; i++) {
		for (j = 0; j < nn; j++) {
			if (i == j) {
				if (fabs(imtrx[i][j] - 1.0) > 1.0e-5) {
					fprintf(stderr, "ERROR: eigen vector in checkevector 1\n");
					exit(1);
				}
			} else {
				if (fabs(imtrx[i][j]) > 1.0e-5) {
					fprintf(stderr, "ERROR: eigen vector in checkevector 2\n");
					exit(1);
				}
			}
		}
	}
} /*_ checkevector */
#endif /* DEBUG */


void 
getrsr(a, ftpm)
dmattpmty a;
dvectpmty ftpm;
{
	int i, j;
	double api;
	dvectpmty forg;
#ifdef NUC
	double alpha, beta, beta1, beta2, alpy, alpr;
#endif /* NUC */

#ifndef NUC
	api = 0.05;
#else  /* NUC */
	api = 0.25;
#endif /* NUC */

	if (Rrsr_optn) {
		readrsrf(a, forg, Tpmradix);
	} else if (Poisn_optn) {
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++) a[i][j] = 1.0;
			a[i][i] = 0.0;
			forg[i] = api;
		}
	} else { /* !Poisn_optn */
#ifndef NUC
		if (Mtrev_optn) { /* mtREV */
			mtrev(a, forg);
		} else if (Dayhf_optn) { /* Dayhoff */
			dyhfjtt(a, forg, TRUE);
		} else { /* JTT */
			dyhfjtt(a, forg, FALSE);
		}
#else	/* NUC */
		beta = 1.0;
		beta1 = (beta * 2.0) / (Beta12 + 1.0);
		beta2 = Beta12 * beta1;
		alpha = AlphaBeta;
		alpr  = (alpha * 2.0) / (AlphaYR + 1.0);
		alpy  = AlphaYR * alpr;
        a[0][0] =  0.0;  a[0][1] = alpy;  a[0][2] = beta1; a[0][3] = beta1;
		a[1][0] = alpy;  a[1][1] =  0.0;  a[1][2] = beta2; a[1][3] = beta2;
		a[2][0] = beta1; a[2][1] = beta2; a[2][2] =  0.0;  a[2][3] = alpr;
		a[3][0] = beta1; a[3][1] = beta2; a[3][2] = alpr;  a[3][3] =  0.0;
		forg[0] = 0.25;  forg[1] = 0.25;  forg[2] = 0.25;  forg[3] = 0.25;
#endif	/* NUC */
	} /* Poisn_optn */

#ifdef DEBUG
	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) {
			if (a[i][j] != a[j][i]) {
				fputs("ERROR: Relative Substitution Rate Matrix: ", stderr);
				fprintf(stderr, "a[%d][%d] = %.3f != a[%d][%d]\n",
					i, j, a[i][j], j, i);
				exit(1);
			}
		}
	}
#endif /* DEBUG */

	if (Write_optn && !Topting) { /* Debug_optn */
		puts("\nRelative Substitution Rate Matrix");
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++)
#ifndef NUC
				if (i != j)
					printf("%5.0f", a[i][j]);
				else
					printf("%5s", Cacid3[i]);
#else			/* NUC */
				if (i != j)
					printf("%8.3f", a[i][j]);
				else
					printf("%6s  ", Cacid1[i]);
#endif			/* NUC */
			putchar('\n');
		}
	}

#ifdef TPM
	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++)
			Rtf[i][j] = a[i][j];
	}
#endif /* TPM */

	if (Frequ_optn) {
		for (i = 0; i < Tpmradix - 1; i++) {
			for (j = i + 1; j < Tpmradix; j++) {
				if (Freqemp[i] == Freqemp[j]) {
					Freqemp[i] += 0.00001;
					Freqemp[j] -= 0.00001;
				}
			}
		}
		for (i = 0; i < Tpmradix; i++) ftpm[i] = Freqemp[i];
	} else {
		for (i = 0; i < Tpmradix; i++) ftpm[i] = forg[i];
	}
} /* getrsr */


void 
tranprobmat()
{
	/* make transition probability matrix */
	dmattpmty a, b;
	dvectpmty evali;
	ivectpmty ordr;
	int i, j, k, err;
	double zero, temp;
	boolean error;

	getrsr(a, Freqtpm);

	tpmonepam(a, Freqtpm);

	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++)
			b[i][j] = a[i][j];
	}
	error = FALSE;
	elmhes(a, ordr, Tpmradix);
	eltran(a, Evec, ordr, Tpmradix);
	hqr2(Tpmradix, 1, Tpmradix, &err, a, Evec, Eval, evali);

#ifdef DEBUG
	for (j = 0; j < Tpmradix; j++) {
		for (i = 0, zero = 0.0; i < Tpmradix; i++) {
			for (k = 0; k < Tpmradix; k++)
				zero += b[i][k] * Evec[k][j];
			zero -= Eval[j] * Evec[i][j];
			if (fabs(zero) > 1.0e-5) {
				error = TRUE;
				printf(" ERROR: Eigen System%  .3E", zero);
				if (i % 5 == 0) putchar('\n');
			}
		}
	}
	if (error) {
		fflush(stdout);
		fputs("\nERROR: Eigen Value of Transition Probability Matrix\n",stderr);
		exit(1);
	}
#endif /* DEBUG */

	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++)
			b[i][j] = Evec[i][j];
	}
	luinverse(b, Ievc, Tpmradix);

#ifdef DEBUG
	mproduct(Evec, Ievc, b, Tpmradix, Tpmradix, Tpmradix);
	checkevector(b, Tpmradix);
	if (Debug_optn) preigen();
#endif /* DEBUG */

	for (i = 0; i < Tpmradix - 1; i++) { /* !? */
		for (j = i + 1; j < Tpmradix; j++) {
			temp       = Ievc[i][j];
			Ievc[i][j] = Ievc[j][i];
			Ievc[j][i] = temp;
		}
	}
#if SH
	for (i = 0; i < Tpmradix; i++) Evl2[i] = Eval[i] * Eval[i];
#else
	for (i = 0; i < Tpmradix; i++) {
		temp = log(Eval[i]);
		Eval[i] = temp;
		Evl2[i] = temp * temp;
	}
#endif
} /*_ tranprobmat */


#if VARITPM
void 
varitpm()
{
	/* make transition probability matrix */
	dmattpmty a, b, r, o, rr;
	dvectpmty evali;
	ivectpmty ordr;
	int i, j, k, err, ii, jj, kk;
	double zero, temp;
	double aij, lklorg, lklnew, d1, d2, dd, vari, notch;
	double tlo, tro, tln, trn, tl1, tl2, tr1, tr2, tld, trd, ttl, ttr, varil, varir;

	notch = Percent;
	initpartlkl(Ctree);
	aproxlkl(Ctree);
	lklorg = Ctree->aproxl;

	getrsr(r, Freqtpm);
	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) o[i][j] = r[i][j];
	}
	tpmonepam(o, Freqtpm);
	for (j = 0; j < Tpmradix; j++) rr[j][j] = 0.0;

	for (ii = 1; ii < Tpmradix; ii++) {
	for (jj = 0; jj < ii; jj++) {

	aij = r[ii][jj];
	tlo = o[ii][jj];
	tro = o[jj][ii];

	for (kk = 0; kk < 2; kk++) {

		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++) a[i][j] = r[i][j];
		}
	
		if (kk == 0) {
			a[ii][jj] -= notch;
			a[jj][ii] -= notch;
		} else {
			a[ii][jj] += notch;
			a[jj][ii] += notch;
		}
	
		tpmonepam(a, Freqtpm);
	
		tln = a[ii][jj];
		trn = a[jj][ii];
	
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++)
				b[i][j] = a[i][j];
		}
		elmhes(a, ordr, Tpmradix);
		eltran(a, Evec, ordr, Tpmradix);
		hqr2(Tpmradix, 1, Tpmradix, &err, a, Evec, Eval, evali);
	
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0; j < Tpmradix; j++)
				b[i][j] = Evec[i][j];
		}
		luinverse(b, Ievc, Tpmradix);
	
		for (i = 0; i < Tpmradix - 1; i++) { /* !? */
			for (j = i + 1; j < Tpmradix; j++) {
				temp       = Ievc[i][j];
				Ievc[i][j] = Ievc[j][i];
				Ievc[j][i] = temp;
			}
		}
#if SH
		for (i = 0; i < Tpmradix; i++) Evl2[i] = Eval[i] * Eval[i];
#else
		for (i = 0; i < Tpmradix; i++) {
			temp = log(Eval[i]);
			Eval[i] = temp;
			Evl2[i] = temp * temp;
		}
#endif
	
		initpartlkl(Ctree);
		aproxlkl(Ctree);
		lklnew = Ctree->aproxl;
		if (kk == 0) {
			d1  = (lklorg - lklnew) / notch;
			tl1 = (lklorg - lklnew) / (tlo - tln); 
			tr1 = (lklorg - lklnew) / (tro - trn);
			tld = (tlo - tln) / 2.0;
			trd = (tro - trn) / 2.0;
		} else {
			d2  = (lklnew - lklorg) / notch;
			tl2 = (lklorg - lklnew) / (tln - tlo); 
			tr2 = (lklorg - lklnew) / (trn - tro);
			tld += (tln - tlo) / 2.0;
			trd += (trn - tro) / 2.0;
		}
		/*
		printf("%9.3f%9.3f%9.3f%9.3f%9.6f%9.6f\n",
			tlo*1e5, tln*1e5, tro*1e5, trn*1e5, tld*1e5, trd*1e5);
		*/

	}
	dd  = (d2 - d1) / notch;
	ttl = (tl2 - tl1) / tld;
	ttr = (tr2 - tr1) / trd;
	vari  =  sqrt(1.0 / fabs(dd));
	varil =  sqrt(1.0 / fabs(ttl));
	varir =  sqrt(1.0 / fabs(ttr));
	printf("%3d%3d%2s%2s%9.5f%9.5f%3s%6.0f%6.0f %8.3f%8.3f%8.3f%8.3f\n",
		ii+1, jj+1, Cacid1[ii], Cacid1[jj], d1, d2, (dd < 0.0 ? "mi" : "PL"),
		aij, vari, tlo*100000, varil*100000, tro*100000, varir*100000);
	rr[ii][jj] = aij;
	if (dd < 0.0) {
		rr[jj][ii] = vari;
	} else {
		rr[jj][ii] = 0.0;
	}
	}
	}
	printf("\nnotch:%12.8f\n", notch);
	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) {
			if (j == i) {
				printf("%5.3s", Cacid3[i]);
			} else {
				if (i > j) {
					printf("%5.0f", rr[i][j]);
				} else {
					if (rr[j][i] == 1.0)
						printf("%5.1s", "-");
					else if (rr[i][j] == 0.0)
						printf("%5.1s", "*");
					else
						printf("%5.0f", rr[i][j]);
				}
			}
		} putchar('\n');
	} 
} /*_ varitpm */
#endif /* VARITPM */


void
prfreq()
{
	int i;

	printf("\n%s Acid Frequencies\n", Infomol);
	printf("%3s%4s%8s%7s%7s\n",
		"", "", "Model ", "Data", "     ");
	for (i = 0; i < Tpmradix; i++) {
		printf("%3d%3s%8.3f%8.3f\n",
			i + 1, Cacid1[i], Freqtpm[i], Freqemp[i]);
	}
} /*_ refreq */


void 
tprobmtrx(arc, tpr)
	double arc;
	dmattpmty tpr;
{
	/* TRANSITION PROBABILITY MATRIX */
	register int i, j, k;
	register double temp;
	dvectpmty vexp;
	dmattpmty iexp;

	for (k = 0; k < Tpmradix; k++)
		vexp[k] = exp(arc * Eval[k]);
	for (j = 0; j < Tpmradix; j++) {
		for (k = 0; k < Tpmradix; k++)
			iexp[j][k] = Ievc[j][k] * vexp[k];
	}

	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) {
			temp = 0.0;
			for (k = 0; k < Tpmradix; k++)
				temp += Evec[i][k] * iexp[j][k];
			tpr[i][j] = temp;
		}
	}
} /*_ tprobmtrx */

void 
tprobmtrxt(arc, tpr)
	double arc;
	dmattpmty tpr;
{
	/* TRANSITION PROBABILITY MATRIX */
	register int i, j, k;
	register double temp;
	dvectpmty vexp;
	dmattpmty iexp;

	for (k = 0; k < Tpmradix; k++)
		vexp[k] = exp(arc * Eval[k]);
	for (j = 0; j < Tpmradix; j++) {
		for (k = 0; k < Tpmradix; k++)
			iexp[j][k] = Ievc[j][k] * vexp[k];
	}

	for (i = 0; i < Tpmradix; i++) {
		for (j = 0; j < Tpmradix; j++) {
			temp = 0.0;
			for (k = 0; k < Tpmradix; k++)
				temp += Evec[i][k] * iexp[j][k];
			tpr[j][i] = temp; /* i <-> j */
		}
	}
} /*_ tprobmtrxt */


void 
tdiffmtrx(arc, tpr, td1, td2)
	double arc;
	dmattpmty tpr, td1, td2;
{
	/* TRANSITION PROBABILITY AND DIFFRENCE MATRIX */
	register int i, j, k;
	register double x, tp, t1, t2;
	dvectpmty vexp;
	dmattpmty iexp;
	dvector evc;

	for (k = 0; k < Tpmradix; k++)
		vexp[k] = exp(arc * Eval[k]);
	for (j = 0; j < Tpmradix; j++) {
		for (k = 0; k < Tpmradix; k++)
			iexp[j][k] = Ievc[j][k] * vexp[k];
	}

	for (i = 0; i < Tpmradix; i++) {
		evc = Evec[i];
		for (j = 0; j < Tpmradix; j++) {
			tp = t1 = t2 = 0.0;
			for (k = 0; k < Tpmradix; k++) {
				x = evc[k] * iexp[j][k];
				tp += x;
				t1 += x * Eval[k];
				t2 += x * Evl2[k];
			}
			tpr[i][j] = tp;
			td1[i][j] = t1;
			td2[i][j] = t2;
		}
	}
} /*_ tdiffmtrx */


#if 0
void 
tprobmtrx2(arc, tpr) /* Ievc is not Transposition matrix */
	double arc;
	dmattpmty tpr;
{
	/* TRANSITION PROBABILITY MATRIX */
	register int i, j, k;
	register double s0, s1, s2, s3, s4, s5, s6, s7;
	register double s8, s9, s10, s11, s12, s13, s14, s15;
	register double b0, b1, b2, b3, c0, c1, c2, c3, v0, v1;
	dmattpmty iexp;

	for (k = 0; k < Tpmradix; k++) {
		s0 = exp(arc * Eval[k]);
		for (j = 0; j < Tpmradix; j++)
			iexp[k][j] = Ievc[k][j] * s0;
	}
	for (i = 0; i < Tpmradix; i += 4) {
		for (j = 0; j < Tpmradix; j += 4) {
			s0  = 0.0; s1  = 0.0; s2  = 0.0; s3  = 0.0;
			s4  = 0.0; s5  = 0.0; s6  = 0.0; s7  = 0.0;
			s8  = 0.0; s9  = 0.0; s10 = 0.0; s11 = 0.0;
			s12 = 0.0; s13 = 0.0; s14 = 0.0; s15 = 0.0;
			b0 = Evec[i  ][0];
			b1 = Evec[i+1][0];
			c0 = iexp[0][j];
			c1 = iexp[0][j+1];
			c2 = iexp[0][j+2];
			c3 = iexp[0][j+3];
			v0 = b0 * c0;
			v1 = b0 * c1;
			for (k = 0; k < Tpmradix-1; k++) {
				s0  += v0;
				s4  += v1;
				s8  += b0 * c2;
				s12 += b0 * c3;
				s1  += b1 * c0;
				s5  += b1 * c1;
				s9  += b1 * c2;
				s13 += b1 * c3;
				b0 = Evec[i  ][k+1];
				b1 = Evec[i+1][k+1];
				b2 = Evec[i+2][k];
				b3 = Evec[i+3][k];
				s2  += b2 * c0;
				s6  += b2 * c1;
				s10 += b2 * c2;
				s14 += b2 * c3;
				s3  += b3 * c0;
				s7  += b3 * c1;
				s11 += b3 * c2;
				s15 += b3 * c3;
				c0 = iexp[k+1][j];
				c1 = iexp[k+1][j+1];
				c2 = iexp[k+1][j+2];
				c3 = iexp[k+1][j+3];
				v0 = b0 * c0;
				v1 = b0 * c1;
			}
			s0  += v0;
			s4  += v1;
			s8  += b0 * c2;
			s12 += b0 * c3;
			s1  += b1 * c0;
			s5  += b1 * c1;
			s9  += b1 * c2;
			s13 += b1 * c3;
			b2 = Evec[i+2][k];
			b3 = Evec[i+3][k];
			s2  += b2 * c0;
			s6  += b2 * c1;
			s10 += b2 * c2;
			s14 += b2 * c3;
			s3  += b3 * c0;
			s7  += b3 * c1;
			s11 += b3 * c2;
			s15 += b3 * c3;
			tpr[i  ][j]   = s0;
			tpr[i  ][j+1] = s4;
			tpr[i  ][j+2] = s8;
			tpr[i  ][j+3] = s12;
			tpr[i+1][j]   = s1;
			tpr[i+1][j+1] = s5;
			tpr[i+1][j+2] = s9;
			tpr[i+1][j+3] = s13;
			tpr[i+2][j]   = s2;
			tpr[i+2][j+1] = s6;
			tpr[i+2][j+2] = s10;
			tpr[i+2][j+3] = s14;
			tpr[i+3][j]   = s3;
			tpr[i+3][j+1] = s7;
			tpr[i+3][j+2] = s11;
			tpr[i+3][j+3] = s15;
		}
	}
} /*_ tprobmtrx */
#endif


#if 0
void 
tdiffmtrx2(arc, tpr, td1, td2) /* Ievc is not Transposition matrix */
	double arc;
	dmattpmty tpr, td1, td2;
{
	/* TRANSITION PROBABILITY MATRIX */
	register int i, j, k;
	register double s00, s01, s10, s11;
	register double t00, t01, t10, t11;
	register double u00, u01, u10, u11;
	register double v00, v01, v10, v11;
	register double b0, b1, c0, c1;
	register double x, y, z;
	dvectpmty ex0, ex1, ex2;

	for (k = 0; k < Tpmradix; k++) {
		x = exp(arc * Eval[k]);
		ex0[k] = x;
		ex1[k] = Eval[k] * x;
		ex2[k] = Evl2[k] * x;
	}

	for (i = 0; i < Tpmradix; i += 2) {
		for (j = 0; j < Tpmradix; j += 2) {
			s00 = 0.0; s01 = 0.0; s10 = 0.0; s11 = 0.0;
			t00 = 0.0; t01 = 0.0; t10 = 0.0; t11 = 0.0;
			u00 = 0.0; u01 = 0.0; u10 = 0.0; u11 = 0.0;

			b0 = Evec[i][0];
			c0 = Ievc[0][j];
			c1 = Ievc[0][j+1];
			for (k = 0; k < Tpmradix-1; k++) {
				x = ex0[k];
				y = ex1[k];
				z = ex2[k];
				v00 = b0 * c0;
				v01 = b0 * c1;
				s00 += v00 * x;
				s01 += v01 * x;
				t00 += v00 * y;
				t01 += v01 * y;
				u00 += v00 * z;
				u01 += v01 * z;
				b0 = Evec[i][k+1];
				b1 = Evec[i+1][k];
				v10 = b1 * c0;
				v11 = b1 * c1;
				s10 += v10 * x;
				s11 += v11 * x;
				t10 += v10 * y;
				t11 += v11 * y;
				u10 += v10 * z;
				u11 += v11 * z;
				c0 = Ievc[k+1][j];
				c1 = Ievc[k+1][j+1];
			}
			x = ex0[Tpmradix-1];
			y = ex1[Tpmradix-1];
			z = ex2[Tpmradix-1];
				v00 = b0 * c0;
				v01 = b0 * c1;
				s00 += v00 * x;
				s01 += v01 * x;
				t00 += v00 * y;
				t01 += v01 * y;
				u00 += v00 * z;
				u01 += v01 * z;
				b1 = Evec[i+1][k];
				v10 = b1 * c0;
				v11 = b1 * c1;
				s10 += v10 * x;
				s11 += v11 * x;
				t10 += v10 * y;
				t11 += v11 * y;
				u10 += v10 * z;
				u11 += v11 * z;
			tpr[i  ][j  ] = s00;
			tpr[i  ][j+1] = s01;
			tpr[i+1][j  ] = s10;
			tpr[i+1][j+1] = s11;
			td1[i  ][j  ] = t00;
			td1[i  ][j+1] = t01;
			td1[i+1][j  ] = t10;
			td1[i+1][j+1] = t11;
			td2[i  ][j  ] = u00;
			td2[i  ][j+1] = u01;
			td2[i+1][j  ] = u10;
			td2[i+1][j+1] = u11;
		}
	}
} /*_ tdiffmtrx */
#endif
