// =============================================================== //
//                                                                 //
//   File      : pars_klprops.hxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PARS_KLPROPS_HXX
#define PARS_KLPROPS_HXX

class AW_window;
class AW_root;

AW_window *create_kernighan_properties_window(AW_root *aw_root);

#else
#error pars_klprops.hxx included twice
#endif // PARS_KLPROPS_HXX
