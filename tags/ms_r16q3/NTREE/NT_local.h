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

#ifndef NT_LOCAL_PROTO_H
#include "NT_local_proto.h"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#define nt_assert(cond) arb_assert(cond)

#define AWAR_NTREE_MAIN_WINDOW_COUNT "tmp/mainwin_count" // changes whenever a new NT main window is created

#define MAX_NT_WINDOWS          5
#define MAX_NT_WINDOWS_NULLINIT NULL,NULL,NULL,NULL,NULL

struct NT_global : virtual Noncopyable {
    AW_root *aw_root;
    GBDATA  *gb_main;

    NT_global()
        : aw_root(NULL),
          gb_main(NULL)
    {}
};

extern NT_global GLOBAL;

#else
#error NT_local.h included twice
#endif // NT_LOCAL_H
