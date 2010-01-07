/* Reconstruct GBT-tree from Ntree */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "CT_mem.hxx"
#include "CT_part.hxx"
#include "CT_ntree.hxx"
#include "CT_rbtree.hxx"

#define RMSTRLEN 81

char **name_tbl = NULL;






/* Initialize the module */
void rb_init(char **names)
{
    name_tbl = names;
}


/* get the name of a leaf from the index */
char *get_name(int idx)
{
    char *t;
    t = strdup(name_tbl[idx]);
    return t;
}


/* build a remark with the percentage representation of the partition */
char *rb_remark(const char *info, int perc, char *txt)
{
    char *txt2;

    txt2 = (char *) getmem(RMSTRLEN);
    sprintf(txt2, "%s%i%%", info, perc/100);
    if(txt) {
        strcat(txt, txt2);
        free(txt2);
    }
    else {
        txt = txt2;
    }

    return txt;
}


/* doing all the work for rb_gettree() :-)*/
/* convert a Ntree into a GBT-Tree */
RB_INFO *rbtree(NT_NODE *tree, GBT_TREE *father)
{
    NSONS *nsonp;
    int idx;
    GBT_TREE *gbtnode;
    RB_INFO *info, *info_res;


    gbtnode = (GBT_TREE *) getmem(sizeof(GBT_TREE));
    info = (RB_INFO *) getmem(sizeof(RB_INFO));

    gbtnode->father = father;

    info->node = gbtnode;                                /* return-information */
    info->percent = tree->part->percent;
    info->len = tree->part->len;
    nsonp = tree->son_list;
    if(!nsonp) {                                         /* if node is leaf */
        idx = calc_index(tree->part);
        gbtnode->name = strdup((name_tbl[idx]));     /* test: get_name(idx)); */
        gbtnode->is_leaf = true;
        return info;
    }

    gbtnode->is_leaf = false;
    if (info->percent < 10000) {
        gbtnode->remark_branch = rb_remark("", info->percent, gbtnode->remark_branch);
    }

    /* leftson */
    info_res = rbtree(nsonp->node, gbtnode);
    gbtnode->leftson = info_res->node;
    gbtnode->leftlen = info_res->len;
    free((char *)info_res);

    nsonp = nsonp->next;
    if(!nsonp) return info;

    /* rightson */
    info_res = rbtree(nsonp->node, gbtnode);
    gbtnode->rightson = info_res->node;
    gbtnode->rightlen = info_res->len;
    free((char *)info_res);

    return info;
}


/* reconstruct GBT Tree from Ntree. Ntree is not destructed afterwards! */
GBT_TREE *rb_gettree(NT_NODE *tree)
{
    RB_INFO *info;
    GBT_TREE *gbttree;

    info = rbtree(tree, NULL);
    gbttree = info->node;
    free((char *)info);

    return gbttree;
}
