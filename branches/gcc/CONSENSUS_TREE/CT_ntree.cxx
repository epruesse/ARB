// ============================================================= //
//                                                               //
//   File      : CT_ntree.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_ntree.hxx"
#include <arbdbt.h>

// Einen Binaerbaum erzeugen ueber einen Multitree

static NT_NODE *ntree = NULL;


const NT_NODE *ntree_get() {
    // returns the current ntree
    return ntree;
}


static NT_NODE *new_ntnode(PART*& p) {
    // build a new node and store the partition p in it
    NT_NODE *n  = (NT_NODE *) getmem(sizeof(NT_NODE));
    n->part     = p;
    n->son_list = NULL;

    p = NULL;
    return n;
}


static void del_tree(NT_NODE *tree) {
    // delete the tree
    if (tree) {
        for (NSONS *nsonp = tree->son_list; nsonp;) {
            NSONS *nson_next = nsonp->next;
            del_tree(nsonp->node);
            free(nsonp);
            nsonp = nson_next;
        }
        tree->son_list = NULL;

        // now is leaf
        delete tree->part;
        tree->part = NULL;
        freenull(tree);
    }
}


void ntree_init(const PartitionSize *registry) {
    // Initialization of the tree
    arb_assert(!ntree);              // forgot to call ntree_cleanup ?
    PART *root = registry->create_root();
    ntree      = new_ntnode(root); // Set root to completely filled partition
}

void ntree_cleanup() {
    // Destruct old tree
    del_tree(ntree);
    ntree = NULL;
}

#if 0
// test if the tree is already complete (all necessary partitions are inserted)
static int ntree_cont(int len)
{
    return (ntree_count<len);
}
#endif

int ntree_count_sons(const NT_NODE *tree) {
    int sons = 0;
    if (tree->son_list) {
        for (NSONS *node = tree->son_list; node; node = node->next) {
            sons++;
        }
    }
    return sons;
}

static void move_son(NT_NODE *f_node, NT_NODE *s_node, NSONS *nson) {
    // Move son from parent-sonlist to new sonlist
    // nson is pointer on element in parent-sonlist
    // sonlist is new sonlist where to move in

    // Move out of parent-sonlist
    
    if (nson == f_node->son_list) f_node->son_list = f_node->son_list->next;
    if (nson->prev) nson->prev->next = nson->next;
    if (nson->next) nson->next->prev = nson->prev;

    // Move in node-sonlist
    nson->next = s_node->son_list;
    nson->prev = NULL;
    
    if (s_node->son_list) s_node->son_list->prev = nson;
    s_node->son_list                             = nson;
}



static int ins_ntree(NT_NODE *tree, PART*& newpart) {
    /* Construct a multitree under the constraint,
     * that the final tree may result in a binary tree.
     *
     * To ensure this, it is important to follow two ideas:
     *
     * 1. a son only fits below a father
     *    - if the father has all son-bits set AND
     *    - the father is different from the son (so it is possible to add a brother) 
     *
     * 2. brothers are distinct (i.e. they do not share any bits)
     */

    // Tree is leaf
    if (!tree->son_list) {
#if defined(DUMP_PART_INSERTION)
        fputs("ins_ntree part=", stdout); newpart->print();
#endif

        tree->son_list       = (NSONS *) getmem(sizeof(NSONS));
        tree->son_list->node = new_ntnode(newpart);

        return 1;
    }

    arb_assert(newpart->is_subset_of(tree->part)); // @@@ should be invariant for entering this function (really ensured by caller?)

    // test if part fits under one son of tree. if so, recurse.
    for (NSONS *nsonp = tree->son_list; nsonp; nsonp=nsonp->next) {
        const PART *sonpart = nsonp->node->part;
        if (newpart->is_subset_of(sonpart)) {
            if (newpart->equals(sonpart)) return 0; // already inserted -> drop

            arb_assert(newpart->is_real_son_of(sonpart));
            int res = ins_ntree(nsonp->node, newpart);
            arb_assert(contradicted(newpart, res));
            return res;
        }
    }

    // Now we are sure 'newpart' is not a son (of any of my sons)!
    // -> Test whether it is a brother of a son
    // If it is neither brother nor son -> don't fit here
    for (NSONS *nsonp = tree->son_list; nsonp; nsonp=nsonp->next) {
        const PART *sonpart = nsonp->node->part;
        if (sonpart->overlaps_with(newpart)) {
            if (!sonpart->is_subset_of(newpart)) {
                arb_assert(newpart);
                return 0;
            }
            arb_assert(sonpart->is_real_son_of(newpart));
            // accept if nsonp is son of newpart (will be pulled down below) 
        }
    }

#if defined(DUMP_PART_INSERTION)
        fputs("ins_ntree part=", stdout); newpart->print();
#endif

    // Okay, insert part here ...
    NT_NODE *newntnode = new_ntnode(newpart);

    // Move sons from parent-sonlist into the new sons sonlist
    {
        NSONS *nsonp = tree->son_list;
        while (nsonp) {
            NSONS      *nsonp_next = nsonp->next;
            const PART *sonpart    = nsonp->node->part;
            if (sonpart->is_subset_of(newntnode->part)) {
                arb_assert(sonpart->is_real_son_of(newntnode->part));
                move_son(tree, newntnode, nsonp);
            }
            nsonp = nsonp_next;
        }
    }

    // insert nsons-elem in son-list of father
    {
        NSONS *new_son = (NSONS *) getmem(sizeof(NSONS));
    
        new_son->node = newntnode;
        new_son->prev = NULL;
        new_son->next = tree->son_list;

        if (tree->son_list) tree->son_list->prev = new_son;
        tree->son_list                           = new_son;
    }

    arb_assert(!newpart);

    return 1;
}



void insert_ntree(PART*& part) {
    /* Insert a partition in the NTree.
     * 
     * Tries both representations, normal and inverse partition,  which
     * represent the two subtrees at both sides of one edge.
     *
     * If neither can be inserted, the partition gets dropped.
     */

    arb_assert(part->is_valid());

    bool firstCall = ntree->son_list == NULL;
    if (firstCall) {
        part->set_len(part->get_len()/2); // insert as root-edge -> distribute length

        PART *inverse = part->clone();
        inverse->invert();

        ASSERT_RESULT(bool, true, ins_ntree(ntree, part));
        ASSERT_RESULT(bool, true, ins_ntree(ntree, inverse));

        arb_assert(!inverse);
    }
    else {
        if (!ins_ntree(ntree, part)) {
            part->invert();
            if (!ins_ntree(ntree, part)) {
#if defined(DUMP_PART_INSERTION)
                fputs("insert_ntree drops part=", stdout); part->print();
#endif
                delete part; // drop non-fitting partition
                part = NULL;
            }
        }
    }
    arb_assert(!part);
}

// --------------------------------------------------------------------------------

#if defined(NTREE_DEBUG_FUNCTIONS)

inline void do_indent(int indent) {
    for (int i = 0; i<indent; ++i) fputc(' ', stdout);
}

void print_ntree(NT_NODE *tree, int indent) {
    // testfunction to print a NTree

    NSONS *nsonp;
    if (tree == NULL) {
        do_indent(indent); 
        fputs("tree is empty\n", stdout);
        return;
    }

    // print father
    do_indent(indent);
    fputs("(\n", stdout);

    indent++;

    do_indent(indent);
    tree->part->print();

    // and sons
    for (nsonp=tree->son_list; nsonp; nsonp = nsonp->next) {
        print_ntree(nsonp->node, indent);
    }

    indent--;
    
    do_indent(indent);
    fputs(")\n", stdout);
}

#define FAIL_IF_NOT_WELLFORMED

#if defined(FAIL_IF_NOT_WELLFORMED)
#define SHOW_FAILURE() arb_assert(0)
#else
#define SHOW_FAILURE() 
#endif

bool is_well_formed(const NT_NODE *tree) {
    // checks whether
    // - tree has sons
    // - all sons are part of father 
    // - all sons are distinct
    // - father is sum of sons

    int sons = ntree_count_sons(tree);
    bool well_formed = true;

    if (!sons) {
        if (tree->part->get_members() != 1) { // leafs should contain single species
            well_formed = false;
            SHOW_FAILURE();
        }
    }
    else {
        arb_assert(tree->son_list);

        PART *pmerge = 0;
        for (NSONS *nson = tree->son_list; nson; nson = nson->next) {
            PART *pson = nson->node->part;

            if (!pson->is_subset_of(tree->part)) {
                well_formed = false;
                SHOW_FAILURE(); // son is not a subset of father
            }
            if (pmerge) {
                if (pson->overlaps_with(pmerge)) {
                    well_formed  = false;
                    SHOW_FAILURE(); // sons are not distinct
                }
                pmerge->add_members_from(pson);
            }
            else {
                pmerge = pson->clone();
            }
            if (!is_well_formed(nson->node)) {
                well_formed = false;
                SHOW_FAILURE(); // son is not well formed
            }
        }
        arb_assert(pmerge);
        if (tree->part->differs(pmerge)) {
            well_formed = false;

#if defined(FAIL_IF_NOT_WELLFORMED)
            printf("tree with %i sons {\n", sons);
            for (NSONS *nson = tree->son_list; nson; nson = nson->next) {
                PART *pson = nson->node->part;
                fputs("  pson   =", stdout); pson->print();
            }
            printf("} end of tree with %i sons\n", sons);

            fputs("tree part=", stdout); tree->part->print();
            fputs("pmerge   =", stdout); pmerge->print();
#endif
            SHOW_FAILURE(); // means: father is not same as sum of sons
        }
        delete pmerge;
    }
    return well_formed;
}

#endif

