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
#include "CT_ctree.hxx"

#include <arb_sort.h>

// Hashtabelle fuer parts

static void free_HNODE(HNODE*& hn) {
    while (hn) {
        HNODE *next = hn->next;
        delete hn->part;
        free(hn);
        
        hn = next;
    }
    hn = NULL;
}

PartRegistry::PartRegistry()
    : hash_size(3373),
      max_part_percent(0),
      Sortedlist(NULL)
{
    typedef HNODE *HNODEptr;
    Hashlist = new HNODEptr[hash_size];
    for (int i = 0; i<hash_size; ++i) {
        Hashlist[i] = NULL;
    }
}

PartRegistry::~PartRegistry() {
    for (int i=0; i< hash_size; i++) free_HNODE(Hashlist[i]);
    delete [] Hashlist;
    free_HNODE(Sortedlist);
}

PART *PartRegistry::get_part(int source_trees) {
    /*! return the first element (with the highest hitnumber) from the linear sorted
     * list and calculate percentaged appearance of this partition in all trees and
     * calculate the average pathlength.
     * The element is removed from the list afterwards
     */
    PART *p = NULL;
    if (Sortedlist) {
        HNODE *hnp = Sortedlist;
        Sortedlist = hnp->next;
        p          = hnp->part;
        free(hnp);

        // @@@ code below is badly placed 
        p->set_len(p->get_len()/(float)p->get_percent());
        p->set_percent((p->get_percent()*10000)/source_trees);
    }
    return p;
}

void  PartRegistry::insert(PART*& part, int weight) {
    /*! insert part in hashtable with weight (destroys/reassigns part)
     * @param part the one to insert, is destructed afterwards
     * @param weight  the weight of the part
     */

    part->standardize();

    unsigned key = part->key();
    key %= hash_size;

    HNODE *hp = 0;
    if (Hashlist[key]) {
        for (hp=Hashlist[key]; hp; hp=hp->next) {
            if (hp->part->equals(part)) break;
        }
    }

    part->set_percent(weight);
    part->set_len(part->get_len()*weight); // @@@

    if (hp) {
        hp->part->set_percent(hp->part->get_percent()+part->get_percent());
        hp->part->set_len(hp->part->get_len() + part->get_len());
        delete part;
    }
    else {
        hp            = (HNODE *) getmem(sizeof(HNODE));
        hp->part      = part;
        hp->next      = Hashlist[key];
        Hashlist[key] = hp;
    }

    part = NULL;
    track_max_part_percent(hp->part->get_percent());
}

int PART::strict_cmp(const PART *other) const {
    int centerdist1 = distance_to_tree_center();
    int centerdist2 = other->distance_to_tree_center();

    int cmp = centerdist1-centerdist2; // insert central edges first

    if (!cmp) {
        double fcmp = get_len() - other->get_len(); // insert bigger len first
        cmp = fcmp<0 ? -1 : (fcmp>0 ? 1 : 0);

        if (!cmp) {
            cmp = id - other->id; // strict by definition
            arb_assert(cmp);
        }
    }

    return cmp;
}

static int HNODE_order(const void *v1, const void *v2, void *) {
    // defines a strict order on HNODEs
    const HNODE *h1 = (const HNODE *)v1;
    const HNODE *h2 = (const HNODE *)v2;

    return h1->part->strict_cmp(h2->part);
}


void  PartRegistry::build_sorted_list() {
    /*! sort the current hash list in a linear sorted list, the current hash
     * is empty afterwards. I use a simple trick speed up the function:
     * build for every hitnumber one list and put all elements with that hitnumber
     * in it. After that i connect the list together, what results in a sorted list
     */

    arb_assert(!Sortedlist);

    int list_count = max_part_percent+1;

    HNODE  **heads = (HNODE**)getmem(list_count*sizeof(*heads));
    HNODE  **tails = (HNODE**)getmem(list_count*sizeof(*tails));
    size_t  *size  = (size_t*)getmem(list_count*sizeof(*size));

    // build one list for each branch-probability
    for (int i=0; i< hash_size; i++) {
        HNODE *hnp = Hashlist[i];
        while (hnp) {
            HNODE *hnp_next = hnp->next;

            int prio = hnp->part->get_percent();
            arb_assert(prio>0);

            if (hnp->part->is_leaf_edge()) {
                prio = 0; // add leaf edges in a final step 
            }

            size[prio]++;

            arb_assert(prio >= 0 && prio <= max_part_percent);

            if (heads[prio]) {
                hnp->next = heads[prio];
                heads[prio] = hnp;
            }
            else {
                heads[prio] = tails[prio] = hnp;
                hnp->next = NULL;
            }

            hnp = hnp_next;
        }
        Hashlist[i] = NULL;
    }

    // sort each list (should be strict sort)
    for (int idx = 0; idx<list_count; idx++) {
        if (heads[idx]) {
            HNODE **array = (HNODE**)getmem(size[idx]*sizeof(*array));
            {
                HNODE *head = heads[idx];
                for (size_t a = 0; head; a++, head = head->next) {
                    array[a] = head;
                }
            }
            GB_sort((void**)array, 0, size[idx], HNODE_order, NULL);
            for (size_t a = 1; a<size[idx]; ++a) {
                array[a-1]->next = array[a];
            }

            heads[idx] = array[0];
            tails[idx] = array[size[idx]-1];

            tails[idx]->next = NULL;

            free(array);
        }
    }

    // concatenate lists in reverse order
    HNODE *head = NULL;
    HNODE *tail = NULL;
    for (int prio = list_count-1; prio>=0; prio--) {
        if (heads[prio]) {
            if (!head) {
                head = heads[prio];
                tail = tails[prio];
            }
            else {
                tail->next = heads[prio];
                tail = tails[prio];
            }
        }
    }

    free(size);
    free(tails);
    free(heads);

    Sortedlist = head;
}


