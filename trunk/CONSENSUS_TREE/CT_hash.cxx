// ============================================================= //
//                                                               //
//   File      : CT_hash.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_hash.hxx"
#include "CT_ntree.hxx"
#include "CT_mem.hxx"

#include <arbdbt.h>

/* Hashtabelle fuer parts */

int Hash_max_count=0;
HNODE *Hashlist[HASH_MAX];
HNODE *Sortedlist = NULL;

void hash_init() {
    /*! initialize Hashtable and free old data */
    Hash_max_count = 0;
    hash_free();
}


void hash_settreecount(int tree_count) {
    /*! set number of trees */
    Tree_count = tree_count;
}


void hash_free() {
    /*! free Hashtable and Sortedlist */
    int i;
    HNODE *hnp, *hnp_help;

    for (i=0; i< HASH_MAX; i++) {
        hnp = Hashlist[i];
        while (hnp) {
            hnp_help = hnp->next;
            part_free(hnp->part);
            free((char *)hnp);
            hnp = hnp_help;
        }
        Hashlist[i] = NULL;
    }

    hnp = Sortedlist;
    while (hnp) {
        hnp_help = hnp->next;
        part_free(hnp->part);
        free((char *)hnp);
        hnp = hnp_help;
    }
    Sortedlist = NULL;
}


PART *hash_getpart() {
    /*! return the first element (with the highest hitnumber) from the linear sorted
     * list and calculate percentaged appearance of this partition in all trees and
     * calculate the average pathlength.
     * The element is removed from the list afterwards
     */
    HNODE *hnp;
    PART *p;

    if (!Sortedlist) return NULL;

    hnp = Sortedlist;
    Sortedlist = hnp->next;
    p = hnp->part;
    free((char *)hnp);
    p->len /= (float) p->percent;
    p->percent *= 10000;
    p->percent /= Tree_count;

    return  p;
}


void hash_insert(PART *part, int weight) {
    /*! insert part in hashtable with weight
     * @param part the one to insert, is destructed afterwards
     * @param weight  the weight of the part
     */
    int key;
    HNODE *hp;

    part_standard(part);

    key = part_key(part);
    key %= HASH_MAX;

    if (Hashlist[key]) {
        for (hp=Hashlist[key]; hp; hp=hp->next) {
            if (part_cmp(hp->part, part)) { /* if in list */
                /* tree-add tree-id */
                hp->part->percent += weight;
                hp->part->len += ((float) weight) * part->len;
                part_free(part);
                if (hp->part->percent > Hash_max_count)
                    Hash_max_count = hp->part->percent;
                return;
            }
        }
    }

    /* Not yet in list -> insert */
    hp = (HNODE *) getmem(sizeof(HNODE));
    part->percent = weight;
    part->len *= ((float) weight);
    hp->part = part;
    if (weight > Hash_max_count)
        Hash_max_count = weight;

    if (!Hashlist[key]) {
        Hashlist[key] = hp;
        return;
    }

    hp->next = Hashlist[key];
    Hashlist[key] = hp;
}


void build_sorted_list() {
    /*! sort the current hash list in a linear sorted list, the current hash
     * is empty afterwards. I use a simple trick speed up the function:
     * build for every hitnumber one list and put all elements with that hitnumber
     * in it. After that i connect the list together, what results in a sorted list
     */
    HNODE *hnp, *hnp_help;
    HNODE **heads, **tails, *head, *tail;
    int i, idx;

    heads = (HNODE **) getmem(Hash_max_count*sizeof(HNODE *));
    tails = (HNODE**)  getmem(Hash_max_count*sizeof(HNODE *));

    /* build one list for each countvalue */

    for (i=0; i< HASH_MAX; i++) {
        hnp = Hashlist[i];
        while (hnp) {
            hnp_help = hnp->next;
            idx = hnp->part->percent-1;
            if (heads[idx]) {
                hnp->next = heads[idx];
                heads[idx] = hnp;
            }
            else {
                heads[idx] = tails[idx] = hnp;
                hnp->next = NULL;
            }

            hnp = hnp_help;
        }
        Hashlist[i] = NULL;
    }

    head = NULL;
    tail = NULL;
    /* concatenate lists */
    for (i=Hash_max_count-1; i>=0; i--) {
        if (heads[i]) {
            if (!head) {
                head = heads[i];
                tail = tails[i];
            }
            else {
                tail->next = heads[i];
                tail = tails[i];
            }
        }
    }

    free((char *)heads);
    free((char *)tails);

    Sortedlist = head;
}


void hash_print() {
    /*! testfunction to print the hashtable */
    int i;
    HNODE *hnp;

    printf("\n HASHtable \n");

    for (i=0; i< HASH_MAX; i++) {
        printf("Key: %d \n", i);
        hnp = Hashlist[i];
        while (hnp) {
            printf("node: count %d node ", hnp->part->percent);
            part_print(hnp->part); printf(" (%d)\n", hnp->part->p[0]);
            hnp = hnp->next;
        }
    }
}


void sorted_print() {
    /*! testfunction to print the sorted linear list */
    HNODE *hnp;

    printf("\n sorted HASHlist \n");

    hnp = Sortedlist;
    while (hnp) {
        printf("node: count %d node ", hnp->part->percent);
        part_print(hnp->part); printf("\n");
        hnp = hnp->next;
    }
}
