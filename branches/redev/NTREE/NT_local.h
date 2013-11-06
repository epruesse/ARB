// ============================================================ //
//                                                              //
//   File      : NT_local.h                                     //
//   Purpose   : NTREE local defines                            //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef NT_LOCAL_H
#define NT_LOCAL_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif


#define nt_assert(cond) arb_assert(cond)

#define MAX_NT_WINDOWS          5
#define MAX_NT_WINDOWS_NULLINIT NULL,NULL,NULL,NULL,NULL

struct NT_global {
    AW_root           *aw_root;
    GBDATA            *gb_main;
    AW_Window_Creator  window_creator;

    NT_global()
        : aw_root(NULL),
          gb_main(NULL),
          window_creator(NULL)
    {}
};

extern NT_global GLOBAL;

#ifndef NT_LOCAL_PROTO_H
#include "NT_local_proto.h"
#endif

#else
#error NT_local.h included twice
#endif // NT_LOCAL_H
