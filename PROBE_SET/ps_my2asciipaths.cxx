// =============================================================== //
//                                                                 //
//   File      : ps_my2asciipaths.cxx                              //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_node.hxx"

//  GLOBALS

typedef pair<bool, SpeciesID> Step;
static vector<Step> *__PATH = new vector<Step>;

static void PS_print_paths(const PS_NodePtr& _ps_node) {
    //  recursively print the paths to the leafs

    // path
    __PATH->push_back(Step(_ps_node->hasInverseProbes(), _ps_node->hasProbes() ? _ps_node->getNum() : -(_ps_node->getNum())));

    // children
    if (_ps_node->hasChildren()) {
        for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin(); i != _ps_node->getChildrenEnd(); ++i) {
            PS_print_paths(i->second);
        }
    }
    else {
        // print path in leaf nodes
        printf("[%4zu] ", __PATH->size());
        for (vector<Step>::const_iterator i=__PATH->begin(); i != __PATH->end(); ++i) {
            printf("%4i%c ", i->second, i->first ? '+' : ' ');
        }
        printf("\n");
    }

    // path
    __PATH->pop_back();
}


//  ====================================================
//  ====================================================

int main(int argc,   char *argv[]) {

    // open probe-set-database
    if (argc < 2) {
        printf("Missing arguments\n Usage %s <database name>\n", argv[0]);
        exit(1);
    }

    const char *input_DB_name = argv[1];

    printf("Opening probe-set-database '%s'..\n", input_DB_name);
    PS_FileBuffer *fb = new PS_FileBuffer(input_DB_name, PS_FileBuffer::READONLY);
    PS_NodePtr     root(new PS_Node(-1));
    root->load(fb);
    printf("loaded database (enter to continue)\n");


    for (PS_NodeMapConstIterator i = root->getChildrenBegin(); i != root->getChildrenEnd(); ++i) {
        PS_print_paths(i->second);
    }

    printf("(enter to continue)\n");

    delete fb;
    root.SetNull();
    printf("root should be destroyed now\n");
    printf("(enter to continue)\n");

    return 0;
}
