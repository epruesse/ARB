#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif


/* getseq.c */
void getsize P_((FILE *ifp, int *maxspc, int *numsite, char **commentp));
void getid P_((FILE *ifp, char **identif, char **sciname, char **engname, int *nl, int *notu));
void getsites P_((FILE *ifp, cmatrix seqchar, int numsite, int *nl, int *notu));
void getseq P_((FILE *ifp, char **identif, char **sciname, char **engname, cmatrix seqchar, int maxspc, int numsite));
void getidsites P_((FILE *ifp, cmatrix identif, cmatrix seqchar, int numsite, int *nl, int *notu));
void getseqs P_((FILE *ifp, char **identif, cmatrix seqchar, int maxspc, int numsite));
void getseqi P_((FILE *ifp, char **identif, cmatrix seqchar, int maxspc, int numsite));
void fputid P_((FILE *ofp, char *name, int maxcolumn));
void prsequence P_((FILE *ofp, char **identif, char **seqchar, int maxspc, int maxsite));

/* seqproc.c */
void convseq P_((imatrix seqconint, int maxspc, int numptrn));
void getfreqepm P_((cmatrix seqchar, double *freqemp, int maxspc, int maxsite));
void convfreq P_((double *freqemp));
void a_radixsort P_((cmatrix seqchar, ivector alias, int maxspc, int maxsite, int *numptrn));
void condenceseq P_((cmatrix seqchar, ivector alias, imatrix seqconint, ivector weight, int maxspc, int maxsite, int numptrn));
void getnumsites P_((imatrix seqconint, ivector numsites, ivector weight, int numspc, int numptrn));
void prcondenceseq P_((char **identif, imatrix seqconint, ivector weight, int numspc, int numsite, int numptrn));

/* mltree.c */
Node **new_npvector P_((int n));
void free_npvector P_((Node **v));
Tree *new_tree P_((int maxspc, int maxibrnch, int numptrn, imatrix seqconint));
int getbuftree P_((int numspc, cmatrix identif));
void changedistan P_((dmatrix distanmat, dvector distanvec, int numspc));
void getproportion P_((double *proportion, dvector distanvec, int maxpair));
Infotree *newinfotrees P_((int numtree, int buftree));
Infoaltree *newinfoaltrees P_((int numaltree, int buftree));
void getnumtree P_((FILE *ifp, int *numtree));
void getusertree P_((FILE *ifp, cvector strtree, int buftree));
Node *internalnode P_((Tree *tr, char **cpp, int *ninode, char *st));
void constructtree P_((Tree *tr, cvector strtree));
void prcurtree P_((Tree *tr));
void pathing P_((Tree *tr));
void lslength P_((Tree *tr, dvector distanvec, int numspc));
void slslength P_((Tree *tr, dmatrix dmat, int ns));
void fmlength P_((Tree *tr, dmatrix dmat, int ns));
void resulttree P_((Tree *tr));
void bootstrap P_((Infotree *infotrs, LPMATRIX lklptrn));
void tabletree P_((Infotree *infotrs, LPMATRIX lklptrn));
void tableinfo P_((Infotree *infotrs));
void rerootq P_((Tree *tr, int numspc));
void outlklhd P_((LPMATRIX lklptrn));
void putsortseq P_((Tree *tr));

/* altree.c */
Tree *new_atree P_((int maxspc, int maxibrnch, int numptrn, imatrix seqconint));
Node *new_dnode P_((void));
Node *new_anode P_((void));
Node ***new_nodematrix P_((int nrow, int ncol));
void free_nodematrix P_((Node ***m));
void aproxlkl P_((Tree *tr));
void aproxlkl P_((Tree *tr));
void praproxlkl P_((Tree *tr));
void aproxtree P_((Tree *tr, int ntr));
void wedge P_((Tree *tr, int onode, Node **poolnode, Node **addposition, ivector poolorder, Node *op));
void autoconstruction P_((Tree *tr, int onode, Node **poolnode, Node **addposition, ivector poolorder));
Node *inbranode P_((Tree *tr, char **cpp, int *nenode, int numorder, Node ***poolnode2, cvector st));
void streeinit P_((Tree *tr, cvector strtree, Node **poolnode, Node **addposition, ivector poolorder));
void atreeinit P_((Tree *tr, Node **poolnode, Node **addposition, ivector poolorder));
void tablealtree P_((int nt));

/* qltree.c */
Infoqltree *newinfoqltrees P_((int n, int maxbrnch));
Infoaddtree *newinfoaddtree P_((int buftree));
void initturn P_((Tree *tr));
void randturn P_((Tree *tr));
void convertdistan P_((Tree *tr, int numspc, dmatrix distanmat, dvector distanvec));
void praproxlkl2 P_((FILE *fp, Tree *tr));
int addotu P_((Tree *tr, Node *cp, Node *np, Node *ip, int cnspc));
int addotual P_((Tree *tr, Node *cp, Node *np, Node *ip, dvector lengs));
void roundtree P_((Tree *tr, int cnspc, Infoqltree *infoqltrees, Infoqltree *qhead, Infoqltree *qtail));
void qtreeinit P_((Tree *tr));
void tableaddtree P_((Infoaddtree *head, int numaddtree));

/* sltree.c */
Tree *new_stree P_((int maxspc, int maxibrnch, int numptrn, imatrix seqconint));
Infosltree *newinfosltrees P_((int num, int maxbrnch));
void insertbranch P_((Node *ibp, Node *np));
void deletebranch P_((Node *ibp, Node *np));
void movebranch P_((Node *jbp, Node *ip));
void removebranch P_((Node *jbp, Node *ip));
void subpathing P_((Node *np));
void copylength P_((Tree *tr, dvector lengs));
Node *sdml P_((Tree *tr, Node *op));
void decomposition P_((Tree *tr, int n, Infosltree *infosltrees));
void stardecomp P_((Tree *tr, int maxibrnch));
dcube new_dcubesym P_((int nrow, int ncol));
void free_dcubesym P_((dcube c));
void ystardecomp P_((Tree *tr));
void xstardecomp P_((Tree *tr));

/* njtree.c */
Tree *new_njtree P_((int maxspc, int maxibrnch, int numptrn, imatrix seqconint));
void free_njtree P_((Tree *tr, int maxspc, int maxibrnch));
double emledis P_((double dis, Node *ip, Node *kp));
double imledis P_((double dis, Node *ip, Node *kp));
void redmat P_((dmatrix dmat, double dij, Node **psotu, ivector otu, int restsp, int ii, int jj, int ns));
void enjtree P_((Tree *tr, dmatrix distan, int ns, boolean flag));

/* njmtree.c */
void initsubplkl P_((Node *op));
void mlepartlen P_((Node *ip, Node *jp, Node **rotup, int nr, int ns));
void remldmat P_((dmatrix dmat, double dij, Node **psotu, Node **rotup, int otui, int otuj, int ns));
void njmtree P_((Tree *tr, dmatrix distan, int ns, boolean flag));

/* prtree.c */
void putctopology P_((Tree *tr));
void fputctopology P_((FILE *fp, Tree *tr));
void fputcphylogeny P_((FILE *fp, Tree *tr));
void prtopology P_((Tree *tr));
void strctree P_((Tree *tr, char *ltree));

/* pstree.c */
void psdicter P_((FILE *fp));
void pstree P_((FILE *fp, Tree *tr));

/* matrixut.c */
void maerror P_((char *message));
fvector new_fvector P_((int n));
fmatrix new_fmatrix P_((int nrow, int ncol));
fcube new_fcube P_((int ntri, int nrow, int ncol));
void free_fvector P_((fvector v));
void free_fmatrix P_((fmatrix m));
void free_fcube P_((fcube c));
dvector new_dvector P_((int n));
dmatrix new_dmatrix P_((int nrow, int ncol));
dcube new_dcube P_((int ntri, int nrow, int ncol));
void free_dvector P_((dvector v));
void free_dmatrix P_((dmatrix m));
void free_dcube P_((dcube c));
cvector new_cvector P_((int n));
cmatrix new_cmatrix P_((int nrow, int ncol));
ccube new_ccube P_((int ntri, int nrow, int ncol));
void free_cvector P_((cvector v));
void free_cmatrix P_((cmatrix m));
void free_ccube P_((ccube c));
ivector new_ivector P_((int n));
imatrix new_imatrix P_((int nrow, int ncol));
icube new_icube P_((int ntri, int nrow, int ncol));
void free_ivector P_((ivector v));
void free_imatrix P_((imatrix m));
void free_icube P_((icube c));

/* mygetopt.c */
int mygetopt P_((int argc, char **argv, char *optstring));

/* protml.c */
void copyright P_((void));
void usage P_((void));
void helpinfo P_((void));
int main P_((int argc, char **argv));
void header P_((FILE *ofp, int *maxspc, int *numsite, char **commentp));
void headerd P_((FILE *ofp, int *maxspc, int *numsite, char **commentp));
void pml P_((FILE *ifp, FILE *ofp));

/* protproc.c */
int isacid P_((int c));
int acid2int P_((int c));
char acid2chint P_((int c));
char chint2acid P_((int c));
char int2acid P_((int i));

/* dyhfjtt.c */
void dyhfjtt P_((dmattpmty r, double *f, boolean flag));

/* mtrev24.c */
void mtrev P_((dmattpmty r, double *f));

/* tranprb.c */
void elmhes P_((dmattpmty a, int *ordr, int n));
void eltran P_((dmattpmty a, dmattpmty zz, int *ordr, int n));
void hqr2 P_((int n, int low, int hgh, int *err, dmattpmty h, dmattpmty zz, double *wr, double *wi));
void readrsrf P_((dmattpmty r, dvectpmty f, int n));
void tpmonepam P_((dmattpmty a, double *f));
void luinverse P_((dmattpmty omtrx, dmattpmty imtrx, int size));
void mproduct P_((dmattpmty am, dmattpmty bm, dmattpmty cm, int na, int nb, int nc));
void preigen P_((void));
void checkevector P_((dmattpmty imtrx, int nn));
void getrsr P_((dmattpmty a, dvectpmty ftpm));
void tranprobmat P_((void));
void varitpm P_((void));
void prfreq P_((void));
void tprobmtrx P_((double arc, dmattpmty tpr));
void tprobmtrxt P_((double arc, dmattpmty tpr));
void tdiffmtrx P_((double arc, dmattpmty tpr, dmattpmty td1, dmattpmty td2));
void tprobmtrx2 P_((double arc, dmattpmty tpr));
void tdiffmtrx2 P_((double arc, dmattpmty tpr, dmattpmty td1, dmattpmty td2));

/* distan.c */
void distance P_((dmatrix distanmat, cmatrix seqchar, int maxspc, int numsite));
void lddistance P_((dmatrix distanmat, cmatrix seqchar, int maxspc, int numsite));
void tdistan P_((ivector seqi, ivector seqj, dmatrix probk, ivector weight, int nptrn, double *len, double *lvari, dcube triprob));
void tdistan2 P_((ivector seqi, ivector seqj, ivector seqk, ivector seqw, int nsite, double *len, double *lvari, dcube triprob));
void tridistance P_((dmatrix distanmat, imatrix seqconint, ivector weight, int maxspc, int numptrn));
void tridistance2 P_((dmatrix distanmat, cmatrix seqchar, int maxspc, int numsite));
void putdistance P_((cmatrix identif, cmatrix sciname, cmatrix engname, dmatrix distanmat, int maxspc));
void checkseq P_((imatrix seqconint, int maxspc, int numptrn));

/* mlklhd.c */
double probnormal P_((double z));
double uprobnormal P_((double z));
void copypart1 P_((Node *op, Node *cp));
void prodpart1 P_((Node *op, Node *cp));
void prodpart P_((Node *op));
void partilkl P_((Node *op));
void partelkl P_((Node *op));
void partelkl2 P_((Node *op));
void initpartlkl P_((Tree *tr));
void regupartlkl P_((Tree *tr));
void mlibranch P_((Node *op, double eps, int nloop));
void mlebranch P_((Node *op, double eps, int nloop));
void mlebranch2 P_((Node *op, double eps, int nloop));
void evallkl P_((Node *op));
Node *mlikelihood P_((Tree *tr));
void ribranch P_((Node *op));
Node *relibranch P_((Node *op));
void mlvalue P_((Tree *tr, Infotree *infotrs));
void reroot P_((Tree *tr, Node *rp));
void sorttree P_((Tree *tr, Node *rp));
void chroot P_((Tree *tr, int s1, int s2));
void noexch P_((Node *rp, ivector exchstate));
void reliml P_((Tree *tr, Node *op, double lklorg, LPVECTOR mlklptrn, double *rel));
void localbp P_((dmatrix reliprob, LPVECTOR mlklptrn, LPCUBE rlklptrn, ivector whichml, int nb, int ns));
void reliabranch P_((Tree *tr));
void annealing P_((Tree *tr));
void qlrsearch P_((Tree *tr));

#undef P_
