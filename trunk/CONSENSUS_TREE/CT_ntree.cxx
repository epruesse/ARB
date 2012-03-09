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

static NT_NODE *ntree       = NULL;
static int      ntree_count = 0;


// returns the referenz of the actual NTree
NT_NODE *ntree_get()
{
    return ntree;
}


// build a new node and store the partition p in it
static NT_NODE *new_ntnode(PART *p)
{
    NT_NODE *n;

    n = (NT_NODE *) getmem(sizeof(NT_NODE));
    n->part = p;
    n->son_list = NULL;
    return n;
}


// delete the tree
static void del_tree(NT_NODE *tree)
{
    NSONS *nsonp, *nson_help;

    if (!tree) return;

    for (nsonp=tree->son_list; nsonp;) {
        nson_help = nsonp->next;
        del_tree(nsonp->node);
        free(nsonp);
        nsonp = nson_help;
    }
    tree->son_list = NULL;

    // now is leaf
    part_free((tree->part));
    tree->part = NULL;
    freenull(tree);
}


// Initialization of the tree
void ntree_init() {
    arb_assert(!ntree); // forgot to call ntree_cleanup ? 

    // Set root to max. partition
    PART *r     = part_root();
    ntree = new_ntnode(r);

    ntree_count = 0;
}
void ntree_cleanup() {
    // Destruct old tree
    del_tree(ntree);
    ntree = NULL;
    ntree_count = 0;
}

#if 0
// test if the tree is already complete (all necessary partitions are inserted)
static int ntree_cont(int len)
{
    return (ntree_count<len);
}
#endif

// Move son from parent-sonlist to new sonlist
// nson is pointer on element in parent-sonlist
// sonlist is new sonlist where to move in
static void insert_son(NT_NODE *f_node, NT_NODE *s_node, NSONS *nson)
{

    // Move out of parent-sonlist
    if (nson == f_node->son_list)
        f_node->son_list = f_node->son_list->next;
    if (nson->prev)
        nson->prev->next = nson->next;
    if (nson->next)
        nson->next->prev = nson->prev;

    // Move in node-sonlist
    nson->next = s_node->son_list;
    nson->prev = NULL;
    if (s_node->son_list)
        s_node->son_list->prev = nson;
    s_node->son_list = nson;
}


/* Construct a multitree under that constrain that it is possible for the tree
   be a binary tree in the end. To enable this it is important to follow two
   ideas:
   1. a son fits only under a father if the father has every bit set that the son
   has plus it should be possible to insert as many sons as necessary to result
   in a binary tree
   2. for any two brothers A,B: brotherA and brotherB == 0                       */
static int ins_ntree(NT_NODE *tree, PART *newpart)
{
    NSONS *nsonp;
    NSONS *nsonp_h;
    NT_NODE *newntnode;

    // Tree is leaf
    if (!tree->son_list) {
        tree->son_list = (NSONS *) getmem(sizeof(NSONS));
        tree->son_list->node = new_ntnode(newpart);
        return 1;
    }

    // test if part fit under one son of tree -> recursion
    for (nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
        if (is_son_of(newpart, nsonp->node->part)) {
            return ins_ntree(nsonp->node, newpart);
        }
    }

    // If partition is not a son maybe it is a brother
    // If it is neither brother nor son -> don't fit here
    for (nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
        if (!are_brothers(nsonp->node->part, newpart)) {
            if (!is_son_of(nsonp->node->part, newpart)) {
                return 0;
            }
        }
    }

    // Okay, insert part here ...
    newntnode = new_ntnode(newpart);

    // Move sons from parent-sonlist in nt_node-sonlist
    nsonp = tree->son_list;
    while (nsonp) {

        nsonp_h = nsonp->next;
        if (is_son_of(nsonp->node->part, newpart)) {
            insert_son(tree, newntnode, nsonp);
        }
        nsonp = nsonp_h;
    }

    // insert nsons-elem in son-list of father
    nsonp = (NSONS *) getmem(sizeof(NSONS));
    nsonp->node = newntnode;
    nsonp->prev = NULL;
    nsonp->next = tree->son_list;
    if (tree->son_list)
        tree->son_list->prev = nsonp;
    tree->son_list = nsonp;
    return 1;
}



/* Insert a partition in the NTree. To do this I try to insert it in both
   possible representations. ins_ntree does the whole work. If it fits in none
   of them I delete the partition.
   attention: The partition is destructed afterwards.
   The tree is never empty, because it is initialized with a root */
void insert_ntree(PART *part)
{
    ntree_count++;
    if (!ins_ntree(ntree, part)) {
        // @@@ really ok to insert the inverted subtree here ? 
        part_invert(part);
        if (!ins_ntree(ntree, part)) {
            ntree_count--;
            part_free(part);
        }
    }
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

    // indent++;

    do_indent(indent);
    part_print(tree->part);
    fputc('\n', stdout);

    // and sons
    for (nsonp=tree->son_list; nsonp; nsonp = nsonp->next) {
        print_ntree(nsonp->node, indent);
    }

    // indent--;
    
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

int ntree_count_sons(NT_NODE *tree) {
    int sons = 0;
    if (tree->son_list) {
        for (NSONS *node = tree->son_list; node; node = node->next) {
            sons++;
        }
    }
    return sons;
}

bool is_well_formed(NT_NODE *tree) {
    // checks whether
    // - tree has sons
    // - all sons are part of father 
    // - all sons are distinct
    // - father is sum of sons
    
    int sons = ntree_count_sons(tree);

    if (!sons) return false;

    bool well_formed = true;

    PART *pmerge = part_new();
    for (NSONS *nson = tree->son_list; nson; nson = nson->next) {
        PART *pson = nson->node->part;

        if (!is_son_of(pson, tree->part)) well_formed = false;
        if (!are_brothers(pson, pmerge)) well_formed   = false;
        part_or(pson, pmerge);
    }
    if (!parts_equal(tree->part, pmerge)) well_formed = false;
    part_free(pmerge);
    return well_formed;
}

#endif

