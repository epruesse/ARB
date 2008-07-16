/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2004               *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef PG_DEF_HXX
#define PG_DEF_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pg_assert(bed) arb_assert(bed)

#else
#error pg_def.hxx included twice
#endif // PG_DEF_HXX
