// =============================================================== //
//                                                                 //
//   File      : AP_sequence.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_sequence.hxx"

char *AP_sequence::mutation_per_site = 0;
char *AP_sequence::static_mutation_per_site[3] = { 0,0,0 };
long AP_sequence::global_combineCount;

AP_sequence::~AP_sequence(void) { ; }
AP_FLOAT AP_sequence::real_len(void) { return 0.0; }

AP_sequence::AP_sequence(ARB_Tree_root *rooti){
    cashed_real_len = -1.0;
    is_set_flag = AP_FALSE;
    sequence_len = 0;
    update = 0;
    costs = 0.0;
    root = rooti;
}

void AP_sequence::set_gb(GBDATA *gb_data){
    this->set(GB_read_char_pntr(gb_data));
}
