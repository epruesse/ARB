// =============================================================== //
//                                                                 //
//   File      : pars_main.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PARS_MAIN_HXX
#define PARS_MAIN_HXX

#define MIN_SEQUENCE_LENGTH 20

class AW_root;
class AWT_graphic_tree;

extern struct NT_global {
    AW_root *awr;
    AWT_graphic_tree *tree;
} *GLOBAL_NT;

AWT_graphic_tree *PARS_generate_tree(AW_root *root);


#else
#error pars_main.hxx included twice
#endif // PARS_MAIN_HXX
