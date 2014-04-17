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

#include "CT_ntree.hxx"
#include "CT_ctree.hxx"
#include <arb_sort.h>


struct RB_INFO {
    GBT_LEN     len;
    RootedTree *node;
    int         percent;    // branch probability [0..100]
};

static int GBT_TREE_order(const GBT_TREE *t1, const GBT_TREE *t2) {
    // define a strict order on trees

    int cmp = t1->is_leaf - t2->is_leaf; // leafs first
    if (!cmp) {
        GBT_LEN l1 = t1->leftlen+t1->rightlen;
        GBT_LEN l2 = t2->leftlen+t2->rightlen;

        cmp = double_cmp(l1, l2); // NOW REALLY insert smaller len first
                                  // (change had no effect on test results)
        if (!cmp) {
            if (t1->is_leaf) {
                cmp = strcmp(t1->name, t2->name);
            }
            else {
                int cll = GBT_TREE_order(t1->leftson, t2->leftson);
                int clr = GBT_TREE_order(t1->leftson, t2->rightson);
                int crl = GBT_TREE_order(t1->rightson, t2->leftson);
                int crr = GBT_TREE_order(t1->rightson, t2->rightson);

                cmp = cll+clr+crl+crr;
                if (!cmp) {
                    cmp = cll;
                    arb_assert(cmp); // order not strict enough
                }
            }
        }
    }
    return cmp;
}

static int RB_INFO_order(const void *v1, const void *v2, void *) {
    // defines a strict order on RB_INFOs

    const RB_INFO *i1 = (const RB_INFO *)v1;
    const RB_INFO *i2 = (const RB_INFO *)v2;

    int cmp = i1->percent - i2->percent; // insert more probable branches first

    if (!cmp) {
        cmp = double_cmp(i1->len, i2->len); // NOW REALLY insert smaller len first
                                            // (change slightly affected test results in "weak" tree-parts)
        if (!cmp) {
            cmp = GBT_TREE_order(i1->node, i2->node);
        }
    }
    return cmp;
}

RB_INFO *ConsensusTree::rbtree(const NT_NODE *tree, TreeRoot *root) {
    // doing all the work for rb_gettree() :-)
    // convert a Ntree into a GBT-Tree

    RootedTree *tnode = DOWNCAST(RootedTree*,root->makeNode());
    tnode->father     = NULL;

    RB_INFO *info = (RB_INFO *) getmem(sizeof(RB_INFO));
    info->node    = tnode;                             // return-information
    info->percent = int(tree->part->get_weight()*100.0+.5);
    info->len     = tree->part->get_len();

    NSONS *nsonp = tree->son_list;
    if (!nsonp) {                                        // if node is leaf
        int idx = tree->part->index();

        tnode->name    = strdup(get_species_name(idx));
        tnode->is_leaf = true;
    }
    else {
        tnode->is_leaf = false;
        arb_assert(!tnode->get_remark());
        if (info->percent < 100) {
            tnode->set_bootstrap(info->percent);
        }

        int      multifurc = ntree_count_sons(tree);
        RB_INFO *son_info[multifurc];

        {
            int sidx = 0;
            while (nsonp) {
                son_info[sidx++] = rbtree(nsonp->node, root);
                nsonp            = nsonp->next;
            }

            GB_sort((void**)son_info, 0, sidx, RB_INFO_order, NULL); // bring sons into strict order
        }

        while (multifurc>1) {
            int didx = 0;
            for (int sidx1 = 0; sidx1<multifurc; sidx1 += 2) {
                int sidx2 = sidx1+1;
                if (sidx2<multifurc) {
                    RootedTree *mf;
                    RB_INFO    *sinfo;

                    if (multifurc > 2) {
                        mf    = DOWNCAST(RootedTree*, root->makeNode());
                        sinfo = (RB_INFO *) getmem(sizeof(RB_INFO));

                        mf->father = NULL;

                        sinfo->percent = 0; // branch never really occurs (artificial multifurcation in binary tree)
                        sinfo->len     = 0.0;
                        sinfo->node    = mf;
                    }
                    else { // last step
                        mf    = tnode;
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
        arb_assert(info->node->father == NULL);
    }
    return info;
}


SizeAwareTree *ConsensusTree::rb_gettree(const NT_NODE *tree) {
    // reconstruct GBT Tree from Ntree. Ntree is not destructed afterwards!
    RB_INFO       *info   = rbtree(tree, new TreeRoot(new SizeAwareNodeFactory, true));
    SizeAwareTree *satree = DOWNCAST(SizeAwareTree*, info->node);
    satree->announce_tree_constructed();
    ASSERT_VALID_TREE(satree);
    free(info);
    return satree;
}
