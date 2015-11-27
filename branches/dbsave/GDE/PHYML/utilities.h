/*

PHYML :  a program that  computes maximum likelihood  phylogenies from
DNA or AA homologous sequences

Copyright (C) Stephane Guindon. Oct 2003 onward

All parts of  the source except where indicated  are distributed under
the GNU public licence.  See http://www.opensource.org for details.

*/

#ifndef UTILITIES_H
#define UTILITIES_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define VERSION "v2.4.5"

extern int    NODE_DEG_MAX;
extern int    BRENT_ITMAX;
extern double BRENT_CGOLD;
extern double BRENT_ZEPS;
extern double MNBRAK_GOLD;
extern double MNBRAK_GLIMIT;
extern double MNBRAK_TINY;
extern double ALPHA_MIN;
extern double ALPHA_MAX;
extern double BL_MIN;
extern double BL_START;
extern double BL_MAX;
extern double MIN_DIFF_LK;
extern double GOLDEN_R;
extern double GOLDEN_C;
extern int    T_MAX_FILE;
extern int    T_MAX_LINE;
extern int    T_MAX_NAME;
extern int    T_MAX_SEQ;
extern int    N_MAX_INSERT;
extern int    N_MAX_OTU;
extern double UNLIKELY;
extern double NJ_SEUIL;
extern int    MAX_TOPO_DIST;
extern double ROUND_MAX;
extern double DIST_MAX;
extern int    LIM_SCALE;
extern double LIM_SCALE_VAL;
extern double AROUND_LK;
extern double PROP_STEP;
extern int    T_MAX_ALPHABET;
extern double MDBL_MAX;
extern double MDBL_MIN;
extern int    POWELL_ITMAX;
extern double LINMIN_TOL;

#define For(i,n)                     for(i=0; i<n; i++)
#define Fors(i,n,s)                  for(i=0; i<n; i+=s)
#define PointGamma(prob,alpha,beta)  PointChi2(prob,2.0*(alpha))/(2.0*(beta))
#define SHFT2(a,b,c)                 (a)=(b);(b)=(c);
#define SHFT3(a,b,c,d)               (a)=(b);(b)=(c);(c)=(d);
#define MAX(a,b)                     ((a)>(b)?(a):(b))
#define MIN(a,b)                     ((a)<(b)?(a):(b))
#define SIGN(a,b)                    ((b) > 0.0 ? fabs(a) : -fabs(a))
#define SHFT(a,b,c,d)                (a)=(b);(b)=(c);(c)=(d);

#define NT 0 /* nucleotides */
#define AA 1 /* amino acids */

#define ACGT 0 /* A,G,G,T encoding */
#define RY   1 /* R,Y     encoding */


#define  NODE_DEG_MAX          50
#define  BRENT_ITMAX          100
#define  BRENT_CGOLD    0.3819660
#define  BRENT_ZEPS        1.e-10
#define  MNBRAK_GOLD     1.618034
#define  MNBRAK_GLIMIT      100.0
#define  MNBRAK_TINY       1.e-20
#define  ALPHA_MIN           0.04
#define  ALPHA_MAX            100
#define  BL_MIN            1.e-10
#define  BL_START          1.e-03
#define  BL_MAX            1.e+05
#define  MIN_DIFF_LK       1.e-06
#define  GOLDEN_R      0.61803399
#define  GOLDEN_C  (1.0-GOLDEN_R)
#define  T_MAX_FILE           200
#define  T_MAX_LINE        100000
#define  T_MAX_NAME           100
#define  T_MAX_SEQ        1000000
#define  N_MAX_INSERT          20
#define  N_MAX_OTU           4000
#define  UNLIKELY          -1.e10
#define  NJ_SEUIL             0.1
#define  ROUND_MAX            100
#define  DIST_MAX            2.00
#define  AROUND_LK           50.0
#define  PROP_STEP            1.0
#define  T_MAX_ALPHABET       100
#define  MDBL_MIN   2.225074E-308
#define  MDBL_MAX   1.797693E+308
#define  POWELL_ITMAX         200
#define  LINMIN_TOL       2.0E-04
#define  LIM_SCALE              3
#define  LIM_SCALE_VAL     1.E-50
/*   LIM_SCALE =           300; */
/*   LIM_SCALE_VAL   = 1.E-500; */


/*********************************************************/

typedef struct __Node {
  struct __Node         **v; /* table of pointers to neighbor nodes. Dimension = 2 x n_otu - 3 */
  struct __Node ***bip_node; /* three lists of pointer to tip nodes. One list for each direction */
  struct __Edge         **b; /* table of pointers to neighbor branches */
  char          ***bip_name; /* three lists of tip node names. One list for each direction */
  int             *bip_size; /* Size of each of the three lists from bip_node */
  double                 *l; /* lengths of the (three or one) branches connected to one internal node */
  int                   num; /* node number */
  char                *name; /* taxon name (if exists) */
  int                   tax; /* tax = 1 -> external node, else -> internal node */
  int          check_branch; /* check_branch=1 is the corresponding branch is labelled with '*' */
  double             *score; /* score used in BIONJ to determine the best pair of nodes to agglomerate */
}node;


/*********************************************************/

typedef struct __Edge {
  /*
    syntax :  (node) [edge]
(left_1) .                   .(right_1)
          \ (left)  (right) /
           \._____________./
           /    [b_fcus]   \
          /                 \
(left_2) .                   .(right_2)

  */

  struct __Node               *left,*rght; /* node on the left/right side of the edge */
  int         l_r,r_l,l_v1,l_v2,r_v1,r_v2;
  /* these are directions (i.e., 0, 1 or 2): */
  /* l_r (left to right) -> left[b_fcus->l_r] = right */
  /* r_l (right to left) -> right[b_fcus->r_l] = left */
  /* l_v1 (left node to first node != from right) -> left[b_fcus->l_v1] = left_1 */
  /* l_v2 (left node to secnd node != from right) -> left[b_fcus->l_v2] = left_2 */
  /* r_v1 (right node to first node != from left) -> right[b_fcus->r_v1] = right_1 */
  /* r_v2 (right node to secnd node != from left) -> right[b_fcus->r_v2] = right_2 */

  int                                 num; /* branch number */
  double                                l; /* branch length */
  double                           best_l; /* best branch length found so far */
  double                            l_old; /* old branch length */

  int                           bip_score; /* score of the bipartition generated by the corresponding edge
					      bip_score = 1 iif the branch is fond in both trees to be compared,
					      bip_score = 0 otherwise. */
  double                         nj_score; /* score of the agglomeration that generated that branch in BIONJ */
  double                          diff_lk; /* difference of likelihood between the current topological
					      configuration at this branch and the best alternative one */

  int                       get_p_lk_left; /* 1 if the likelihood of the subtree on the left has to be computed */
  int                       get_p_lk_rght; /* 1 if the likelihood of the subtree on the right has to be computed */
  int                        ud_p_lk_left; /* 1 if the likelihood of the subtree on the left is up to date */
  int                        ud_p_lk_rght; /* 1 if the likelihood of the subtree on the right is up to date */
  double                         site_dlk; /* derivative of the likelihood (deprecated) */
  double                        site_d2lk; /* 2nd derivative of the likelihood (deprecated) */
  double                     *site_dlk_rr; /* derivative of the likelihood conditional on the current relative rate */
  double                    *site_d2lk_rr; /* 2nd derivative of the likelihood conditional on the current relative rate  */
  double        ***p_lk_left,***p_lk_rght; /* likelihoods of the subtree on the left and
					      right side (for each site and each relative rate category) */
  double **site_p_lk_rght, **site_p_lk_left; /* deprecated */
  double ***Pij_rr,***dPij_rr,***d2Pij_rr; /* matrix of change probabilities and its first and secnd derivates */

  double                              *ql; /* ql[0], ql[1], ql[2] give the likelihood of the three topological
					      configurations around that branch */
  int                           best_conf;   /* best topological configuration :
						((left_1,left_2),right_1,right_2) or
						((left_1,right_2),right_1,left_2) or
						((left_1,right_1),right_1,left_2)  */

  int                         num_st_left; /* number of the subtree on the left side */
  int                         num_st_rght; /* number of the subtree on the right side */


  /* Below are the likelihood scaling factors (used in functions
     `Get_All_Partial_Lk_Scale' in lk.c */
  int                          scale_left;
  int                          scale_rght;
  double            site_sum_scale_f_left;
  double            site_sum_scale_f_rght;
  double                site_scale_f_left;
  double                site_scale_f_rght;
  double                *sum_scale_f_left;
  double                *sum_scale_f_rght;


  double                          bootval; /* bootstrap value (if exists) */
}edge;

/*********************************************************/

typedef struct __Arbre {
  struct __Node                         *root; /* root node */
  struct __Node                       **noeud; /* array of nodes that defines the tree topology */
  struct __Edge                     **t_edges; /* array of edges */
  struct __Arbre                    *old_tree; /* old copy of the tree */
  struct __Arbre                   *best_tree; /* best tree found so far */
  struct __Model                         *mod; /* substitution model */
  struct __AllSeq                       *data; /* sequences */
  struct __Option                      *input; /* input parameters */
  struct __Matrix                        *mat; /* pairwise distance matrix */

  int                                 has_bip; /*if has_bip=1, then the structure to compare
						 tree topologies is allocated, has_bip=0 otherwise */
  double                          min_diff_lk; /* min_diff_lk is the minimum taken among the 2n-3
						  diff_lk values */

  int                              both_sides; /* both_sides=1 -> a pre-order and a post-order tree
						  traversals are required to compute the likelihood
						  of every subtree in the phylogeny*/
  int                                   n_otu; /* number of taxa */

  int                               curr_site; /* current site of the alignment to be processed */
  int                               curr_catg; /* current class of the discrete gamma rate distribution */
  double                           best_loglk; /* highest value of the loglikelihood found so far */

  double                            tot_loglk; /* loglikelihood */
  double                    *tot_loglk_sorted; /* used to compute tot_loglk by adding sorted terms to minimize CPU errors */
  double                          *tot_dloglk; /* first derivative of the likelihood with respect to
						  branch lengths */
  double                         *tot_d2loglk; /* second derivative of the likelihood with respect to
						  branch lengths */
  double                             *site_lk; /* vector of likelihoods at individual sites */

  double                    **log_site_lk_cat; /* loglikelihood at individual sites and for each class of rate*/

  double                      unconstraint_lk; /* unconstrained (or multinomial) likelihood  */

  int                                  n_swap; /* number of NNIs performed */
  int                               n_pattern; /* number of distinct site patterns */
  int                      has_branch_lengths; /* =1 iff input tree displays branch lengths */
  int                          print_boot_val; /* if print_boot_val=1, the bootstrap values are printed */
}arbre;


/*********************************************************/

typedef struct __Seq {
  char    *name; /* sequence name */
  int       len; /* sequence length */
  char    *state; /* sequence itself */
}seq;

/*********************************************************/


typedef struct __AllSeq {
  seq         **c_seq;             /* compressed sequences      */
  int          *invar;             /* 1 -> states are identical, 0 states vary */
  double        *wght;             /* # of each site in c_seq */
  int           n_otu;             /* number of taxa */
  int       clean_len;             /* uncrunched sequences lenghts without gaps */
  int      crunch_len;             /* crunched sequences lengths */
  double       *b_frq;             /* observed state frequencies */
  int        init_len;             /* length of the uncompressed sequences */
  int         *ambigu;             /* ambigu[i]=1 is one or more of the sequences at site
				      i display an ambiguous character */
  int       *sitepatt;             /* this array maps the position of the patterns in the
				      compressed alignment to the positions in the uncompressed
				      one */
}allseq;

/*********************************************************/

typedef struct __Matrix { /* mostly used in BIONJ */
  double    **P,**Q,**dist; /* observed proportions of transition, transverion and  distances
			       between pairs of  sequences */
  arbre              *tree; /* tree... */
  int              *on_off; /* on_off[i]=1 if column/line i corresponds to a node that has not
			       been agglomerated yet */
  int                n_otu; /* number of taxa */
  char              **name; /* sequence names */
  int                    r; /* number of nodes that have not been agglomerated yet */
  struct __Node **tip_node; /* array of pointer to the leaves of the tree */
  int             curr_int; /* used in the NJ/BIONJ algorithms */
  int               method; /* if method=1->NJ method is used, BIONJ otherwise */
}matrix;

/*********************************************************/

typedef struct __Model {
  int      whichmodel;
/*
 1 => JC69
 2 => K2P
 3 => F81
 4 => HKY85
 5 => F84
 6 => TN93
 7 => GTR
11 => Dayhoff
12 => JTT
13 => MtREV
*/
  int              ns; /* number of states (4 for ADN, 20 for AA) */
  double          *pi; /* states frequencies */
  int        datatype; /* 0->DNA, 1->AA */

  /* ADN parameters */
  double        kappa; /* transition/transversion rate */
  double       lambda; /* parameter used to define the ts/tv ratios in the F84 and TN93 models */
  double        alpha; /* gamma shapa parameter */
  double     *r_proba; /* probabilities of the substitution rates defined by the discrete gamma distribution */
  double          *rr; /* substitution rates defined by the discrete gamma distribution */
  int          n_catg; /* number of categories in the discrete gamma distribution */
  double       pinvar; /* proportion of invariable sites */
  int           invar; /* =1 iff the substitution model takes into account invariable sites */

  /* Below are 'old' values of some substitution parameters (see the comments above) */
  double    alpha_old;
  double    kappa_old;
  double   lambda_old;
  double   pinvar_old;

  char  *custom_mod_string; /* string of characters used to define custom
			       models of substitution */
  double       **rr_param; /* table of pointers to relative rate parameters of the GTR or custom model */
  double *rr_param_values; /* relative rate parameters of the GTR or custom model */
  int      **rr_param_num; /* each line of this 2d table gives a serie of equal relative rate parameter number */
                           /* A<->C : number 0 */
                           /* A<->G : number 1 */
                           /* A<->T : number 2 */
                           /* C<->G : number 3 */
                           /* C<->T : number 4 */
                           /* G<->T : number 5 */
                           /* For example, [0][2][3]
			                   [1]
					   [4][5]
                              corresponds to the model 010022, i.e.,
			      (A<->C = A<->T = C<->T) != (A<->G) != (C<->T = G<->T)
			   */
  int      *n_rr_param_per_cat; /* [3][1][2] for the previous example */
  int          n_diff_rr_param; /* number of different relative substitution rates in the custom model */

  int    update_eigen; /* update_eigen=1-> eigen values/vectors need to be updated */

  double    ***Pij_rr; /* matrix of change probabilities */
  double   ***dPij_rr; /* first derivative of the change probabilities with respect to branch length */
  double  ***d2Pij_rr; /* second derivative of the change probabilities with respect to branch length */


  int         seq_len; /* sequence length */
  /* AA parameters */
  /* see PMat_Empirical in models.c for AA algorithm explanation */
  double    *mat_Q; /* 20x20 amino-acids substitution rates matrix */
  double   *mat_Vr; /* 20x20 right eigenvectors of mat_Q */
  double   *mat_Vi; /* 20x20 inverse matrix of mat_Vr */
  double   *vct_ev; /* eigen values */
  double        mr; /* mean rate = branch length/time interval */
                    /* mr = -sum(i)(vct_pi[i].mat_Q[ii]) */
  double *vct_eDmr; /* diagonal terms of a 20x20 diagonal matrix */
                    /* term n = exp(nth eigenvalue of mat_Q / mr) */
  int     stepsize; /* stepsize=1 for nucleotide models, 3 for codon models */
  int        n_otu; /* number of taxa */
  struct __Optimiz *s_opt; /* pointer to parameters to optimize */
  int bootstrap; /* bootstrap values are computed if bootstrap > 0.
		    The value give the number of replicates */
  double      *user_b_freq; /* user-defined nucleotide frequencies */


}model;

/*********************************************************/

typedef struct __Option { /* mostly used in 'options.c' */
  char                   *seqfile; /* sequence file name */
  char                 *modelname; /* name of the model */
  struct __Model             *mod; /* substitution model */
  int                 interleaved; /* interleaved or sequential sequence file format ? */
  int                   inputtree; /* =1 iff a user input tree is used as input */
  struct __Arbre            *tree; /* pointer to the current tree */
  char             *inputtreefile; /* input tree file name */
  FILE                    *fp_seq; /* pointer to the sequence file */
  FILE             *fp_input_tree; /* pointer to the input tree file */
  FILE              *fp_boot_tree; /* pointer to the bootstrap tree file */
  FILE             *fp_boot_stats; /* pointer to the statistics file */
  int            print_boot_trees; /* =1 if the bootstrapped trees are printed in output */
  char           *phyml_stat_file; /* name of the statistics file */
  char           *phyml_tree_file; /* name of the tree file */
  char             *phyml_lk_file; /* name of the file in which the likelihood of the model is written */
  int   phyml_stat_file_open_mode; /* opening file mode for statistics file */
  int   phyml_tree_file_open_mode; /* opening file mode for tree file */
  int                 n_data_sets; /* number of data sets to be analysed */
  int                     n_trees; /* number of trees */
  int                     seq_len; /* sequence length */
  int            n_data_set_asked; /* number of bootstrap replicates */
  struct __Seq             **data; /* pointer to the uncompressed sequences */
  struct __AllSeq        *alldata; /* pointer to the compressed sequences */
  char                  *nt_or_cd; /* nucleotide or codon data ? (not used) */
}option;

/*********************************************************/

typedef struct __Optimiz { /* parameters to be optimised (mostly used in 'optimiz.c') */
  int           print; /* =1 -> verbose mode  */

  int       opt_alpha; /* =1 -> the gamma shape parameter is optimised */
  int       opt_kappa; /* =1 -> the ts/tv ratio parameter is optimised */
  int      opt_lambda; /* =1 -> the F84|TN93 model specific parameter is optimised */
  int      opt_pinvar; /* =1 -> the proportion of invariants is optimised */
  int       opt_bfreq; /* =1 -> the nucleotide frequencies are optimised */
  int    opt_rr_param; /* =1 -> the relative rate parameters of the GTR or the customn model are optimised */
  int  opt_free_param; /* if opt_topo=0 and opt_free_param=1 -> the numerical parameters of the
			  model are optimised. if opt_topo=0 and opt_free_param=0 -> no parameter is
			  optimised */
  int          opt_bl; /* =1 -> the branch lengths are optimised */
  int        opt_topo; /* =1 -> the tree topology is optimised */
  double      init_lk; /* initial loglikelihood value */
  int        n_it_max; /* maximum bnumber of iteration during an optimisation step */
  int        last_opt; /* =1 -> the numerical parameters are optimised further while the
			  tree topology remains fixed */
}optimiz;

/*********************************************************/

typedef struct __Qmat{
  double **u_mat;   /* right eigen vectors             */
  double **v_mat;   /* left eigen vectors = inv(u_mat) */
  double *root_vct; /* eigen values                    */
  double *q;        /* instantaneous rate matrix       */
}qmat;

/*********************************************************/

double bico(int n,int k);
double factln(int n);
double gammln(double xx);
double Pbinom(int N,int ni,double p);
void Plim_Binom(double pH0,int N,double *pinf,double *psup);
double LnGamma(double alpha);
double IncompleteGamma(double x,double alpha,double ln_gamma_alpha);
double PointChi2(double prob,double v);
double PointNormal(double prob);
int DiscreteGamma(double freqK[],double rK[],double alfa,double beta,int K,int median);
arbre *Read_Tree(char *s_tree);
void Make_All_Edges_Light(node *a,node *d);
void Make_All_Edges_Lk(node *a,node *d,arbre *tree);
void R_rtree(char *s_tree,node *pere,arbre *tree,int *n_int,int *n_ext);
void Clean_Multifurcation(char **subtrees,int current_deg,int end_deg);
char **Sub_Trees(char *tree,int *degree);
int Next_Par(char *s,int pos);
char *Write_Tree(arbre *tree);
void R_wtree(node *pere,node *fils,char *s_tree,arbre *tree);
void Init_Tree(arbre *tree);
void Make_Edge_Light(node *a,node *d);
void Init_Edge_Light(edge *b);
void Make_Edge_Dirs(edge *b,node *a,node *d);
void Make_Edge_Lk(node *a,node *d,arbre *tree);
void Make_Node_Light(node *n);
void Init_Node_Light(node *n);
void Make_Node_Lk(node *n);
seq **Get_Seq(option *input,int rw);
seq **Read_Seq_Sequential(FILE *in,int *n_otu);
seq **Read_Seq_Interleaved(FILE *in,int *n_otu);
int Read_One_Line_Seq(seq ***data,int num_otu,FILE *in);
void Uppercase(char *ch);
allseq *Compact_Seq(seq **data,option *input);
allseq *Compact_CSeq(allseq *data,model *mod);
void Get_Base_Freqs(allseq *data);
void Get_AA_Freqs(allseq *data);
arbre *Read_Tree_File(FILE *fp_input_tree);
void Init_Tree_Edges(node *a,node *d,arbre *tree,int *cur);
void Exit(char *message);
void *mCalloc(int nb,size_t size);
void *mRealloc(void *p,int nb,size_t size);
arbre *Make_Light_Tree_Struct(int n_otu);
int Sort_Double_Decrease(const void *a,const void *b);
void qksort(double *A,int ilo,int ihi);
void Print_Site(allseq *alldata,int num,int n_otu,char *sep,int stepsize);
void Print_Seq(seq **data,int n_otu);
void Print_CSeq(FILE *fp,allseq *alldata);
void Order_Tree_Seq(arbre *tree,seq **data);
void Order_Tree_CSeq(arbre *tree,allseq *data);
matrix *Make_Mat(int n_otu);
void Init_Mat(matrix *mat,allseq *data);
arbre *Make_Tree(allseq *data);
void Print_Dist(matrix *mat);
void Print_Node(node *a,node *d,arbre *tree);
void Share_Lk_Struct(arbre *t_full,arbre *t_empt);
void Init_Constant();
void Print_Mat(matrix *mat);
int Sort_Edges_Diff_Lk(arbre *tree,edge **sorted_edges,int n_elem);
void NNI(arbre *tree,edge *b_fcus,int do_swap);
void Swap(node *a,node *b,node *c,node *d,arbre *tree);
void Update_All_Partial_Lk(edge *b_fcus,arbre *tree);
void Update_SubTree_Partial_Lk(edge *b_fcus,node *a,node *d,arbre *tree);
double Update_Lk_At_Given_Edge(edge *b_fcus,arbre *tree);
void Update_PMat_At_Given_Edge(edge *b_fcus,arbre *tree);
allseq *Make_Seq(int n_otu,int len,char **sp_names);
allseq *Copy_CData(allseq *ori,model *mod);
optimiz *Alloc_Optimiz();
void Init_Optimiz(optimiz *s_opt);
int Filexists(char *filename);
FILE *Openfile(char *filename,int mode);
void Print_Fp_Out(FILE *fp_out,time_t t_beg,time_t t_end,arbre *tree,option *input,int n_data_set);
void Print_Fp_Out_Lines(FILE *fp_out,time_t t_beg,time_t t_end,arbre *tree,option *input,int n_data_set);
void Alloc_All_P_Lk(arbre *tree);
matrix *K2P_dist(allseq *data,double g_shape);
matrix *JC69_Dist(allseq *data,model *mod);
matrix *Hamming_Dist(allseq *data,model *mod);
int Is_Ambigu(char *state,int datatype,int stepsize);
void Check_Ambiguities(allseq *data,int datatype,int stepsize);
int Assign_State(char *c,int datatype,int stepsize);
void Bootstrap(arbre *tree);
void Update_BrLen_Invar(arbre *tree);
void Getstring_Stdin(char *file_name);
void Print_Freq(arbre *tree);
double Num_Derivatives_One_Param(double(*func)(arbre *tree),arbre *tree,double f0,double *param,double stepsize,double *err,int precise);
void Num_Derivative_Several_Param(arbre *tree,double *param,int n_param,double stepsize,double(*func)(arbre *tree),double *derivatives);
int Compare_Two_States(char *state1,char *state2,int state_size);
void Copy_One_State(char *from,char *to,int state_size);
model *Make_Model_Basic();
void Make_Model_Complete(model *mod);
model *Copy_Model(model *ori);
void Set_Defaults_Input(option *input);
void Set_Defaults_Model(model *mod);
void Set_Defaults_Optimiz(optimiz *s_opt);
void Copy_Optimiz(optimiz *ori,optimiz *cpy);
void Get_Bip(node *a,node *d,arbre *tree);
void Alloc_Bip(arbre *tree);
int Sort_Double_Increase(const void *a,const void *b);
int Sort_String(const void *a,const void *b);
void Compare_Bip(arbre *tree1,arbre *tree2);
void Test_Multiple_Data_Set_Format(option *input);
int Are_Compatible(char *statea,char *stateb,int stepsize,int datatype);
void Hide_Ambiguities(allseq *data);
void Print_Site_Lk(arbre *tree, FILE *fp);
#endif




