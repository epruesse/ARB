#include <stdio.h>
#include "arbdb.h"
#include "arbdbt.h"


GB_transaction::GB_transaction(GBDATA *gb_main)
{
    if (gb_main)	GB_push_transaction(gb_main);
    this->gbd = gb_main;
}

GB_transaction::~GB_transaction()
{
    if (this->gbd) GB_pop_transaction(this->gbd);
}

int GB_info(struct gb_data_base_type2 *gbd){
    return GB_info((GBDATA *)gbd);
}
