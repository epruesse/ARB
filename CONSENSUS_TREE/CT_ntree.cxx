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
#include "CT_mem.hxx"
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
        part_free(tree->part);
        tree->part = NULL;
        freenull(tree);
    }
}


void ntree_init() {
    // Initialization of the tree
    arb_assert(!ntree);              // forgot to call ntree_cleanup ?
    PART *root = part_root();
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
        fputs("ins_ntree part=", stdout);
        part_print(newpart);
        printf(" dist2center=%i\n", distance_to_tree_center(newpart));
#endif

        tree->son_list       = (NSONS *) getmem(sizeof(NSONS));
        tree->son_list->node = new_ntnode(newpart);

        return 1;
    }

    // test if part fit under one son of tree -> recursion
    for (NSONS *nsonp = tree->son_list; nsonp; nsonp=nsonp->next) {
        if (is_son_of(newpart, nsonp->node->part)) {
            int res = ins_ntree(nsonp->node, newpart);
            arb_assert(contradicted(newpart, res));
            return res;
        }
    }

    // If partition is not a son maybe it is a brother
    // If it is neither brother nor son -> don't fit here
    for (NSONS *nsonp = tree->son_list; nsonp; nsonp=nsonp->next) {
        if (!are_brothers(nsonp->node->part, newpart)) {
            // newpart is not distinct from nsonp
            if (!is_son_of(nsonp->node->part, newpart)) {
                arb_assert(newpart);
                return 0;
            }
            // accept if nsonp is son of newpart (will be pulled down below) 
        }
    }

#if defined(DUMP_PART_INSERTION)
        fputs("ins_ntree part=", stdout);
        part_print(newpart);
        printf(" dist2center=%i\n", distance_to_tree_center(newpart));
#endif

    // Okay, insert part here ...
    NT_NODE *newntnode = new_ntnode(newpart);

    // Move sons from parent-sonlist into the new sons sonlist
    {
        NSONS *nsonp = tree->son_list;
        while (nsonp) {
            NSONS *nsonp_next = nsonp->next;
            if (is_son_of(nsonp->node->part, newntnode->part)) {
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

    arb_assert(part_is_valid(part));

    bool firstCall = ntree->son_list == NULL;
    if (firstCall) {
        part->len /= 2; // insert as root-edge -> distribute length

        PART *inverse = part_new();
        part_copy(part, inverse);
        part_invert(inverse);

        ASSERT_RESULT(bool, true, ins_ntree(ntree, part));
        ASSERT_RESULT(bool, true, ins_ntree(ntree, inverse));

        arb_assert(!inverse);
    }
    else {
        if (!ins_ntree(ntree, part)) {
            part_invert(part);
            if (!ins_ntree(ntree, part)) {
                part_free(part); // drop non-fitting partition
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
    part_print(tree->part);
    fputc('\n', stdout);

    // and sons
    for (nsonp=tree->son_list; nsonp; nsonp = nsonp->next) {
        print_ntree(nsonp->node, indent);
    }

    indent--;
    
    do_indent(indent);
    fputs(")\n", stdout);
}

void print_ntindex(NT_NODE *tree) {
    // Testfunction to print the indexnumbers of the tree

    NSONS *nsonp;
    PART  *p = part_new();

    // print father
    printf("(");
    for (nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
        part_or(nsonp->node->part, p);
    }
    printf("%d", tree->part->p[0]);

    // and sons
    for (nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
        print_ntindex(nsonp->node);
    }

    printf(")");

    part_free(p);
}

bool is_well_formed(const NT_NODE *tree) {
    // checks whether
    // - tree has sons
    // - all sons are part of father 
    // - all sons are distinct
    // - father is sum of sons

    int sons = ntree_count_sons(tree);

    if (!sons) return part_size(tree->part) == 1; // leafs should contain single species

    bool well_formed = true;

    PART *pmerge = part_new();
    for (NSONS *nson = tree->son_list; nson; nson = nson->next) {
        PART *pson = nson->node->part;

        if (!is_son_of(pson, tree->part)) well_formed = false;
        if (!are_brothers(pson, pmerge)) well_formed  = false;
        part_or(pson, pmerge);
        if (!is_well_formed(nson->node)) well_formed = false;
    }
    if (!parts_equal(tree->part, pmerge)) well_formed = false;
    part_free(pmerge);
    return well_formed;
}

#endif

