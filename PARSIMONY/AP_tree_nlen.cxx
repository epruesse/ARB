#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <arbdb.h>
#include <arbdbt.h>
//
#include <awt_tree.hxx>
#include <awt_seq_dna.hxx>
#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "pars_debug.hxx"
#include "ap_tree_nlen.hxx"
// #include <aw_root.hxx>

using namespace std;

#define ap_assert(x) arb_assert(x)

// ---------------------------------
//      Section base operations:
// ---------------------------------

//  constructor/destructor
//  dup
//  check_update
//  copy
//  ostream&<<

AP_tree_nlen::AP_tree_nlen(AP_tree_root *Tree_root)
    : AP_tree(Tree_root)
{
    //    this->init();
    kernighan = AP_NONE;
    sequence  = NULL;

    edge[0]  = edge[1] = edge[2] = NULL;
    index[0] = index[1] = index[2] = 0;
    distance = INT_MAX;

    //    cout << "AP_tree_nlen-constructor\n";
}

AP_tree *AP_tree_nlen::dup(void)
{
    return (AP_tree *) new AP_tree_nlen(this->tree_root);
}

AP_UPDATE_FLAGS AP_tree_nlen::check_update(void)
{
    AP_UPDATE_FLAGS res = this->AP_tree::check_update();

    if (res == AP_UPDATE_RELOADED) {
        return AP_UPDATE_OK;
    }

    return res;
}

void AP_tree_nlen::copy(AP_tree_nlen *tree)
{
    // like = operator
    // but copies sequence if is leaf

    this->is_leaf = tree->is_leaf;
    this->leftlen = tree->leftlen;
    this->rightlen = tree->rightlen;
    this->gb_node = tree->gb_node;

    if(tree->name != NULL) {
        this->name = strdup(tree->name);
    }
    else {
        this->name = NULL;
    }

    if (is_leaf == AP_TRUE) {
        if (tree->sequence) {
            this->sequence = tree->sequence;
        }
        else {
            cout << "empty sequence at leaf";
            this->sequence = 0;
        }
    }
}

ostream& operator<<(ostream& out, const AP_tree_nlen& node)
{
    static int notTooDeep;

    out << ' ';

    if (&node==NULL) {
        out << "NULL";
    }
    if (node.is_leaf) {
        out << ((void *)&node) << '(' << node.name << ')';
    }
    else {
        if (notTooDeep) {
            out << ((void *)&node);
            if (!node.father) out << " (ROOT)";
        }
        else {
            notTooDeep = 1;

            out << "NODE(" << ((void *)&node);

            if (!node.father) {
                out << " (ROOT)";
            }
            else {
                out << ", father=" << node.father;
            }

            out << ", leftson=" << node.leftson
                << ", rightson=" << node.rightson
                << ", edge[0]=" << *(node.edge[0])
                << ", edge[1]=" << *(node.edge[1])
                << ", edge[2]=" << *(node.edge[2])
                << ")";

            notTooDeep = 0;
        }
    }

    return out << ' ';
}

/********************************************

Section Edge operations:

    unusedEdge
    edgeTo
    nextEdge
    unlinkAllEdges
    destroyAllEdges

********************************************/

int AP_tree_nlen::unusedEdge() const
{
    int e;

    for (e=0; e<3; e++) if (edge[e]==NULL) return e;

    cout << "No unused edge found at" << *this << '\n';
    return -1;
}

AP_tree_edge* AP_tree_nlen::edgeTo(const AP_tree_nlen *neighbour) const
{
    int e;

    for (e=0; e<3; e++) {
        if (edge[e]!=NULL && edge[e]->node[1-index[e]]==neighbour) {
            return edge[e];
        }
    }
    cout << "AP_tree_nlen::edgeTo: " << *this << "\nhas no edge to " << *neighbour << '\n';
    GB_CORE;
    return NULL;
}

AP_tree_edge* AP_tree_nlen::nextEdge(const AP_tree_edge *thisEdge) const
    // returns the next edge adjacent to the actual node
    //
    // if thisEdge == NULL then we return the "first" edge (edge[0])
    // otherwise we return the next edge following 'thisEdge' in the array edge[]
{
    return edge[ thisEdge ? ((indexOf(thisEdge)+1) % 3) : 0 ];
}

void AP_tree_nlen::unlinkAllEdges(AP_tree_edge **edgePtr1, AP_tree_edge **edgePtr2, AP_tree_edge **edgePtr3)
{
    ap_assert(edge[0]!=NULL);
    ap_assert(edge[1]!=NULL);
    ap_assert(edge[2]!=NULL);

    *edgePtr1 = edge[0]->unlink();
    *edgePtr2 = edge[1]->unlink();
    *edgePtr3 = edge[2]->unlink();
}

void AP_tree_nlen::linkAllEdges(AP_tree_edge *edge1, AP_tree_edge *edge2, AP_tree_edge *edge3)
{
    ap_assert(edge[0]==NULL);
    ap_assert(edge[1]==NULL);
    ap_assert(edge[2]==NULL);

    edge1->relink(this,Father()->Father() ? Father() : Brother());
    edge2->relink(this,Leftson());
    edge3->relink(this,Rightson());
}

/********************************************

Section Tree operations:

    insert
    remove
    swap
    set_root
    move
    costs

********************************************/


/*-----------------------
  insert
  -----------------------*/
GB_ERROR AP_tree_nlen::insert(AP_tree *new_brother)
{
    //
    //  inserts a node at the father-edge of new_brother
    //
    //

    AP_tree      *pntr;
    GB_ERROR      error      = 0;
    AP_tree_nlen *newBrother = dynamic_cast<AP_tree_nlen*>(new_brother);
    ap_assert(new_brother);

    AP_tree_edge *oldEdge;

    ap_main->push_node(new_brother, STRUCTURE);

    if (new_brother->father) {
        ap_main->push_node(new_brother->father, BOTH);
        for (pntr = new_brother->father->father; pntr; pntr = pntr->father) {
            ap_main->push_node(pntr, SEQUENCE);
        }

        if (new_brother->father->father) {
            oldEdge = newBrother->edgeTo(newBrother->Father())->unlink();
            error = this->AP_tree::insert(new_brother);
            oldEdge->relink(Father(),Father()->Father());
        }
        else { // insert to son of root
            oldEdge = newBrother->edgeTo(newBrother->Brother())->unlink();
            error = this->AP_tree::insert(new_brother);
            oldEdge->relink(Father(),Father()->Brother());
        }

        new AP_tree_edge(this,Father());
        new AP_tree_edge(Father(),newBrother);
    }
    else { // insert at root
        AP_tree_nlen    *lson = newBrother->Leftson(),
            *rson = newBrother->Rightson();

        ap_main->push_node(lson, STRUCTURE);
        ap_main->push_node(rson, STRUCTURE);

        oldEdge = lson->edgeTo(rson)->unlink();
#if defined(DEBUG)
        cout << "old Edge = " << oldEdge << '\n';
#endif // DEBUG

        error = this->AP_tree::insert(new_brother);

        oldEdge->relink(this,newBrother);
        new AP_tree_edge(newBrother,rson);
        new AP_tree_edge(newBrother,lson);
    }

    return error;
}
/*-----------------------
  remove
  -----------------------*/
GB_ERROR AP_tree_nlen::remove(void)
    //
    // Removes the node and its father from the tree:
    //
    //       grandpa                grandpa
    //           /                    /
    //          /                    /
    //    father        =>        brother
    //       /     \                                            .
    //      /       \                                           .
    //   this       brother
    //
    // One of the edges is relinked between brother and grandpa.
    // The other two edges are lost. This is not very relevant in respect to
    // memory usage because very few remove()s are really performed - the majority
    // is undone by a pop().
    // In the last case the two unlinked edges will be re-used, cause their
    // memory location was stored in the tree-stack.
    //
{
    GB_ERROR error = 0;
    AP_tree         *pntr;
    AP_tree_edge    *oldEdge;
    AP_tree_nlen    *oldBrother = Brother();


    if (father == 0) {
        return (char *)GB_export_error("AP_tree_nlen::remove(void) Tried to remove ROOT ");
    }

    ap_main->push_node(this, STRUCTURE);
    ap_main->push_node(brother(), STRUCTURE);

    for (pntr = father->father; pntr; pntr = pntr->father) {
        ap_main->push_node(pntr, SEQUENCE);
    }

    if (father->father) {
        AP_tree_nlen *grandPa = Father()->Father();

        ap_main->push_node(father, BOTH);
        ap_main->push_node(grandPa, STRUCTURE);

        edgeTo(Father())->unlink();
        Father()->edgeTo(oldBrother)->unlink();

        if (grandPa->father) {
            oldEdge = Father()->edgeTo(grandPa)->unlink();
            error = this->AP_tree::remove();
            oldEdge->relink(oldBrother,grandPa);
        }
        else { // remove grandson of root
            AP_tree_nlen *uncle = Father()->Brother();
            ap_main->push_node(uncle,STRUCTURE);

            oldEdge = Father()->edgeTo(uncle)->unlink();
            error = this->AP_tree::remove();
            oldEdge->relink(oldBrother,uncle);
        }
    }
    else { // remove son of root
        AP_tree_nlen *lson = Brother()->Leftson();
        AP_tree_nlen *rson = Brother()->Rightson();

        if (!lson || !rson) {
            error = "Your tree contains less than 2 species (cannot load it)";
        }
        else {
            ap_main->push_node(lson,STRUCTURE);
            ap_main->push_node(rson,STRUCTURE);
            ap_main->push_node(father, ROOT);

            ap_main->set_tree_root(oldBrother);

            //  delete edgeTo(oldBrother);
            oldBrother->edgeTo(lson)->unlink();
            //  delete oldBrother->edgeTo(lson);
            oldEdge = oldBrother->edgeTo(rson)->unlink();
            error = this->AP_tree::remove();
            oldEdge->relink(lson,rson);
        }
    }

    return error;
}

/*-----------------------
  swap
  -----------------------*/
GB_ERROR AP_tree_nlen::swap_assymetric(AP_TREE_SIDE mode)
{
    // mode AP_LEFT exchanges leftson with brother
    // mode AP_RIGHT exchanges rightson with brother

    AP_tree *pntr;
    AP_tree_nlen *oldBrother, *movedSon;
    GB_ERROR error = 0;
    AP_tree_edge *edge1, *edge2;

    //    cout << "swap" << *this << '\n';

    if (is_leaf == AP_TRUE) return AP_tree::swap_assymetric(mode);
    if (father == 0)        return AP_tree::swap_assymetric(mode);

    oldBrother = Brother();

    switch (mode) {
        case AP_LEFT: movedSon = Leftson(); break;
        case AP_RIGHT: movedSon = Rightson();  break;
        default: movedSon = NULL;  break;
    }

    if (!father->father) {
        //son of root case
        //take leftson of brother to exchange with

        if (oldBrother->is_leaf == AP_TRUE) return 0; // no swap needed !

        AP_tree_nlen *nephew = oldBrother->Leftson();

        ap_main->push_node(this, BOTH);
        ap_main->push_node(movedSon,STRUCTURE);
        ap_main->push_node(father, SEQUENCE);
        ap_main->push_node(nephew, STRUCTURE);
        ap_main->push_node(oldBrother, BOTH);

        edge1 = edgeTo(movedSon)->unlink();
        edge2 = oldBrother->edgeTo(nephew)->unlink();

        error = this->AP_tree::swap_assymetric(mode);

        edge1->relink(this,nephew);
        edge2->relink(oldBrother,movedSon);
    }
    else {
        ap_main->push_node(this, BOTH);
        ap_main->push_node(father, BOTH);
        ap_main->push_node(oldBrother, STRUCTURE);
        ap_main->push_node(movedSon,STRUCTURE);

        // from father to root buffer all sequences

        for (pntr = father->father; pntr ; pntr = pntr->father) {
            ap_main->push_node(pntr, SEQUENCE);
        }

        edge1 = edgeTo(movedSon)->unlink();
        edge2 = Father()->edgeTo(oldBrother)->unlink();

        error = AP_tree::swap_assymetric(mode);

        edge1->relink(this,oldBrother);
        edge2->relink(Father(),movedSon);
    }

    return error;
}


/*-----------------------
  set_root
  -----------------------*/
GB_ERROR AP_tree_nlen::set_root() {
    AP_tree   *pntr;

    if (!father || !father->father) { // already root
        return 0;
    }

    // from this to root buffer the nodes

    ap_main->push_node(this , STRUCTURE);
    AP_tree *old_brother = 0;

    for  (pntr = father; pntr->father; pntr = pntr->father) {
        ap_main->push_node( pntr, BOTH);
        old_brother= pntr;
    }

    AP_tree *old_root = pntr;

    if (old_brother) {
        old_brother = old_brother->brother();
        ap_main->push_node( old_brother , STRUCTURE);
    }

    ap_main->push_node(old_root, ROOT);

    return this->AP_tree::set_root();
}

/*-----------------------
  move
  -----------------------*/
GB_INLINE void sort(AP_tree_edge **e1, AP_tree_edge **e2) {
    if ((*e1)->Age() > (*e2)->Age()) {
        AP_tree_edge *tmp = *e1;
        *e1 = *e2;
        *e2 = tmp;
    }
}

GB_INLINE void sort(AP_tree_edge **e1, AP_tree_edge **e2, AP_tree_edge **e3) {
    sort(e1,e2);
    sort(e2,e3);
    sort(e1,e2);
}

const char *AP_tree_nlen::move(AP_tree *new_brother,AP_FLOAT rel_pos)
{
    AP_tree *pntr;

    //    cout << "move" << *this << "to new_brother" << *((AP_tree_nlen*)new_brother) << rel_pos << '\n';

    if (!father) {
        AW_ERROR ("AP_tree_nlen::move: You cannot move the root");
        return "You cannot move the root";
    }

    if (!new_brother->father) {
        AW_ERROR ("AP_tree_nlen::move: You cannot move to the root");
        return "You cannot move to the root";
    }

    ap_main->push_node(this , STRUCTURE);
    ap_main->push_node(brother(), STRUCTURE);

    if (father->father) {
        AP_tree *grandpa = father->father;

        ap_main->push_node(father , BOTH);

        if (grandpa->father) {
            ap_main->push_node(grandpa, BOTH);
            for  (pntr = grandpa->father; pntr; pntr = pntr->father) {
                ap_main->push_node( pntr, SEQUENCE);
            }
        }
        else { // grandson of root
            ap_main->push_node(grandpa, ROOT);
            ap_main->push_node(father->brother(), STRUCTURE);
        }
    }
    else { // son of root
        ap_main->push_node(father , ROOT);

        if (!brother()->is_leaf) {
            ap_main->push_node(brother()->leftson, STRUCTURE);
            ap_main->push_node(brother()->rightson, STRUCTURE);
        }
    }

    ap_main->push_node(new_brother , STRUCTURE);
    if (new_brother->father) {
        if (new_brother->father->father) {
            ap_main->push_node(new_brother->father , BOTH);
        }
        else { // move to son of root
            ap_main->push_node(new_brother->father , BOTH);
            ap_main->push_node(new_brother->brother(), STRUCTURE);
        }

        for  (pntr = new_brother->father->father; pntr; pntr = pntr->father) {
            ap_main->push_node( pntr, SEQUENCE);
        }
    }

    const char *res = "Da fehlt wohl was...";
    AP_tree_nlen    *thisFather = Father(),
        *newBrother = (AP_tree_nlen*)new_brother,
        *grandFather = thisFather->Father(),
        *oldBrother = Brother(),
        *newBrothersFather = newBrother->Father();
    int edgesChange = ! (father==new_brother || new_brother->father==this->father);
    AP_tree_edge *e1,*e2,*e3,*e4;

    if (edgesChange) {
        if (thisFather==newBrothersFather->Father()) { // son -> son of brother
            if (grandFather) {
                if(grandFather->Father()) {
                    //          cout << "son -> son of brother\n";

                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(oldBrother)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);              // sort by age (e1==oldest edge)
                    e1->relink(oldBrother,grandFather);     // use oldest edge at remove position
                    thisFather->linkAllEdges(e2,e3,e4);
                }
                else { // grandson of root -> son of brother
                    AP_tree_nlen *uncle = thisFather->Brother();

                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(oldBrother)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);  // sort by age (e1==oldest edge)
                    e1->relink(oldBrother,uncle);
                    thisFather->linkAllEdges(e2,e3,e4);
                }
            }
            else { // son of root -> grandson of root
                oldBrother->unlinkAllEdges(&e1,&e2,&e3);
                res = this->AP_tree::move(new_brother,rel_pos);
                thisFather->linkAllEdges(e1,e2,e3);
            }
        }
        else if (grandFather==newBrothersFather) { // son -> brother of father
            if (grandFather->father) {
                thisFather->unlinkAllEdges(&e1,&e2,&e3);
                e4 = grandFather->edgeTo(newBrother)->unlink();

                res = this->AP_tree::move(new_brother,rel_pos);

                sort(&e1,&e2,&e3);
                e1->relink(oldBrother,grandFather);
                thisFather->linkAllEdges(e2,e3,e4);
            }
            else { // no edges change if we move grandson of root -> son of root
                res = this->AP_tree::move(new_brother,rel_pos);
            }
        }
        else {
            //  now we are sure, the minimal distance
            //  between 'this' and 'newBrother' is 4 edges
            //  or if the root-edge is between them, the
            //  minimal distance is 3 edges

            if (!grandFather) { // son of root
                cout << "move son of root\n";

                oldBrother->unlinkAllEdges(&e1,&e2,&e3);
                e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                res = this->AP_tree::move(new_brother,rel_pos);

                sort(&e1,&e2,&e3);
                e1->relink(oldBrother->Leftson(),oldBrother->Rightson()); // new root-edge
                thisFather->linkAllEdges(e2,e3,e4);     // old root
            }
            else if (!grandFather->Father()) { // grandson of root
                if (newBrothersFather->Father()->Father()==NULL) { // grandson of root -> grandson of root
                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);
                    e1->relink(oldBrother,newBrothersFather);   // new root-edge
                    thisFather->linkAllEdges(e2,e3,e4);
                }
                else {
                    AP_tree_nlen *uncle = thisFather->Brother();

                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);
                    e1->relink(oldBrother,uncle);
                    thisFather->linkAllEdges(e2,e3,e4);
                }
            }
            else {
                if (newBrothersFather->Father()==NULL) { // move to son of root
                    AP_tree_nlen *newBrothersBrother = newBrother->Brother();

                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(newBrothersBrother)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);
                    e1->relink(oldBrother,grandFather);
                    thisFather->linkAllEdges(e2,e3,e4);
                }
                else { // simple independent move
                    thisFather->unlinkAllEdges(&e1,&e2,&e3);
                    e4 = newBrother->edgeTo(newBrothersFather)->unlink();

                    res = this->AP_tree::move(new_brother,rel_pos);

                    sort(&e1,&e2,&e3);
                    e1->relink(oldBrother,grandFather);
                    thisFather->linkAllEdges(e2,e3,e4);
                }
            }
        }
    }
    else { // edgesChange==0
        res = this->AP_tree::move(new_brother,rel_pos);
    }

    return res;
}

/*-----------------------
  costs
  -----------------------*/
extern long global_combineCount;

AP_FLOAT AP_tree_nlen::costs(void) { /* cost of a tree (number of changes ..) */
    if (! this->tree_root->sequence_template) return 0.0;
    this->parsimony_rek();
    return (AP_FLOAT)this->mutation_rate;
}

/*******************************************

Section Buffer Operations:

    unhash_sequence()   // only deletes an existing parsimony sequence
    push()
    pop()
    clear()

********************************************/

void AP_tree_nlen::unhash_sequence() {
    //removes the current sequence
    //(not leaf)

    if (sequence != 0) {
        if (!is_leaf) {
            sequence->is_set_flag = AP_FALSE;
        }
    }

    return;
}

AP_BOOL AP_tree_nlen::clear(unsigned long datum, unsigned long user_buffer_count) {
    // returns AP_TRUE if the first element is removed
    // AP_FALSE if it is copied into the previous level
    // according if user_buffer is greater than datum

    //    cout << "clear\n";

    AP_tree_buffer * buff;
    AP_BOOL         result;

    if (!this->stack_level == datum)
    {
        AW_ERROR("AP_tree_nlen::clear: internal control number check failed");
        return AP_FALSE;
    }

    buff = stack.pop();

    if (buff->controll == datum - 1 || user_buffer_count >= datum) {
        //previous node is buffered

        if (buff->mode & SEQUENCE) delete buff->sequence;

        stack_level = buff->controll;
        delete  buff;
        result      = AP_TRUE;
    }
    else {
        stack_level = datum - 1;
        stack.push(buff);
        result      = AP_FALSE;
    }

    return result;
}


AP_BOOL AP_tree_nlen::push(AP_STACK_MODE mode, unsigned long datum) {
    // according to mode
    // tree_structure / sequence is buffered in the node

    AP_tree_buffer *new_buff;
    AP_BOOL ret;

    if (is_leaf && !(STRUCTURE & mode)) return AP_FALSE;    // tips push only structure

    if (this->stack_level == datum) {
        AP_tree_buffer *last_buffer = stack.get_first();
        if (sequence &&(mode & SEQUENCE)) sequence->is_set_flag = AP_FALSE;
        if (0 == (mode & ~last_buffer->mode)) { // already buffered
            return AP_FALSE;
        }
        new_buff = last_buffer;
        ret = AP_FALSE;
    }
    else {
        new_buff           = new AP_tree_buffer;
        new_buff->count    = 1;
        new_buff->controll = stack_level;
        new_buff->mode     = NOTHING;

        stack.push(new_buff);
        this->stack_level = datum;
        ret = AP_TRUE;
    }

    if ( (mode & STRUCTURE) && !(new_buff->mode & STRUCTURE) ) {
        //  cout << "push structure " << *this << '\n';
        new_buff->father   = father;
        new_buff->leftson  = leftson;
        new_buff->rightson = rightson;
        new_buff->leftlen  = leftlen;
        new_buff->rightlen = rightlen;
        new_buff->gb_node  = gb_node;
        new_buff->distance = distance;

        for (int e=0; e<3; e++) {
            new_buff->edge[e]      = edge[e];
            new_buff->edgeIndex[e] = index[e];
            if (edge[e]) {
                new_buff->edgeData[e]  = edge[e]->data;
            }
        }
    }

    if ( (mode & SEQUENCE) && !(new_buff->mode & SEQUENCE) ) {
        if (sequence) {
            new_buff->sequence      = sequence;
            new_buff->mutation_rate = mutation_rate;
            mutation_rate           = 0.0;
            sequence                = 0;
        }
        else {
            new_buff->sequence = 0;
            AW_ERROR("Sequence not found %s",this->name);
        }
    }

    new_buff->mode = (AP_STACK_MODE)(new_buff->mode|mode);

    return ret;
}

void AP_tree_nlen::pop(unsigned long datum) { /* pop old tree costs */
    AP_tree_buffer *buff;

    if (stack_level != datum) {
        AW_ERROR("AP_tree_nlen::pop(): Error in Node Stack");
    }

    buff = stack.pop();

    AP_STACK_MODE   mode = buff->mode;

    if (mode&STRUCTURE) {
        //  cout << "pop structure " << this << '\n';

        father   = buff->father;
        leftson  = buff->leftson;
        rightson = buff->rightson;
        leftlen  = buff->leftlen;
        rightlen = buff->rightlen;
        gb_node  = buff->gb_node;
        distance = buff->distance;

        for (int e=0; e<3; e++) {
            edge[e] = buff->edge[e];

            if (edge[e]) {
                index[e] = buff->edgeIndex[e];

                edge[e]->index[index[e]] = e;
                edge[e]->node[index[e]]  = this;
                edge[e]->data            = buff->edgeData[e];
            }
        }
    }

    if (mode&SEQUENCE) {
        if (sequence) delete sequence;

        sequence      = buff->sequence;
        mutation_rate = buff->mutation_rate;
    }

    if (ROOT==mode) {
        //  cout << "root popped:" << this << "\n";
        ap_main->set_tree_root(this);
    }

    stack_level = buff->controll;
    delete buff;
}

/********************************************************
Section Parsimony:
********************************************************/

void AP_tree_nlen::parsimony_rek(void)
{
    if (sequence && sequence->is_set_flag) return;

    if (is_leaf) {
        sequence->is_set_flag = AP_TRUE;
        return;
    }

    if (!Leftson()->sequence || !Leftson()->sequence->is_set_flag  ) Leftson()->parsimony_rek();
    if (!Rightson()->sequence|| !Rightson()->sequence->is_set_flag ) Rightson()->parsimony_rek();

    if (!Leftson()->sequence->is_set_flag || !Rightson()->sequence->is_set_flag) {
        AW_ERROR("AP_tree_nlen::parsimony_rek:  Cannot set sequence");
        return;
    }

    if (sequence == 0) sequence = tree_root->sequence_template->dup();

    AP_FLOAT mutations_for_combine = sequence->combine(Leftson()->sequence, Rightson()->sequence);
    mutation_rate                  = Leftson()->mutation_rate + Rightson()->mutation_rate + mutations_for_combine;

#if defined(DEBUG) && 0
    printf("mutation-rates: left=%f right=%f combine=%f overall=%f %s\n",
           Leftson()->mutation_rate, rightson->mutation_rate, mutations_for_combine, mutation_rate,
           fullname());
#endif // DEBUG

    sequence->is_set_flag = AP_TRUE;
}

/********************************************************
Section NNI
********************************************************/

AP_FLOAT AP_tree_nlen::nn_interchange_rek(AP_BOOL openclosestatus, int &Abort,int deep, AP_BL_MODE mode, GB_BOOL skip_hidden)
{
    if (!father)
    {
        return rootEdge()->nni_rek(openclosestatus,Abort,deep,skip_hidden,mode);
    }

    if (!father->father)
    {
        AP_tree_edge *e = rootEdge();

        return e->nni_rek(openclosestatus,Abort,deep,skip_hidden,mode,e->otherNode(this));
    }

    return edgeTo(Father())->nni_rek(openclosestatus,Abort,deep,skip_hidden,mode,Father());
}



/********************************************************
Section Kernighan-Lin
********************************************************/

void            AP_tree_nlen::
kernighan_rek(int rek_deep, int *rek_2_width, int rek_2_width_max, const int rek_deep_max,
              double(*funktion) (double, double *, int), double *param_liste, int param_anz,
              AP_FLOAT pars_best, AP_FLOAT pars_start, AP_FLOAT pars_prev,
              AP_KL_FLAG rek_width_type, AP_BOOL *abort_flag)
{
    //
    // rek_deep Rekursionstiefe
    // rek_2_width Verzweigungsgrad
    // neg_counter zaehler fuer die Aeste in denen Kerninghan Lin schon angewendet wurde
    // funktino Funktion fuer den dynamischen Schwellwert
    // pars_ Verschiedene Parsimonywerte

    AP_FLOAT help, pars[8];
    //acht parsimony werte
    AP_tree_nlen * pars_refpntr[8];
    //zeiger auf die naechsten Aeste
    int             help_ref, pars_ref[8];
    //referenzen auf die vertauschten parsimonies
    static AP_TREE_SIDE pars_side_ref[8];
    //linker oder rechter ast
    int             i, t, bubblesort_change = 0;
    //
    int             rek_width, rek_width_static = 0, rek_width_dynamic = 0;
    AP_FLOAT        schwellwert = funktion(rek_deep, param_liste, param_anz) + pars_start;

    //parameterausgabe

#if 0
    cout << "DEEP  " << rek_deep << "   ";
    cout << "  maxdeep " << rek_deep_max << " suche " << rek_width_type << " abbruch " << *abort_flag;
    cout << "\npbest    " << pars_best << " pstart  " << pars_start << " pprev  " << pars_prev << "\nParam : ";
    for (i = 0; i < param_anz; i++)
        cout << i << " : " << param_liste[i] << "    ";
    cout << "\nSchwellwert  " << schwellwert << "\n";
    cout.flush();
#endif
    if (rek_deep >= rek_deep_max)       return;
    if (is_leaf == AP_TRUE)         return;
    if (*abort_flag == AP_TRUE)     return;

    //
    //Referenzzeiger auf die vier Kanten und
    // zwei swapmoeglichkeiten initialisieren
    //
    AP_tree *this_brother = this->brother();
    if (rek_deep == 0) {
        for (i = 0; i < 8; i+=2) {
            pars_side_ref[i] = AP_LEFT;
            pars_side_ref[i+1] = AP_RIGHT;
        }
        pars_refpntr[0] = pars_refpntr[1] = (AP_tree_nlen *) this;
        pars_refpntr[2] = pars_refpntr[3] = 0;
        pars_refpntr[4] = pars_refpntr[5] = 0;
        pars_refpntr[6] = pars_refpntr[7] = 0;
        //      cout << "NEW REKURSION\n\n";
    }else{
        pars_refpntr[0] = pars_refpntr[1] = (AP_tree_nlen *) this->leftson;
        pars_refpntr[2] = pars_refpntr[3] = (AP_tree_nlen *) this->rightson;
        if (father->father != 0) {
            //Referenzzeiger falls nicht an der Wurzel
            pars_refpntr[4] = pars_refpntr[5] = (AP_tree_nlen *) this->father;
            pars_refpntr[6] = pars_refpntr[7] = (AP_tree_nlen *) this_brother;
        } else {
            //an der Wurzel nehme linken und rechten Sohns des Bruders
            if (this->brother()->is_leaf == AP_FALSE) {
                pars_refpntr[4] = pars_refpntr[5] = (AP_tree_nlen *) this_brother->leftson;
                pars_refpntr[6] = pars_refpntr[7] = (AP_tree_nlen *) this_brother->rightson;
            } else {
                pars_refpntr[4] = pars_refpntr[5] = 0;
                pars_refpntr[6] = pars_refpntr[7] = 0;
            }
        }
    }


    if (!father) return;        // no kl at root

    //
    //parsimony werte bestimmen
    //

    // Wurzel setzen

    ap_main->push();
    this->set_root();
    (*ap_main->tree_root)->costs();

    int             visited_subtrees = 0;
    int     better_subtrees = 0;
    for (i = 0; i < 8; i++) {
        pars_ref[i] = i;
        pars[i] = -1;

        if (!pars_refpntr[i])   continue;
        if (pars_refpntr[i]->is_leaf) continue;
        if (!pars_refpntr[i]->kernighan == AP_NONE) continue;
        if (pars_refpntr[i]->gr.hidden) continue;
        if (pars_refpntr[i]->father->gr.hidden) continue;

        //nur wenn kein Blatt ist
        ap_main->push();
        pars_refpntr[i]->swap_assymetric(pars_side_ref[i]);
        pars[i] = (*ap_main->tree_root)->costs();
        if (pars[i] < pars_best) {
            better_subtrees++;
            pars_best      = pars[i];
            rek_width_type = AP_BETTER;
        }
        if (pars[i] < schwellwert) {
            rek_width_dynamic++;
        }
        ap_main->pop();
        visited_subtrees ++;

    }
    //Bubblesort, in pars[0] steht kleinstes element
    //
    //!!CAUTION ! !The original parsimonies will be exchanged


    for (i=7,t=0;t<i;t++) {
        if (pars[t] <0) {
            pars[t]     = pars[i];
            pars_ref[t] = i;
            t--;
            i--;
        }
    }

    bubblesort_change = 0;
    for (t = visited_subtrees - 1; t > 0; t--) {
        for (i = 0; i < t; i++) {
            if (pars[i] > pars[i+1] ) {
                bubblesort_change = 1;
                help_ref          = pars_ref[i];
                pars_ref[i]       = pars_ref[i + 1];
                pars_ref[i + 1]   = help_ref;
                help              = pars[i];
                pars[i]           = pars[i + 1];
                pars[i + 1]       = help;
            }
        }
        if (bubblesort_change == 0)
            break;
    }

    display_out(pars, visited_subtrees, pars_prev, pars_start, rek_deep);
    //Darstellen


    //  for (i=0;i<visited_subtrees;i++)  cout << "  " << pars[i];


    if (rek_deep < rek_2_width_max) rek_width_static = rek_2_width[rek_deep];
    else rek_width_static                            = 1;

    rek_width = visited_subtrees;
    if (rek_width_type == AP_BETTER) {
        rek_width =  better_subtrees;
    }
    else {
        if (rek_width_type & AP_STATIC) {
            if (rek_width> rek_width_static) rek_width = rek_width_static;
        }
        if (rek_width_type & AP_DYNAMIK) {
            if (rek_width> rek_width_dynamic) rek_width = rek_width_dynamic;
        }else   if (!(rek_width_type & AP_STATIC)) {
            if (rek_width> 1) rek_width = 1;
        }

    }

    if (rek_width > visited_subtrees)   rek_width = visited_subtrees;

    //  cout << "   rek_width:  " << rek_width << "\n";
    //  cout.flush();

    for (i=0;i < rek_width; i++) {
        ap_main->push();
        pars_refpntr[pars_ref[i]]->kernighan = pars_side_ref[pars_ref[i]];
        //Markieren
        pars_refpntr[pars_ref[i]]->swap_assymetric(pars_side_ref[pars_ref[i]]);
        //vertausche seite
        (*ap_main->tree_root)->parsimony_rek();
        switch (rek_width_type) {
            case AP_BETTER:{
                //starte kerninghan_rek mit rekursionstiefe 3, statisch
                AP_BOOL flag = AP_FALSE;
                cout << "found better !\n";
                pars_refpntr[pars_ref[i]]->kernighan_rek(rek_deep + 1, rek_2_width,
                                                         rek_2_width_max, rek_deep_max + 4,
                                                         funktion, param_liste, param_anz,
                                                         pars_best, pars_start, pars[i],
                                                         AP_STATIC, &flag);
                *abort_flag = AP_TRUE;
                break;
            }
            default:
                pars_refpntr[pars_ref[i]]->kernighan_rek(rek_deep + 1, rek_2_width,
                                                         rek_2_width_max, rek_deep_max,
                                                         funktion, param_liste, param_anz,
                                                         pars_best, pars_start, pars[i],
                                                         rek_width_type, abort_flag);
                break;
        }
        pars_refpntr[pars_ref[i]]->kernighan = AP_NONE;
        //Demarkieren
        if (*abort_flag == AP_TRUE) {
            cout << "   parsimony:  " << pars_best << "took: " << i <<"\n";
            for (i=0;i<visited_subtrees;i++)  cout << "  " << pars[i];
            cout << "\n";
            if (!rek_deep){
                cout << "NEW REKURSION\n\n";
            }
            cout.flush();

            ap_main->clear();
            break;
        } else {
            ap_main->pop();
        }
    }
    if (*abort_flag == AP_TRUE) {       // pop/clear wegen set_root
        ap_main->clear();
    } else {
        ap_main->pop();
    }
    return;
}

/*************************************************************************
Section Crossover List (Funktionen die die crossover liste aufbauen):

    createListRekUp
    createListRekSide
    createList

**************************************************************************/

#if 0

void addToList(AP_CO_LIST *list,int *number,AP_tree_nlen *pntr,CO_LISTEL& wert0,CO_LISTEL& wert1) {
    if (wert0.isLeaf == AP_TRUE) {
        list[*number].leaf0 = wert0.refLeaf;
        list[*number].node0 = -10;
    } else {
        list[*number].leaf0 = 0;
        list[*number].node0 = wert0.refNode;
    }
    if (wert1.isLeaf == AP_TRUE) {
        list[*number].leaf1 = wert1.refLeaf;
        list[*number].node1 = -1;
    } else {
        list[*number].leaf1 = 0;
        list[*number].node1 = wert1.refNode;
    }
    list[*number].pntr = pntr;
    return;
}

void AP_tree_nlen::createListRekUp(AP_CO_LIST *list,int *cn) {
    if (this->is_leaf == AP_TRUE) {
        refUp.init    = AP_TRUE;
        refUp.isLeaf  = AP_TRUE;
        refUp.refLeaf = gb_node;
        return;
    }
    if (refUp.init == AP_FALSE) {
        if (leftson->refUp.init == AP_FALSE) leftson->createListRekUp(list,cn);
        if (rightson->refUp.init == AP_FALSE) rightson->createListRekUp(list,cn);

        refUp.init    = AP_TRUE;
        refUp.isLeaf  = AP_FALSE;
        refUp.refNode = *cn;
        (*cn)++;


        addToList(list,cn,this,leftson->refUp,rightson->refUp);
        if (father == 0) { // at root
            refRight.init = rightson->refUp.init;
            if ((refRight.isLeaf = rightson->refUp.isLeaf) == AP_TRUE) {
                refRight.refLeaf = rightson->refUp.refLeaf;
            }
            else {
                refRight.refNode = rightson->refUp.refNode;
            }

            refLeft.init = leftson->refUp.init;
            if ((refLeft.isLeaf = leftson->refUp.isLeaf) == AP_TRUE) {
                refLeft.refLeaf = leftson->refUp.refLeaf;
            }
            else {
                refLeft.refNode = leftson->refUp.refNode;
            }
        }
    }
    return;
}

void AP_tree_nlen::createListRekSide(AP_CO_LIST *list,int *cn) {
    //
    // has to be called after createListRekUp !!
    if (refRight.init == AP_FALSE) {
        refRight.init    = AP_TRUE;
        refRight.isLeaf  = AP_FALSE;
        refRight.refNode = *cn;
        (*cn)++;

        if (father->leftson == this) {
            if (father->refLeft.init == AP_FALSE) father->createListRekSide(list,cn);
            addToList(list,cn,this,leftson->refUp,father->refLeft);
        } else {
            if (father->refRight.init ==  AP_FALSE) father->createListRekSide(list,cn);
            addToList(list,cn,this,leftson->refUp,father->refRight);
        }
    }
    if (refLeft.init == AP_FALSE) {
        refLeft.init    = AP_TRUE;
        refLeft.isLeaf  = AP_FALSE;
        refLeft.refNode = *cn;
        (*cn)++;
        if (father->leftson == this) {
            if (father->refLeft.init == AP_FALSE) father->createListRekSide(list,cn);
            addToList(list,cn,this,rightson->refUp,father->refLeft);
        } else {
            if (father->refRight.init == AP_FALSE) father->createListRekSide(list,cn);
            addToList(list,cn,this,rightson->refUp,father->refRight);
        }
    }
    return;
}


AP_CO_LIST * AP_tree_nlen::createList(int *size)
{
    // returns an list with all
    // tree combinations
    AP_CO_LIST *list;
    int number = 0;
    if (father !=0) {
        AW_ERROR("AP_tree_nlen::createList may be called with damaged tree");
        return 0;
    }
    this->test_tree_rek();
    // 3*knotenvisited_subtrees +1 platz reservieren
    list = (AP_CO_LIST *) calloc((gr.node_sum-gr.leaf_sum)*3+1,sizeof(AP_CO_LIST));
    createListRekUp(list,&number);
    createListRekSide(list,&number);
    *size = number;
    return list;
}

#endif

/*************************************************************************
Section Misc stuff:

    sortByName
    test
    fullname

**************************************************************************/

const char* AP_tree_nlen::sortByName()
{
    if (name) return name;  // leaves

    const char *n1 = Leftson()->sortByName();
    const char *n2 = Rightson()->sortByName();

    if (strcmp(n1,n2)<0) return n1;

    AP_tree::swap_sons();

    return n2;
}

int AP_tree_nlen::test(void) const
{
    int edges = 0;

    for (int e=0; e<3; e++) if (edge[e]!=NULL) edges++;

    if (!sequence) cout << "Node" << *this << "has no sequence\n";

    if (father) {
        if (father->father == (AP_tree *)this) {
            cout << "Ooops! I am my own grandfather! How is this possible?\n" <<
                *this << '\n' <<
                *Father() << '\n';
        }

        if (is_leaf) {
            if (edges!=1) cout << "Leaf-Node" << *this << "has" << edges << " edges\n";
        }
        else {
            if (edges!=3) cout << "Inner-Node" << *this << "has" << edges << " edges\n";
        }

        int e;

        for (e=0; e<3; e++) {
            if (edge[e]) {
                if (edge[e]->isConnectedTo(this)) {
                    AP_tree_nlen *neighbour = edge[e]->otherNode(this);

                    if ( ! (neighbour==father || neighbour==leftson || neighbour==rightson)) {
                        if (father->father==NULL) {
                            if (!(father->leftson==neighbour || father->rightson==neighbour)) {
                                cout << "Neighbour is not brother (at root)\n";
                            }
                        }
                        else {
                            cout << "Edge " << edge[e] << " connects the nodes"
                                 << *this << "and" << *(edge[e]->otherNode(this))
                                 << "(they are not neighbours)\n";
                        }
                    }
                }
                else {
                    cout << "Node" << *this
                         << "is connected to wrong edge"
                         << edge[e] << '\n';
                }
            }
        }
    }
    else {
        if (edges) {
            cout << "Root" << *this << "has edges!\n";
        }
    }

    test_tree();    // AP_tree::

    return 0;
}

const char *AP_tree_nlen::fullname() const
{
    if (!name) {
        static char *buffer;
        char        *lName = strdup(Leftson()->fullname());
        char        *rName = strdup(Rightson()->fullname());
        int          len   = strlen(lName)+strlen(rName)+4;

        if (buffer) free(buffer);

        buffer = (char*)malloc(len);

        strcpy(buffer,"[");
        strcat(buffer,lName);
        strcat(buffer,",");
        strcat(buffer,rName);
        strcat(buffer,"]");

        free(lName);
        free(rName);

        return buffer;
    }

    return name;
}


char* AP_tree_nlen::getSequence()
{
    char *s;

    costs();
    AP_sequence_parsimony *pseq = (AP_sequence_parsimony*)sequence;
    ap_assert(pseq->is_set_flag);
    s = new char[pseq->sequence_len];
    memcpy(s,pseq->sequence,(unsigned int)pseq->sequence_len);

    return s;
}

