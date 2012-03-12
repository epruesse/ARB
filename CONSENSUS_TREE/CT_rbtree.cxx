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
    arb_assert(!name_tbl);
    name_tbl = &names; // @@@ use a copy for safety ?
}

void rb_cleanup() {
    name_tbl = NULL;
}

static char *get_name(int idx) {
    // get the name of a leaf from the index
    char *t;
    t = strdup((*name_tbl)[idx]);
    return t;
}


static char *rb_remark(const char *info, int perc, char *txt) {
    // build a remark with the percentage representation of the partition
    char *txt2 = (char *) getmem(RMSTRLEN);
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

static RB_INFO *rbtree(NT_NODE *tree, GBT_TREE *father) {
    // doing all the work for rb_gettree() :-)
    // convert a Ntree into a GBT-Tree
    
    GBT_TREE *gbtnode = (GBT_TREE *) getmem(sizeof(GBT_TREE));
    gbtnode->father   = father;

    RB_INFO *info = (RB_INFO *) getmem(sizeof(RB_INFO));
    info->node    = gbtnode;                             // return-information
    info->percent = tree->part->percent;
    info->len     = tree->part->len;

    NSONS *nsonp = tree->son_list;
    if (!nsonp) {                                        // if node is leaf
        int idx = calc_index(tree->part);

        gbtnode->name    = get_name(idx);
        gbtnode->is_leaf = true;
    }
    else {
        gbtnode->is_leaf = false;
        if (info->percent < 10000) {
            gbtnode->remark_branch = rb_remark("", info->percent, gbtnode->remark_branch);
        }

        int      multifurc = ntree_count_sons(tree);
        RB_INFO *son_info[multifurc];

        {
            int sidx = 0;
            while (nsonp) {
                son_info[sidx++] = rbtree(nsonp->node, NULL);
                nsonp            = nsonp->next;
            }
        }

        while (multifurc>1) {
            int didx = 0;
            for (int sidx1 = 0; sidx1<multifurc; sidx1 += 2) {
                int sidx2 = sidx1+1;
                if (sidx2<multifurc) {
                    GBT_TREE *mf;
                    RB_INFO *sinfo;

                    if (multifurc > 2) {
                        mf    = (GBT_TREE *) getmem(sizeof(GBT_TREE));
                        sinfo = (RB_INFO *) getmem(sizeof(RB_INFO));

                        mf->father = NULL;

                        sinfo->percent = 0; // branch never really occurs (artificial multifurcation in binary tree)
                        sinfo->len     = 0.0;
                        sinfo->node    = mf;
                    }
                    else { // last step
                        mf    = gbtnode;
                        sinfo = info;
                    }

                    mf->leftson = son_info[sidx1]->node;
                    mf->leftlen = son_info[sidx1]->len;

                    mf->rightson = son_info[sidx2]->node;
                    mf->rightlen = son_info[sidx2]->len;

                    mf->leftson->father  = mf;
                    mf->rightson->father = mf;

                    freenull(son_info[sidx1]);
                    freenull(son_info[sidx2]);

                    son_info[didx++] = sinfo;
                }
                else {
                    son_info[didx++] = son_info[sidx1];
                }
            }
            multifurc = didx;
        }

        arb_assert(multifurc == 1);
        arb_assert(son_info[0] == info);
        arb_assert(info->node->father == father);
    }
    return info;
}


GBT_TREE *rb_gettree(NT_NODE *tree) {
    // reconstruct GBT Tree from Ntree. Ntree is not destructed afterwards!
    RB_INFO  *info    = rbtree(tree, NULL);
    GBT_TREE *gbttree = info->node;
    free(info);
    return gbttree;
}
