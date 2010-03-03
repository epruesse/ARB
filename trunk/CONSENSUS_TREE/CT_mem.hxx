// ============================================================= //
//                                                               //
//   File      : CT_mem.hxx                                      //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_MEM_HXX
#define CT_MEM_HXX

void *getmem(size_t size);

#else
#error CT_mem.hxx included twice
#endif // CT_MEM_HXX
