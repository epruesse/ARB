// ============================================================= //
//                                                               //
//   File      : CT_rbtree.cxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

// Reconstruct GBT-tree from Ntree

#include "CT_rbtree.hxx"
#include "CT_mem.hxx"

#include <arbdbt.h>
#include <arb_strarray.h>


#define RMSTRLEN 81

static const CharPtrArray *name_tbl = NULL;

void rb_init(const CharPtrArray& names) {
    // Initialize the module
    name_tbl = &names; // @@@ use a copy for safety ? 
}


static char *get_name(int idx) {
    // get the name of a leaf from the index
    char *t;
    t = strdup((*name_tbl)[idx]);
    return t;
}


// build a remark with the percentage representation of the partition
static char *rb_remark(const char *info, int perc, char *txt)
{
    char *txt2;

    txt2 = (char *) getmem(RMSTRLEN);
    sprintf(txt2, "%s%i%%", info, perc/100);
    if (txt) {
        strcat(txt, txt2);
        free(txt2);
    }
    else {
        txt = txt2;
    }

    return txt;
}


// doing all the work for rb_gettree() :-)
// convert a Ntree into a GBT-Tree
static RB_INFO *rbtree(NT_NODE *tree, GBT_TREE *father)
{
    NSONS *nsonp;
    int idx;
    GBT_TREE *gbtnode;
    RB_INFO *info, *info_res;


    gbtnode = (GBT_TREE *) getmem(sizeof(GBT_TREE));
    info = (RB_INFO *) getmem(sizeof(RB_INFO));

    gbtnode->father = father;

    info->node = gbtnode;                                // return-information
    info->percent = tree->part->percent;
    info->len = tree->part->len;
    nsonp = tree->son_list;
    if (!nsonp) {                                        // if node is leaf
        idx = calc_index(tree->part);
        gbtnode->name = get_name(idx);
        gbtnode->is_leaf = true;
        return info;
    }


    gbtnode->is_leaf = false;
    if (info->percent < 10000) {
        gbtnode->remark_branch = rb_remark("", info->percent, gbtnode->remark_branch);
    }

    // leftson
    info_res = rbtree(nsonp->node, gbtnode);
    gbtnode->leftson = info_res->node;
    gbtnode->leftlen = info_res->len;
    free(info_res);

    nsonp = nsonp->next;
    if (!nsonp) {
        arb_assert(0); // @@@ invalid tree would be generated here (only leftson)
        return info;
    }

    // rightson
    info_res = rbtree(nsonp->node, gbtnode);
    gbtnode->rightson = info_res->node;
    gbtnode->rightlen = info_res->len;
    free(info_res);

    arb_assert(nsonp->next == NULL); // otherwise some sons would be silently dropped

    return info;
}


// reconstruct GBT Tree from Ntree. Ntree is not destructed afterwards!
GBT_TREE *rb_gettree(NT_NODE *tree)
{
    RB_INFO *info;
    GBT_TREE *gbttree;

    info = rbtree(tree, NULL);
    gbttree = info->node;
    free(info);

    return gbttree;
}
