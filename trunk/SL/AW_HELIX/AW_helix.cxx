// ==================================================================== //
//                                                                      //
//   File      : AW_helix.cxx                                           //
//   Purpose   : Wrapper for BI_helix + AW-specific functions           //
//   Time-stamp: <Tue Dec/21/2004 17:10 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "AW_helix.hxx"

#include <cctype>
#include <cstring>
#include <aw_device.hxx>
#include <aw_window.hxx>

struct {
    const char *awar;
    BI_PAIR_TYPE pair_type;
} helix_awars[] = {
    { "Strong_Pair", HELIX_STRONG_PAIR },
    { "Normal_Pair", HELIX_PAIR },
    { "Weak_Pair", HELIX_WEAK_PAIR },
    { "No_Pair", HELIX_NO_PAIR },
    { "User_Pair", HELIX_USER0 },
    { "User_Pair2", HELIX_USER1 },
    { "User_Pair3", HELIX_USER2 },
    { "User_Pair4", HELIX_USER3 },
    { "Default", HELIX_DEFAULT },
    { "Non_Standart_aA", HELIX_NON_STANDART0 },
    { "Non_Standart1", HELIX_NON_STANDART1 },
    { "Non_Standart2", HELIX_NON_STANDART2 },
    { "Non_Standart3", HELIX_NON_STANDART3 },
    { "Non_Standart4", HELIX_NON_STANDART4 },
    { "Non_Standart5", HELIX_NON_STANDART5 },
    { "Non_Standart6", HELIX_NON_STANDART6 },
    { "Non_Standart7", HELIX_NON_STANDART7 },
    { "Non_Standart8", HELIX_NON_STANDART8 },
    { "Non_Standart9", HELIX_NON_STANDART9 },
    { "Not_Non_Standart", HELIX_NO_MATCH },
    { 0, HELIX_NONE },
};

AW_helix::AW_helix(AW_root * aw_root)
    // : BI_helix()
{
    int i;
    char awar[256];

    // _init();
    int j;
    for (j=0; helix_awars[j].awar; j++){
        i = helix_awars[j].pair_type;
        sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
        aw_root->awar_string( awar,pairs[i])->add_target_var(&pairs[i]);
        sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
        aw_root->awar_string( awar, char_bind[i])->add_target_var(&char_bind[i]);
    }
    // deleteable = 0;
}

char AW_helix::get_symbol(char left, char right, BI_PAIR_TYPE pair_type){
    left = toupper(left);
    right = toupper(right);
    int i;
    int erg;
    if (pair_type< HELIX_NON_STANDART0) {
        erg = *char_bind[HELIX_DEFAULT];
        for (i=HELIX_STRONG_PAIR; i< HELIX_NON_STANDART0; i++){
            if (_check_pair(left,right,(BI_PAIR_TYPE)i)){
                erg = *char_bind[i];
                break;
            }
        }
    }else{
        erg = *char_bind[HELIX_NO_MATCH];
        if (_check_pair(left,right,pair_type)) erg =  *char_bind[pair_type];
    }
    if (!erg) erg = ' ';
    return erg;
}

char *AW_helix::seq_2_helix(char *sequence,char undefsymbol){
    size_t size2 = strlen(sequence);
    bi_assert(size2<=size); // if this fails there is a sequence longer than the alignment
    char *helix = (char *)GB_calloc(sizeof(char),size+1);
    register unsigned long i,j;
    for (i=0; i<size2; i++) {
        if ( i<size && entries[i].pair_type) {
            j = entries[i].pair_pos;
            helix[i] = get_symbol(sequence[i],sequence[j],
                                  entries[i].pair_type);
        }else{
            helix[i] = undefsymbol;
        }
        if (helix[i] == ' ') helix[i] = undefsymbol;
    }
    return helix;
}

int BI_show_helix_on_device(AW_device *device, int gc, const char *opt_string, size_t opt_string_size, size_t start, size_t size,
                            AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                            AW_CL cduser, AW_CL cd1, AW_CL cd2)
{
    AWUSE(opt_ascent);AWUSE(opt_descent);
    AW_helix *THIS = (AW_helix *)cduser;
    char *buffer = GB_give_buffer(size+1);
    register unsigned long i,j,k;

    for (k=0; k<size; k++) {
        i = k+start;
        if ( i<THIS->size && THIS->entries[i].pair_type) {
            j = THIS->entries[i].pair_pos;
            char pairing_character = '.';
            if (j < opt_string_size){
                pairing_character = opt_string[j];
            }
            buffer[k] = THIS->get_symbol(opt_string[i],pairing_character,
                                         THIS->entries[i].pair_type);
        }else{
            buffer[k] = ' ';
        }
    }
    buffer[size] = 0;
    return device->text(gc,buffer,x,y,0.0,(AW_bitset)-1,cd1,cd2);
}

int AW_helix::show_helix( void *devicei, int gc1 , char *sequence,
                          AW_pos x, AW_pos y,
                          AW_bitset filter,
                          AW_CL cd1, AW_CL cd2){

    if (!entries) return 0;
    AW_device *device = (AW_device *)devicei;
    return device->text_overlay(gc1, sequence, 0, x , y, 0.0 , filter, (AW_CL)this, cd1, cd2,
                                1.0,1.0, BI_show_helix_on_device);
}



AW_window *create_helix_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "HELIX_PROPS", "HELIX_PROPERTIES");

    aws->at           ( 10,10 );
    aws->auto_space(3,3);
    aws->callback     ( AW_POPDOWN );
    aws->create_button( "CLOSE", "CLOSE", "C" );
    aws->at_newline();

    aws->label_length( 18 );
    int i;
    int j;
    int ex= 0,ey = 0;
    char awar[256];
    for (j=0; helix_awars[j].awar; j++){

        aws->label_length( 25 );
        i = helix_awars[j].pair_type;

        if (i != HELIX_DEFAULT && i!= HELIX_NO_MATCH ) {
            sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
            aws->label(helix_awars[j].awar);
            aws->callback(awcbs);
            aws->create_input_field(awar,20);
        }else{
            aws->create_button(0,helix_awars[j].awar);
            aws->at_x(ex);
        }
        if (!j) aws->get_at_position(&ex,&ey);

        sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
        aws->callback(awcbs);
        aws->create_input_field(awar,3);
        aws->at_newline();
    }
    aws->window_fit();
    return (AW_window *)aws;
}


