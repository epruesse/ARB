#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <math.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>

#include "ap_pos_var_pars.hxx"

#define ap_assert(cond) arb_assert(cond)

extern GBDATA *gb_main;


AP_pos_var::AP_pos_var(GBDATA *gb_maini,char *ali_namei, long ali_leni, int isdna, char *tree_namei) {
    memset((char  *)this, 0, sizeof(AP_pos_var) ) ;
    this->gb_main   = gb_maini;
    this->ali_name  = strdup(ali_namei);
    this->is_dna    = isdna;
    this->ali_len   = ali_leni;
    this->tree_name = strdup(tree_namei);
}

AP_pos_var::~AP_pos_var() {
    free(ali_name);
    free(tree_name);
    free(transitions);
    free(transversions);
    int i;
    for (i=0;i<256;i++) free(frequencies[i]);
}

long    AP_pos_var::getsize(GBT_TREE *tree){
    if (!tree) return 0;
    if (tree->is_leaf) return 1;
    return getsize(tree->leftson) + getsize(tree->rightson) + 1;
}

const char *AP_pos_var::parsimony(GBT_TREE *tree, GB_UINT4 *bases, GB_UINT4 *tbases){
    GB_ERROR error = 0;
    timer ++;
    register long i;
    register long l,r;

    if (tree->is_leaf) {
        unsigned char *sequence;
        long           seq_len = ali_len;
        if (!tree->gb_node) return 0; // zombie

        GBDATA *gb_data = GBT_read_sequence(tree->gb_node,ali_name);
        if (!gb_data) return 0; // no sequence
        if (GB_read_string_count(gb_data) < seq_len)
            seq_len     = GB_read_string_count(gb_data);
        sequence        = (unsigned char*)GB_read_char_pntr(gb_data);

        for (i = 0; i< seq_len; i++ ) {
            l = char_2_freq[sequence[i]];
            if (l) {
                ap_assert(frequencies[l]);
                frequencies[l][i]++;
            }
        }

        if (bases){
            for (i = 0; i< seq_len; i++ ) bases[i] = char_2_transition[sequence[i]];
        }
        if (tbases){
            for (i = 0; i< seq_len; i++ ) tbases[i] = char_2_transversion[sequence[i]];
        }
        return 0;
    }
    if (aw_status(timer/(double)treesize)) return "Operation aborted";

    GB_UINT4 *ls = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);
    GB_UINT4 *rs = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);
    GB_UINT4 *lts = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);
    GB_UINT4 *rts = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);

    if (!error) error = this->parsimony(tree->leftson,ls,lts);
    if (!error) error = this->parsimony(tree->rightson,rs,rts);
    if (!error){
        for (i=0; i< ali_len; i++ ) {
            l = ls[i];
            r = rs[i];
            if ( l & r ) {
                if (bases) bases[i] = l&r;
            }else{
                transitions[i] ++;
                if (bases) bases[i] = l|r;
            }
            l = lts[i];
            r = rts[i];
            if ( l & r ) {
                if (tbases) tbases[i] = l&r;
            }else{
                transversions[i] ++;
                if (tbases) tbases[i] = l|r;
            }
        }
    }

    free(lts);
    free(rts);

    free(ls);
    free(rs);
    return error;
}


// Calculate the positional variability: control procedure
GB_ERROR AP_pos_var::retrieve( GBT_TREE *tree){
    GB_ERROR error = 0;
    int i;

    if (is_dna) {
        long           base;
        unsigned char *char_2_bitstring;
        char_2_freq[(unsigned char)'a'] = 'A';
        char_2_freq[(unsigned char)'A'] = 'A';
        char_2_freq[(unsigned char)'c'] = 'C';
        char_2_freq[(unsigned char)'C'] = 'C';
        char_2_freq[(unsigned char)'g'] = 'G';
        char_2_freq[(unsigned char)'G'] = 'G';
        char_2_freq[(unsigned char)'t'] = 'U';
        char_2_freq[(unsigned char)'T'] = 'U';
        char_2_freq[(unsigned char)'u'] = 'U';
        char_2_freq[(unsigned char)'U'] = 'U';
        char_2_bitstring = (unsigned char *)AP_create_dna_to_ap_bases();
        for (i=0;i<256;i++) {
            int j;
            if (i=='-') j = '.'; else j = i;
            base = char_2_transition[i] = char_2_bitstring[j];
            char_2_transversion[i] = 0;
            if (base & (AP_A | AP_G) ) char_2_transversion[i] = 1;
            if (base & (AP_C | AP_T) ) char_2_transversion[i] |= 2;
        }
        delete [] char_2_bitstring;
    }
    else {
        long base;
        awt_pro_a_nucs_convert_init(gb_main);
        long *char_2_bitstring = awt_pro_a_nucs->pro_2_bitset;
        for (i=0;i<256;i++){
            char_2_transversion[i] = 0;
            base = char_2_transition[i] = char_2_bitstring[i];
            if (base) char_2_freq[i] = toupper(i);
        }
        delete [] char_2_bitstring;
    }
    this->treesize = this->getsize(tree);
    this->timer = 0;

    for (i=0;i<256;i++) {
        int j;
        if ( (j = char_2_freq[i]) ) {
            if (!frequencies[j]) {
                frequencies[j] = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);
            }
        }
    }

    transitions = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);
    transversions = (GB_UINT4 *)calloc(sizeof(GB_UINT4),(int)ali_len);

    error = this->parsimony(tree);

    return error;
}

char *AP_pos_var::delete_old_sai( char *sai_name ){
    GBDATA *gb_extended = GBT_find_SAI(gb_main,sai_name);
    if (!gb_extended) return 0;
    GBDATA *gb_ali = GB_search(gb_extended, ali_name, GB_FIND);
    if (!gb_ali) return 0;
    GB_ERROR error = GB_delete(gb_ali);
    return (char *)error;
}

char *AP_pos_var::save_sai( char *sai_name ){
    char buffer[1024];
    double logbase = sqrt(2.0);
    double b = .75;
    double max_rate = 1.0;
    int i,j;


    GBDATA *gb_extended = GBT_create_SAI(gb_main,sai_name);


    {   sprintf(buffer,"%s/_TYPE",ali_name);
    GBDATA *gb_description  = GB_search( gb_extended, buffer, GB_STRING);
    sprintf(buffer,"PVP: Positional Variability by Parsimony: tree '%s' ntaxa %li",tree_name, treesize/2);
    GB_write_string( gb_description,buffer);
    }


    char *data = (char *)calloc(1,(int)ali_len+1);
    int *sum = (int *)calloc(sizeof(int), (int)ali_len);
    for (j=0;j<256;j++) {       // get sum of frequencies
        if (!frequencies[j]) continue;
        for (i=0;i<ali_len;i++) {
            sum[i] += frequencies[j][i];
        }
        char freqname[100];
        if (j<'A' || j>'Z') continue;
        sprintf(freqname,"FREQUENCIES/N%c",j);
        GBDATA *gb_freq = GBT_add_data( gb_extended, ali_name, freqname, GB_INTS);
        GB_write_ints(gb_freq,frequencies[j],ali_len);
    }
    {
        GBDATA *gb_transi = GBT_add_data( gb_extended, ali_name,"FREQUENCIES/TRANSITIONS", GB_INTS);
        GB_write_ints(gb_transi,transitions,ali_len);
        GBDATA *gb_transv = GBT_add_data( gb_extended, ali_name,"FREQUENCIES/TRANSVERSIONS", GB_INTS);
        GB_write_ints(gb_transv,transversions,ali_len);
    }
    int max_categ = 0;
    double lnlogbase = log(logbase);
    for (i=0;i<ali_len;i++) {
        double rate;
        if (sum[i] * 10 <= treesize) {
            data[i] = '.';
            continue;
        }
        rate = transitions[i]/ (double)sum[i];
        if (rate == 0.0) {
            data[i] = '-';
            continue;
        }
        if (rate >= b * .95) {
            rate = b * .95;
        }
        rate = -b * log(1-rate/b);
        if (rate > max_rate) rate = max_rate;
        rate /= max_rate;       // scaled  1.0 == fast rate
        // ~0.0 slow rate
        double dcat = -log(rate)/lnlogbase;
        int icat = (int)dcat;
        if (icat > 35) icat = 35;
        if (icat >= max_categ) max_categ = icat +1;
        data[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[icat];
    }

    GBDATA *gb_data  = GBT_add_data( gb_extended, ali_name, "data", GB_STRING);
    GB_write_string(gb_data,data);

    {       // Generate Categories
        void *strstruct = GBS_stropen(1000);
        for (i = 0; i<max_categ; i++) {
            GBS_floatcat(strstruct, pow(1.0/logbase, i) );
            GBS_chrcat(strstruct,' ');
        }
        char *h = GBS_strclose(strstruct);
        sprintf(buffer,"%s/_CATEGORIES",ali_name);
        GBDATA *gb_categories  = GB_search( gb_extended, buffer, GB_STRING);
        GB_write_string(gb_categories, h);
        free(h);
    }

    free(sum);
    free(data);
    return 0;
}

// Calculate the positional variability: window interface
void AP_calc_pos_var_pars(AW_window *aww) {
    AW_root        *root  = aww->get_root();
    GB_transaction  dummy(gb_main);
    char           *tree_name;
    GB_ERROR        error = 0;

    aw_openstatus("Calculating positional variability");
    aw_status("Loading Tree");
    GBT_TREE *tree;
    {           // get tree
        tree_name = root->awar(AWAR_PVP_TREE)->read_string();
        tree = GBT_read_tree(gb_main,tree_name,sizeof(GBT_TREE));
        if (!tree) {
            error = "Please select a valid tree";
        }
        else {
            GBT_link_tree(tree,gb_main, GB_TRUE);
        }
    }

    if (!error) {
        aw_status("Counting Mutations");

        char *ali_name = GBT_get_default_alignment(gb_main);
        long  ali_len  = GBT_get_alignment_len(gb_main,ali_name);

        if (ali_len <=0) {
            error = "Please select a valid alignment";
        }
        else {
            GB_alignment_type  at       = GBT_get_alignment_type(gb_main, ali_name);
            int                isdna    = at==GB_AT_DNA || at==GB_AT_RNA;
            char              *sai_name = root->awar(AWAR_PVP_SAI)->read_string();
            AP_pos_var         pv(gb_main, ali_name, ali_len, isdna, tree_name);

            error             = pv.delete_old_sai(sai_name);
            if (!error) error = pv.retrieve( tree);
            if (!error) error = pv.save_sai(sai_name);

            free(sai_name);
        }
        free(ali_name);
    }

    if (tree) GBT_delete_tree(tree);
    free(tree_name);

    aw_closestatus();

    if (error) aw_message(error);
    return;
}

AW_window *AP_open_pos_var_pars_window( AW_root *root ){

    GB_transaction dummy(gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CSP_BY_PARSIMONY", "Conservation Profile: Parsimony Method");
    aws->load_xfig("cpro/parsimony.fig");

    root->awar_string(AWAR_PVP_SAI, "POS_VAR_BY_PARSIMONY",AW_ROOT_DEFAULT);
    char *largest_tree = GBT_find_largest_tree(gb_main);
    root->awar_string(AWAR_PVP_TREE, "tree_full",AW_ROOT_DEFAULT);
    root->awar(AWAR_PVP_TREE)->write_string(largest_tree);
    free(largest_tree); largest_tree = 0;

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"pos_var_pars.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("name");
    aws->create_input_field(AWAR_PVP_SAI);

    aws->at("box");
    awt_create_selection_list_on_extendeds(gb_main,aws,AWAR_PVP_SAI);

    aws->at("trees");
    awt_create_selection_list_on_trees(gb_main,aws,AWAR_PVP_TREE);

    aws->at("go");
    aws->highlight();
    aws->callback(AP_calc_pos_var_pars);
    aws->create_button("GO","GO");

    return (AW_window *)aws;
}
