#include <stdio.h>
#include <arbdb.h>

int main(int argc, char **argv) {
    int i, cmp;
    char *path;
    GBDATA *gbmain, *gbdata, *gbname;
    GBDATA *gbali, *gbrnali, *gbspec;
    char *gbseqname; 

    if (argc<2) path = ":";
    else 	path = argv[1];

    if (!(gbmain=GB_open(path,"r"))) return (-1);

    GB_begin_transaction(gbmain);

    if(!(gbali=GB_search(gbmain, "presets/allignment", GB_FIND))) GB_abort_transaction(gbmain);

    for (gbrnali=GB_find(gbali, "allignment_type", "%rna%",down_level); gbrnali; gbrnali=GB_find(gbrnali, 0,"%rna%", this_level+search_next))
    {
        gbseqname = GB_read_string(GB_search(GB_get_father(gbrnali), "allignment_name", GB_FIND));
        gbspec = GB_search(gbmain, "species_data/species", GB_FIND);

        for (gbname = GB_find(gbspec, "name", 0, down_level); gbname; gbname = GB_find(gbname, 0, 0, this_level+search_next)) {
            if(strcmp(GB_read_char_pntr(gbname), gbseqname)) {
                gbdata = GB_search(GB_get_father(gbname), "ali_xxx/data", GB_find);
                fprintf(stdout, "%20s: %50s\n", GB_read_char_pntr(gbseqname), GB_read_char_pntr(gbdata));
                break;
            }
            else {
                if(strcmp(GB_read_char_pntr(gbname), GB_read_char_pntr(gbseqname)) < 0) continue;
                else /* > */ break;
            }
        }
	
    }

    GB_commit_transaction(gbmain);
    GB_close(gbmain);

}