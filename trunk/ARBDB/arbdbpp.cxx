#include <stdio.h>
#include "arbdb.h"
#include "arbdbt.h"


GB_transaction::GB_transaction(GBDATA *gb_main)
    : ta_main(gb_main)
    , ta_open(false)
    , ta_err(NULL)
{
    if (ta_main) {
        ta_err = GB_push_transaction(ta_main);
        if (!ta_err) {
            ta_open = true;
        }
    }
    else {
        ta_err = "NULL-Transaction";
    }
}

GB_ERROR GB_transaction::close(GB_ERROR error) {
    // abort transaction if error != NULL

    if (error) {
        if (ta_err) {
            ta_err = GBS_global_string("%s\n(previous error: %s)", error, ta_err);
        }
        else {
            ta_err = error;
        }
    }

#if defined(DEVEL_RALF)
#warning check for exported error here (when GB_export_error gets redesigned)
#endif // DEVEL_RALF
    
    if (ta_open) {
        ta_err  = GB_end_transaction(ta_main, ta_err);
        ta_open = false;
    }

    return ta_err;
}

GB_transaction::~GB_transaction() {
    if (ta_open) {
        GB_ERROR error = close(NULL);
        if (error) {
            fprintf(stderr, "Error while closing transaction: %s\n", error);
            gb_assert(0); // you need to manually use close()
    }
}
}


int GB_info(struct gb_data_base_type2 *gbd){
    return GB_info((GBDATA *)gbd);
}
