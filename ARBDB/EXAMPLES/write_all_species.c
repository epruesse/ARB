#include <stdio.h>
/* #include <malloc.h> */
#include <arbdb.h>
int main(int argc,char **argv)
{
    GBDATA   *gb_main,*gb_species,*gb_speciesname;
    char     *path;
    GB_ERROR  fehler;
    char     *species_name;

    if(argc==1) path = ":";
    else path        = argv[1];

    gb_main = GB_open(path,"r");
    if(!gb_main)
    {
        printf("could not open database '%s'\n", path);
        return(-1);
    }
    fehler=GB_begin_transaction(gb_main);
    if(fehler)
    {
        printf("fehler:%s\n",fehler);
        return(-1);
    }
    gb_species=GB_search(gb_main,"species_data/species",GB_FIND);
    for(; gb_species; gb_species=GB_find(gb_species,0,0,SEARCH_NEXT_BROTHER))
    {
        gb_speciesname=GB_search(gb_species,"name",GB_FIND);
        species_name=GB_read_string(gb_speciesname);
        printf("%s",species_name);
        free(species_name);
        gb_speciesname=GB_search(gb_species,"full_name",GB_FIND);
        if(!gb_speciesname) printf("\n");
        else
        {
            species_name=GB_read_string(gb_speciesname);
            printf("   %s\n",species_name);
            free(species_name);
        }
    }
    GB_commit_transaction(gb_main);
}
