#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adlocal.h"
/* #include <arbdb.h> */


GBDATA *GB_follow_link(GBDATA *gb_link){
    char *s;
    const char *link;
    char c;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_link);
    GBDATA *result;
    GB_Link_Follower lf;
    link = (char *)GB_read_link_pntr(gb_link);
    if (!link) return 0;
    s = strchr(link,':');
    if (!s){
        GB_export_error("Your link '%s' does not contain a ':' character",link);
        return 0;
    }
    c = *s;
    *s = 0;
    lf = (GB_Link_Follower)GBS_read_hash(Main->resolve_link_hash,link);
    *s = c;
    if (!lf){
        GB_export_error("Your link tag '%s' is unknown to the system",link);
        return 0;
    }
    result = lf(GB_get_root(gb_link),gb_link,s+1);
    return result;
}


GB_ERROR GB_install_link_follower(GBDATA *gb_main, const char *link_type, GB_Link_Follower link_follower){
    GB_ERROR error =0;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->resolve_link_hash){
        Main->resolve_link_hash = GBS_create_hash(256,0);
    }
    error = GB_check_link_name(link_type);
    if (error) return error;
    GBS_write_hash(Main->resolve_link_hash, link_type,(long)link_follower);
    return 0;
}


