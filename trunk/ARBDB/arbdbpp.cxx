#include <stdio.h>
#include "arbdb.h"
#include "arbdbt.h"


GB_transaction::GB_transaction(GBDATA *gb_main)
    : ta_main(gb_main)
    , aborted(false)
{
    if (ta_main) GB_push_transaction(ta_main);
}

GB_transaction::~GB_transaction() {
    if (ta_main) {
        if (aborted) GB_abort_transaction(ta_main);
        else         GB_pop_transaction(ta_main);
    }
}

int GB_info(struct gb_data_base_type2 *gbd){
    return GB_info((GBDATA *)gbd);
}
