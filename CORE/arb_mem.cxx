// ================================================================= //
//                                                                   //
//   File      : arb_mem.cxx                                         //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Elmar Pruesse in September 2014                        //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "arb_mem.h"
#include "arb_msg.h"

int arb_alloc_aligned_intern(void** tgt, size_t len) {
    int error = posix_memalign(tgt, 16, len);
    if (error != 0) {
        GB_export_errorf("Failed to allocate memory: %s", strerror(error));
    }
    return error;
}

