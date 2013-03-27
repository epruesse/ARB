/* --------------------------------------------------------------------------------
 *  File      : ARB.default.xs
 *  Purpose   : Skeleton and predefined functions for ARB perl interface  
 *  Copyright : Lehrstuhl fuer Mikrobiologie, TU Muenchen
 * -------------------------------------------------------------------------------- */

#ifndef __cplusplus
#error please compile as C++
#endif

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "debug.h"
#include "arbdb.h"
#include "arbdbt.h"
#include "adGene.h"
#include "ad_p_prot.h"
#include "adperl.h"
#include "ARB_ext.c"

static GB_shell4perl perl_shell;

/* --------------------------------------------------------------------------------
 * if you get errors about undefined functions like
 *
 *       GBP_charPtr_2_XXX
 *       GBP_XXX_2_charPtr
 * 
 * you need to define them in ../ARBDB/adperl.c@enum_conversion_functions
 */

/* --------------------------------------------------------------------------------
 * the following functions are hand-coded in ARB.default.xs:
 */

MODULE = ARB PACKAGE = ARB PREFIX = P2A_
PROTOTYPES: ENABLE


MODULE = ARB PACKAGE = BIO PREFIX = P2AT_


# --------------------------------------------------------------------------------
# functions below are auto-generated by ../PERLTOOLS/arb_proto_2_xsub.cxx
# using prototypes from the following files:
#
#    ../ARBDB/ad_prot.h
#    ../ARBDB/ad_t_prot.h
#    ../ARBDB/adGene.h
# --------------------------------------------------------------------------------

