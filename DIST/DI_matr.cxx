#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <math.h>
#include <time.h>
#include <limits.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_global.hxx>
#include <awt.hxx>

#include <awt_tree.hxx>
#include "dist.hxx"
#include <BI_helix.hxx>
#include <awt_csp.hxx>
#include <CT_ctree.hxx>
#include "di_matr.hxx"
#include "di_protdist.hxx"
#include "di_view_matrix.hxx"

// --------------------------------------------------------------------------------

#define AWAR_DIST_FILTER_PREFIX      AWAR_DIST_PREFIX "filter/"
#define AWAR_DIST_COLUMN_STAT_PREFIX AWAR_DIST_PREFIX "col_stat/"
#define AWAR_DIST_TREE_PREFIX        AWAR_DIST_PREFIX "tree/"

#define AWAR_DIST_MATRIX_DNA_BASE    AWAR_DIST_PREFIX "dna/matrix"
#define AWAR_DIST_MATRIX_DNA_ENABLED "tmp/" AWAR_DIST_MATRIX_DNA_BASE "/enable"

#define AWAR_DIST_WHICH_SPECIES AWAR_DIST_PREFIX "which_species"
#define AWAR_DIST_ALIGNMENT     AWAR_DIST_PREFIX "alignment"

#define AWAR_DIST_FILTER_ALIGNMENT AWAR_DIST_FILTER_PREFIX "alignment"
#define AWAR_DIST_FILTER_NAME      AWAR_DIST_FILTER_PREFIX "name"
#define AWAR_DIST_FILTER_FILTER    AWAR_DIST_FILTER_PREFIX "filter"
#define AWAR_DIST_FILTER_SIMPLIFY  AWAR_DIST_FILTER_PREFIX "simplify"

#define AWAR_DIST_COLUMN_STAT_ALIGNMENT AWAR_DIST_COLUMN_STAT_PREFIX "alignment"
#define AWAR_DIST_COLUMN_STAT_NAME      AWAR_DIST_COLUMN_STAT_PREFIX "name"

#define AWAR_DIST_TREE_STD_NAME  AWAR_DIST_COLUMN_STAT_PREFIX "tree_name"
#define AWAR_DIST_TREE_LIST_NAME AWAR_DIST_COLUMN_STAT_PREFIX "list_tree_name"
#define AWAR_DIST_TREE_SORT_NAME AWAR_DIST_COLUMN_STAT_PREFIX "sort_tree_name"
#define AWAR_DIST_TREE_COMP_NAME AWAR_DIST_COLUMN_STAT_PREFIX "compr_tree_name"

#define AWAR_DIST_BOOTSTRAP_COUNT AWAR_DIST_PREFIX "bootstrap/count"

#define AWAR_DIST_CANCEL_CHARS AWAR_DIST_PREFIX "cancel/chars"

#define AWAR_DIST_SAVE_MATRIX_TYPE     AWAR_DIST_SAVE_MATRIX_BASE "/type"
#define AWAR_DIST_SAVE_MATRIX_FILENAME AWAR_DIST_SAVE_MATRIX_BASE "/file_name"

// --------------------------------------------------------------------------------

PHMATRIX   *PHMATRIX::ROOT = 0;
PH_dmatrix *ph_dmatrix     = 0;
AWT_csp    *global_csp     = 0;
AW_CL       dist_filter_cl = 0;

AP_matrix DI_dna_matrix(AP_MAX);

static void delete_matrix_cb(AW_root *dummy)
{
    AWUSE(dummy);
    delete PHMATRIX::ROOT;
    PHMATRIX::ROOT = 0;
    if (ph_dmatrix){
        ph_dmatrix->resized();      // @@@ OLIVER
        ph_dmatrix->display();
    }
}

static AW_window *create_dna_matrix_window(AW_root *aw_root){
    AW_window_simple *aws = 0;
    aws = new AW_window_simple();
    aws->init( aw_root, "SET_DNA_MATRIX","SET MATRIX");
    aws->auto_increment(50,50);
    aws->button_length(10);
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE");

    aws->callback(AW_POPUP_HELP,(AW_CL)"user_matrix.hlp");
    aws->create_button("HELP", "HELP");

    aws->at_newline();

    DI_dna_matrix.create_input_fields(aws,AWAR_DIST_MATRIX_DNA_BASE);
    aws->window_fit();
    return aws;
}

void create_matrix_variables(AW_root *aw_root, AW_default def)
{
    GB_transaction dummy(gb_main);
    DI_dna_matrix.set_description("","");
    DI_dna_matrix.x_description[AP_A] = strdup("A");
    DI_dna_matrix.x_description[AP_C] = strdup("C");
    DI_dna_matrix.x_description[AP_G] = strdup("G");
    DI_dna_matrix.x_description[AP_T] = strdup("TU");
    DI_dna_matrix.x_description[AP_S] = strdup("GAP");

    DI_dna_matrix.y_description[AP_A] = strdup("A");
    DI_dna_matrix.y_description[AP_C] = strdup("C");
    DI_dna_matrix.y_description[AP_G] = strdup("G");
    DI_dna_matrix.y_description[AP_T] = strdup("TU");
    DI_dna_matrix.y_description[AP_S] = strdup("GAP");

    DI_dna_matrix.create_awars(aw_root,AWAR_DIST_MATRIX_DNA_BASE);

    aw_root->awar_int(AWAR_DIST_MATRIX_DNA_ENABLED, 0)->add_callback(delete_matrix_cb); // user matrix disabled by default
    {
        GBDATA *gbd = GB_search((GBDATA *)aw_root->application_database, AWAR_DIST_MATRIX_DNA_BASE, GB_FIND);
        GB_add_callback(gbd, GB_CB_CHANGED, (GB_CB)delete_matrix_cb, 0);
    }

    aw_root->awar_string("tmp/dummy_string", "0", def);

    aw_root->awar_string(AWAR_DIST_WHICH_SPECIES, "marked", def)->add_callback(delete_matrix_cb);
    {
        char *default_ali = GBT_get_default_alignment(gb_main);
        aw_root->awar_string( AWAR_DIST_ALIGNMENT,"",def)      ->add_callback(delete_matrix_cb)->write_string(default_ali);
        free(default_ali);
    }
    aw_root->awar_string(AWAR_DIST_FILTER_ALIGNMENT, "none", def);
    aw_root->awar_string(AWAR_DIST_FILTER_NAME,      "none", def);
    aw_root->awar_string(AWAR_DIST_FILTER_FILTER,    "",     def)->add_callback(delete_matrix_cb);
    aw_root->awar_int   (AWAR_DIST_FILTER_SIMPLIFY,  0,      def)->add_callback(delete_matrix_cb);

    aw_root->awar_string(AWAR_DIST_COLUMN_STAT_NAME,      "none", def); 
    aw_root->awar_string(AWAR_DIST_COLUMN_STAT_ALIGNMENT, "none", def); 

    aw_root->awar_string(AWAR_DIST_CANCEL_CHARS, ".", def)->add_callback(delete_matrix_cb); 
    aw_root->awar_int   (AWAR_DIST_CORR_TRANS,   0,   def)->add_callback(delete_matrix_cb); 

    aw_root->awar(AWAR_DIST_FILTER_ALIGNMENT)->map(AWAR_DIST_ALIGNMENT);
    aw_root->awar(AWAR_DIST_COLUMN_STAT_ALIGNMENT)->map(AWAR_DIST_ALIGNMENT);

    aw_create_selection_box_awars(aw_root, AWAR_DIST_SAVE_MATRIX_BASE, ".", "", "infile", def);
    aw_root->awar_int(  AWAR_DIST_SAVE_MATRIX_TYPE,    0,  def);

    aw_root->awar_string( AWAR_DIST_TREE_STD_NAME,"tree_temp",def);
    aw_root->awar_string(AWAR_DIST_TREE_LIST_NAME, "?????", def);
    aw_root->awar_string(AWAR_DIST_TREE_SORT_NAME, "?????", def)->add_callback(delete_matrix_cb);
    aw_root->awar_string(AWAR_DIST_TREE_COMP_NAME, "?????", def)->add_callback(delete_matrix_cb);
    
    aw_root->awar_int(AWAR_DIST_BOOTSTRAP_COUNT, 1000, def); 

    // aw_root->awar_string( "dist/compress/tree_name","tree_main",def);
    // aw_root->awar_float( "dist/alpha",1.0,def);

    GB_push_transaction(gb_main);

    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GB_add_callback(gb_species_data,GB_CB_CHANGED,(GB_CB)delete_matrix_cb,0);

    GB_pop_transaction(gb_main);

}

PHENTRY::PHENTRY(GBDATA *gbd,class PHMATRIX *phmatri)
{
    memset((char *)this,0,sizeof(PHENTRY));
    phmatrix = phmatri;

    GBDATA *gb_ali = GB_find(gbd,phmatrix->use,0,down_level);
    if (!gb_ali) return;
    GBDATA *gb_data = GB_find(gb_ali,"data",0,down_level);
    if (!gb_data) return;

    char *seq = GB_read_char_pntr(gb_data);
    if (phmatrix->is_AA) {
        sequence = (AP_sequence *)(sequence_protein = new AP_sequence_simple_protein(phmatrix->tree_root));
    }else{
        sequence = (AP_sequence *)(sequence_parsimony = new AP_sequence_parsimony(phmatrix->tree_root));
    }
    sequence->set(seq);

    GBDATA *gb_name = GB_find(gbd,"name",0,down_level);
    name = GB_read_string(gb_name);

    GBDATA *gb_full_name = GB_find(gbd,"full_name",0,down_level);
    if (gb_full_name) full_name = GB_read_string(gb_full_name);

}

PHENTRY::PHENTRY(char *namei,class PHMATRIX *phmatri)
{
    memset((char *)this,0,sizeof(PHENTRY));
    phmatrix = phmatri;
    this->name = strdup(namei);
}

PHENTRY::~PHENTRY(void)
{
    delete sequence_protein;
    delete sequence_parsimony;
    free(name);
    free(full_name);

}

PHMATRIX::PHMATRIX(GBDATA *gb_maini,AW_root *awr)
{
    memset((char *)this,0,sizeof(PHMATRIX));
    aw_root = awr;
    gb_main = gb_maini;
}

char *PHMATRIX::unload(void)
{
    free(use);
    use = 0;
    long i;
    for (i=0;i<nentries;i++){
        delete entries[i];
    }
    delete tree_root;
//     delete [] entries;
    free(entries);
    entries = 0;
    nentries = 0;
    return 0;
}

PHMATRIX::~PHMATRIX(void)
{
    unload();
    delete matrix;
}

extern "C" int qsort_strcmp(const void *str1ptr, const void *str2ptr) {
    GB_CSTR s1 = *(GB_CSTR*)str1ptr;
    GB_CSTR s2 = *(GB_CSTR*)str2ptr;

    return strcmp(s1, s2);
}

char *PHMATRIX::load(char *usei,AP_filter *filter,AP_weights *weights,AP_smatrix *,int all, GB_CSTR sort_tree_name, bool show_warnings)
{
    delete tree_root;
    tree_root = new AP_tree_root(gb_main,0,0);
    tree_root->filter = filter;
    tree_root->weights = weights;
    this->use = strdup(usei);
    this->gb_main = ::gb_main;
    GB_push_transaction(gb_main);

    seq_len          = GBT_get_alignment_len(gb_main,use);
    is_AA            = GBT_is_alignment_protein(gb_main,use);
    gb_species_data  = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    entries_mem_size = 1000;

    entries = (PHENTRY **)calloc(sizeof(PHENTRY*),(size_t)entries_mem_size);

    nentries = 0;
    GBDATA *gb_species;
    PHENTRY *phentry;

    int tree_size;
    GBT_TREE *sort_tree = GBT_read_tree_and_size(gb_main, sort_tree_name, sizeof(GBT_TREE), &tree_size);
    tree_size++;
    GB_CSTR *species_in_sort_tree = 0;

    int no_of_species = all ? GBT_recount_species(gb_main) : GBT_count_marked_species(gb_main);
    int in_tree_species = 0;
    int out_tree_species = no_of_species;
    int unknown_species_in_tree = 0;

    if (!sort_tree) {
        if (show_warnings) {
            aw_message("No valid tree given to sort matrix (using default database order)");
        }
    }
    else {
        species_in_sort_tree = GBT_get_species_names_of_tree(sort_tree);

        for (int i=0; i<tree_size; i++) { // read all marked species in tree
            gb_species = GBT_find_species_rel_species_data(gb_species_data, species_in_sort_tree[i]);
            if (!gb_species) {
                if (show_warnings) {
                    aw_message(GB_export_error("Species '%s' found in tree '%s' but NOT in database.",
                                               species_in_sort_tree[i], sort_tree_name));
                }
                unknown_species_in_tree++;
                continue;
            }
            if (all || GB_read_flag(gb_species)) {
                in_tree_species++;
                phentry = new PHENTRY(gb_species,this);
                if (phentry->sequence) {    // a species found
                    entries[nentries++] = phentry;
                    if (nentries == entries_mem_size) {
                        entries_mem_size +=1000;
                        entries = (PHENTRY **)realloc((MALLOC_T)entries,(size_t)(sizeof(PHENTRY*)*entries_mem_size));
                    }
                }else{
                    delete phentry;
                }
            }
        }
        out_tree_species = no_of_species-in_tree_species; // # of species not loaded yet (because not in tree)
        gb_assert(out_tree_species>=0);
        if (out_tree_species) {
            qsort(species_in_sort_tree, tree_size, sizeof(*species_in_sort_tree), qsort_strcmp); // sort species names
#if defined(DEBUG) && 0
            printf("Sorted tree:\n");
            for (int x=0; x<tree_size; x++) {
                printf("- %s\n", species_in_sort_tree[x]);
            }
#endif
        }
    }

    if (unknown_species_in_tree && show_warnings) {
        aw_message(GB_export_error("We found %i species in tree '%s' which are not in database.\n"
                                   "This does not affect the current calculation, but you should think about it.",
                                   unknown_species_in_tree, sort_tree_name));
    }

    gb_assert(out_tree_species>=0);
    if (out_tree_species) { // there are species outside the tree (or no valid sort tree selected) => load all marked species not loaded yet
        int count = 0;

        if (all) gb_species = GBT_first_species_rel_species_data(gb_species_data);
        else     gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);

        while (gb_species) {
            int load_species = 0;

            if (in_tree_species) { // test if species already loaded
                char *species_name = 0;
                GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
                if (gb_name) {
                    species_name = GB_read_string(gb_name);
                    if (bsearch(&species_name, species_in_sort_tree, tree_size, sizeof(*species_in_sort_tree), qsort_strcmp)==0) {
                        load_species = 1;
#if defined(DEBUG) && 0
                        printf("Not in tree: %s\n", species_name);
#endif
                    }
                    free(species_name);
                }
            }
            else {
                load_species = 1;
            }

            if (load_species) {
                count++;
                phentry = new PHENTRY(gb_species,this);
                if (phentry->sequence) {    // a species found
                    entries[nentries++] = phentry;
                    if (nentries == entries_mem_size) {
                        entries_mem_size +=1000;
                        entries = (PHENTRY **)realloc((MALLOC_T)entries,(size_t)(sizeof(PHENTRY*)*entries_mem_size));
                    }
                }else{
                    delete phentry;
                }
            }

            if (all) gb_species = GBT_next_species(gb_species);
            else     gb_species = GBT_next_marked_species(gb_species);
        }
    }

    free(species_in_sort_tree);
    if (sort_tree) GBT_delete_tree(sort_tree);

    GB_pop_transaction(gb_main);

    return 0;
}


/******************************************************************************************
            Calculate the global rate_matrix
******************************************************************************************/
void PHMATRIX::clear(PH_MUT_MATR &hits)
{
    int i,j;
    for (i=0;i<AP_MAX;i++){
        for (j=0;j<AP_MAX;j++){
            hits[i][j] = 0;
        }
    }
}

void PHMATRIX::make_sym(PH_MUT_MATR &hits)
{
    int i,j;
    for (i=AP_A;i<AP_MAX;i*=2){
        for (j=AP_A;j<=i;j*=2){
            hits[i][j] = hits[j][i] = hits[i][j] + hits[j][i];
        }
    }
}

void PHMATRIX::rate_write(PH_MUT_MATR &hits,FILE *out){
    int i,j;
    for (i=AP_A;i<AP_MAX;i*=2){
        for (j=AP_A;j<AP_MAX;j*=2){
            fprintf(out,"%5li ",hits[i][j]);
        }
        fprintf(out,"\n");
    }
}

long *PHMATRIX::create_helix_filter(BI_helix *helix,AP_filter *filter){
    long *result = (long *)calloc(sizeof(long),(size_t)filter->real_len);
    long *temp = (long *)calloc(sizeof(long),(size_t)filter->real_len);
    int i,count;
    count = 0;
    for (i=0;i<filter->filter_len;i++) {
        if (filter->filter_mask[i]) {
            temp[i] = count;
            if (helix->entries[i].pair_type== HELIX_PAIR) {
                result[count] = helix->entries[i].pair_pos;
            }else{
                result[count] = -1;
            }
            count++;
        }
    }
    while (--count >=0){
        if (result[count] >= 0) {
            result[count] = temp[result[count]];
        }
    }
    free((char *)temp);
    return result;
}

GB_ERROR PHMATRIX::calculate_rates(PH_MUT_MATR &hrates,PH_MUT_MATR &nrates,PH_MUT_MATR &pairs,long *filter){

    if (nentries<=1) {
        return "There are no species selected";
    }
    register long   row,col,pos,s_len;
    register char *seq1,*seq2;
    s_len = tree_root->filter->real_len;
    this->clear(hrates);
    this->clear(nrates);
    this->clear(pairs);
    for (row = 0; row<nentries;row++){
        double gauge = (double)row/(double)nentries;
        if (aw_status(gauge*gauge)) return "Aborted";
        for (col=0; col<row; col++) {
            seq1 = entries[row]->sequence_parsimony->sequence;
            seq2= entries[col]->sequence_parsimony->sequence;
            for (pos = 0; pos < s_len; pos++){
                if(filter[pos]>=0) {
                    hrates[*seq1][*seq2]++;
                }else{
                    nrates[*seq1][*seq2]++;
                }
                seq1++; seq2++;
            }
        }
    }
    for (row = 0; row<nentries;row++){
        seq1 = entries[row]->sequence_parsimony->sequence;
        for (pos = 0; pos < s_len; pos++){
            if(filter[pos]>=0) {
                pairs[seq1[pos]][seq1[filter[pos]]]++;
            }
        }
    }
    return 0;
}

/******************************************************************************************
    Some test functions to check correlated base correction
******************************************************************************************/


GB_ERROR PHMATRIX::haeschoe(const char *path)
{
    PH_MUT_MATR temp,temp2,pairs;
    static BI_helix *helix = 0;
    if (!helix) helix = new BI_helix();
    const char *error = helix->init(gb_main);
    if (error) return error;

    FILE *out = fopen(path,"w");
    if (!out) return "Cannot open file";

    aw_openstatus("Calculating Distanz Matrix");
    aw_status("Calculating global rate matrix");

    fprintf(out,"Pairs in helical regions:\n");
    long *filter = create_helix_filter(helix,tree_root->filter);
    error = calculate_rates(temp,temp2,pairs,filter);
    if (error){
        aw_closestatus();
        fclose(out);
        return error;
    }
    rate_write(pairs,out);
    make_sym(temp);make_sym(temp2);
    fprintf(out,"\nRatematrix helical parts:\n");
    rate_write(temp,out);
    fprintf(out,"\nRatematrix non helical parts:\n");
    rate_write(temp2,out);

    register long   row,col,pos,s_len;
    register char *seq1,*seq2;
    s_len = tree_root->filter->real_len;
    fprintf(out,"\nDistanzmatrix (Helixdist Helixlen Nonhelixdist Nonhelixlen):");
    fprintf(out,"\n%li",nentries);
    const int MAXDISTDEBUG = 1000;
    double distdebug[MAXDISTDEBUG];
    for (pos = 0;pos<MAXDISTDEBUG;pos++) distdebug[pos] = 0.0;
    for (row = 0; row<nentries;row++){
        if ( nentries > 100 || (row&0xf == 0)){
            double gauge = (double)row/(double)nentries;
            if (aw_status(gauge*gauge)) {
                aw_closestatus();
                return "Aborted";
            }
        }
        fprintf (out,"\n%s  ",entries[row]->name);

        for (col=0; col<row; col++) {
            seq1 = entries[row]->sequence_parsimony->sequence;
            seq2= entries[col]->sequence_parsimony->sequence;
            this->clear(temp);
            this->clear(temp2);
            for (pos = 0; pos < s_len; pos++){
                if(filter[pos]>=0) {
                    temp[*seq1][*seq2]++;
                }else{
                    temp2[*seq1][*seq2]++;
                }
                seq1++; seq2++;
            }
            long hdist= 0, hall2 = 0;
            int i,j;
            for (i=AP_A;i<AP_MAX;i*=2){
                for (j=AP_A;j<AP_MAX;j*=2){
                    hall2 += temp[i][j];
                    if (i!=j) hdist += temp[i][j];
                }
            }

            long dist= 0, all = 0;
            for (i=AP_A;i<=AP_T;i*=2){
                for (j=AP_A;j<=AP_T;j*=2){
                    all += temp2[i][j];
                    if (i!=j) dist += temp2[i][j];
                }
            }
            fprintf(out,"(%4li:%4li %4li:%4li) ",hdist,hall2, dist,all);
            if (all>100){
                distdebug[dist*MAXDISTDEBUG/all] = (double)hdist/(double)hall2;
            }
        }
    }

    fprintf (out,"\n");
    fprintf (out,"\nhdist/dist:\n");

    for (pos = 1;pos<MAXDISTDEBUG;pos++) {
        if (distdebug[pos]) {
            fprintf(out,"%4f %5f\n",(double)pos/(double)MAXDISTDEBUG,distdebug[pos]/(double)pos*(double)MAXDISTDEBUG);
        }
    }
    aw_closestatus();
    fclose(out);

    return 0;
}

char *PHMATRIX::calculate_overall_freqs(double rel_frequencies[AP_MAX], char *cancel)
{
    long            hits2[AP_MAX];
    long            sum = 0;
    int     i,row;
    register    char *seq1;
    register    int pos;
    register    int b;
    register long s_len = tree_root->filter->real_len;

    memset((char *) &hits2[0], 0, sizeof(hits2));
    for (row = 0; row < nentries; row++) {
        seq1 = entries[row]->sequence_parsimony->sequence;
        for (pos = 0; pos < s_len; pos++) {
            b = *(seq1++);
            if (cancel[b]) continue;
            hits2[b]++;
        }
    }
    for (i = 0; i < AP_MAX; i++)    sum += hits2[i];
    for (i = 0; i < AP_MAX; i++)    rel_frequencies[i] = hits2[i] / (double) sum;
    return 0;
}

double PHMATRIX::corr(double dist, double b, double & sigma){
    const double eps = 0.01;
    double ar = 1.0 - dist/b;
    sigma = 1000.0;
    if (ar< eps) return 3.0;
    sigma = b/ar;
    return - b * log(1-dist/b);
}

GB_ERROR PHMATRIX::calculate(AW_root *awr, char *cancel, double /*alpha*/, PH_TRANSFORMATION transformation)
{

    if (transformation == PH_TRANSFORMATION_HAESCH) {
        GB_ERROR error = haeschoe("outfile");
        if (error) return error;
        return "Your matrizes have been written to 'outfile'\nSorry I can not make a tree";
    }
    int user_matrix_enabled = awr->awar(AWAR_DIST_MATRIX_DNA_ENABLED)->read_int();
    if (user_matrix_enabled){   // set transformation Matrix
        switch (transformation){
            case PH_TRANSFORMATION_NONE:
            case PH_TRANSFORMATION_SIMILARITY:
            case PH_TRANSFORMATION_JUKES_CANTOR:
                break;
            default:
                return GB_export_error("Sorry not implemented:\n"
                                       "    This kind of distance correction does not support\n"
                                       "    a user defined matrix");
        }
        DI_dna_matrix.read_awars(awr,AWAR_DIST_MATRIX_DNA_BASE);
        DI_dna_matrix.normize();
    }



    matrix = new AP_smatrix(nentries);

    register long s_len = tree_root->filter->real_len;
    long    hits[AP_MAX][AP_MAX];
    int row;
    int col;
    size_t  i;

    if (nentries<=1) {
        return "There are no species selected";
    }
    memset(&cancel_columns[0],0,256);

    for (i=0;i<strlen(cancel);i++){
        int j = cancel[i];
        j = AP_sequence_parsimony::table[j];
        cancel_columns[j] = 1;
    }
    long    columns;
    double b;
    long frequencies[AP_MAX];
    double rel_frequencies[AP_MAX];
    double S_square=0;

    switch(transformation){
        case PH_TRANSFORMATION_FELSENSTEIN:
            this->calculate_overall_freqs(rel_frequencies,cancel_columns);
            S_square = 0.0;
            for (i=0;i<AP_MAX; i++) S_square+= rel_frequencies[i]*rel_frequencies[i];
            break;
        default:    break;
    };

    for (row = 0; row<nentries;row++){
        if ( nentries > 100 || (row&0xf == 0)){
            double gauge = (double)row/(double)nentries;
            if (aw_status(gauge*gauge)) {
                aw_closestatus();
                return "Aborted";
            }
        }
        for (col=0; col<=row; col++) {
            columns = 0;
            register char *seq1 = entries[row]->sequence_parsimony->sequence;
            register char *seq2= entries[col]->sequence_parsimony->sequence;
            b = 0.0;
            switch(transformation){
                case  PH_TRANSFORMATION_JUKES_CANTOR:
                    b = 0.75;
                case  PH_TRANSFORMATION_NONE:
                case  PH_TRANSFORMATION_SIMILARITY:{
                    double  dist = 0.0;
                    if (user_matrix_enabled){
                        memset((char *)hits,0,sizeof(long) * AP_MAX * AP_MAX);
                        int pos;
                        for (pos = s_len; pos >=0; pos--){
                            hits[*(seq1++)][*(seq2++)]++;
                        }
                        int x,y;
                        double diffsum = 0.0;
                        double all_sum = 0.001;
                        for (x = AP_A; x < AP_MAX; x*=2){
                            for (y = AP_A; y < AP_MAX; y*=2){
                                if (x==y){
                                    all_sum += hits[x][y];
                                }else{
                                    diffsum += hits[x][y] * DI_dna_matrix.get(x,y);
                                    all_sum += hits[x][y] * DI_dna_matrix.get(x,y);
                                }
                            }
                        }
                        dist = diffsum / all_sum;
                    }else{
                        register int pos;
                        register int b1,b2;
                        for (pos = s_len; pos >=0; pos--){
                            b1 =*(seq1++);
                            b2 =*(seq2++);
                            if (cancel_columns[b1]) continue;
                            if (cancel_columns[b2]) continue;
                            columns++;
                            if (b1&b2) continue;
                            dist+=1.0;
                        }
                        if (columns== 0) columns = 1;
                        dist /= columns;
                    }
                    if (transformation==PH_TRANSFORMATION_SIMILARITY){
                        dist =  (1.0-dist);
                    }else   if (b){
                        double sigma;
                        dist = this->corr(dist,b,sigma);
                    }
                    matrix->set(row,col,dist);
                    break;
                }
                case PH_TRANSFORMATION_KIMURA:
                case PH_TRANSFORMATION_OLSEN:
                case PH_TRANSFORMATION_FELSENSTEIN:{
                    register int pos;
                    double  dist = 0.0;
                    long N,P,Q,M;
                    double p,q;
                    memset((char *)hits,0,sizeof(long) * AP_MAX * AP_MAX);
                    for (pos = s_len; pos >=0; pos--){
                        hits[*(seq1++)][*(seq2++)]++;
                    }
                    switch(transformation){
                        case PH_TRANSFORMATION_KIMURA:
                            P = hits[AP_A][AP_G] +
                                hits[AP_G][AP_A] +
                                hits[AP_C][AP_T] +
                                hits[AP_T][AP_C];
                            Q = hits[AP_A][AP_C] +
                                hits[AP_A][AP_T] +
                                hits[AP_C][AP_A] +
                                hits[AP_T][AP_A] +
                                hits[AP_G][AP_C] +
                                hits[AP_G][AP_T] +
                                hits[AP_C][AP_G] +
                                hits[AP_T][AP_G];
                            M = hits[AP_A][AP_A] +
                                hits[AP_C][AP_C] +
                                hits[AP_G][AP_G] +
                                hits[AP_T][AP_T];
                            N = P+Q+M;
                            if (N==0) N=1;
                            p = (double)P/(double)N;
                            q = (double)Q/(double)N;
                            dist = - .5 * log(
                                              (1.0-2.0*p-q)*sqrt(1.0-2.0*q)
                                              );
                            break;

                        case PH_TRANSFORMATION_OLSEN:
                        case PH_TRANSFORMATION_FELSENSTEIN:

                            memset((char *)frequencies,0,
                                   sizeof(long) * AP_MAX);

                            N = 0;
                            M = 0;

                            for (i=0;i<AP_MAX;i++) {
                                if (cancel_columns[i]) continue;
                                unsigned int j;
                                for (j=0;j<i;j++) {
                                    if (cancel_columns[j]) continue;
                                    frequencies[i] +=
                                        hits[i][j]+
                                        hits[j][i];
                                }
                                frequencies[i] += hits[i][i];
                                N += frequencies[i];
                                M += hits[i][i];;
                            }
                            if (N==0) N=1;
                            if (transformation == PH_TRANSFORMATION_OLSEN){ // Calc sum square freq inividually for each line
                                S_square = 0.0;
                                for (i=0;i<AP_MAX; i++) S_square+= frequencies[i]*frequencies[i];
                                b = 1.0 - S_square/((double)N*(double)N);
                            }else{
                                b = 1.0 - S_square;
                            }

                            dist = ((double)(N-M)) / (double) N;
                            double sigma;
                            dist = this->corr(dist,b,sigma);
                            break;

                        default: return "Sorry: Transformation not implemented";
                    }
                    matrix->set(row,col,dist);
                    break;
                }
                default:;
            }   /* switch */
        }
    }
    return 0;
}

GB_ERROR PHMATRIX::calculate_pro(PH_TRANSFORMATION transformation){
    ph_cattype whichcat;
    ph_codetype whichcode = universal;

    switch (transformation){
        case PH_TRANSFORMATION_NONE:
            whichcat = none;
            break;
        case PH_TRANSFORMATION_SIMILARITY:
            whichcat = similarity;
            break;
        case PH_TRANSFORMATION_KIMURA:
            whichcat = kimura;
            break;
        case PH_TRANSFORMATION_PAM:
            whichcat = pam;
            break;
        case PH_TRANSFORMATION_CATEGORIES_HALL:
            whichcat = hall;
            break;
        case PH_TRANSFORMATION_CATEGORIES_BARKER:
            whichcat = george;
            break;
        case PH_TRANSFORMATION_CATEGORIES_CHEMICAL:
            whichcat = chemical;
            break;
        default:
            return "This correction is not available for protein data";
    }
    matrix = new AP_smatrix(nentries);

    ph_protdist prodist(whichcode, whichcat, nentries,entries, tree_root->filter->real_len,matrix);
    return prodist.makedists();
}


static GB_ERROR ph_calculate_matrix_cb(AW_window *aww, AW_CL bootstrap_flag, AW_CL cl_show_warnings) // returns "Aborted" if stopped by user
{
    bool show_warnings = (bool)cl_show_warnings;
    GB_push_transaction(gb_main);
    if (PHMATRIX::ROOT){
        GB_pop_transaction(gb_main);
        return 0;
    }
    AW_root *aw_root = aww->get_root();
    char    *use = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
    long ali_len = GBT_get_alignment_len(gb_main,use);
    if (ali_len<=0) {
        GB_pop_transaction(gb_main);
        delete use;
        aw_message("Please select a valid alignment");
        return "Error";
    }
    if (!bootstrap_flag){
        aw_openstatus("Calculating Matrix");
        aw_status("Read the database");
    }

    PHMATRIX *phm = new PHMATRIX(gb_main,aw_root);
    phm->matrix_type = PH_MATRIX_FULL;
    char    *cancel = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();

    AP_filter *ap_filter = awt_get_filter(aw_root,dist_filter_cl);
    if (bootstrap_flag){
        ap_filter->enable_bootstrap(0);
    }

    AP_weights *ap_weights = new AP_weights;
    ap_weights->init(ap_filter);

    AP_smatrix *ap_ratematrix = 0;

    char *load_what = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
    char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();
    int all_flag = 0;
    if (!strcmp(load_what,"all")) all_flag = 1;
    phm->load(use,ap_filter,ap_weights,ap_ratematrix,all_flag, sort_tree_name, show_warnings);
    free(sort_tree_name);
    GB_pop_transaction(gb_main);
    if (aw_status()) {
        phm->unload();
        return "Aborted";
    }

    PH_TRANSFORMATION trans;
    trans = (PH_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();

    GB_ERROR error;
    if (phm->is_AA){
        error = phm->calculate_pro(trans);
    }else{
        error = phm->calculate(aw_root,cancel,0.0,trans);
    }
    if (!bootstrap_flag){
        aw_closestatus();
    }
    delete phm->ROOT;
    if (error && strcmp(error,"Aborted")){
        aw_message(error);
        delete phm;
        phm->ROOT = 0;
    }else{
        phm->ROOT = phm;

    }
    free(load_what);

    free(cancel);
    delete ap_filter;
    delete ap_weights;
    delete ap_ratematrix;

    free(use);
    return error;
}


static void ph_view_matrix_cb(AW_window *aww){
    GB_ERROR error = ph_calculate_matrix_cb(aww,0,true);
    if (error) return;

    static AW_window *viewer = 0;
    if (!ph_dmatrix){
        ph_dmatrix = new PH_dmatrix();
    }

    if (!viewer) viewer = PH_create_view_matrix_window(aww->get_root(), ph_dmatrix);

    ph_dmatrix->init();
    ph_dmatrix->display();
    viewer->show();
}

static void ph_save_matrix_cb(AW_window *aww)
{           // save the matrix
    GB_ERROR error = ph_calculate_matrix_cb(aww,0, true);
    if (error) return;

    char              *filename = aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_FILENAME)->read_string();
    enum PH_SAVE_TYPE  type     = (enum PH_SAVE_TYPE)aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_TYPE)->read_int();

    PHMATRIX::ROOT->save(filename,type);
    free(filename);

    if (error) {
        aw_message(error);
    }
    else {
        awt_refresh_selection_box(aww->get_root(), AWAR_DIST_SAVE_MATRIX_BASE);
        aww->hide();
    }
}

AW_window *create_save_matrix_window(AW_root *aw_root, char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_MATRIX", "Save Matrix");
    aws->load_xfig("sel_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CANCEL","C");


    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"save_matrix.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->at("user");
    aws->create_option_menu(AWAR_DIST_SAVE_MATRIX_TYPE);
    aws->insert_default_option("Phylip Format (Lower Triangular Matrix)","P",PH_SAVE_PHYLIP_COMP);
    aws->insert_option("Readable (using NDS)","R",PH_SAVE_READABLE);
    aws->insert_option("Tabbed (using NDS)","R",PH_SAVE_TABBED);
    aws->update_option_menu();

    awt_create_selection_box((AW_window *)aws,base_name);

    aws->at("save2");aws->callback(ph_save_matrix_cb);
    aws->create_button("SAVE", "SAVE","S");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel2");
    aws->create_button("CLOSE", "CANCEL","C");

    return aws;
}

static AW_window *awt_create_select_cancel_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_CHARS_TO_CANCEL_COLOUM", "CANCEL SELECT");
    aws->load_xfig("ph_cancel.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("cancel");
    aws->create_input_field(AWAR_DIST_CANCEL_CHARS,12);

    return (AW_window *)aws;
}

static const char *enum_trans_to_string[] = {
    "none",
    "similarity",
    "jukes_cantor",
    "felsenstein",

    "pam",
    "hall",
    "barker",
    "chemical",

    "haesch",
    "kimura",
    "olsen",
    "felsenstein voigt",
    "olsen voigt",
    "max ml"
};

static int ph_calculate_tree_show_status(int loop_count, int bootstrap_count) {
    // returns value of aw_status()
    int result = 0;
    if (bootstrap_count) {
        result = aw_status(GBS_global_string("Tree #%i of %i", loop_count, bootstrap_count));
        if (!result) result = aw_status(loop_count/double(bootstrap_count));
    }
    else {
        result = aw_status(GBS_global_string("Tree #%i of %u", loop_count, UINT_MAX));
        if (!result) result = aw_status(loop_count/double(UINT_MAX));
    }
    return result;
}

static void ph_calculate_tree_cb(AW_window *aww, AW_CL bootstrap_flag)
{
    AW_root   *aw_root         = aww->get_root();
    GB_ERROR   error           = 0;
    GBT_TREE  *tree            = 0;
    char     **all_names       = 0;
    int        loop_count      = 0;    
    bool       close_stat      = false;
    int        bootstrap_count = aw_root->awar(AWAR_DIST_BOOTSTRAP_COUNT)->read_int();

    {
        char *tree_name = aw_root->awar(AWAR_DIST_TREE_STD_NAME)->read_string();
        error           = GBT_check_tree_name(tree_name);
        free(tree_name);
    }

    int starttime        = time(0);
    int trees_per_second = -1;
    int last_status_upd  = 0;

    if (!error) {
        {
            const char *stat_msg = 0;
            if (bootstrap_flag) {
                if (bootstrap_count) stat_msg = "Calculating bootstrap trees";
                else stat_msg = "Calculating bootstrap trees (KILL to stop)";
            }
            else stat_msg = "Calculating tree";

            aw_openstatus(stat_msg);
            close_stat = true;
        }

        if (bootstrap_flag){
            loop_count++;
            ph_calculate_tree_show_status(loop_count, bootstrap_count);

            gb_assert(PHMATRIX::ROOT == 0);
            ph_calculate_matrix_cb(aww,bootstrap_flag, true);

            PHMATRIX *matr = PHMATRIX::ROOT;
            if (!matr) {
                error = "unexpected error in ph_calculate_matrix_cb (data missing)";
            }
            else {
                all_names = (char **)calloc(sizeof(char *),(size_t)matr->nentries+2);
                for (long i=0;i<matr->nentries;i++){
                    all_names[i] = strdup(matr->entries[i]->name);
                }
                ctree_init(matr->nentries,all_names);
            }
        }
    }

    do {
        if (error) break;

        if (bootstrap_flag) {
            bool show_update = true;
            if (trees_per_second == -1) {
                int timediff = time(0)-starttime;
                if (timediff>10) {
                    trees_per_second = loop_count/timediff;
                }
            }
            else {
                if ((loop_count-last_status_upd)*4 < trees_per_second) {
                    show_update = false; // do not update more often than 4 times a second
                }
            }

            loop_count++;
            if (show_update) { // max. once per second
                if (ph_calculate_tree_show_status(loop_count, bootstrap_count)) {
                    break; // user abort
                }
                last_status_upd = loop_count;
            }

            delete PHMATRIX::ROOT; PHMATRIX::ROOT = 0;
        }

        gb_assert(PHMATRIX::ROOT == 0);
        error = ph_calculate_matrix_cb(aww, bootstrap_flag, bootstrap_flag ? false : true);
        if (error && strcmp(error,"Aborted") == 0) {
            error = 0;          // clear error (otherwise no tree will be read below)
            break;              // end of bootstrap
        }

        if (!PHMATRIX::ROOT) {
            error = "unexpected error in ph_calculate_matrix_cb (data missing)";
            break;
        }

        PHMATRIX  *matr  = PHMATRIX::ROOT;
        char     **names = (char **)calloc(sizeof(char *),(size_t)matr->nentries+2);
        for (long i=0;i<matr->nentries;i++){
            names[i] = matr->entries[i]->name;
        }
        tree = neighbourjoining(names,matr->matrix->m,matr->nentries,sizeof(GBT_TREE));

#if 0
        // save all generated trees for debugging
        FILE *out = fopen(GBS_global_string("tree_%i",loop_count),"w");
        GBT_export_tree(gb_main,out,tree,GB_FALSE);
        fclose(out);
#endif

        if (bootstrap_flag){
            insert_ctree(tree,1);
            GBT_delete_tree(tree); tree = 0;
        }
        free(names);

    } while (bootstrap_flag && loop_count != bootstrap_count);

    if (!error) {
        if (bootstrap_flag) tree = get_ctree();

        char *tree_name = aw_root->awar(AWAR_DIST_TREE_STD_NAME)->read_string();
        GB_begin_transaction(gb_main);
        error = GBT_write_tree(gb_main,0,tree_name,tree);
        if (error) {
            GB_abort_transaction(gb_main);
            aw_message(error);
        }
        else {
            //char *filter_name     = aw_root->awar(AWAR_DIST_FILTER_NAME)->read_string();
            char       *filter_name = AWT_get_combined_filter_name(aw_root, "dist");
            int         transr      = aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();
            const char *comment     = GBS_global_string("PRG=dnadist CORR=%s FILTER=%s PKG=ARB", enum_trans_to_string[transr], filter_name);

            free(filter_name);
            GBT_write_tree_rem(gb_main,tree_name,comment);
            GB_commit_transaction(gb_main);
        }
        free(tree_name);
    }

    if (tree) GBT_delete_tree(tree);

    if (close_stat) aw_closestatus();
    aw_status(); // remove 'abort' flag

    if (bootstrap_flag) {
        if (all_names) GBT_free_names(all_names);
    }
#if defined(DEBUG)
    else {
        gb_assert(all_names == 0);
    }
#endif // DEBUG

    delete PHMATRIX::ROOT; PHMATRIX::ROOT = 0;

    if (error) aw_message(error);
}


static void ph_autodetect_callback(AW_window *aww)
{
    GB_push_transaction(gb_main);
    if (PHMATRIX::ROOT){
        delete PHMATRIX::ROOT;
        PHMATRIX::ROOT = 0;
    }
    AW_root *aw_root = aww->get_root();
    char    *use = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
    long ali_len = GBT_get_alignment_len(gb_main,use);
    if (ali_len<=0) {
        GB_pop_transaction(gb_main);
        delete use;
        aw_message("Please select a valid alignment");
        return;
    }

    aw_openstatus("Analysing data...");
    aw_status("Read the database");

    PHMATRIX *phm = new PHMATRIX(gb_main,aw_root);
    char    *filter_str = aw_root->awar(AWAR_DIST_FILTER_FILTER)->read_string();
    char    *cancel = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();

    AP_filter *ap_filter = new AP_filter;
    long flen = strlen(filter_str);
    if (flen == ali_len){
        ap_filter->init(filter_str,"0",ali_len);
    }else{
        if (flen){
            aw_message("WARNING: YOUR FILTER LEN IS NOT THE ALIGNMENT LEN\nFILTER IS TRUNCATED WITH ZEROS OR CUTTED");
            ap_filter->init(filter_str,"0",ali_len);
        }else{
            ap_filter->init(ali_len);
        }
    }

    AP_weights *ap_weights = new AP_weights;
    ap_weights->init(ap_filter);

    AP_smatrix *ap_ratematrix = 0;

    char    *load_what = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
    char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();
    int all_flag = 0;
    if (!strcmp(load_what,"all")) all_flag = 1;

    delete phm->ROOT;
    phm->ROOT = 0;
    phm->load(use,ap_filter,ap_weights,ap_ratematrix,all_flag, sort_tree_name, true);
    free(sort_tree_name);
    GB_pop_transaction(gb_main);

    aw_status("Search Correction");

    PH_TRANSFORMATION trans;
    trans = (PH_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();


    char *error = phm->analyse(aw_root);
    aw_closestatus();

    delete phm;
    if (error) aw_message(error);

    free(load_what);

    free(cancel);
    delete ap_filter;
    delete ap_weights;
    delete ap_ratematrix;

    free(use);
    free(filter_str);
}

static void ap_exit(AW_window *)
{
    if (gb_main) GB_exit(gb_main);
    exit(0);
}

static void ph_calculate_full_matrix_cb(AW_window *aww){
    if (PHMATRIX::ROOT && PHMATRIX::ROOT->matrix_type == PH_MATRIX_FULL) return;
    delete_matrix_cb(0);
    ph_calculate_matrix_cb(aww,0, true);
    if (ph_dmatrix){
        ph_dmatrix->resized();
        ph_dmatrix->display();
    }
}

static void ph_compress_tree_cb(AW_window *aww){
    GB_transaction dummy(gb_main);
    char *treename = aww->get_root()->awar(AWAR_DIST_TREE_COMP_NAME)->read_string();
    GB_ERROR error = 0;
    GBT_TREE *tree = GBT_read_tree(gb_main,treename,sizeof(GBT_TREE));
    if (!tree) error = "Please select a valid tree first";

    if (PHMATRIX::ROOT && PHMATRIX::ROOT->matrix_type!=PH_MATRIX_COMPRESSED){
        delete_matrix_cb(0);        // delete wrong matrix !!!
    }

    ph_calculate_matrix_cb(aww,0, true);
    if (!PHMATRIX::ROOT) error = "Cannot calculate your matrix";

    if (!error) {
        error = PHMATRIX::ROOT->compress(tree);
    }

    if (tree) GBT_delete_tree(tree);

    if (error) {
        aw_message(error);
    }
    else {
        //  aww->hide();
        if (ph_dmatrix){
            ph_dmatrix->resized();
            ph_dmatrix->display();
        }
    }
    free(treename);
}

/*
  AW_window *create_select_compress_tree(AW_root *awr){
  static AW_window_simple *aws = 0;
  if (aws) return (AW_window *)aws;

  aws = new AW_window_simple;
  aws->init( awr, "SELECT_TREE_TO_COMPRESS_MATRIX", "SELECT A TREE TO COMPRESS MATRIX", 400, 200 );
  aws->at(10,10);
  aws->auto_space(0,0);
  awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,"dist/compress/tree_name");
  aws->at_newline();

  aws->callback(AW_POPDOWN);
  aws->create_button("CLOSE", "CLOSE","C");

  aws->callback(ph_compress_tree_cb);
  aws->help_text("compress_tree.hlp");
  aws->create_button("GO", "DO IT","D");

  aws->callback(AW_POPUP_HELP,(AW_CL)"compress_tree.hlp");
  aws->create_button("HELP", "HELP","H");

  aws->window_fit();
  return (AW_window *)aws;
  }
*/

static void ph_define_sort_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_LIST_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->write_string(tree_name);
    free(tree_name);
}
static void ph_define_compression_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_LIST_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_COMP_NAME)->write_string(tree_name);
    free(tree_name);
}

static void ph_define_save_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_LIST_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_STD_NAME)->write_string(tree_name);
    free(tree_name);
}

AW_window *create_matrix_window(AW_root *aw_root) {
    AW_window_simple_menu *aws = new AW_window_simple_menu;
    aws->init( aw_root, "NEIGHBOR JOINING", "NEIGHBOR JOINING");
    aws->load_xfig("ph_ge_ma.fig");
    aws->button_length( 10 );

    aws->at("close");
    aws->callback(ap_exit);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"dist.hlp");
    aws->create_button("HELP", "HELP","H");


    GB_push_transaction(gb_main);

    //  aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    //  aws->create_button("CLOSE","C");

    aws->create_menu(0, "FILE", "F", "HELP for Window", AWM_ALL);
    aws->insert_menu_topic("macros", "Macros  ...", "M", "macro.hlp", AWM_ALL, (AW_CB)AW_POPUP, (AW_CL)awt_open_macro_window,(AW_CL)"NEIGHBOR_JOINING");
    aws->insert_menu_topic("quit",   "Quit",        "Q", "quit.hlp",  AWM_ALL, (AW_CB)ap_exit,  0,  0                                                 );

    aws->create_menu(0, "Properties", "P", "properties.hlp", AWM_ALL);
    aws->insert_menu_topic("frame_props", "Frame ...",                                 "F", "props_frame.hlp", AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0);
    aws->insert_menu_topic("save_props",  "Save Properties (in ~/.arb_prop/dist.arb)", "S", "savedef.hlp",     AWM_ALL, (AW_CB)AW_save_defaults, 0, 0       );

    aws->insert_help_topic("help", "help ...", "h", "dist.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"dist.hlp", 0);

    aws->at( "which_species" );
    aws->create_option_menu( AWAR_DIST_WHICH_SPECIES );
    aws->insert_option( "all", "a", "all" );
    aws->insert_default_option( "marked",  "m", "marked" );
    aws->update_option_menu();

    aws->at("which_alignment");
    awt_create_selection_list_on_ad(gb_main, (AW_window *)aws,AWAR_DIST_ALIGNMENT,"*=");

    aws->at("filter_select");
    AW_CL filtercd = awt_create_select_filter(aws->get_root(),gb_main,AWAR_DIST_FILTER_NAME);
    dist_filter_cl = filtercd;
    aws->callback(AW_POPUP,(AW_CL)awt_create_select_filter_win,filtercd);
    aws->create_button("SELECT_FILTER", AWAR_DIST_FILTER_NAME);

    aws->at("weights_select");
    global_csp = new AWT_csp(gb_main,aws->get_root(),AWAR_DIST_COLUMN_STAT_NAME);
    aws->callback(AW_POPUP,(AW_CL)create_csp_window, (AW_CL)global_csp);
    aws->create_button("SELECT_COL_STAT",AWAR_DIST_COLUMN_STAT_NAME);

    aws->at("which_cancel");
    aws->create_input_field(AWAR_DIST_CANCEL_CHARS,12);

    aws->at("cancel_select");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)awt_create_select_cancel_window);
    aws->create_button("SELECT_CANCEL_CHARS", "Select","C");

    aws->at("change_matrix");
    aws->callback(AW_POPUP,(AW_CL)create_dna_matrix_window,0);
    aws->create_button("EDIT_MATRIX", "Edit Matrix");

    aws->at("enable");
    aws->create_toggle(AWAR_DIST_MATRIX_DNA_ENABLED);

    aws->at("which_correction");
    aws->create_option_menu(AWAR_DIST_CORR_TRANS, NULL ,"");
    aws->insert_option("none",                    "n", (int)PH_TRANSFORMATION_NONE               );
    aws->insert_option("similarity",              "n", (int)PH_TRANSFORMATION_SIMILARITY         );
    aws->insert_option("jukes-cantor (dna)",      "c", (int)PH_TRANSFORMATION_JUKES_CANTOR       );
    aws->insert_option("felsenstein (dna)",       "f", (int)PH_TRANSFORMATION_FELSENSTEIN        );
    aws->insert_option("olsen (dna)",             "o", (int)PH_TRANSFORMATION_OLSEN              );
    aws->insert_option("felsenstein/voigt (exp)", "1", (int)PH_TRANSFORMATION_FELSENSTEIN_VOIGT  );
    aws->insert_option("olsen/voigt (exp)",       "2", (int)PH_TRANSFORMATION_OLSEN_VOIGT        );
    aws->insert_option("haesch (exp)",            "f", (int)PH_TRANSFORMATION_HAESCH             );
    aws->insert_option("kimura (pro)",            "k", (int)PH_TRANSFORMATION_KIMURA             );
    aws->insert_option("PAM (protein)",           "c", (int)PH_TRANSFORMATION_PAM                );
    aws->insert_option("Cat. Hall(exp)",          "c", (int)PH_TRANSFORMATION_CATEGORIES_HALL    );
    aws->insert_option("Cat. Barker(exp)",        "c", (int)PH_TRANSFORMATION_CATEGORIES_BARKER  );
    aws->insert_option("Cat.Chem (exp)",          "c", (int)PH_TRANSFORMATION_CATEGORIES_CHEMICAL);
    // aws->insert_option("Maximum Likelihood (exp)", "l", (int)PH_TRANSFORMATION_NONE);
    aws->insert_default_option("unknown", "u", (int)PH_TRANSFORMATION_NONE);

    aws->update_option_menu();

    aws->button_length(18);

    //     aws->at("compress");
    //     aws->callback(AW_POPUP,  (AW_CL)create_select_compress_tree,0);
    //     aws->help_text("compress_tree.hlp");
    //     aws->create_button("CALC_COMPRESSED_MATRIX", "Calculate\nCompressed M. ...","C");

    aws->at("compress");
    aws->callback(ph_compress_tree_cb);
    aws->create_button("CALC_COMPRESSED_MATRIX", "Calculate\nCompressed Matrix","C");

    aws->at("calculate");
    aws->callback(ph_calculate_full_matrix_cb);
    aws->create_button("CALC_FULL_MATRIX", "Calculate\nFull Matrix","F");

    aws->button_length(13);

    aws->at("save_matrix");
    aws->callback(AW_POPUP, (AW_CL)create_save_matrix_window,(AW_CL)AWAR_DIST_SAVE_MATRIX_BASE);
    aws->create_button("SAVE_MATRIX", "Save matrix","M");

    aws->at("view_matrix");
    aws->callback(ph_view_matrix_cb);
    aws->create_button("VIEW_MATRIX", "View matrix","V");

    aws->at("tree_list");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_DIST_TREE_LIST_NAME);

    aws->button_length(18);

    aws->at("t_calculate");
    aws->callback(ph_calculate_tree_cb,0);
    aws->create_button("CALC_TREE", "Calculate \ntree","C");

    aws->at("bootstrap");
    aws->callback(ph_calculate_tree_cb,1);
    aws->create_button("CALC_BOOTSTRAP_TREE", "Calculate \nbootstrap tree");

    aws->at("bcount");
    aws->create_input_field(AWAR_DIST_BOOTSTRAP_COUNT, 7);

    aws->at("autodetect");   // auto
    aws->callback(ph_autodetect_callback);
    aws->create_button("AUTODETECT_CORRECTION", "AUTODETECT","A");

    aws->at("sort_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_SORT_NAME,12);
    aws->at("compr_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_COMP_NAME,12);
    aws->at("calc_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_STD_NAME,12);

    aws->button_length(22);

    aws->at("use_compr_tree");
    aws->callback(ph_define_compression_tree_name_cb);
    aws->create_button("USE_COMPRESSION_TREE", "Use to compress", "");

    aws->at("use_sort_tree");
    aws->callback(ph_define_sort_tree_name_cb);
    aws->create_button("USE_SORT_TREE", "Use to sort", "");

    aws->at("use_existing");
    aws->callback(ph_define_save_tree_name_cb);
    aws->create_button("USE_NAME", "Use as new tree name", "");

    GB_pop_transaction(gb_main);
    return (AW_window *)aws;
}
