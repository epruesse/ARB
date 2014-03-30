/* Einen Binaerbaum erzeugen ueber einen Multitree */

#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "CT_mem.hxx"
#include "CT_part.hxx"
#include "CT_ntree.hxx"

NT_NODE *ntree = NULL;
int ntree_count=0;


/* returns the referenz of the actual NTree */ 
NT_NODE *ntree_get(void)
{
	return ntree;
}


/* testfunction to print a NTree */
void print_ntree(NT_NODE *tree)
{
	NSONS *nsonp;
	if(tree == NULL) {
		printf("tree is empty\n");
		return;
	}

	/* print father */
	printf("(");
	part_print(tree->part);
	
	/* and sons */
	for(nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
		print_ntree(nsonp->node);
	}

	printf(")");
}	

/* Testfunction to print the indexnumbers of the tree */
void print_ntindex(NT_NODE *tree)
{
	NSONS *nsonp;
	PART *p;
	
	p = part_new();
	
	/* print father */
	printf("(");
	for(nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
		part_or(nsonp->node->part, p);
	}
	printf("%d",tree->part->p[0]);   /*calc_index(p, tree->part));*/
	
	/* and sons */
	for(nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
		print_ntindex(nsonp->node);
	}

	printf(")");
}


/* build a new node and store the parttion p in it */
NT_NODE *new_ntnode(PART *p)
{
	NT_NODE *n;
	
	n = (NT_NODE *) getmem(sizeof(NT_NODE));
	n->part = p;
	n->son_list = NULL;
	return n;
}


/* delete the tree */
void del_tree(NT_NODE *tree)
{
	NSONS *nsonp, *nson_help;

	if(!tree) return;

	for(nsonp=tree->son_list; nsonp;) {
		nson_help = nsonp->next;
		del_tree(nsonp->node);
		free((char *)nsonp);
		nsonp = nson_help;
	}
	tree->son_list = NULL;

	/* now is leaf  */
	part_free((tree->part));
	tree->part = NULL;
	free((char *)tree);
	tree = NULL;
}


/* Initialisation of the tree */
void ntree_init(void)
{
	PART *r;
	
	/* Destruct old Tree */
	del_tree(ntree);
 	/* Set Root = max. Patition */
	ntree = NULL;
	r=part_root();
	ntree=new_ntnode(r);
	
 	ntree_count = 0; 
}
 

/* test if the tree is already complete (all necessary partitions are inserted) */
int ntree_cont(int len)
{
 	return(ntree_count<len);
}


/* Move son from parent-sonlist to new sonlist */
/* nson is pointer on element in parent-sonlist */
/* sonlist is new sonlist where to move in */
void insert_son(NT_NODE *f_node, NT_NODE *s_node, NSONS *nson)
{

	/* Move out of parent-sonlist */
	if(nson == f_node->son_list) 
		f_node->son_list = f_node->son_list->next;
	if(nson->prev) 
		nson->prev->next = nson->next;
	if(nson->next) 
		nson->next->prev = nson->prev;

	/* Move in node-sonlist */
	nson->next = s_node->son_list;
	nson->prev = NULL;
	if(s_node->son_list) 
		s_node->son_list->prev = nson;
	s_node->son_list = nson;
}


/* Construct a multitree under that constrain that it is posssible for the tree
   be a binaerytree in the end. To enable this it is important to follow two
   ideas:
   1. a son fits only under a father if the father has every bit set that the son
      has plus it should be possible to insert as many sons as necessary to result
      in a binaerytree
   2. for any two brothers A,B: brotherA and brotherB == 0                       */
int ins_ntree(NT_NODE *tree, PART *newpart)
{
	NSONS *nsonp;
	NSONS *nsonp_h;
	NT_NODE *newntnode; 
	
	/* Tree is leaf */
	if(!tree->son_list) {
		tree->son_list = (NSONS *) getmem(sizeof(NSONS));
		tree->son_list->node = new_ntnode(newpart);
		return 1;
	}
	
	/* test if part fit under one son of tree -> recursion */
	for(nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
		if(son(newpart, nsonp->node->part)) {
			return ins_ntree(nsonp->node, newpart);
		}
	}

        /* If partition is not a son maybe it is a brother */ 
        /* If it is neither brother nor son -> don't fit here */
	for(nsonp=tree->son_list; nsonp; nsonp=nsonp->next) {
		if(!brothers(nsonp->node->part, newpart)) {
			if(!son(nsonp->node->part, newpart)) {
				return 0;
			}
		}
	}
	
	/* Okay, insert part here ... */
	newntnode = new_ntnode(newpart);
	
	/* Move sons from parent-sonlist in nt_node-sonlist */
	nsonp = tree->son_list;
	while(nsonp) {
		
		nsonp_h = nsonp->next;
		if(son(nsonp->node->part, newpart)) {
			insert_son(tree, newntnode, nsonp);
		}
		nsonp = nsonp_h;
	}
	
	/* insert nsons-elem in son-list of father */
	nsonp = (NSONS *) getmem(sizeof(NSONS));
	nsonp->node = newntnode;
	nsonp->prev = NULL;
	nsonp->next = tree->son_list;
	if(tree->son_list)
		tree->son_list->prev = nsonp;
	tree->son_list = nsonp;
	return 1;
} 
 
 
 
/* Insert a partition in the NTree. To do this I try two insert it in both
   possible representations. ins_ntree do the hole work. If it fit in none 
   of them I delete the partition.
   attention: The parttion is detruced afterwards.
              The tree is never empty, because it is initalised with a root */
void insert_ntree(PART *part)
{
	ntree_count++;
 	if(!ins_ntree(ntree, part)) {
 		part_invert(part);
 		if(!ins_ntree(ntree, part)) {
 			ntree_count--;
			part_free(part);
 		}
 	}
} 







