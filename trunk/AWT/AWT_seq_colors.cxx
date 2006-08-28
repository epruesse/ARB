#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>


#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include "awt_seq_colors.hxx"
#include "awt.hxx"


static int default_set = 0; // number of default set
static const char *default_sets[AWT_SEQ_COLORS_MAX_SET] = {
    //A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
    "=2=0=3=0=0=0=4=0=0=0=0=0=0=6=0=0=0=0=0=5=5=0=0=0=0=0", // A, C, G, TU and N in 5 colors
    "R2=0Y3=0=0=0R2=0=0=0=0=0=0=0=0=0=0=2=0Y3Y3=0=0=0=3=0", // AG and CTU in 2 colors
    "=0=5=0=5=7=7=0=5=7=7=3=7=3=9=7=7=7=3=3=0=0=5=3=7=3=7", // ambiguity
    // "=2=4=7=4=4=6=2=5=3=0=5=3=3=4=0=2=4=5=2=2=0=3=6=0=6=4", // Protein colors (old defaults)
    "=7=0=7=8=2=9=8=9=3=0=2=3=7=8=0=8=2=2=2=2=0=3=9=0=9=0", // Protein colors
    "=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0",
};

static bool seq_color_awars_created = false;

// --------------------------------------------------------------------------------

static const char *default_characters(int elem) {
    static char result[3] = "xX";

    result[0] = 'a'+elem;
    result[1] = 'A'+elem;

    return result;
}
static const char *default_color(int set, int elem) {
    // returns default color numbers for seq-color-set
    static char result[3] = "=0";
    const char *pos       = default_sets[set]+2*elem;

    result[0] = pos[0];
    result[1] = pos[1];

    return result;
}

static void awt_awar_changed_cb(GBDATA *, int *cl, GB_CB_TYPE){
    AWT_seq_colors *sc = (AWT_seq_colors *)cl;
    sc->reload();
}

static void create_seq_color_awars(AW_root *awr, AWT_seq_colors *asc) {
    awt_assert(!seq_color_awars_created);

    awr->awar_int(AWAR_SEQ_NAME_SELECTOR_NA, default_set, AW_ROOT_DEFAULT)->add_callback((AW_RCB)awt_awar_changed_cb,(AW_CL)asc,0);;
    awr->awar_int(AWAR_SEQ_NAME_SELECTOR_AA, default_set, AW_ROOT_DEFAULT)->add_callback((AW_RCB)awt_awar_changed_cb,(AW_CL)asc,0);; //ykadi
    for (int elem = 0; elem<AWT_SEQ_COLORS_MAX_ELEMS; ++elem) {
        const char *awar_name = GBS_global_string(AWAR_SEQ_NAME_STRINGS_TEMPLATE, elem);
        awr->awar_string(awar_name, default_characters(elem));

        for (int set = 0; set<AWT_SEQ_COLORS_MAX_SET; ++set) {
            awar_name = GBS_global_string(AWAR_SEQ_NAME_TEMPLATE, set, elem);
            awr->awar_string(awar_name, default_color(set, elem));
        }
    }

    seq_color_awars_created = true;
}

AW_window *create_seq_colors_window(AW_root *awr, AWT_seq_colors *asc){
    char                     buf[256];
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    if (!seq_color_awars_created) create_seq_color_awars(awr, asc);

    aws = new AW_window_simple;
    aws->init( awr, "SEQUENCE_COLOR_MAPPING", "SEQUENCE COLORS");

    aws->at           ( 10,10 );
    aws->auto_space(0,3);

    aws->callback     ( AW_POPDOWN );aws->create_button( "CLOSE", "CLOSE", "C" );
    aws->callback     ( AW_POPUP_HELP,(AW_CL)"sequence_colors.hlp" );aws->create_button( "HELP", "HELP" );
    aws->at_newline();

    //    aws->label_length( 6 ); // @@@ has no effect
    //    aws->button_length( 6 );
    int set;
    int elem;

    aws->label("Select Colors For Nucleotides (NA) :");
    aws->create_toggle_field(AWAR_SEQ_NAME_SELECTOR_NA,1);
    for (set = 0; set < AWT_SEQ_COLORS_MAX_SET;set++){
        sprintf(buf,"S_%i",set);
        aws->insert_toggle( buf," ",set );
    }
    aws->update_toggle_field();
    aws->at_newline();

    aws->label("Select Colors For Amino Acids (AA) :");
    aws->create_toggle_field(AWAR_SEQ_NAME_SELECTOR_AA,1);
    for (set = 0; set < AWT_SEQ_COLORS_MAX_SET;set++){
        sprintf(buf,"S_%i",set);
        aws->insert_toggle( buf," ",set );
    }
    aws->update_toggle_field();
    aws->at_newline();

     aws->label_length( 6 ); 
     aws->button_length( 6 );
 
    for (int big_columns = 0; big_columns <= 1; ++big_columns) {
        aws->create_button(0,"Char");
        for (set = 0; set < AWT_SEQ_COLORS_MAX_SET;set++){
            sprintf(buf,"S %i",set);
            aws->create_button(0,buf);
        }

        buf[0] = 0; aws->create_button(0,buf); // empty
    }
    aws->at_newline();
    aws->auto_space(2,2);

    for (elem = 0; elem < (AWT_SEQ_COLORS_MAX_ELEMS/2); elem++){
        for (int big_columns = 0; big_columns <= 1; ++big_columns) {
            int my_elem = elem+big_columns*AWT_SEQ_COLORS_MAX_ELEMS/2;

            sprintf(buf,AWAR_SEQ_NAME_STRINGS_TEMPLATE,my_elem);
            aws->create_input_field(buf,4);
            for (set = 0; set < AWT_SEQ_COLORS_MAX_SET;set++){
                sprintf(buf,AWAR_SEQ_NAME_TEMPLATE,set,my_elem);
                aws->create_input_field(buf,4);
            }
            if (big_columns == 0)  {
                buf[0] = 0; aws->create_button(0,buf); // empty
            }
        }
        aws->at_newline();
    }

    aws->window_fit();
    return (AW_window *)aws;
}

void AWT_seq_colors::run_cb(){
    if (callback && aww) callback(aww,cd1,cd2);
}

void AWT_seq_colors::reload(){
    char buf[256];
    GB_transaction dummy(gb_def);
    memset(char_2_gc,base_gc,256);
    int i;
    for (i=0;i<256;i++) char_2_char[i] = i;
    long set = GBT_read_int2(gb_def,AWAR_SEQ_NAME_SELECTOR_NA, default_set);
    if (set <0 || set >= AWT_SEQ_COLORS_MAX_SET) return;
    int elem;
    int s2;
    for (elem = 0; elem < AWT_SEQ_COLORS_MAX_ELEMS; elem++){
        sprintf(buf,AWAR_SEQ_NAME_STRINGS_TEMPLATE,elem);
        unsigned char *sc = (unsigned char *)GBT_read_string2(gb_def,buf,default_characters(elem));
        if (!cbexists){
            GBDATA *gb_ne = GB_search(gb_def,buf,GB_STRING);
            GB_add_callback(gb_ne,GB_CB_CHANGED,awt_awar_changed_cb,(int *)this);
            for (s2=0;s2 <  AWT_SEQ_COLORS_MAX_SET; s2++){
                sprintf(buf,AWAR_SEQ_NAME_TEMPLATE,s2,elem);
                GBT_read_string2(gb_def,buf,default_color(s2, elem));
                gb_ne = GB_search(gb_def,buf,GB_STRING);
                GB_add_callback(gb_ne,GB_CB_CHANGED,awt_awar_changed_cb,(int *)this);
            }
        }
        sprintf(buf,AWAR_SEQ_NAME_TEMPLATE,(int)set,elem);
        char *val = GBT_read_string(gb_def,buf);
        if (strlen(val) != 2 || val[1] >'9' || val[1] < '0'){
            aw_message(GB_export_error("Error in Color Lookup Table: '%s' is not of type X#",val));
            delete val;
            delete sc;
            continue;
        }
        for (i=0;sc[i];i++){
            char_2_gc[sc[i]] = val[1]-'0' + base_gc;
            if (val[0] != '='){
                char_2_char[sc[i]] = val[0];
            }
        }
        free(val);
        free(sc);
    }
    {
        memset(char_2_gc_aa,base_gc,256);
        int i;
        for (i=0;i<256;i++) char_2_char_aa[i] = i;
        long set = GBT_read_int2(gb_def,AWAR_SEQ_NAME_SELECTOR_AA, default_set);
        if (set <0 || set >= AWT_SEQ_COLORS_MAX_SET) return;
        int elem;
        int s2;
        for (elem = 0; elem < AWT_SEQ_COLORS_MAX_ELEMS; elem++){
            sprintf(buf,AWAR_SEQ_NAME_STRINGS_TEMPLATE,elem);
            unsigned char *sc = (unsigned char *)GBT_read_string2(gb_def,buf,default_characters(elem));
            if (!cbexists){
                GBDATA *gb_ne = GB_search(gb_def,buf,GB_STRING);
                GB_add_callback(gb_ne,GB_CB_CHANGED,awt_awar_changed_cb,(int *)this);
                for (s2=0;s2 <  AWT_SEQ_COLORS_MAX_SET; s2++){
                    sprintf(buf,AWAR_SEQ_NAME_TEMPLATE,s2,elem);
                    GBT_read_string2(gb_def,buf,default_color(s2, elem));
                    gb_ne = GB_search(gb_def,buf,GB_STRING);
                    GB_add_callback(gb_ne,GB_CB_CHANGED,awt_awar_changed_cb,(int *)this);
                }
            }
            sprintf(buf,AWAR_SEQ_NAME_TEMPLATE,(int)set,elem);
            char *val = GBT_read_string(gb_def,buf);
            if (strlen(val) != 2 || val[1] >'9' || val[1] < '0'){
                aw_message(GB_export_error("Error in Color Lookup Table: '%s' is not of type X#",val));
                delete val;
                delete sc;
                continue;
            }
            for (i=0;sc[i];i++){
                char_2_gc_aa[sc[i]] = val[1]-'0' + base_gc;
                if (val[0] != '='){
                    char_2_char_aa[sc[i]] = val[0];
                }
            }
            free(val);
            free(sc);
        }
    }
    cbexists = 1;
    run_cb();
}

AWT_seq_colors::AWT_seq_colors(GBDATA *gb_default, int _base_gc,
                               AW_CB _cb,AW_CL _cd1,AW_CL _cd2){
    aww = 0;
    cd1 = _cd1;
    cd2 = _cd2;
    callback = _cb;
    gb_def = gb_default;
    base_gc = _base_gc;
    GB_transaction dummy(gb_def);
    cbexists = 0;
    this->reload();
}



AWT_reference::AWT_reference(GBDATA *_gb_main){
    reference = 0;
    ref_len = 0;
    gb_main = _gb_main;
    init_species_name = 0;
}

void AWT_reference::init() {
    delete reference;
    reference = 0;
    ref_len = 0;
    delete init_species_name;
    init_species_name = 0;
}

void AWT_reference::expand_to_length(int len){
    if (len > ref_len){
        char *ref2 = (char *)GB_calloc(sizeof(char),len+1);
        if (reference ) strcpy(ref2,reference);
        free(reference);
        reference = ref2;
        ref_len = len;
    }
}

void AWT_reference::init(const char *species_name, const char *alignment_name) {

    awt_assert(species_name);
    awt_assert(alignment_name);

    GB_transaction dummy(gb_main);
    GBDATA *gb_species = GBT_find_species(gb_main,species_name);

    init();
    if (gb_species){
        GBDATA *gb_data = GBT_read_sequence(gb_species,alignment_name);
        if (gb_data){
            reference = GB_read_as_string(gb_data);
            if (reference) {
                ref_len = strlen(reference);
                init_species_name = strdup(species_name);
            }
        }
    }
}

void AWT_reference::init(const char *name, const char *sequence_data, int len) {

    awt_assert(name);
    awt_assert(sequence_data);
    awt_assert(len>0);

    init();

    reference = (char*)GB_calloc(sizeof(char), len+1);
    memcpy(reference, sequence_data, len);
    reference[len] = 0;
    ref_len = len;
    init_species_name = strdup(name);
}

AWT_reference::~AWT_reference(){
    delete reference;
}
