/*
 * pstree.c   Adachi, J.   1996.01.13
 * Copyright (C) 1995-1996 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "protml.h"

#define OTUNUMBERING 0

#if A4PAPER /* A4 size */
#define PAPERHEIGHT 825 /* pt 840 */
#define PAPERWIDTH  580 /* pt */
#define TREEHEIGHT  700 /* pt */
#define TREEWIDTH   450 /* pt */
#else /* US letter size (no test) */
#define PAPERHEIGHT 784 /* pt */
#define PAPERWIDTH  596 /* pt */
#define TREEHEIGHT  700 /* pt */
#define TREEWIDTH   450 /* pt */
#endif


void
psdicter(fp)
FILE *fp;
{
	fputs("%\n/$MolphyDict 200 dict def \n", fp);
	fputs("$MolphyDict begin\n", fp);
	fputs("$MolphyDict /mtrx matrix put\n", fp);
	fputs("/Fid /Helvetica-Bold def",fp);
	fputs(" % You can change the font of species name.\n",fp);
	fputs("/Fsc /Helvetica-BoldOblique def",fp);
	fputs(" % You can change the font of scientific name.\n",fp);
	fputs("/Fbp /Helvetica-Narrow def",fp);
	fputs(" % You can change the font of bootstrap probabilities.\n",fp);
	fputs("/FTR  /Times-Roman def\n", fp);
	fputs("/FTB  /Times-Bold def\n", fp);
	fputs("/FTI  /Times-Italic def\n", fp);
	fputs("/FTBI /Times-BoldItalic def\n", fp);
	fputs("/FH   /Helvetica def\n", fp);
	fputs("/FHB  /Helvetica-Bold def\n", fp);
	fputs("/FHO  /Helvetica-Oblique def\n", fp);
	fputs("/FHBO /Helvetica-BoldOblique def\n", fp);
	fputs("/FHN  /Helvetica-Narrow def\n", fp);
	fputs("/FS   /Symbol def\n", fp);
	fputs("/FBDI /Bookman-DemiItalic def\n", fp);
	fputs("/FNCSI /NewCenturySchlbk-Italic def\n", fp);
	fputs("/ff {findfont} bind def\n", fp);
	fputs("/sf {scalefont setfont} bind def\n", fp);
	fputs("/l  {lineto} bind def\n", fp);
	fputs("/m  {moveto} bind def\n", fp);
	fputs("/rl {rlineto} bind def\n", fp);
	fputs("/rm {rmoveto} bind def\n", fp);
	fputs("/s  {stroke} bind def\n", fp);
	fputs("/n  {newpath} bind def\n", fp);
	fputs("/c  {closepath} bind def\n", fp);
	fputs("/cp {charpath} bind def\n", fp);
	fputs("/sh {show} bind def\n", fp);
	fputs("/gs {gsave} bind def\n", fp);
	fputs("/gr {grestore} bind def\n", fp);
	fputs("/sg {setgray} bind def\n", fp);
	fputs("/gc {dup dup currentrgbcolor 4 -2 roll mul 4 -2 roll\n", fp);
	fputs("\tmul 4 -2 roll mul setrgbcolor} bind def\n", fp);
	fputs("/CS {dup stringwidth pop neg 2 div 0 rmoveto} def\n", fp);
	fputs("/RS {dup stringwidth pop neg 0 rmoveto} def\n", fp);
	fputs("/RDS {dup stringwidth exch neg exch neg rmoveto} def\n", fp);
	fputs("/BB {stringwidth pop dup neg 0 rl exch 0 exch rl 0 rl c 1 sg fill s 0 sg} def\n", fp);
	fputs("/RR {dup stringwidth pop neg 0 rmoveto show} def\n", fp);
#if 0
	fputs("/BP {false charpath gsave 0 setgray fill grestore stroke} def\n", fp);
#else
	fputs("/BP {show} def\n", fp);
#endif
#if 0
	fputs("/col0  {0 0 0 setrgbcolor} bind def\n", fp);
	fputs("/col1  {0 0 1 setrgbcolor} bind def\n", fp);
	fputs("/col2  {0 1 0 setrgbcolor} bind def\n", fp);
	fputs("/col3  {0 1 1 setrgbcolor} bind def\n", fp);
	fputs("/col4  {1 0 0 setrgbcolor} bind def\n", fp);
	fputs("/col5  {1 0 1 setrgbcolor} bind def\n", fp);
	fputs("/col6  {1 1 0 setrgbcolor} bind def\n", fp);
	fputs("/col7  {1 1 1 setrgbcolor} bind def\n", fp);
	fputs("/col8  {.68 .85  .9 setrgbcolor} bind def\n", fp);
	fputs("/col9  {  0 .39   0 setrgbcolor} bind def\n", fp);
	fputs("/col10 {.65 .17 .17 setrgbcolor} bind def\n", fp);
	fputs("/col11 {  1 .51   0 setrgbcolor} bind def\n", fp);
	fputs("/col12 {.63 .13 .94 setrgbcolor} bind def\n", fp);
	fputs("/col13 {  1 .75  .8 setrgbcolor} bind def\n", fp);
	fputs("/col14 { .7 .13 .13 setrgbcolor} bind def\n", fp);
	fputs("/col15 {  1 .84   0 setrgbcolor} bind def\n", fp);
#endif
	fputs("end % $MolphyDict\n", fp);
	fputs("/$MolphyBegin {$MolphyDict begin /$MolphyEnteredState save def} def\n", fp);
	fputs("/$MolphyEnd {$MolphyEnteredState restore end} def\n", fp);
	fputs("%%EndProlog\n%\n", fp);
} /* psdicter */


void
pstree(fp, tr)
FILE *fp;
Tree *tr;
{
	Node *cp, *rp, *kp, *bp, *ap;
	char *name, date[32];
	int len, ns, x, xmax, xs, xsmax, s, smax, y, yi, dy, depth, maxdepth;
	int fonts, fontc, height, width, xorigin, yorigin, db, rel, rfonts, iscale;
	double xscale, yscale, lscale, rfonth, fontw;
	ivector yf, yl;

	fputs("%!PS-Adobe-1.0 EPSF-1.0\n", fp);
	fputs("%%Title: (MOLPHY's tree file)\n", fp);
	fprintf(fp, "%%%%Creator: MOLPHY Version %s by Jun Adachi\n", VERSION);
	strftime(date, 32, "%c", localtime(&Ct0));
	fprintf(fp, "%%%%CreationDate: %s\n", date);
	/* fputs("%%For: Jun Adachi\n", fp); */
	fputs("%%Orientation: Portrait\n", fp);

	xscale = yscale = 1.0;
	ns = Numspc;
	dy = (int)(TREEHEIGHT / (ns + 3));
	if (dy > 20) {
		dy = 20;
		fonts = 12;
		rfonts = 8;
	} else if (dy > 12) {
		fonts = 10;
		rfonts = 8;
	} else if (dy > 6) {
		fonts = dy - 1;
		rfonts = fonts - 1;
	} else if (dy > 4) {
		fonts = dy;
		rfonts = fonts - 1;
	} else {
		dy = 4;
		fonts = 5;
		rfonts = fonts - 1;
	}
	fontc = (int)(fonts * 2.0 / 5.0);
	fontw = fonts * 0.5;
	rfonts = (int)(fonts*4/5);
	rfonth = rfonts * 0.7;
	db = 1 + (int)(fonts/5);
	height = y = dy * (ns + 3);

	x = xsmax = depth = maxdepth = 0;

	cp = rp = tr->rootp;
	do {
		cp = cp->isop->kinp;
		len = (int)(cp->length * 10.0 + 0.5);
		if (len == 0) len = 1;
		if (cp->descen) x += len;
		if (cp->isop == NULL) { /* external node */
			name = ((Sciname[cp->num] && (Sciname[cp->num][0] != '\0'))
                    ? Sciname[cp->num] : Identif[cp->num]); /* @@@OLIVER */
			s    = (int)(fontw * (strlen(name) + 1) + 10);
			if (Engname[cp->num] && Engname[cp->num][0] != '\0') { /* @@@OLIVER */
				s += (int)(fontw * (strlen(Engname[cp->num]) + 1));
			}
			xs = x + s;
			if (xs > xsmax) {
				xsmax = xs;
				xmax  = x;
				smax  = s;
			}
			cp = cp->kinp;
		} else { /* internal node */
			if (cp->descen) {
				depth++;
				if (depth > maxdepth) maxdepth = depth;
			} else {
				depth--;
			}
		}
		if (!cp->descen) x -= len;
	} while (cp != rp);

	lscale = (double)(TREEWIDTH - smax) / (double)xmax;
	width = TREEWIDTH;
	xorigin = (PAPERWIDTH  - width ) / 2;
	yorigin = (PAPERHEIGHT - height) / 2;

	fprintf(fp, "%%%%BoundingBox: %d %d %d %d\n",
		xorigin - 5, yorigin, xorigin + width, yorigin + height);
	fputs("%%Pages: 1\n", fp);
	fputs("%%EndComments\n", fp);

	psdicter(fp);

	fputs("$MolphyBegin\n", fp);

	fprintf(fp, "FTR ff 6 sf");
	fprintf(fp, " %d %d m", PAPERWIDTH - 10, PAPERHEIGHT - 10);
	strftime(date, 32, "%x", localtime(&Ct0));
#ifndef NJ
	fprintf(fp, " (%s %s %s %s %d OTUs %d sites %s) RDS sh %% COMMENT\n",
		Prog_name, VERSION, date, Modelname, Maxspc, Numsite, Comment);
#else
	fprintf(fp, " (%s %s %s %d OTUs %s) RDS sh %% COMMENT\n",
		Prog_name, VERSION, date, Numspc, Comment);
#endif
	fprintf(fp, "%% 0.01 setlinewidth n %d %d m %d %d l %d %d l %d %d l c s\n",
		xorigin-5, yorigin, xorigin+width, yorigin,
		xorigin+width, yorigin+height, xorigin-5, yorigin+height);

	fprintf(fp, "%d %d translate\n", xorigin, yorigin);
	fprintf(fp, "%.3f %.3f scale\n", xscale, yscale);
	fprintf(fp, "%d setlinecap %d setlinejoin\n", 2, 0);
	fprintf(fp, "%.3f setlinewidth\n", Numspc > 20 ? (0.5) : (1.0));
	/*
	if (Sciname[0][0] == '\0')
		fprintf(fp, "Fid ff %d sf\n", fonts);
	else
		fprintf(fp, "Fsc ff %d sf\n", fonts);
	*/
	yf = new_ivector(maxdepth + 1);
	yl = new_ivector(maxdepth + 1);
	x = depth = 0;

	cp = rp = tr->rootp;
	do {
		bp = cp;
		kp = cp->isop;
		cp = kp->kinp;
		len = (int)(cp->length * 10.0 * lscale + 0.5);
		if (len == 0) len = 1;
		if (cp->descen) x += len;
		if (cp->isop == NULL) { /* external node */
			y -= dy;
			fprintf(fp, "n %3d %3d m %3d %3d l s ", x - len, y, x, y);
			fprintf(fp, "  %3d %3d m", x + (int)fontw, y-fontc);
			fprintf(fp, " %% %3d\n", cp->num+1);
			if (Sciname[cp->num]== 0 || Sciname[cp->num][0] == '\0') { /* @@@OLIVER */
				fprintf(fp, " Fid ff %d sf (%s) sh", fonts, Identif[cp->num]);
			} else {
				fprintf(fp, " Fsc ff %d sf (%s) sh", fonts, Sciname[cp->num]);
			}
			if (Engname[cp->num] && Engname[cp->num][0] != '\0') { /* @@@OLIVER */
				fprintf(fp, " Fid ff %d sf ( %s) sh\n",fonts,Engname[cp->num]);
			} else {
				fprintf(fp, "\n");
			}
#if OTUNUMBERING
			fprintf(fp, "\t(  %d) sh %% otunumbering\n", cp->num+1);
#endif
			cp = cp->kinp;
			if (bp->descen)       yf[depth] = y;
			if (cp->isop->descen) yl[depth] = y;
			if (cp == rp->isop)   yf[depth] = y;
			if (cp == rp)         yl[depth] = y;
		} else { /* internal node */
			if (cp->descen) {
				depth++;
			} else {
				yi = (yf[depth] + yl[depth]) / 2;
				depth--;
				if (cp == rp->isop) yf[depth] = yi;
				if (cp == rp)       yl[depth] = yi;
				if (cp->isop->descen) {
					yl[depth] = yi;
				} else {
					for (ap = cp; ap->isop != cp; ap = ap->isop) ;
					if (ap->descen) yf[depth] = yi;
				}
				fprintf(fp, "n %3d %3d m %3d %3d l s ", x - len, yi, x, yi);
				fprintf(fp, "n %3d %3d m %3d %3d l s\t%% %3d\n",
					x, yf[depth+1], x, yl[depth+1], cp->num+1);
			}
		}
		if (!cp->descen) x -= len;
	} while (cp != rp);
	fprintf(fp, "n %3d %3d m %3d %3d l s\t%% 0\n", x, yf[depth], x, yl[depth]);
	y -= (dy * 1.0);

	if (100*lscale < 400) {
		iscale = 100;
	} else {
		iscale = 10;
	}
	fprintf(fp, "n %3d %3d m %3d %3d l %3d %3d l %3d %3d l s\n", x,y,
		x,y-fontc, x+(int)(iscale*lscale),y-fontc, x+(int)(iscale*lscale),y);
	fprintf(fp, "%%\nFH ff %d sf\n", fonts);
	fprintf(fp, "%3d %3d m (%s substitutions/site) sh\n",
			x, y-fontc-fonts, (iscale == 100 ? "0.1" : "0.01"));

#ifndef NJ
if (Relia_optn) {
	fprintf(fp, "%%\nFbp ff %d sf\n", rfonts); /* /FH */
	y = height;
	x = depth = 0;
	cp = rp = tr->rootp;
	do {
		bp = cp;
		kp = cp->isop;
		cp = kp->kinp;
		len = (int)(cp->length * 10.0 * lscale + 0.5);
		if (len == 0) len = 1;
		if (cp->descen) x += len;
		if (cp->isop == NULL) { /* external node */
			y -= dy;
			cp = cp->kinp;
			if (bp->descen)       yf[depth] = y;
			if (cp->isop->descen) yl[depth] = y;
			if (cp == rp->isop)   yf[depth] = y;
			if (cp == rp)         yl[depth] = y;
		} else { /* internal node */
			if (cp->descen) {
				depth++;
			} else {
				yi = (yf[depth] + yl[depth]) / 2;
				depth--;
				if (cp == rp->isop) yf[depth] = yi;
				if (cp == rp)       yl[depth] = yi;
				if (cp->isop->descen) {
					yl[depth] = yi;
				} else {
					for (ap = cp; ap->isop != cp; ap = ap->isop) ;
					if (ap->descen) yf[depth] = yi;
				}
				if (Relistat[cp->num - Maxspc] >= 0) {
					rel = (int)(Reliprob[cp->num - Maxspc][0]*100.0+0.5);
					fprintf(fp, "%3d %3d m %.2f (%d) BB ",
						x-db, yi+db, rfonth, rel);
					fprintf(fp, "%3d %3d m (%d) RR\t%% %3d\n",
						x-db, yi+db, rel, cp->num+1);
					/* fprintf(fp, "  %3d %3d m gs ", x-db, yi+db);
					fprintf(fp, "(%d) RS BP\n", rel); */
				}
			}
		}
		if (!cp->descen) x -= len;
	} while (cp != rp);
}
#endif /* NJ */

	free_ivector(yf);
	free_ivector(yl);

	fputs("%\nshowpage\n", fp);
	fputs("$MolphyEnd\n", fp);
	fputs("% %%EOF\n", fp);

} /* pstree */
