#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <set>
#include "ps_node.hxx"



//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {
    // create probe-set-database
    if (argc < 4) {
        printf("Missing arguments\n Usage %s <probequality> <probelength> <output filename> [print]\n",argv[0]);
        exit(1);
    }

    unsigned short  quality        = atoi(argv[1]);
    unsigned short  probelength    = atoi(argv[2]);
    const char     *output_DB_name = argv[3];

    printf( "creating probe-set-database '%s'..", output_DB_name );
    int ps_db = open( output_DB_name, O_WRONLY | O_CREAT | O_EXCL , S_IRUSR | S_IWUSR );
    if (ps_db == -1) {
        printf( "error : %s already exists or can't be created\n",output_DB_name );
        exit(1);
    }
    printf( "done\n" );

    // create sample tree
    printf( "making sample tree..." );
    PS_NodePtr root(new PS_Node(-1));

    for (int id = 10; id < 15; ++id) {
        PS_NodePtr new_child( new PS_Node( id ) );
        root->addChild( new_child );
    }

    for (PS_NodeMapIterator child = root->getChildrenBegin(); child != root->getChildrenEnd(); ++child ) {
    
        for (int id = child->second->getNum()*100; id < (child->second->getNum()*100)+10; ++id) {

            PS_NodePtr new_child( new PS_Node( id ) );
		
            if (random() % 3 != 0) {
                for (int pnr = 0; pnr < 100; ++pnr ) {
                    PS_ProbePtr new_probe( new PS_Probe );
                    new_probe->length     = probelength;
                    new_probe->quality    = quality;
                    new_probe->GC_content = (unsigned short) (random() % probelength);
                    new_child->addProbe( new_probe );
                }
            }
            child->second->addChild( new_child );
        }
    }
    printf( "done (enter to continue)\n" );
    getchar();

    if (argc >= 4) {
        root->print();
        printf( "\n(enter to continue)\n" );
        getchar();
    }

    // write sample tree
    root->save( ps_db );
    close( ps_db );

    // clean up
    root.SetNull();
    printf( "root should be destroyed now (enter to continue)\n" );
    getchar();

    return 0;
}
