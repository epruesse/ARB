#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "awt.hxx"
#include "awt_tree.hxx"
#include "BI_helix.hxx"
#include "awt_csp.hxx"



void awt_csp_rescan_sais(AW_root *awr, AW_CL csp_cd){
    AWT_csp *csp = (AWT_csp *)csp_cd;
    GB_transaction dummy(csp->gb_main);
    free(csp->alignment_name);
    free(csp->type_path);
    csp->alignment_name = awr->awar(csp->awar_alignment)->read_string();
    csp->type_path = GBS_string_eval(csp->alignment_name,"*=*1/_TYPE",0);
    if (csp->sai_sel_box_id) {
        awt_create_selection_list_on_extendeds_update(0,csp->sai_sel_box_id);
    }
}

AWT_csp::AWT_csp(GBDATA *gb_maini, AW_root *awri, const char *awar_template) {
    // awar_template    .../name is replaced by /alignment
    memset((char *)this,0,sizeof(AWT_csp));
    this->gb_main = gb_maini;
    this->awr = awri;
    this->awar_name     = GBS_string_eval(awar_template,AWT_CSP_AWAR_CSP_NAME,0);
    this->awar_alignment    = GBS_string_eval(awar_template,AWT_CSP_AWAR_CSP_ALIGNMENT,0);
    this->awar_smooth   = GBS_string_eval(awar_template,AWT_CSP_AWAR_CSP_SMOOTH,0);
    this->awar_enable_helix = GBS_string_eval(awar_template,AWT_CSP_AWAR_CSP_ENABLE_HELIX,0);
    awr->awar_string(awar_name, "NONE");
    awr->awar_string(awar_alignment);
    awr->awar_int(awar_smooth);
    awr->awar_int(awar_enable_helix, 1);
    awr->awar(this->awar_alignment)->add_callback( awt_csp_rescan_sais, (AW_CL)this);
    awt_csp_rescan_sais(awr, (AW_CL)this);
}

void AWT_csp::exit(){
    delete [] weights; weights = NULL;
    delete [] rates;   rates   = NULL;
    delete [] ttratio; ttratio = NULL;
    delete [] is_helix;is_helix= NULL;
    delete [] mut_sum; mut_sum = NULL;
    delete [] freq_sum;freq_sum= NULL;
    delete desc;    desc    = NULL;
    int i;
    for (i=0;i<256;i++){
        delete [] frequency[i];
        frequency[i] = NULL;
    }
}
AWT_csp::~AWT_csp(void){
    this->exit();
    delete awar_name;
    delete awar_alignment;
    delete awar_smooth;
    delete awar_enable_helix;
}


GB_ERROR AWT_csp::go(AP_filter *filter){
    this->exit();
    GB_transaction dummy(this->gb_main);
    long alignment_length = GBT_get_alignment_len(this->gb_main, alignment_name);
    GB_ERROR error = 0;
    if (alignment_length <= 1)
        return GB_export_error( "Unknown Alignment Size: Name '%s'\n"
                                "   Select a Valid Alignment",alignment_name);
    if (filter && filter->filter_len != alignment_length)
        return GB_export_error( "Incompatible filter_len" );
    seq_len = 0;

    char *sai_name = awr->awar(awar_name)->read_string();
    GBDATA *gb_sai = GBT_find_SAI(this->gb_main, sai_name);
    if (!gb_sai)
        error= GB_export_error("Please select a valid Column Statistic");
    GBDATA *gb_ali = 0;
    GBDATA *gb_freqs = 0;
    if (!error) {
        gb_ali = GB_find(gb_sai,alignment_name,0,down_level);
        if (!gb_ali) error = GB_export_error("Please select a valid Column Statist");
    }
    if (!error) {
        gb_freqs = GB_find(gb_ali,"FREQUENCIES",0,down_level);
        if (!gb_ali) error = GB_export_error("Please select a valid Column Statist");
    }
    if (error) {
        free(sai_name);
        return error;
    }
    if (filter)         seq_len = (size_t)filter->real_len;

    else            seq_len = (size_t)alignment_length;

    unsigned long i;
    unsigned long j;
    delete [] weights; weights = new GB_UINT4[seq_len];
    delete [] rates;   rates   = new float[seq_len];
    delete [] ttratio; ttratio = new float[seq_len];
    delete [] is_helix;is_helix = new char[seq_len];
    delete [] mut_sum; mut_sum = new GB_UINT4[seq_len];
    delete [] freq_sum;freq_sum    = new GB_UINT4[seq_len];
    delete desc;    desc = 0;
    for (i=0;i<256;i++) { delete frequency[i];frequency[i]=0;}

    long use_helix = awr->awar(awar_enable_helix)->read_int();

    for (j=i=0;i<seq_len;i++) {
        is_helix[i] = 0;
        weights[i] = 1;
    }

    if (!error && use_helix) {            // calculate weigths and helix filter
        BI_helix helix;
        error = helix.init(this->gb_main,alignment_name);
        if (error){
            aw_message(error);
            error = 0;
            goto no_helix;
        }
        error = 0;
        for (j=i=0;i<(unsigned long)alignment_length;i++) {
            if (!filter || filter->filter_mask[i]) {
                if (helix.size > i && helix.entries[i].pair_type== HELIX_PAIR) {
                    is_helix[j] = 1;
                    weights[j] = 1;
                }else{
                    is_helix[j] = 0;
                    weights[j] = 2;
                }
                j++;
            }
        }
    }
 no_helix:

    for (i=0;i<seq_len;i++) freq_sum[i] = 0;

    GBDATA *gb_freq;
    GB_UINT4 *freqi[256];
    for (i=0;i<256; i++) freqi[i] = 0;
    int wf;                 // ********* read the frequence statistic
    for (   gb_freq = GB_find(gb_freqs, 0, 0, down_level);
            gb_freq;
            gb_freq = GB_find(gb_freq, 0, 0, this_level | search_next) ) {
        char *key = GB_read_key(gb_freq);
        if (key[0] == 'N' && key[1] && !key[2]) {
            wf = key[1];
            freqi[wf] = GB_read_ints(gb_freq);
        }
        free(key);
    }

    GB_UINT4 *minmut = 0;
    GBDATA *gb_minmut = GB_find(gb_freqs,"TRANSITIONS",0,down_level);
    if (gb_minmut) minmut = GB_read_ints(gb_minmut);

    GB_UINT4 *transver = 0;
    GBDATA *gb_transver = GB_find(gb_freqs,"TRANSVERSIONS",0,down_level);
    if (gb_transver) transver = GB_read_ints(gb_transver);
    unsigned long max_freq_sum = 0;
    for (wf = 0; wf<256;wf++) {     // ********* calculate sum of mutations
        if (!freqi[wf]) continue;   // ********* calculate nbases for each col.
        for (j=i=0;i<(unsigned long)alignment_length;i++) {
            if (filter && !filter->filter_mask[i]) continue;
            freq_sum[j] += freqi[wf][i];
            mut_sum[j] = minmut[i];
            j++;
        }
    }
    for (i=0;i<seq_len;i++)
        if (freq_sum[i] > max_freq_sum)
            max_freq_sum = freq_sum[i];
    unsigned long min_freq_sum = DIST_MIN_SEQ(max_freq_sum);

    for (wf = 0; wf<256;wf++) {
        if (!freqi[wf]) continue;
        frequency[wf] = new float[seq_len];
        for (j=i=0;i<(unsigned long)alignment_length;i++) {
            if (filter && !filter->filter_mask[i]) continue;
            if (freq_sum[j] > min_freq_sum) {
                frequency[wf][j] = freqi[wf][i]/(float)freq_sum[j];
            }else{
                frequency[wf][j] = 0;
                weights[j] = 0;
            }
            j++;
        }
    }

    for (j=i=0;i<(unsigned long)alignment_length;i++) { // ******* calculate rates
        if (filter && !filter->filter_mask[i]) continue;
        if (!weights[j]) {
            rates[j] = 1.0;
            ttratio[j] = 0.5;
            j++;
            continue;
        }
        rates[j] = (mut_sum[j] / (float)freq_sum[j]);
        if (transver[i] > 0) {
            ttratio[j] = (minmut[i] - transver[i]) / (float)transver[i];
        }else{
            ttratio[j] = 2.0;
        }
        if (ttratio[j] < 0.05) ttratio[j] = 0.05;
        if (ttratio[j] > 5.0) ttratio[j] = 5.0;
        j++;
    }

    // ****** normalize rates

    double sum_rates                   = 0;
    for (i=0;i<seq_len;i++) sum_rates += rates[i];
    sum_rates                         /= seq_len;
    for (i=0;i<seq_len;i++) rates[i]  /= sum_rates;

    free(transver);
    free(minmut);
    for (i=0;i<256;i++) free(freqi[i]);
    free(sai_name);

    return error;
}

void AWT_csp::print(void) {
    unsigned int j;
    int wf;
    double sum_rates[2], sum_tt[2];
    long count[2];
    sum_rates[0] = sum_rates[1] = sum_tt[0] = sum_tt[1] = 0;
    count[0] = count[1] = 0;
    if (!seq_len) return;
    for (j=0;j<seq_len;j++){
        if (!weights[j]) continue;
        if (is_helix[j]) {
            printf("#");
        }else{
            printf(".");
        }
        printf( "%i:    minmut %5i  freqs %5i   rates  %5f  tt %5f  ",
                j, mut_sum[j], freq_sum[j], rates[j], ttratio[j]);
        for (wf = 0;wf<256;wf++) {
            if (!frequency[wf]) continue;
            printf("%c:%5f ",wf,frequency[wf][j]);
        }
        sum_rates[is_helix[j]] += rates[j];
        sum_tt[is_helix[j]] += ttratio[j];
        count[is_helix[j]]++;
        printf("w: %i\n",weights[j]);
    }
    printf("Helical Rates %5f   Non Hel. Rates  %5f\n",
           sum_rates[1]/count[1], sum_rates[0]/count[0]);
    printf("Helical TT %5f  Non Hel. TT %5f\n",
           sum_tt[1]/count[1], sum_tt[0]/count[0]);
}

char *awt_csp_sai_filter(GBDATA *gb_extended, AW_CL csp_cd) {
    AWT_csp *csp = (AWT_csp *)csp_cd;
    GBDATA *gb_type = GB_search(gb_extended, csp->type_path,GB_FIND);
    if (!gb_type) return 0;
    if (GBS_string_cmp( GB_read_char_pntr(gb_type),"PV?:*",0) == 0) {
        GBDATA *gb_name = GB_find(gb_extended,"name",0,down_level);
        void *strstruct = GBS_stropen(1024);
        GBS_strcat(strstruct,GB_read_char_pntr(gb_name));
        GBS_strcat(strstruct,":      <");
        GBS_strcat(strstruct,GB_read_char_pntr(gb_type));
        GBS_strcat(strstruct,">");
        return GBS_strclose(strstruct);
    }
    return 0;
}

void create_selection_list_on_csp(AW_window *aws, AWT_csp *csp){
    GB_transaction dummy(csp->gb_main);
    csp->sai_sel_box_id = awt_create_selection_list_on_extendeds(csp->gb_main,
                                                                 aws, csp->awar_name, awt_csp_sai_filter, (AW_CL)csp);
}

AW_window *create_csp_window(AW_root *aw_root, AWT_csp *csp){
    GB_transaction dummy(csp->gb_main);
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_CSP", "Select Column Statistic");
    aws->load_xfig("awt/col_statistic.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"awt_csp.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->at("box");
    create_selection_list_on_csp(aws,csp);

    aws->at("smooth");
    aws->create_toggle_field(csp->awar_smooth);
    aws->insert_toggle("Calculate each column (nearly) independently","D",0);
    aws->insert_toggle("Smooth parameter estimates a little","M",1);
    aws->insert_toggle("Smooth parameter estimates across columns","S",2);
    aws->update_toggle_field();

    aws->at("helix");
    aws->label("Use Helix Information (SAI 'HELIX')");
    aws->create_toggle(csp->awar_enable_helix);
    return (AW_window *)aws;
}
