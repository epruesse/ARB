/*  ====================================================================  */
/*                                                                        */
/*    File      : adGene.h                                                */
/*    Purpose   : Specific defines for ARB genome                         */
/*    Time-stamp: <Mon Jul/09/2007 14:00 MET Coder@ReallySoft.de>         */
/*                                                                        */
/*                                                                        */
/*  Coded by Ralf Westram (coder@reallysoft.de) in July 2002              */
/*  Copyright Department of Microbiology (Technical University Munich)    */
/*                                                                        */
/*  Visit our web site at: http://www.arb-home.de/                        */
/*                                                                        */
/*  ====================================================================  */

#ifndef ADGENE_H
#define ADGENE_H

#ifndef ARBDB_H
#include "arbdb.h"
#endif

#define GENOM_ALIGNMENT "ali_genom"
#define GENOM_DB_TYPE "genom_db" // main flag (true=genom db, false/missing=normal db)


#else
#error adGene.h included twice
#endif /* ADGENE_H */

