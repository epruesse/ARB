#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "arbdb.h"

GB_UNDO_TYPE GBP_undo_type(char *type){
    GB_UNDO_TYPE utype = GB_UNDO_NONE;
    if (!strcasecmp("undo",type)) utype = GB_UNDO_UNDO;
    if (!strcasecmp("redo",type)) utype = GB_UNDO_REDO;
    if (utype == GB_UNDO_NONE){
        GBK_terminate("Usage: ARB::undo(gb_main, 'undo'/'redo')");
    }
    return utype;
}

int GBP_search_mode(char *search_mode){
    if (!strcasecmp(search_mode,"this")) return this_level;
    if (!strcasecmp(search_mode,"down")) return down_level;
    if (!strcasecmp(search_mode,"down_2")) return down_2_level;
    if (!strcasecmp(search_mode,"this_next")) return this_level | search_next;
    if (!strcasecmp(search_mode,"down_next")) return down_level | search_next;
    GB_warning("Error: ARB::find    Cannot recognize your search_mode '%s'"
               "    Possible choices: 'this' 'down' 'down_2' 'this_next'"
               "                'down_next'", search_mode );
    return down_level;
}

static const char *gbp_typeconvert[] = {
    "NONE",
    "BIT",
    "BYTE",
    "INT",
    "FLOAT",
    "-----",
    "BITS",
    "----",
    "BYTES",
    "INTS",
    "FLOATS",
    "-----",
    "STRING",
    "------",
    "------",
    "CONTAINER",
    0
};


const char *GBP_type_to_string(GB_TYPES type){
    if (type >= GB_TYPE_MAX) {
        GB_warning("Unknown Type");
        return "????";
    }
    return gbp_typeconvert[type];
}


GB_TYPES GBP_gb_types(char *type_name){
    int i;
    if (!type_name) return GB_NONE;
    if (type_name[0] == 0) return GB_NONE;
    for (i=0;i<GB_TYPE_MAX;i++) {
        if (!strcasecmp(gbp_typeconvert[i],type_name)) return (GB_TYPES)i;
    }
    GB_warning("ERROR: Unknown type %s (probably used in ARB::create or ARB::search",type_name);
    fprintf(stderr,"ERROR: Unknown type %s",type_name);
    fprintf(stderr,"    Possible Choices:\n");
    for (i=0;i<GB_TYPE_MAX;i++) {
        fprintf(stderr,"        %s\n",gbp_typeconvert[i]);
    }
    return GB_NONE;
}

GB_UNDO_TYPE GBP_undo_types(const char *type_name){
    if (!strcasecmp(type_name,"undo")) return GB_UNDO_UNDO;
    if (!strcasecmp(type_name,"redo")) return GB_UNDO_REDO;
    GB_internal_errorf("Cannot convert '%s' to undo type,\n"
                       " only 'redo' / 'undo' allowed\n", type_name);
    return GB_UNDO_NONE;
}

const char *GBP_undo_type_2_string(GB_UNDO_TYPE type){
    if (type == GB_UNDO_UNDO) return "UNDO";
    if (type == GB_UNDO_REDO) return "REDO";
    return "????";
}
