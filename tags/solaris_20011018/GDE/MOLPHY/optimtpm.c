/*
 * optimtpm.c   Adachi, J.   1994.11.16
 * Copyright (C) 1992-1994 J. Adachi & M. Hasegawa; All rights reserved.
 */

double
abfunc(ab)
double ab;
{
	Node *rp;

	AlphaBeta = ab;
	tranprobmat();
	fmlength(Ctree, Distanmat, Maxspc);
/*	lslength(Ctree, Distanvec, Maxspc); */
	initpartlkl(Ctree);
	rp = (Node *)mlikelihood(Ctree);
	return Ctree->lklhd;
} /* abfunc */


double
ayrfunc(ayr)
double ayr;
{
	Node *rp;

	AlphaYR = ayr;
	tranprobmat();
	fmlength(Ctree, Distanmat, Maxspc);
/*	lslength(Ctree, Distanvec, Maxspc); */
	initpartlkl(Ctree);
	rp = (Node *)mlikelihood(Ctree);
	return Ctree->lklhd;
} /* ayrfunc */


double
b12func(b12)
double b12;
{
	Node *rp;

	Beta12 = b12;
	tranprobmat();
	fmlength(Ctree, Distanmat, Maxspc);
/*	lslength(Ctree, Distanvec, Maxspc); */
	initpartlkl(Ctree);
	rp = (Node *)mlikelihood(Ctree);
	return Ctree->lklhd;
} /* b12func */


double
optimab(x1, x2, x3, f1, f2, f3, func, eps)
double x1, x2, x3, f1, f2, f3;
double (*func)();
double eps;
{
	int i;
	double xa, fa, dl, dr, gold;

	gold = 0.38197;
	for (i = 0; (x3 - x1) > eps; i++) {
		if (Info_optn)
			printf("%-3d%8.3f%8.3f%8.3f%13.3f%13.3f%13.3f\n",
				i,x1,x2,x3,f1,f2,f3);
		dl = x2 - x1;
		dr = x3 - x2;
		if (dl > dr) {
			xa = x2 - gold * dl;
		} else {
			xa = x2 + gold * dr;
		}
		fa = (*func)(xa);
		if (dl > dr) {
			if (fa > f2) {
				x3 = x2; f3 = f2; x2 = xa; f2 = fa;
			} else {
				x1 = xa; f1 = fa;
			}
		} else {
			if (fa > f2) {
				x1 = x2; f1 = f2; x2 = xa; f2 = fa;
			} else {
				x3 = xa; f3 = fa;
			}
		}
		if (i > 30) break;
	}
	if (f3 > f2) {
		x2 = x3; f2 = f3;
	} else if (f1 > f2) {
		x2 = x1; f2 = f1;
	}
/*	printf("  opt. para: %.3f  Max. ln likelihood: %.3f\n", x2, f2); */
	return x2;
} /* optimab */


void
optimtpm()
{
	int i, loop;
	char buf[64];
	double x1, x2, x3, f1, f2, f3, ab, ayr, b12, eps1, eps2, eps;
	double abold, ayrold, b12old, dab, dayr, db12;
	boolean ayr_mode, b12_mode;

#if 0
	for (i = 5; i < 500; i += 5) {
		ab = 0.1 * i;
		f1 = abfunc(ab);
		printf("%5.1f %15.3f\n", ab, f1);
	}
	exit(1);
#endif

	Topting = TRUE;
	eps = 0.005;
/*	printf("Alpha/Beta: %.3f  AlphaY/AlphaR: %.3f\n", AlphaBeta, AlphaYR); */
	ayr_mode = FALSE;
	b12_mode = FALSE;
	if (Toptim_optn == 1) {
		eps1 = eps;
	} else {
		if (Toptim_optn == 3) {
			ayr_mode = TRUE;
		} else if (Toptim_optn == 5) {
			b12_mode = TRUE;
		} else {
			ayr_mode = TRUE;
			b12_mode = TRUE;
		}
		eps1 = 4.0;
	}
	x1 = 10.0;
	x3 = 15.0;
	f1 = abfunc(x1);
	f3 = abfunc(x3);
	if (f1 > f3) {
		x2 = x1; f2 = f1;
		x1 = MINAB;
		f1 = abfunc(x1);
	} else {
		x2 = x3; f2 = f3;
		x3 = MAXAB;
		f3 = abfunc(x3);
	}
	AlphaBeta = ab = optimab(x1, x2, x3, f1, f2, f3, abfunc, eps1);

	if (ayr_mode) {
		if (Info_optn) putchar('\n');
		eps2 = 0.1;
		x1 = 0.5;
		x3 = 1.0;
		f1 = ayrfunc(x1);
		f3 = ayrfunc(x3);
		if (f1 > f3) {
			x2 = x1; f2 = f1;
			x1 = MINAYR;
			f1 = ayrfunc(x1);
		} else {
			x2 = x3; f2 = f3;
			x3 = MAXAYR;
			f3 = ayrfunc(x3);
		}
		AlphaYR = ayr = optimab(x1, x2, x3, f1, f2, f3, ayrfunc, eps2);

		dab = 9.0;
		dayr = 0.2;
		if (Info_optn) putchar('\n');
		if (Info_optn) printf("A/B: %.3f  Ay/Ar: %.3f", ab,ayr);
		if (Info_optn) printf("  %.4f %.4f", dab,dayr);
		if (Info_optn) putchar('\n');
		for (loop = 0; dab > eps || dayr > eps; loop++) {
			if (loop < 1) {
				eps1 = 2.0;  eps2 = 0.05;
			} else if (loop < 2) {
				eps1 = 0.5;  eps2 = eps;
			} else if (loop < 3) {
				eps1 = 0.05; eps2 = eps;
			} else {
				eps1 = eps; eps2 = eps;
			}
	
			if (dab < eps) dab = eps;
			x1 = ab;
			(ab + 1.0) < MAXAB ? (x3 = ab + 1.0) : (x3 = MAXAB);
			f1 = abfunc(x1);
			f3 = abfunc(x3);
			if (f1 > f3) {
				x2 = x1; f2 = f1;
				(x2 - dab) > MINAB ? (x1 = x2 - dab) : (x1 = MINAB);
				f1 = abfunc(x1);
			} else {
				x2 = x3; f2 = f3;
				(x2 + dab) < MAXAB ? (x3 = x2 + dab) : (x3 = MAXAB);
				f3 = abfunc(x3);
			}
			abold = ab;
			AlphaBeta = ab = optimab(x1, x2, x3, f1, f2, f3, abfunc, eps1);
			dab = fabs(ab - abold);
	
			if (dayr < eps) dayr = eps;
			(ayr - 0.02) > MINAYR ? (x1 = ayr - 0.02) : (x1 = MINAYR);
			x3 = ayr;
			f1 = ayrfunc(x1);
			f3 = ayrfunc(x3);
			if (f1 > f3) {
				x2 = x1; f2 = f1;
				(x2 - dayr) > MINAYR ? (x1 = x2 - dayr) : (x1 = MINAYR);
				f1 = ayrfunc(x1);
			} else {
				x2 = x3; f2 = f3;
				(x2 + dayr) < MAXAYR ? (x3 = x2 + dayr) : (x3 = MAXAYR);
				f3 = ayrfunc(x3);
			}
			ayrold = ayr;
			AlphaYR = ayr = optimab(x1, x2, x3, f1, f2, f3, ayrfunc, eps2);
			dayr = fabs(ayr - ayrold);
			if (Info_optn) putchar('\n');
			if (Info_optn) printf("A/B: %.3f  Ay/Ar: %.3f", ab,ayr);
			if (Info_optn) printf("  %.4f %.4f", dab,dayr);
			if (Info_optn) putchar('\n');
		}
	}

	if (b12_mode) {
		if (Info_optn) putchar('\n');
		eps2 = 0.1;
		x1 = 0.5;
		x3 = 1.0;
		f1 = b12func(x1);
		f3 = b12func(x3);
		if (f1 > f3) {
			x2 = x1; f2 = f1;
			x1 = MINB12;
			f1 = b12func(x1);
		} else {
			x2 = x3; f2 = f3;
			x3 = MAXB12;
			f3 = b12func(x3);
		}
		Beta12 = b12 = optimab(x1, x2, x3, f1, f2, f3, b12func, eps2);

		dab = 9.0;
		db12 = 0.2;
		if (Info_optn) putchar('\n');
		if (Info_optn) printf("A/B: %.3f  B1/B2: %.3f", ab,b12);
		if (Info_optn) printf("  %.4f %.4f", dab,db12);
		if (Info_optn) putchar('\n');
		for (loop = 0; dab > eps || db12 > eps; loop++) {
			if (loop < 1) {
				eps1 = 2.0;  eps2 = 0.05;
			} else if (loop < 2) {
				eps1 = 0.5;  eps2 = eps;
			} else if (loop < 3) {
				eps1 = 0.05; eps2 = eps;
			} else {
				eps1 = eps; eps2 = eps;
			}
	
			if (dab < eps) dab = eps;
			x1 = ab;
			(ab + 1.0) < MAXAB ? (x3 = ab + 1.0) : (x3 = MAXAB);
			f1 = abfunc(x1);
			f3 = abfunc(x3);
			if (f1 > f3) {
				x2 = x1; f2 = f1;
				(x2 - dab) > MINAB ? (x1 = x2 - dab) : (x1 = MINAB);
				f1 = abfunc(x1);
			} else {
				x2 = x3; f2 = f3;
				(x2 + dab) < MAXAB ? (x3 = x2 + dab) : (x3 = MAXAB);
				f3 = abfunc(x3);
			}
			abold = ab;
			AlphaBeta = ab = optimab(x1, x2, x3, f1, f2, f3, abfunc, eps1);
			dab = fabs(ab - abold);
	
			if (db12 < eps) db12 = eps;
			(b12 - 0.02) > MINB12 ? (x1 = b12 - 0.02) : (x1 = MINB12);
			x3 = b12;
			f1 = b12func(x1);
			f3 = b12func(x3);
			if (f1 > f3) {
				x2 = x1; f2 = f1;
				(x2 - db12) > MINB12 ? (x1 = x2 - db12) : (x1 = MINB12);
				f1 = b12func(x1);
			} else {
				x2 = x3; f2 = f3;
				(x2 + db12) < MAXB12 ? (x3 = x2 + db12) : (x3 = MAXB12);
				f3 = b12func(x3);
			}
			b12old = b12;
			Beta12 = b12 = optimab(x1, x2, x3, f1, f2, f3, b12func, eps2);
			db12 = fabs(b12 - b12old);
			if (Info_optn) putchar('\n');
			if (Info_optn) printf("A/B: %.3f  B1/B2: %.3f", ab,b12);
			if (Info_optn) printf("  %.4f %.4f", dab,db12);
			if (Info_optn) putchar('\n');
		}
	}

	if (!Ctacit_optn) {
		printf("Alpha/Beta: %.3f", AlphaBeta);
		if (AlphaYR != 1.0) printf("  AlphaY/AlphaR: %.3f", AlphaYR);
		if (Beta12  != 1.0) printf("  Beta1/Beta2: %.3f", Beta12);
		putchar('\n');
	}

#if 0
	*Modelname = '\0';
	sprintf(buf, "A/B:%.2f", AlphaBeta);
	strcat(Modelname, buf);
	if (AlphaYR != 1.0) {
		sprintf(buf, " Ay/Ar:%.2f", AlphaYR);
		strcat(Modelname, buf);
	}
	if (Beta12 != 1.0) {
		sprintf(buf, " B1/B2:%.2f", Beta12);
		strcat(Modelname, buf);
	}
	if (Frequ_optn) strcat(Modelname, " F");
/*	printf("\"%s\"\n", Modelname); */
#endif

	Topting = FALSE;
} /* optimtpm */
