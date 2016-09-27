// =============================================================== //
//                                                                 //
//   File      : ps_show_result.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_defs.hxx"
#include "ps_bitmap.hxx"

//  ====================================================
//  ====================================================

int main(int argc,   char *argv[]) {

   // open probe-set-database
    if (argc < 2) {
        printf("Missing argument\n Usage %s <file name>\n ", argv[0]);
        exit(1);
    }

    const char *input_filename   = argv[1];
    printf("Opening result file '%s'..\n", input_filename);
    PS_FileBuffer *file = new PS_FileBuffer(input_filename, PS_FileBuffer::READONLY);

    long size;
    SpeciesID id1, id2;
    printf("\nloading no matches : ");
    file->get_long(size);
    printf("%li", size);
    for (long i=0; i < size; ++i) {
        if (i % 5 == 0) printf("\n");
        file->get_int(id1);
        file->get_int(id2);
        printf("%5i %-5i   ", id1, id2);
    }
    printf("\n\nloading one matches : ");
    file->get_long(size);
    printf("%li\n", size);
    long path_length;
    SpeciesID path_id;
    for (long i=0; i < size; ++i) {
        file->get_int(id1);
        file->get_int(id2);
        file->get_long(path_length);
        printf("%5i %-5i path(%6li): ", id1, id2, path_length);
        while (path_length-- > 0) {
            file->get_int(path_id);
            printf("%i ", path_id);
        }
        printf("\n");
    }
    printf("\nloading preset bitmap\n");
    PS_BitMap_Counted *map = new PS_BitMap_Counted(file);
    map->print(stdout);
    delete map;

    return 0;
}
