// =============================================================== //
//                                                                 //
//   File      : DI_matr.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_protdist.hxx"
#include "di_clusters.hxx"
#include "dist.hxx"
#include "di_view_matrix.hxx"
#include "di_matr.hxx"
#include "di_awars.hxx"

#include <neighbourjoin.hxx>
#include <AP_seq_dna.hxx>
#include <AP_filter.hxx>
#include <BI_helix.hxx>
#include <CT_ctree.hxx>
#include <awt_macro.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>
#include <awt_csp.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>

#include <gui_aliview.hxx>

#include <climits>
#include <ctime>

#define di_assert(cond) arb_assert(cond)

// --------------------------------------------------------------------------------

#define AWAR_DIST_BOOTSTRAP_COUNT AWAR_DIST_PREFIX "bootstrap/count"
#define AWAR_DIST_CANCEL_CHARS    AWAR_DIST_PREFIX "cancel/chars"

#define AWAR_DIST_COLUMN_STAT_ALIGNMENT AWAR_DIST_COLUMN_STAT_PREFIX "alignment"
#define AWAR_DIST_COLUMN_STAT_NAME      AWAR_DIST_COLUMN_STAT_PREFIX "name"

#define AWAR_DIST_TREE_SORT_NAME "tmp/" AWAR_DIST_TREE_PREFIX "sort_tree_name"
#define AWAR_DIST_TREE_COMP_NAME "tmp/" AWAR_DIST_TREE_PREFIX "compr_tree_name"
#define AWAR_DIST_TREE_STD_NAME  AWAR_DIST_TREE_PREFIX "tree_name"

#define AWAR_DIST_SAVE_MATRIX_TYPE     AWAR_DIST_SAVE_MATRIX_BASE "/type"
#define AWAR_DIST_SAVE_MATRIX_FILENAME AWAR_DIST_SAVE_MATRIX_BASE "/file_name"

// --------------------------------------------------------------------------------

DI_MATRIX  *DI_MATRIX::ROOT = 0;
DI_dmatrix *di_dmatrix      = 0;

AP_matrix DI_dna_matrix(AP_MAX);

static void delete_matrix_cb(AW_root *dummy)
{
    AWUSE(dummy);
    delete DI_MATRIX::ROOT;
    DI_MATRIX::ROOT = 0;
    if (di_dmatrix){
        di_dmatrix->resized();
        di_dmatrix->display(false);
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

void DI_create_matrix_variables(AW_root *aw_root, AW_default def, AW_default db)
{
    GB_transaction dummy(db);
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
        char *default_ali = GBT_get_default_alignment(db);
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
    aw_root->awar_int(AWAR_DIST_CORR_TRANS, (int)DI_TRANSFORMATION_SIMILARITY, def)->add_callback(delete_matrix_cb);

    aw_root->awar(AWAR_DIST_FILTER_ALIGNMENT)     ->map(AWAR_DIST_ALIGNMENT);
    aw_root->awar(AWAR_DIST_COLUMN_STAT_ALIGNMENT)->map(AWAR_DIST_ALIGNMENT);

    aw_create_selection_box_awars(aw_root, AWAR_DIST_SAVE_MATRIX_BASE, ".", "", "infile", def);
    aw_root->awar_int(AWAR_DIST_SAVE_MATRIX_TYPE, 0, def);

    aw_root->awar_string(AWAR_DIST_TREE_STD_NAME,  "tree_nj", def);
    {
        char *currentTree = aw_root->awar_string(AWAR_TREE, "", db)->read_string();
        aw_root->awar_string(AWAR_DIST_TREE_CURR_NAME, currentTree, def);
        aw_root->awar_string(AWAR_DIST_TREE_SORT_NAME, currentTree, def)->add_callback(delete_matrix_cb);
        free(currentTree);
    }
    aw_root->awar_string(AWAR_DIST_TREE_COMP_NAME, "?????", def)->add_callback(delete_matrix_cb);

    aw_root->awar_int(AWAR_DIST_BOOTSTRAP_COUNT, 1000, def);

    aw_root->awar_float(AWAR_DIST_MIN_DIST, 0.0);
    aw_root->awar_float(AWAR_DIST_MAX_DIST, 0.0);

    aw_root->awar_string(AWAR_SPECIES_NAME, "", db);

    DI_create_cluster_awars(aw_root, def, db);

#if defined(DEBUG)
    AWT_create_db_browser_awars(aw_root, def);
#endif // DEBUG

    {
        GB_push_transaction(db);

        GBDATA *gb_species_data = GB_search(db,"species_data",GB_CREATE_CONTAINER);
        GB_add_callback(gb_species_data,GB_CB_CHANGED,(GB_CB)delete_matrix_cb,0);

        GB_pop_transaction(db);
    }
}

DI_ENTRY::DI_ENTRY(GBDATA *gbd, DI_MATRIX *phmatri) {
    memset((char *)this,0,sizeof(DI_ENTRY));
    phmatrix = phmatri;

    GBDATA *gb_ali = GB_entry(gbd, phmatrix->get_aliname());
    if (gb_ali) {
        GBDATA *gb_data = GB_entry(gb_ali,"data");
        if (gb_data) {
            if (phmatrix->is_AA) {
                sequence = sequence_protein = new AP_sequence_simple_protein(phmatrix->get_aliview());
            }
            else {
                sequence = sequence_parsimony = new AP_sequence_parsimony(phmatrix->get_aliview());
            }
            sequence->bind_to_species(gbd);
            sequence->lazy_load_sequence(); // load sequence

            name      = GBT_read_string(gbd, "name");
            full_name = GBT_read_string(gbd, "full_name");
        }
    }
}

DI_ENTRY::DI_ENTRY(char *namei, DI_MATRIX *phmatri)
{
    memset((char *)this,0,sizeof(DI_ENTRY));
    phmatrix = phmatri;
    this->name = strdup(namei);
}

DI_ENTRY::~DI_ENTRY(void)
{
    delete sequence_protein;
    delete sequence_parsimony;
    free(name);
    free(full_name);

}

DI_MATRIX::DI_MATRIX(const AliView& aliview_, AW_root *awr) {
    memset((char *)this, 0, sizeof(*this));
    aw_root = awr;
    aliview = new AliView(aliview_);
}

char *DI_MATRIX::unload() {
    for (long i=0; i<nentries; i++) {
        delete entries[i];
    }
    freenull(entries);
    nentries = 0;
    return 0;
}

DI_MATRIX::~DI_MATRIX()
{
    unload();
    delete matrix;
    delete aliview; 
}

extern "C" int qsort_strcmp(const void *str1ptr, const void *str2ptr) {
    GB_CSTR s1 = *(GB_CSTR*)str1ptr;
    GB_CSTR s2 = *(GB_CSTR*)str2ptr;

    return strcmp(s1, s2);
}

char *DI_MATRIX::load(LoadWhat what, GB_CSTR sort_tree_name, bool show_warnings, GBDATA **species_list) {
    GBDATA     *gb_main = get_gb_main();
    const char *use     = get_aliname();
    
    GB_push_transaction(gb_main);

    seq_len          = GBT_get_alignment_len(gb_main,use);
    is_AA            = GBT_is_alignment_protein(gb_main,use);
    gb_species_data  = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    entries_mem_size = 1000;

    entries = (DI_ENTRY **)calloc(sizeof(DI_ENTRY*),(size_t)entries_mem_size);

    nentries = 0;
    DI_ENTRY *phentry;

    int tree_size;
    GBT_TREE *sort_tree = GBT_read_tree_and_size(gb_main, sort_tree_name, sizeof(GBT_TREE), &tree_size);
    tree_size++;
    GB_CSTR *species_in_sort_tree = 0;

    int no_of_species = -1;
    switch (what) {
        case DI_LOAD_ALL:
            no_of_species = GBT_get_species_count(gb_main);
            break;
        case DI_LOAD_MARKED:
            no_of_species = GBT_count_marked_species(gb_main);
            break;
        case DI_LOAD_LIST:
            di_assert(species_list);
            for (no_of_species = 0; species_list[no_of_species]; ++no_of_species) ;
            break;
    }
    int in_tree_species         = 0;
    int out_tree_species        = no_of_species;
    int unknown_species_in_tree = 0;

    if (!sort_tree) {
        if (show_warnings) {
            aw_message("No valid tree given to sort matrix (using default database order)");
        }
    }
    else {
        species_in_sort_tree = GBT_get_species_names_of_tree(sort_tree);

        {
            GB_NUMHASH *shallLoad = 0;
            if (what == DI_LOAD_LIST) {
                shallLoad = GBS_create_numhash(no_of_species);
                for (int i = 0; species_list[i]; ++i) {
                    GBS_write_numhash(shallLoad, (long)species_list[i], 1);
                }
            }

            for (int i=0; i<tree_size; i++) { // read all marked species in tree
                GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, species_in_sort_tree[i]);
                if (!gb_species) {
                    if (show_warnings) {
                        aw_message(GB_export_errorf("Species '%s' found in tree '%s' but NOT in database.",
                                                    species_in_sort_tree[i], sort_tree_name));
                    }
                    unknown_species_in_tree++;
                    continue;
                }

                if ((what == DI_LOAD_ALL)                                ||
                    (what == DI_LOAD_MARKED && GB_read_flag(gb_species)) ||
                    (what == DI_LOAD_LIST && GBS_read_numhash(shallLoad, (long)gb_species)))
                {
                    in_tree_species++;
                    phentry = new DI_ENTRY(gb_species,this);
                    if (phentry->sequence) {    // a species found
                        entries[nentries++] = phentry;
                        if (nentries == entries_mem_size) {
                            entries_mem_size +=1000;
                            entries = (DI_ENTRY **)realloc(entries,(size_t)(sizeof(DI_ENTRY*)*entries_mem_size));
                        }
                    }
                    else {
                        delete phentry;
                    }
                }
            }

            if (shallLoad) GBS_free_numhash(shallLoad);
        }
        out_tree_species = no_of_species-in_tree_species; // # of species not loaded yet (because not in tree)
        di_assert(out_tree_species>=0);
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
        aw_message(GB_export_errorf("We found %i species in tree '%s' which are not in database.\n"
                                    "This does not affect the current calculation, but you should think about it.",
                                    unknown_species_in_tree, sort_tree_name));
    }

    di_assert(out_tree_species>=0);
    if (out_tree_species) { // there are species outside the tree (or no valid sort tree selected) => load all marked species not loaded yet
        int count   = 0;
        int listIdx = 0;

        GBDATA *gb_species = NULL;
        switch (what) {
            case DI_LOAD_ALL:    gb_species = GBT_first_species_rel_species_data(gb_species_data); break;
            case DI_LOAD_MARKED: gb_species = GBT_first_marked_species_rel_species_data(gb_species_data); break;
            case DI_LOAD_LIST:   gb_species = species_list[listIdx++]; break;
        }

        while (gb_species) {
            int load_species = 0;

            if (in_tree_species) { // test if species already loaded
                const char *species_name = GBT_read_name(gb_species);
                if (bsearch(&species_name, species_in_sort_tree, tree_size, sizeof(*species_in_sort_tree), qsort_strcmp)==0) {
                    load_species = 1;
                }
            }
            else {
                load_species = 1;
            }

            if (load_species) {
                count++;
                phentry = new DI_ENTRY(gb_species,this);
                if (phentry->sequence) {    // a species found
                    entries[nentries++] = phentry;
                    if (nentries == entries_mem_size) {
                        entries_mem_size +=1000;
                        entries = (DI_ENTRY **)realloc(entries, sizeof(DI_ENTRY*)*entries_mem_size);
                    }
                }
                else {
                    delete phentry;
                }
            }

            switch (what) {
                case DI_LOAD_ALL:    gb_species = GBT_next_species(gb_species); break;
                case DI_LOAD_MARKED: gb_species = GBT_next_marked_species(gb_species); break;
                case DI_LOAD_LIST:   gb_species = species_list[listIdx++]; break;
            }
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
void DI_MATRIX::clear(DI_MUT_MATR &hits)
{
    int i,j;
    for (i=0;i<AP_MAX;i++){
        for (j=0;j<AP_MAX;j++){
            hits[i][j] = 0;
        }
    }
}

void DI_MATRIX::make_sym(DI_MUT_MATR &hits)
{
    int i,j;
    for (i=AP_A;i<AP_MAX;i*=2){
        for (j=AP_A;j<=i;j*=2){
            hits[i][j] = hits[j][i] = hits[i][j] + hits[j][i];
        }
    }
}

void DI_MATRIX::rate_write(DI_MUT_MATR &hits,FILE *out){
    int i,j;
    for (i=AP_A;i<AP_MAX;i*=2){
        for (j=AP_A;j<AP_MAX;j*=2){
            fprintf(out,"%5li ",hits[i][j]);
        }
        fprintf(out,"\n");
    }
}

long *DI_MATRIX::create_helix_filter(BI_helix *helix, const AP_filter *filter){
    long   *result = (long *)calloc(sizeof(long), filter->get_filtered_length());
    long   *temp   = (long *)calloc(sizeof(long), filter->get_filtered_length());
    int     count  = 0;
    size_t  i;

    for (i=0; i<filter->get_length(); i++) {
        if (filter->use_position(i)) {
            temp[i] = count;
            if (helix->pairtype(i)== HELIX_PAIR) {
                result[count] = helix->opposite_position(i);
            }
            else {
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

GB_ERROR DI_MATRIX::calculate_rates(DI_MUT_MATR &hrates,DI_MUT_MATR &nrates,DI_MUT_MATR &pairs,long *filter){

    if (nentries<=1) {
        return "There are no species selected";
    }
    long row,col,pos,s_len;
    
    const unsigned char *seq1,*seq2;
    s_len = aliview->get_length();
    this->clear(hrates);
    this->clear(nrates);
    this->clear(pairs);
    for (row = 0; row<nentries;row++){
        double gauge = (double)row/(double)nentries;
        if (aw_status(gauge*gauge)) return "Aborted";
        for (col=0; col<row; col++) {
            seq1 = entries[row]->sequence_parsimony->get_usequence();
            seq2 = entries[col]->sequence_parsimony->get_usequence();
            for (pos = 0; pos < s_len; pos++){
                if(filter[pos]>=0) {
                    hrates[*seq1][*seq2]++;
                }
                else {
                    nrates[*seq1][*seq2]++;
                }
                seq1++; seq2++;
            }
        }
    }
    for (row = 0; row<nentries;row++){
        seq1 = entries[row]->sequence_parsimony->get_usequence();
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


GB_ERROR DI_MATRIX::haeschoe(const char *path)
{
    DI_MUT_MATR temp,temp2,pairs;
    static BI_helix *helix = 0;
    if (!helix) helix = new BI_helix();
    const char *error = helix->init(get_gb_main());
    if (error) return error;

    FILE *out = fopen(path,"w");
    if (!out) return "Cannot open file";

    aw_openstatus("Calculating Distance Matrix");
    aw_status("Calculating global rate matrix");

    fprintf(out,"Pairs in helical regions:\n");
    long *filter = create_helix_filter(helix, aliview->get_filter());
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

    long row,col,pos,s_len;

    s_len = aliview->get_length();
    fprintf(out,"\nDistance matrix (Helixdist Helixlen Nonhelixdist Nonhelixlen):");
    fprintf(out,"\n%li",nentries);

    const int MAXDISTDEBUG = 1000;
    double    distdebug[MAXDISTDEBUG];

    for (pos = 0;pos<MAXDISTDEBUG;pos++) distdebug[pos] = 0.0;
    
    for (row = 0; row<nentries;row++){
        if ( nentries > 100 || ((row&0xf) == 0)){
            double gauge = (double)row/(double)nentries;
            if (aw_status(gauge*gauge)) {
                aw_closestatus();
                return "Aborted";
            }
        }
        fprintf (out,"\n%s  ",entries[row]->name);

        for (col=0; col<row; col++) {
            const unsigned char *seq1,*seq2;

            seq1 = entries[row]->sequence_parsimony->get_usequence();
            seq2 = entries[col]->sequence_parsimony->get_usequence();
            this->clear(temp);
            this->clear(temp2);
            
            for (pos = 0; pos < s_len; pos++){
                if (filter[pos]>=0) temp [*seq1][*seq2]++;
                else                temp2[*seq1][*seq2]++;
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

char *DI_MATRIX::calculate_overall_freqs(double rel_frequencies[AP_MAX], char *cancel)
{
    long hits2[AP_MAX];
    long sum   = 0;
    int  i,row;
    int  pos;
    int  b;
    long s_len = aliview->get_length();

    memset((char *) &hits2[0], 0, sizeof(hits2));
    for (row = 0; row < nentries; row++) {
        const char *seq1 = entries[row]->sequence_parsimony->get_sequence();
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

double DI_MATRIX::corr(double dist, double b, double & sigma){
    const double eps = 0.01;
    double ar = 1.0 - dist/b;
    sigma = 1000.0;
    if (ar< eps) return 3.0;
    sigma = b/ar;
    return - b * log(1-dist/b);
}

GB_ERROR DI_MATRIX::calculate(AW_root *awr, char *cancel, double /*alpha*/, DI_TRANSFORMATION transformation)
{

    if (transformation == DI_TRANSFORMATION_HAESCH) {
        GB_ERROR error = haeschoe("outfile");
        if (error) return error;
        return "Your matrices have been written to 'outfile'\nSorry I can not make a tree";
    }
    int user_matrix_enabled = awr->awar(AWAR_DIST_MATRIX_DNA_ENABLED)->read_int();
    if (user_matrix_enabled){   // set transformation Matrix
        switch (transformation){
            case DI_TRANSFORMATION_NONE:
            case DI_TRANSFORMATION_SIMILARITY:
            case DI_TRANSFORMATION_JUKES_CANTOR:
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

    long   s_len = aliview->get_length();
    long   hits[AP_MAX][AP_MAX];
    int    row;
    int    col;
    size_t i;

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
        case DI_TRANSFORMATION_FELSENSTEIN:
            this->calculate_overall_freqs(rel_frequencies,cancel_columns);
            S_square = 0.0;
            for (i=0;i<AP_MAX; i++) S_square+= rel_frequencies[i]*rel_frequencies[i];
            break;
        default:    break;
    };

    for (row = 0; row<nentries;row++){
        if ( nentries > 100 || ((row&0xf) == 0)){
            double gauge = (double)row/(double)nentries;
            if (aw_status(gauge*gauge)) {
                aw_closestatus();
                return "Aborted";
            }
        }
        for (col=0; col<=row; col++) {
            columns    = 0;
            const unsigned char *seq1 = entries[row]->sequence_parsimony->get_usequence();
            const unsigned char *seq2 = entries[col]->sequence_parsimony->get_usequence();
            b          = 0.0;
            switch(transformation){
                case  DI_TRANSFORMATION_JUKES_CANTOR:
                    b = 0.75;
                case  DI_TRANSFORMATION_NONE:
                case  DI_TRANSFORMATION_SIMILARITY:{
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
                                }
                                else {
                                    diffsum += hits[x][y] * DI_dna_matrix.get(x,y);
                                    all_sum += hits[x][y] * DI_dna_matrix.get(x,y);
                                }
                            }
                        }
                        dist = diffsum / all_sum;
                    }
                    else {
                        int pos;
                        int b1,b2;
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
                    if (transformation==DI_TRANSFORMATION_SIMILARITY){
                        dist =  (1.0-dist);
                    }
                    else if (b) {
                        double sigma;
                        dist = this->corr(dist,b,sigma);
                    }
                    matrix->set(row,col,dist);
                    break;
                }
                case DI_TRANSFORMATION_KIMURA:
                case DI_TRANSFORMATION_OLSEN:
                case DI_TRANSFORMATION_FELSENSTEIN:{
                    int    pos;
                    double dist = 0.0;
                    long   N,P,Q,M;
                    double p,q;
                    
                    memset((char *)hits,0,sizeof(long) * AP_MAX * AP_MAX);
                    for (pos = s_len; pos >=0; pos--){
                        hits[*(seq1++)][*(seq2++)]++;
                    }
                    switch(transformation){
                        case DI_TRANSFORMATION_KIMURA:
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

                        case DI_TRANSFORMATION_OLSEN:
                        case DI_TRANSFORMATION_FELSENSTEIN:

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
                            if (transformation == DI_TRANSFORMATION_OLSEN){ // Calc sum square freq individually for each line
                                S_square = 0.0;
                                for (i=0;i<AP_MAX; i++) S_square+= frequencies[i]*frequencies[i];
                                b = 1.0 - S_square/((double)N*(double)N);
                            }
                            else {
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

GB_ERROR DI_MATRIX::calculate_pro(DI_TRANSFORMATION transformation){
    di_cattype whichcat;
    di_codetype whichcode = universal;

    switch (transformation){
        case DI_TRANSFORMATION_NONE:
            whichcat = none;
            break;
        case DI_TRANSFORMATION_SIMILARITY:
            whichcat = similarity;
            break;
        case DI_TRANSFORMATION_KIMURA:
            whichcat = kimura;
            break;
        case DI_TRANSFORMATION_PAM:
            whichcat = pam;
            break;
        case DI_TRANSFORMATION_CATEGORIES_HALL:
            whichcat = hall;
            break;
        case DI_TRANSFORMATION_CATEGORIES_BARKER:
            whichcat = george;
            break;
        case DI_TRANSFORMATION_CATEGORIES_CHEMICAL:
            whichcat = chemical;
            break;
        default:
            return "This correction is not available for protein data";
    }
    matrix = new AP_smatrix(nentries);

    di_protdist prodist(whichcode, whichcat, nentries, entries, aliview->get_length(), matrix);
    return prodist.makedists();
}

static GB_ERROR di_calculate_matrix(AW_window *aww, const WeightedFilter *weighted_filter, bool bootstrap_flag, bool show_warnings) {
    // returns "Aborted" if stopped by user
    GB_push_transaction(GLOBAL_gb_main);
    if (DI_MATRIX::ROOT){
        GB_pop_transaction(GLOBAL_gb_main);
        return 0;
    }
    AW_root *aw_root = aww->get_root();
    char    *use = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
    long ali_len = GBT_get_alignment_len(GLOBAL_gb_main,use);
    if (ali_len<=0) {
        GB_pop_transaction(GLOBAL_gb_main);
        delete use;
        aw_message("Please select a valid alignment");
        return "Error";
    }
    if (!bootstrap_flag){
        aw_openstatus("Calculating Matrix");
        aw_status("Read the database");
    }

    char *cancel = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();

    AliView *aliview = weighted_filter->create_aliview(use);
    if (bootstrap_flag) aliview->get_filter()->enable_bootstrap();

    char *load_what      = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
    char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();

    LoadWhat all_flag = (strcmp(load_what,"all") == 0) ? DI_LOAD_ALL : DI_LOAD_MARKED;
    GB_ERROR error    = 0;
    {
        DI_MATRIX *phm   = new DI_MATRIX(*aliview, aw_root);
        phm->matrix_type = DI_MATRIX_FULL;
        phm->load(all_flag, sort_tree_name, show_warnings, NULL);
        
        free(sort_tree_name);
        GB_pop_transaction(GLOBAL_gb_main);
        if (aw_status()) {
            phm->unload();
            error = "Aborted";
        }
        else {
            DI_TRANSFORMATION trans = (DI_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();

            if (phm->is_AA) error = phm->calculate_pro(trans);
            else error            = phm->calculate(aw_root,cancel,0.0,trans);
        }
        if (!bootstrap_flag) aw_closestatus();
        delete phm->ROOT;
        if (error && strcmp(error,"Aborted")){
            aw_message(error);
            delete phm;
            phm->ROOT = 0;
        }
        else {
            phm->ROOT = phm;
        }
    }
    free(load_what);

    free(cancel);
    delete aliview; 

    free(use);
    return error;
}

static void di_mark_by_distance(AW_window *aww, AW_CL cl_weightedFilter) {
    AW_root *aw_root    = aww->get_root();
    double   lowerBound = aw_root->awar(AWAR_DIST_MIN_DIST)->read_float();
    double   upperBound = aw_root->awar(AWAR_DIST_MAX_DIST)->read_float();

    GB_ERROR error = 0;
    if (lowerBound >= upperBound) {
        error = GBS_global_string("Lower bound (%f) has to be smaller than upper bound (%f)", lowerBound, upperBound);
    }
    else if (lowerBound<0.0 || lowerBound > 1.0) {
        error = GBS_global_string("Lower bound (%f) is not in allowed range [0.0 .. 1.0]", lowerBound);
    }
    else if (upperBound<0.0 || upperBound > 1.0) {
        error = GBS_global_string("Upper bound (%f) is not in allowed range [0.0 .. 1.0]", upperBound);
    }
    else {
        GB_transaction ta(GLOBAL_gb_main);

        char   *selected    = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_selected = 0;

        if (!selected[0]) {
            error = "Please select a species";
        }
        else {
            gb_selected = GBT_find_species(GLOBAL_gb_main, selected);
            if (!gb_selected) {
                error = GBS_global_string("Couldn't find species '%s'", selected);
            }
            else {
                char              *use             = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
                char              *cancel          = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();
                AP_smatrix        *ap_ratematrix   = 0;
                DI_TRANSFORMATION  trans           = (DI_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();
                WeightedFilter    *weighted_filter = (WeightedFilter*)cl_weightedFilter;
                AliView           *aliview         = weighted_filter->create_aliview(use);

                if (!error) {
                    aw_openstatus("Mark species by distance");
                    aw_status("Calculating distances");
                    aw_status(0.0);

                    DI_MATRIX *old_root = DI_MATRIX::ROOT;
                    DI_MATRIX::ROOT = 0;

                    size_t speciesCount = GBT_get_species_count(GLOBAL_gb_main);
                    size_t count        = 0;
                    bool markedSelected = false;

                    for (GBDATA *gb_species = GBT_first_species(GLOBAL_gb_main);
                         gb_species;
                         gb_species = GBT_next_species(gb_species))
                    {
                        DI_MATRIX *phm         = new DI_MATRIX(*aliview, aw_root);
                        phm->matrix_type       = DI_MATRIX_FULL;
                        GBDATA *species_pair[] = { gb_selected, gb_species, NULL };

                        phm->load(DI_LOAD_LIST, NULL, false, species_pair);

                        if (phm->is_AA){
                            error = phm->calculate_pro(trans);
                        }
                        else {
                            error = phm->calculate(aw_root,cancel,0.0,trans);
                        }

                        double dist_value = phm->matrix->get(0, 1);                         // distance or conformance
                        bool   mark       = (lowerBound <= dist_value && dist_value <= upperBound);
                        GB_write_flag(gb_species, mark);

                        if (!markedSelected) {
                            dist_value = phm->matrix->get(0, 0);                                     // distance or conformance to self
                            mark       = (lowerBound <= dist_value && dist_value <= upperBound);
                            GB_write_flag(gb_selected, mark);
                            
                            markedSelected = true;
                        }

                        delete phm;
                        aw_status(++count/(double)speciesCount);
                    }

                    di_assert(DI_MATRIX::ROOT == 0);
                    DI_MATRIX::ROOT = old_root;

                    aw_closestatus();
                }

                delete ap_ratematrix;
                delete aliview;
                free(cancel);
                free(use);
            }
        }

        free(selected);
        error = ta.close(error);
    }

    if (error) {
        aw_message(error);
    }
}

static void selected_species_changed_cb(AW_root* , AW_CL cl_viewer) {
    if (di_dmatrix) {
        AW_window *viewer = reinterpret_cast<AW_window*>(cl_viewer);
        void       redisplay_needed(AW_window *,DI_dmatrix *dis);
        redisplay_needed(viewer, di_dmatrix);
    }
}

static void di_view_matrix_cb(AW_window *aww, AW_CL cl_sparam) {
    save_matrix_params *sparam = (save_matrix_params*)cl_sparam;
    GB_ERROR            error  = di_calculate_matrix(aww, sparam->weighted_filter, 0, true);
    if (error) return;

    if (!di_dmatrix) di_dmatrix = new DI_dmatrix();

    static AW_window *viewer = 0;
    if (!viewer) {
        AW_root *aw_root = aww->get_root();
        viewer           = DI_create_view_matrix_window(aw_root, di_dmatrix, sparam);
        
        AW_awar *awar_sel = aw_root->awar(AWAR_SPECIES_NAME);
        awar_sel->add_callback(selected_species_changed_cb, AW_CL(viewer));
    }

    di_dmatrix->init();
    di_dmatrix->display(false);
    
    viewer->activate();
}

static void di_save_matrix_cb(AW_window *aww, AW_CL cl_weightedFilter) {
    // save the matrix
    WeightedFilter *weighted_filter = (WeightedFilter*)cl_weightedFilter;
    GB_ERROR error = di_calculate_matrix(aww, weighted_filter, 0, true);
    if (!error) {
        char              *filename = aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_FILENAME)->read_string();
        enum DI_SAVE_TYPE  type     = (enum DI_SAVE_TYPE)aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_TYPE)->read_int();

        DI_MATRIX::ROOT->save(filename,type);
        free(filename);
    }
    awt_refresh_selection_box(aww->get_root(), AWAR_DIST_SAVE_MATRIX_BASE);
    aww->hide_or_notify(error);
}

AW_window *DI_create_save_matrix_window(AW_root *aw_root, save_matrix_params *save_params) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init( aw_root, "SAVE_MATRIX", "Save Matrix");
        aws->load_xfig("sel_box_user.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CANCEL","C");


        aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"save_matrix.hlp");
        aws->create_button("HELP", "HELP","H");

        aws->at("user");
        aws->create_option_menu(AWAR_DIST_SAVE_MATRIX_TYPE);
        aws->insert_default_option("Phylip Format (Lower Triangular Matrix)","P",DI_SAVE_PHYLIP_COMP);
        aws->insert_option("Readable (using NDS)","R",DI_SAVE_READABLE);
        aws->insert_option("Tabbed (using NDS)","R",DI_SAVE_TABBED);
        aws->update_option_menu();

        awt_create_selection_box((AW_window *)aws, save_params->awar_base);

        aws->at("save2");
        aws->callback(di_save_matrix_cb, (AW_CL)save_params->weighted_filter);
        aws->create_button("SAVE", "SAVE","S");

        aws->callback( (AW_CB0)AW_POPDOWN);
        aws->at("cancel2");
        aws->create_button("CLOSE", "CANCEL","C");
    }
    return aws;
}

static AW_window *awt_create_select_cancel_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_CHARS_TO_CANCEL_COLUMN", "CANCEL SELECT");
    aws->load_xfig("di_cancel.fig");

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

static int di_calculate_tree_show_status(int loop_count, int bootstrap_count) {
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

static void di_calculate_tree_cb(AW_window *aww, AW_CL cl_weightedFilter, AW_CL bootstrap_flag)
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

    WeightedFilter *weighted_filter = (WeightedFilter*)cl_weightedFilter;

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
            di_calculate_tree_show_status(loop_count, bootstrap_count);

            di_assert(DI_MATRIX::ROOT == 0);
            di_calculate_matrix(aww, weighted_filter, bootstrap_flag, true);

            DI_MATRIX *matr = DI_MATRIX::ROOT;
            if (!matr) {
                error = "unexpected error in di_calculate_matrix_cb (data missing)";
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
                if (di_calculate_tree_show_status(loop_count, bootstrap_count)) {
                    break; // user abort
                }
                last_status_upd = loop_count;
            }

        }

        delete DI_MATRIX::ROOT;
        DI_MATRIX::ROOT = 0;

        error = di_calculate_matrix(aww, weighted_filter, bootstrap_flag, !bootstrap_flag);
        if (error && strcmp(error,"Aborted") == 0) {
            error = 0;          // clear error (otherwise no tree will be read below)
            break;              // end of bootstrap
        }

        if (!DI_MATRIX::ROOT) {
            error = "unexpected error in di_calculate_matrix_cb (data missing)";
            break;
        }

        DI_MATRIX  *matr  = DI_MATRIX::ROOT;
        char     **names = (char **)calloc(sizeof(char *),(size_t)matr->nentries+2);
        for (long i=0;i<matr->nentries;i++){
            names[i] = matr->entries[i]->name;
        }
        tree = neighbourjoining(names,matr->matrix->m,matr->nentries,sizeof(GBT_TREE));

        if (bootstrap_flag){
            insert_ctree(tree,1);
            GBT_delete_tree(tree); tree = 0;
        }
        free(names);

    } while (bootstrap_flag && loop_count != bootstrap_count);

    if (!error) {
        if (bootstrap_flag) tree = get_ctree();

        char *tree_name = aw_root->awar(AWAR_DIST_TREE_STD_NAME)->read_string();
        GB_begin_transaction(GLOBAL_gb_main);
        error = GBT_write_tree(GLOBAL_gb_main,0,tree_name,tree);

        if (!error) {
            char       *filter_name = AWT_get_combined_filter_name(aw_root, "dist");
            int         transr      = aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();
            const char *comment     = GBS_global_string("PRG=dnadist CORR=%s FILTER=%s PKG=ARB", enum_trans_to_string[transr], filter_name);

            error = GBT_write_tree_rem(GLOBAL_gb_main,tree_name,comment);
            free(filter_name);
        }

        error = GB_end_transaction(GLOBAL_gb_main, error);
        free(tree_name);
    }

    if (tree) GBT_delete_tree(tree);

    if (close_stat) aw_closestatus();
    aw_status();                      // remove 'abort' flag

    if (bootstrap_flag) {
        if (all_names) GBT_free_names(all_names);
    }
#if defined(DEBUG)
    else {
        di_assert(all_names == 0);
    }
#endif // DEBUG

    delete DI_MATRIX::ROOT; DI_MATRIX::ROOT = 0;

    if (error) aw_message(error);
}


static void di_autodetect_callback(AW_window *aww)
{
    GB_push_transaction(GLOBAL_gb_main);
    if (DI_MATRIX::ROOT){
        delete DI_MATRIX::ROOT;
        DI_MATRIX::ROOT = 0;
    }
    AW_root *aw_root = aww->get_root();
    char    *use = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
    long ali_len = GBT_get_alignment_len(GLOBAL_gb_main,use);
    if (ali_len<=0) {
        GB_pop_transaction(GLOBAL_gb_main);
        delete use;
        aw_message("Please select a valid alignment");
        return;
    }

    aw_openstatus("Analyzing data...");
    aw_status("Read the database");

    char    *filter_str = aw_root->awar(AWAR_DIST_FILTER_FILTER)->read_string();
    char    *cancel = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();

    AliView *aliview;
    {
        AP_filter *ap_filter = NULL;
        long       flen  = strlen(filter_str);

        if (flen == ali_len){
            ap_filter = new AP_filter(filter_str, "0", ali_len);
        }
        else {
            if (flen) {
                aw_message("WARNING: YOUR FILTER LEN IS NOT THE ALIGNMENT LEN\nFILTER IS TRUNCATED WITH ZEROS OR CUTTED");
                ap_filter = new AP_filter(filter_str, "0", ali_len);
            }
            else {
                ap_filter = new AP_filter(ali_len); // unfiltered
            }
        }

        AP_weights ap_weights(ap_filter);

        aliview = new AliView(GLOBAL_gb_main, *ap_filter, ap_weights, use);
        delete ap_filter;
    }

    DI_MATRIX *phm = new DI_MATRIX(*aliview,aw_root);
    
    char *load_what      = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
    char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();

    LoadWhat all_flag = (strcmp(load_what,"all") == 0) ? DI_LOAD_ALL : DI_LOAD_MARKED;

    delete phm->ROOT;
    phm->ROOT = 0;

    phm->load(all_flag, sort_tree_name, true, NULL);
    free(sort_tree_name);
    GB_pop_transaction(GLOBAL_gb_main);

    aw_status("Search Correction");

    DI_TRANSFORMATION trans;
    trans = (DI_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();

    phm->analyse();
    aw_closestatus();

    delete phm;

    free(load_what);

    free(cancel);
    delete aliview;

    free(use);
    free(filter_str);
}

ATTRIBUTED(__ATTR__NORETURN, static void di_exit(AW_window *aww)) {
    if (GLOBAL_gb_main) {
        aww->get_root()->unlink_awars_from_DB(GLOBAL_gb_main);
        GB_close(GLOBAL_gb_main);
    }
    exit(0);
}

static void di_calculate_full_matrix_cb(AW_window *aww, AW_CL cl_weightedFilter){
    if (DI_MATRIX::ROOT && DI_MATRIX::ROOT->matrix_type == DI_MATRIX_FULL) return;
    delete_matrix_cb(0);
    WeightedFilter *weighted_filter = (WeightedFilter*)cl_weightedFilter;
    di_calculate_matrix(aww, weighted_filter, 0, true);
    if (di_dmatrix){
        di_dmatrix->resized();
        di_dmatrix->display(false);
    }
}

static void di_compress_tree_cb(AW_window *aww, AW_CL cl_weightedFilter) {
    GB_transaction dummy(GLOBAL_gb_main);
    char *treename = aww->get_root()->awar(AWAR_DIST_TREE_COMP_NAME)->read_string();
    GB_ERROR error = 0;
    GBT_TREE *tree = GBT_read_tree(GLOBAL_gb_main,treename,sizeof(GBT_TREE));
    if (!tree) error = "Please select a valid tree first";

    if (DI_MATRIX::ROOT && DI_MATRIX::ROOT->matrix_type!=DI_MATRIX_COMPRESSED){
        delete_matrix_cb(0);        // delete wrong matrix !!!
    }

    WeightedFilter *weighted_filter = (WeightedFilter*)cl_weightedFilter;
    di_calculate_matrix(aww, weighted_filter, 0, true);
    if (!DI_MATRIX::ROOT) error = "Cannot calculate your matrix";

    if (!error) {
        error = DI_MATRIX::ROOT->compress(tree);
    }

    if (tree) GBT_delete_tree(tree);

    if (error) {
        aw_message(error);
    }
    else {
        if (di_dmatrix){
            di_dmatrix->resized();
            di_dmatrix->display(false);
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

  aws->callback(di_compress_tree_cb);
  aws->help_text("compress_tree.hlp");
  aws->create_button("GO", "DO IT","D");

  aws->callback(AW_POPUP_HELP,(AW_CL)"compress_tree.hlp");
  aws->create_button("HELP", "HELP","H");

  aws->window_fit();
  return (AW_window *)aws;
  }
*/

static void di_define_sort_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->write_string(tree_name);
    free(tree_name);
}
static void di_define_compression_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_COMP_NAME)->write_string(tree_name);
    free(tree_name);
}

static void di_define_save_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_STD_NAME)->write_string(tree_name);
    free(tree_name);
}


AW_window *DI_create_matrix_window(AW_root *aw_root) {
    AW_window_simple_menu *aws = new AW_window_simple_menu;
    aws->init( aw_root, "NEIGHBOUR JOINING", "NEIGHBOUR JOINING [ARB_DIST]");
    aws->load_xfig("di_ge_ma.fig");
    aws->button_length( 10 );

    aws->at("close");
    aws->callback(di_exit);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"dist.hlp");
    aws->create_button("HELP", "HELP","H");


    GB_push_transaction(GLOBAL_gb_main);

#if defined(DEBUG)
    AWT_create_debug_menu(aws);
#endif // DEBUG

    aws->create_menu("FILE", "F", AWM_ALL);
    aws->insert_menu_topic("macros", "Macros  ...", "M", "macro.hlp", AWM_ALL, (AW_CB)AW_POPUP, (AW_CL)awt_open_macro_window,(AW_CL)"NEIGHBOUR_JOINING");
    aws->insert_menu_topic("quit",   "Quit",        "Q", "quit.hlp",  AWM_ALL, (AW_CB)di_exit,  0,  0                                                 );

    aws->create_menu("Properties", "P", AWM_ALL);
    aws->insert_menu_topic("frame_props", "Frame ...",                                 "F", "props_frame.hlp", AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0);
    aws->insert_menu_topic("save_props",  "Save Properties (in ~/.arb_prop/dist.arb)", "S", "savedef.hlp",     AWM_ALL, (AW_CB)AW_save_defaults, 0, 0       );

    aws->insert_help_topic("help ...", "h", "dist.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"dist.hlp", 0);

    // ------------------
    //      left side

    aws->at( "which_species" );
    aws->create_option_menu( AWAR_DIST_WHICH_SPECIES );
    aws->insert_option( "all", "a", "all" );
    aws->insert_default_option( "marked",  "m", "marked" );
    aws->update_option_menu();

    aws->at("which_alignment");
    awt_create_selection_list_on_ad(GLOBAL_gb_main, (AW_window *)aws,AWAR_DIST_ALIGNMENT,"*=");

    // filter & weights

    WeightedFilter *weighted_filter = // do NOT free (bound to callbacks)
        new WeightedFilter(GLOBAL_gb_main, aws->get_root(), AWAR_DIST_FILTER_NAME, AWAR_DIST_COLUMN_STAT_NAME);

    aws->at("filter_select");
    aws->callback(AW_POPUP, (AW_CL)awt_create_select_filter_win, (AW_CL)(weighted_filter->get_adfiltercbstruct()));
    aws->create_button("SELECT_FILTER", AWAR_DIST_FILTER_NAME);

    aws->at("weights_select");
    aws->sens_mask(AWM_EXP);
    aws->callback(AW_POPUP,(AW_CL)create_csp_window, (AW_CL)weighted_filter->get_csp());
    aws->create_button("SELECT_COL_STAT",AWAR_DIST_COLUMN_STAT_NAME);
    aws->sens_mask(AWM_ALL);

    aws->at("which_cancel");
    aws->create_input_field(AWAR_DIST_CANCEL_CHARS,12);

    aws->at("cancel_select");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)awt_create_select_cancel_window);
    aws->create_button("SELECT_CANCEL_CHARS", "Info","C");

    aws->at("change_matrix");
    aws->callback(AW_POPUP,(AW_CL)create_dna_matrix_window,0);
    aws->create_button("EDIT_MATRIX", "Edit Matrix");

    aws->at("enable");
    aws->create_toggle(AWAR_DIST_MATRIX_DNA_ENABLED);

    aws->at("which_correction");
    aws->create_option_menu(AWAR_DIST_CORR_TRANS, NULL ,"");
    aws->insert_option("none",                    "n", (int)DI_TRANSFORMATION_NONE               );
    aws->insert_option("similarity",              "n", (int)DI_TRANSFORMATION_SIMILARITY         );
    aws->insert_option("jukes-cantor (dna)",      "c", (int)DI_TRANSFORMATION_JUKES_CANTOR       );
    aws->insert_option("felsenstein (dna)",       "f", (int)DI_TRANSFORMATION_FELSENSTEIN        );
    aws->insert_option("olsen (dna)",             "o", (int)DI_TRANSFORMATION_OLSEN              );
    aws->insert_option("felsenstein/voigt (exp)", "1", (int)DI_TRANSFORMATION_FELSENSTEIN_VOIGT  );
    aws->insert_option("olsen/voigt (exp)",       "2", (int)DI_TRANSFORMATION_OLSEN_VOIGT        );
    aws->insert_option("haesch (exp)",            "f", (int)DI_TRANSFORMATION_HAESCH             );
    aws->insert_option("kimura (pro)",            "k", (int)DI_TRANSFORMATION_KIMURA             );
    aws->insert_option("PAM (protein)",           "c", (int)DI_TRANSFORMATION_PAM                );
    aws->insert_option("Cat. Hall(exp)",          "c", (int)DI_TRANSFORMATION_CATEGORIES_HALL    );
    aws->insert_option("Cat. Barker(exp)",        "c", (int)DI_TRANSFORMATION_CATEGORIES_BARKER  );
    aws->insert_option("Cat.Chem (exp)",          "c", (int)DI_TRANSFORMATION_CATEGORIES_CHEMICAL);
    aws->insert_default_option("unknown", "u", (int)DI_TRANSFORMATION_NONE);

    aws->update_option_menu();

    aws->at("autodetect");   // auto
    aws->callback(di_autodetect_callback);
    aws->sens_mask(AWM_EXP);
    aws->create_button("AUTODETECT_CORRECTION", "AUTODETECT","A");
    aws->sens_mask(AWM_ALL);

    // -------------------
    //      right side


    aws->at("mark_distance");
    aws->callback(di_mark_by_distance, (AW_CL)weighted_filter);
    aws->create_autosize_button("MARK_BY_DIST", "Mark all species");

    aws->at("mark_lower");
    aws->create_input_field(AWAR_DIST_MIN_DIST, 5);
    
    aws->at("mark_upper");
    aws->create_input_field(AWAR_DIST_MAX_DIST, 5);

    // -----------------

    // tree selection

    aws->at("tree_list");
    awt_create_selection_list_on_trees(GLOBAL_gb_main,(AW_window *)aws,AWAR_DIST_TREE_CURR_NAME);

    aws->at("detect_clusters");
    aws->callback(AW_POPUP, (AW_CL)DI_create_cluster_detection_window, (AW_CL)weighted_filter);
    aws->create_autosize_button("DETECT_CLUSTERS", "Detect homogenous clusters in tree","D");

    // matrix calculation

    aws->button_length(18);
    aws->at("calculate");
    aws->callback(di_calculate_full_matrix_cb, (AW_CL)weighted_filter);
    aws->create_button("CALC_FULL_MATRIX", "Calculate\nFull Matrix","F");
    aws->at("compress");
    aws->callback(di_compress_tree_cb, (AW_CL)weighted_filter);
    aws->create_button("CALC_COMPRESSED_MATRIX", "Calculate\nCompressed Matrix","C");

    aws->button_length(13);

    {
        save_matrix_params *sparams = new save_matrix_params; // do not free (bound to callbacks)

        sparams->awar_base       = AWAR_DIST_SAVE_MATRIX_BASE;
        sparams->weighted_filter = weighted_filter;

        aws->at("save_matrix");
        aws->callback(AW_POPUP, (AW_CL)DI_create_save_matrix_window,(AW_CL)sparams);
        aws->create_button("SAVE_MATRIX", "Save matrix","M");

        aws->at("view_matrix");
        aws->callback(di_view_matrix_cb, (AW_CL)sparams);
        aws->create_button("VIEW_MATRIX", "View matrix","V");
    }

    aws->button_length(22);
    aws->at("use_compr_tree");
    aws->callback(di_define_compression_tree_name_cb);
    aws->create_button("USE_COMPRESSION_TREE", "Use to compress", "");
    aws->at("use_sort_tree");
    aws->callback(di_define_sort_tree_name_cb);
    aws->create_button("USE_SORT_TREE", "Use to sort", "");

    aws->at("compr_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_COMP_NAME,12);
    aws->at("sort_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_SORT_NAME,12);

    // tree calculation

    aws->button_length(18);
    aws->at("t_calculate");
    aws->callback(di_calculate_tree_cb, (AW_CL)weighted_filter, 0);
    aws->create_button("CALC_TREE", "Calculate \ntree","C");
    aws->at("bootstrap");
    aws->callback(di_calculate_tree_cb, (AW_CL)weighted_filter, 1);
    aws->create_button("CALC_BOOTSTRAP_TREE", "Calculate \nbootstrap tree");

    aws->button_length(22);
    aws->at("use_existing");
    aws->callback(di_define_save_tree_name_cb);
    aws->create_button("USE_NAME", "Use as new tree name", "");

    aws->at("calc_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_STD_NAME,12);
    
    aws->at("bcount");
    aws->create_input_field(AWAR_DIST_BOOTSTRAP_COUNT, 7);

    GB_pop_transaction(GLOBAL_gb_main);
    return (AW_window *)aws;
}
