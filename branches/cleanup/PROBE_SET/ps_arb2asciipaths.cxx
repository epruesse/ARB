// =============================================================== //
//                                                                 //
//   File      : ps_arb2asciipaths.cxx                             //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_filebuffer.hxx"
#include "ps_pg_tree_functions.hxx"

static IDVector *__PATH = new IDVector;

static void PS_print_paths(GBDATA *_pb_node) {
    //  recursively print the paths to the leafs

    // number and species name
    GBDATA     *data   = GB_entry(_pb_node, "num");
    const char *buffer = GB_read_char_pntr(data);
    SpeciesID   id     = atoi(buffer);

    // probe(s)
    GBDATA *pb_group = GB_entry(_pb_node, "group");
    if (!pb_group) {
        id = -id;
    }

    // path
    __PATH->push_back(id);

    // child(ren)
    GBDATA *pb_child = PS_get_first_node(_pb_node);
    if (pb_child) {
        while (pb_child) {
            PS_print_paths(pb_child);
            pb_child = PS_get_next_node(pb_child);
        }
    }
    else {
        // print path in leaf nodes
        printf("[%6zu] ", __PATH->size());
        for (IDVectorCIter i=__PATH->begin(); i != __PATH->end(); ++i) {
            printf("%6i ", *i);
        }
        printf("\n");
    }

    // path
    __PATH->pop_back();
}


int main(int argc, char *argv[]) {
    GB_ERROR error = 0;
    if (argc < 2) {
        error = GBS_global_string("Missing arguments\n Usage %s <input database name>", argv[0]);
    }
    else {
        const char *input_DB_name = argv[1];
        printf("Loading probe-group-database '%s'..\n", input_DB_name);

        GBDATA *pb_main     = GB_open(input_DB_name, "rwcN"); // open probe-group-database
        if (!pb_main) error = GB_await_error();
        else {
            GB_transaction  ta(pb_main);
            GBDATA         *group_tree = GB_entry(pb_main, "group_tree");
            if (!group_tree) error     = "no 'group_tree' in database";
            else {
                GBDATA *first_level_node     = PS_get_first_node(group_tree);
                if (!first_level_node) error = "no 'node' found in group_tree";
                else {
                    printf("dumping probes... (starting with first toplevel nodes)\n");
                    // print 1st level nodes (and its subtrees)
                    do {
                        PS_print_paths(first_level_node);
                        first_level_node = PS_get_next_node(first_level_node);
                    }
                    while (first_level_node);
                }
            }
        }
    }

    if (error) {
        fprintf(stderr, "Error in %s: %s\n", argv[0], error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


