/*
 * abratio.c   Adachi, J.   1994.11.17
 * Copyright (C) 1992-1994 J. Adachi & M. Hasegawa, All rights reserved.
 */

double 
mlstar(ab)
double ab;
{
	int i, j, k, n, it, numloop, loop;
	double sumlk, sumd1, sumd2, lkld1, lkld2, lklhd, prod, vari, x;
	double arc, arcold, arcdiff, arcpre;
	dmattpmty tprob, tdif1, tdif2;
	dmatrix oprob, rprob;
	ivector dseqi;
	dvector opb;
	dvector tpb, td1, td2;
	double pn0, pn1, pn2, pn3;

	AlphaBeta = ab;
	tranprobmat();
	oprob = new_dmatrix(Numptrn, Tpmradix);
	rprob = new_dmatrix(Numptrn, Tpmradix);
	for (k = 0; k < Numptrn; k++) {
		for (i = 0; i < Tpmradix; i++) {
			for (j = 0, n = 0; j < Tpmradix; j++) {
				if (Seqconint[j][k] == i) n++;
			}
			x = (double)n / (double)Maxspc;
			x < 0.1 ? (oprob[k][i] = 0.1) : (oprob[k][i] = (double)n / Maxspc);
			rprob[k][i] = Freqtpm[i];
		}
	}
	numloop = 10;
	for (n = 0; n < Maxspc; n++) {
		dseqi = Seqconint[n];
		arc = 10.0;
		arcpre = arc;
		for (it = 0; it < numloop; it++) {
			tdiffmtrx(arc, tprob, tdif1, tdif2);
			tpb = *tprob; td1 = *tdif1; td2 = *tdif2;
			lkld1 = lkld2 = 0.0;
			for (k = 0; k < Numptrn; k++) {
				if ((j = dseqi[k]) >= 0) {
					opb = oprob[k];
					pn0 = Freqtpm[0] * opb[0];
					pn1 = Freqtpm[1] * opb[1];
					pn2 = Freqtpm[2] * opb[2];
					pn3 = Freqtpm[3] * opb[3];
					sumlk = pn0 * tpb[j  ] + pn1 * tpb[j+ 4]
						  + pn2 * tpb[j+8] + pn3 * tpb[j+12];
					sumd1 = pn0 * td1[j  ] + pn1 * td1[j+ 4]
						  + pn2 * td1[j+8] + pn3 * td1[j+12];
					sumd2 = pn0 * td2[j  ] + pn1 * td2[j+ 4]
						  + pn2 * td2[j+8] + pn3 * td2[j+12];
					sumd1 /= sumlk;
					lkld1 += sumd1 * Weight[k];
					lkld2 += (sumd2 / sumlk - sumd1 * sumd1) * Weight[k];
				}
			}
			vari = 1.0 / fabs(lkld2);
			arcold = arc;
			arcdiff = - (lkld1 / lkld2);
			arc += arcdiff;
			if (arc > Mlimit && arcpre < 10.0) arc = Llimit;
			if (arc < Llimit) arc = Llimit;
			if (arc > Ulimit) arc = Ulimit;
		/*	printf("mle %8.3f %8.3f %12.7f %12.7f %10.3f\n",
				arc, arcold, arcdiff, lkld1, lkld2); */
			if (fabs(arcold - arc) < EPSILON) break;
		}
	/*	if (Debug) */
			printf("mle%3d%8.3f%8.3f%12.7f%12.7f%12.7f%10.3f\n",
			it+1, arc, arcpre, arcpre-arc, sqrt(vari), lkld1, lkld2);
		tprobmtrx(arc, tprob);
		tpb = *tprob;
		for (k = 0; k < Numptrn; k++) {
			opb = rprob[k];
			if ((j = dseqi[k]) >= 0) {
				opb[0] *= tpb[j];
				opb[1] *= tpb[j+4];
				opb[2] *= tpb[j+8];
				opb[3] *= tpb[j+12];
			}
		}

	}
	for (k = 0, lklhd = 0.0; k < Numptrn; k++) {
		opb = rprob[k];
		sumlk = opb[0] + opb[1] + opb[2] + opb[3];
		lklhd += log(sumlk) * Weight[k];
	}
	free_dmatrix(oprob);
	free_dmatrix(rprob);
	return lklhd;
} /*_ mlstar */


void
abratio()
{
	double x1, x2, x3, f1, f2, f3, ab, eps;

	eps = 0.005;
	x1 = 10.0;
	x3 = 15.0;
	f1 = mlstar(x1);
	printf("%10.5f %10.5f\n", x1, f1);
	f3 = mlstar(x3);
	if (f1 > f3) {
		x2 = x1; f2 = f1;
		x1 = MINAB;
		f1 = mlstar(x1);
	} else {
		x2 = x3; f2 = f3;
		x3 = MAXAB;
		f3 = mlstar(x3);
	}
	AlphaBeta = optimab(x1, x2, x3, f1, f2, f3, mlstar, eps);
	if (!Ctacit_optn) printf("Optimize Alpha/Beta: %.3f\n", AlphaBeta);
} /*_ abratio */
