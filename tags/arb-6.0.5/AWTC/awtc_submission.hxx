// =========================================================== //
//                                                             //
//   File      : awtc_submission.hxx                           //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWTC_SUBMISSION_HXX
#define AWTC_SUBMISSION_HXX


void       AWTC_create_submission_variables(AW_root *root, AW_default db1);
AW_window *AWTC_create_submission_window(AW_root *root, GBDATA *gb_main);

#else
#error awtc_submission.hxx included twice
#endif // AWTC_SUBMISSION_HXX
