// =============================================================== //
//                                                                 //
//   File      : ps_make_sample_db.cxx                             //
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
    // create probe-set-database
    if (argc < 4) {
        printf("Missing arguments\n Usage %s <probequality> <probelength> <output filename> [print]\n", argv[0]);
        exit(1);
    }

    unsigned short  quality        = atoi(argv[1]);
    unsigned short  probelength    = atoi(argv[2]);
    const char     *output_DB_name = argv[3];

    printf("creating probe-set-database '%s'..", output_DB_name);
    PS_FileBuffer *ps_db_fb = new PS_FileBuffer(output_DB_name, false);
    printf("done\n");

    // create sample tree
    printf("making sample tree...");
    PS_NodePtr root(new PS_Node(-1));

    for (int id = 10; id < 15; ++id) {
        PS_NodePtr new_child(new PS_Node(id));

        if (id % 2 != 0) {
            for (int pnr = 0; pnr < 5; ++pnr) {
                PS_ProbePtr new_probe(new PS_Probe);
                new_probe->length     = probelength;
                new_probe->quality    = quality;
                new_probe->GC_content = (unsigned short) (random() % probelength);
                new_child->addProbe(new_probe);
            }
        }

        root->addChild(new_child);
    }

    for (PS_NodeMapIterator child = root->getChildrenBegin(); child != root->getChildrenEnd(); ++child) {

        for (int id = child->second->getNum()*100; id < (child->second->getNum()*100)+10; ++id) {

            PS_NodePtr new_child(new PS_Node(id));

            if (random() % 3 != 0) {
                for (int pnr = 0; pnr < 50; ++pnr) {
                    PS_ProbePtr new_probe(new PS_Probe);
                    new_probe->length     = probelength;
                    new_probe->quality    = quality;
                    new_probe->GC_content = (unsigned short) (random() % probelength);
                    new_child->addProbe(new_probe);
                }
            }

            child->second->addChild(new_child);
        }
    }
    printf("done (enter to continue)\n");
    getchar();

    if (argc >= 4) {
        root->print();
        printf("\n(enter to continue)\n");
        getchar();
    }

    // write sample tree
    root->save(ps_db_fb);

    // clean up
    delete ps_db_fb;
    root.SetNull();
    printf("root should be destroyed now (enter to continue)\n");
    getchar();

    return 0;
}
