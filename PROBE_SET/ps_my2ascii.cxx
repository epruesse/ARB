// =============================================================== //
//                                                                 //
//   File      : ps_my2ascii.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_node.hxx"

//  ====================================================
//  ====================================================

int main(int argc,   char *argv[]) {

    //
    // check arguments
    //
    if (argc < 3) {
        printf("Missing arguments\n Usage %s <input database name> <output database name>\n", argv[0]);
        exit(1);
    }

    //
    // open probe-set-database
    //
    PS_Node       *root          = new PS_Node(-1);
    const char    *input_DB_name = argv[1];
    PS_FileBuffer *ps_db_fb      = new PS_FileBuffer(input_DB_name, PS_FileBuffer::READONLY);

    printf("Opening input-probe-set-database '%s'..\n", input_DB_name);
    root->load(ps_db_fb);
    printf("loaded database (enter to continue)\n");

    //
    // write as ASCII
    //
    const char *output_DB_name = argv[2];
    printf("writing probe-data to %s\n", output_DB_name);
    ps_db_fb->reinit(output_DB_name, PS_FileBuffer::WRITEONLY);
    char *buffer = (char *)malloc(512);
    root->saveASCII(ps_db_fb, buffer);
    printf("(enter to continue)\n");

    //
    // clean up
    //
    free(buffer);
    delete ps_db_fb;
    printf("(enter to continue)\n");

    return 0;
}
