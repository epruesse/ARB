#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream.h>
#include <iomanip.h>
#include <memory.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <string.h>
#include <awt_tree.hxx>

#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "ap_tree_nlen.hxx"

#include <aw_root.hxx>

// #define ap_assert(x) arb_assert(x)

/**************************

    Constructor
    Destructor
    initialize
    relink
    unlink

**************************/
long AP_tree_edge::timeStamp = 0;

AP_tree_edge::AP_tree_edge(AP_tree_nlen *node1, AP_tree_nlen *node2)
{
    // not really necessary, but why not clear all:
    memset((char *)this,0,sizeof(AP_tree_edge));
    age  = timeStamp++;

    // link the nodes:

    relink(node1,node2);
}

AP_tree_edge::~AP_tree_edge()
{
    unlink();
}

static void buildSonEdges(AP_tree_nlen *tree)
    // Builds edges between a node and his two sons.
    // We assume there is already an edge to node's father and there are
    // no edges to his sons.
{
    if (!tree->is_leaf)
    {
        buildSonEdges(tree->Leftson());
        buildSonEdges(tree->Rightson());

        // to ensure the nodes contain the correct distance to the border
        // we MUST build all son edges before creating the father edge

        new AP_tree_edge(tree, tree->Leftson());
        new AP_tree_edge(tree, tree->Rightson());
    }
}

void AP_tree_edge::initialize(AP_tree_nlen *tree)
    // Builds all edges in the hole tree
    // The root node is skipped - instead his two sons are connected with an edge
{
    while (tree->Father()) tree = tree->Father();   // go up to root
    buildSonEdges(tree->Leftson());         // link left subtree
    buildSonEdges(tree->Rightson());            // link right subtree

    // to ensure the nodes contain the correct distance to the border
    // we MUST build all son edges before creating the root edge

    new AP_tree_edge(tree->Leftson(),tree->Rightson()); // link brothers
}

void AP_tree_edge::tailDistance(AP_tree_nlen *n)
{
    ap_assert(!n->is_leaf);    // otherwise call was not neccessary!

    int i0 = n->indexOf(this);          // index of this
    int i1 = i0==0 ? 1 : 0;             // the indices of the other two nodes (beside n)
    int i2 = i1==1 ? 2 : (i0==1 ? 2 : 1);
    AP_tree_edge *e1 = n->edge[i1];     // edges to the other two nodes
    AP_tree_edge *e2 = n->edge[i2];

    cout << "tail n=" << n << " d(n)=" << n->distance << " ";

    if (e1 && e2)
    {
        AP_tree_nlen *n1 = e1->node[1-n->index[i1]]; // the other two nodes
        AP_tree_nlen *n2 = e2->node[1-n->index[i2]];

        cout <<  "n1=" << n1 << " d(n1)=" << n1->distance <<
            " n2=" << n2 << " d(n2)=" << n2->distance << " ";

        if (n1->distance==n2->distance)
        {
            if (n1->distance>n->distance && n2->distance>n->distance)
            {
                // both distances (of n1 and n2) are greather that distance of n
                // so its possible that the nearest connection the border was
                // via node n and we must recalculate the distance into the other
                // directions

                ap_assert(n1->distance==n2->distance);

                cout << "special tailDistance-case\n";
                e1->tailDistance(n1);
                e2->tailDistance(n2);

                if (n1->distance<n2->distance)
                {
                    n->distance = n1->distance+1;
                    if (!e2->distanceOK()) e2->calcDistance();
                }
                else
                {
                    n->distance = n2->distance+1;
                    if (!e1->distanceOK()) e1->calcDistance();
                }
            }
            else
            {
                cout << "tail case 2\n";
                n->distance = n1->distance+1;
            }
        }
        else
        {
            ap_assert(n1->distance != n2->distance);

            if (n1->distance<n2->distance)
            {
                if (n1->distance<n->distance)
                {
                    // in this case the distance via n1 is the same as
                    // the distance via the cutted edge - so we do nothing

                    cout << "tail case 3\n";
                    ap_assert(n1->distance==(n->distance-1));
                }
                else
                {
                    cout << "tail case 4\n";
                    ap_assert(n1->distance==n->distance);
                    ap_assert(n2->distance==(n->distance+1));

                    n->distance = n1->distance+1;
                    e2->tailDistance(n2);
                }
            }
            else
            {
                ap_assert(n2->distance<n1->distance);

                if (n2->distance<n->distance)
                {
                    // in this case the distance via n2 is the same as
                    // the distance via the cutted edge - so we do nothing

                    cout << "tail case 5\n";
                    ap_assert(n2->distance==(n->distance-1));
                }
                else
                {
                    cout << "tail case 6\n";
                    ap_assert(n2->distance==n->distance);
                    ap_assert(n1->distance==(n->distance+1));

                    n->distance = n2->distance+1;
                    e1->tailDistance(n1);
                }
            }
        }

        //  cout << "e1=" << *e1 << endl;
        //  cout << "e2=" << *e2 << endl;

        cout << "d(n)=" << n->distance <<
            " d(n1)=" << n1->distance <<
            " d(n2)=" << n2->distance <<
            " D(e1)=" << e1->Distance() <<
            " D(e2)=" << e2->Distance() <<
            " dtb(e1)=" << e1->distanceToBorder(INT_MAX,n) <<
            " dtb(e2)=" << e2->distanceToBorder(INT_MAX,n) << endl;

        //  ap_assert(e1->Distance()==e1->distanceToBorder(INT_MAX,n));
        //  ap_assert(e2->Distance()==e2->distanceToBorder(INT_MAX,n));
    }
    else
    {
        if (e1)
        {
            AP_tree_nlen *n1 = e1->node[1-n->index[i1]];

            cout << "tail case 7\n";
            ap_assert(n1!=0);
            if (n1->distance>n->distance) e1->tailDistance(n1);
            n->distance = n1->distance+1;

            //      cout << "e1=" << *e1 << endl;

            cout << "d(n)=" << n->distance <<
                " d(n1)=" << n1->distance <<
                " D(e1)=" << e1->Distance() <<
                " dtb(e1)=" << e1->distanceToBorder(INT_MAX,n) << endl;


            //      ap_assert(e1->Distance()==e1->distanceToBorder(INT_MAX,n));
        }
        else if (e2)
        {
            AP_tree_nlen *n2 = e2->node[1-n->index[i2]];

            cout << "tail case 8\n";
            ap_assert(n2!=0);
            cout << "d(n2)=" << n2->distance << " d(n)=" << n->distance << endl;

            if (n2->distance>n->distance) e2->tailDistance(n2);
            n->distance = n2->distance+1;

            //      cout << "e2=" << *e2 << endl;

            cout << "d(n)=" << n->distance <<
                " d(n2)=" << n2->distance <<
                " D(e2)=" << e2->Distance() <<
                " dtb(e2)=" << e2->distanceToBorder(INT_MAX,n) << endl;

            //      ap_assert(e2->Distance()==e2->distanceToBorder(INT_MAX,n));
        }
        else
        {
            cout << "tail case 9\n";
            n->distance = INT_MAX;
        }
    }
}

AP_tree_edge* AP_tree_edge::unlink()
{
    ap_assert(this!=0);

    //    cout << "------ unlink" << endl;

    //    switch (node[0]->distance-node[1]->distance)
    //    {
    //     case -1: // n1->distance > n0->distance
    //  {
    //      ap_assert(!node[1]->is_leaf);
    //      tailDistance(node[1]);
    //      break;
    //  }
    //     case 0:  // equal -> distances remain correct
    //  {
    //      break;
    //  }
    //     case 1:  // n0->distance > n1->distance
    //  {
    //      ap_assert(!node[0]->is_leaf);
    //      tailDistance(node[0]);
    //      break;
    //  }
    //     default:
    //  {
    //      cerr << "error in distances" << endl;
    //  }
    //    }

    node[0]->edge[index[0]] = NULL;
    node[1]->edge[index[1]] = NULL;

    node[0] = NULL;
    node[1] = NULL;

    return this;
}

void AP_tree_edge::calcDistance()
{
    ap_assert(!distanceOK());  // otherwise call was not neccessary
    ap_assert (node[0]->distance!=node[1]->distance);

    if (node[0]->distance < node[1]->distance)  // node[1] has wrong distance
    {
        cout << "dist(" << node[1] << ") " << node[1]->distance;

        if (node[1]->distance==INT_MAX) // not initialized -> do not recurse
        {
            node[1]->distance = node[0]->distance+1;
            cout  << " -> " << node[1]->distance << endl;
        }
        else
        {
            node[1]->distance = node[0]->distance+1;
            cout  << " -> " << node[1]->distance << endl;

            if (!node[1]->is_leaf)
            {
                for (int e=0; e<3; e++)     // recursively correct distance where neccessary
                {
                    AP_tree_edge *ed = node[1]->edge[e];
                    if (ed && ed!=this && !ed->distanceOK()) ed->calcDistance();
                }
            }
        }
    }
    else                        // node[0] has wrong distance
    {
        cout << "dist(" << node[0] << ") " << node[0]->distance;

        if (node[0]->distance==INT_MAX) // not initialized -> do not recurse
        {
            node[0]->distance = node[1]->distance+1;
            cout  << " -> " << node[0]->distance << endl;
        }
        else
        {
            node[0]->distance = node[1]->distance+1;
            cout  << " -> " << node[0]->distance << endl;

            if (!node[0]->is_leaf)
            {
                for (int e=0; e<3; e++)     // recursively correct distance where neccessary
                {
                    AP_tree_edge *ed = node[0]->edge[e];
                    if (ed && ed!=this && !ed->distanceOK()) ed->calcDistance();
                }
            }
        }
    }

    ap_assert(distanceOK());   // invariant of AP_tree_edge (if fully constructed)
}

void AP_tree_edge::relink(AP_tree_nlen *node1,AP_tree_nlen *node2)
{
    //    cout << "------ relink" << endl;

    node[0] = node1;
    node[1] = node2;

    node1->edge[index[0] = node1->unusedEdge()] = this;
    node2->edge[index[1] = node2->unusedEdge()] = this;

    node1->index[index[0]] = 0;
    node2->index[index[1]] = 1;

    //    if (node1->is_leaf) node1->distance = 0;
    //    if (node2->is_leaf) node2->distance = 0;

    //    cout << "relink (dist = (" << node1->distance << ","
    //                     << node2->distance << "))" << endl;

    //    if (!distanceOK()) calcDistance();
}

/**************************
    test
**************************/

int AP_tree_edge::test() const
{
    int ok = 1;     // result is used by
    int n;

    for (n=0; n<2; n++)
    {
        if (node[n]->edge[index[n]]!=this)
        {
            int i;

            for (i=0; i<3; i++)
            {
                if (node[n]->edge[i]==this) break;
            }

            if (i==3)
            {
                cout << *this << " points to wrong node " << node[n]
                     << '\n';
                ok = 0;
            }
            else
            {
                cout << *this << " has wrong index ("
                     << index[n] << "instead of " << i << ")\n";
                ok = 0;
            }
        }
    }

    if (!distanceOK() ||
        (node[0]->is_leaf && node[0]->distance!=0) ||
        (node[1]->is_leaf && node[1]->distance!=0))
    {
        cout << "distance not set correctly at" << *this << '\n';
    }
    else if (Distance()!=distanceToBorder())
    {
        cout << "Distance() != distanceToBorder()" << endl;
    }

    return ok;
}

/**************************

Recursive methods:

    clearValues

**************************/

/*
int AP_tree_edge::clearValues(int deep, AP_tree_nlen *skip)
{
    int cnt = 1;

    used = 0;
    data.distance = 0;

    if (deep--)
    for (int n=0; n<2; n++)
        if (node[n]!=skip && !node[n]->is_leaf)
        for (int e=0; e<3; e++)
            if (node[n]->edge[e]!=this)
            cnt += node[n]->edge[e]->clearValues(deep,node[n]);

    return cnt;
}
*/

void AP_tree_edge::testChain(int deep)
{
    cout << "Building chain (deep=" << deep << ")\n";
    buildChain(deep,GB_FALSE);
    int inChain = dumpChain();
    cout << "Edges in Chain = " << inChain << '\n';
}

int AP_tree_edge::dumpChain(void) const
{
    //    cout << this << '\n';
    return next ? 1+next->dumpChain() : 1;
}

AP_tree_edge* AP_tree_edge::buildChain(int deep, GB_BOOL skip_hidden,
                                       int distanceToInsert,
                                       const AP_tree_nlen *skip,
                                       AP_tree_edge *comesFrom)
{
    AP_tree_edge *last = this;

    data.distance = distanceToInsert++;
    if (comesFrom) comesFrom->next = this;
    this->next = NULL;
    if (skip_hidden){
        if (node[0]->gr.hidden) return last;
        if (node[1]->gr.hidden) return last;
        if ( (!node[0]->gr.has_marked_children) && (!node[1]->gr.has_marked_children)) return last;
    }

    if (deep--)
        for (int n=0; n<2; n++)
            if (node[n]!=skip && !node[n]->is_leaf){
                for (int e=0; e<3; e++){
                    if (node[n]->edge[e]!=this){
                        last = node[n]->edge[e]->buildChain(deep,skip_hidden,distanceToInsert,node[n],last);
                    }
                }
            }

    return last;
}

long AP_tree_edge::sizeofChain(void){
    AP_tree_edge *f;
    long c= 0;
    for (f=this; f ; f = f->next) c++;
    return c;
}

int AP_tree_edge::distanceToBorder(int maxsearch,AP_tree_nlen *skipNode) const
    // return the minimal distance to the border of the tree
    // a return value of 0 means: one of the nodes is a leaf
{
    if ((node[0] && node[0]->is_leaf) || (node[1] && node[1]->is_leaf))
    {
        //  cout << "dtb(" << *this << ")=" << 0 << endl;
        return 0;
    }

    for (int n=0; n<2 && maxsearch; n++)
    {
        if (node[n] && node[n]!=skipNode)
        {
            for (int e=0; e<3; e++)
            {
                AP_tree_edge *ed = node[n]->edge[e];

                if (ed && ed!=this)
                {
                    int dist = ed->distanceToBorder(maxsearch-1, node[n])+1;
                    if (dist<maxsearch) maxsearch = dist;
                }
            }
        }
    }

    //    cout << "dtb(" << *this << ")=" << maxsearch+1 << endl;
    return maxsearch;
}

void AP_tree_edge::countSpecies(int deep,const AP_tree_nlen *skip)
{
    if (!skip)
    {
        speciesInTree = 0;
        nodesInTree   = 0;
    }

    if (deep--)
    {
        for (int n=0; n<2; n++)
        {
            if (node[n]->is_leaf)
            {
                speciesInTree++;
                nodesInTree++;
            }
            else if (node[n]!=skip)
            {
                nodesInTree++;

                for (int e=0; e<3; e++)
                {
                    if (node[n]->edge[e]!=this)
                    {
                        node[n]->edge[e]->countSpecies(deep,node[n]);
                    }
                }
            }
        }
    }
}

/**************************
    tree optimization
**************************/
void ap_init_bootstrap_remark(AP_tree_nlen *son_node){
    int seq_len = son_node->sequence->root->filter->real_len;
    AP_sequence::static_mutation_per_site[0] = (char *)GB_calloc(sizeof(char),seq_len);
    AP_sequence::static_mutation_per_site[1] = (char *)GB_calloc(sizeof(char),seq_len);
    AP_sequence::static_mutation_per_site[2] = (char *)GB_calloc(sizeof(char),seq_len);
}

double fak(int n,int m){
    double res = 1.0;
    while (n>m){
        res *= n;
        n--;
    }
    return res;
}


double ap_calc_bootstrap_remark_sub(int seq_len, char *old, char *ne){
    int i;
    int sum[3];
    sum[0] = 0;
    sum[1] = 0;
    sum[2] = 0;
    for (i=0;i<seq_len;i++){
        int diff = ne[i] - old[i];
        if (diff > 1 || diff < -1){
            GB_export_error("shit shit shit, diff by nni at one position not in [-1,1]: %i:%i - %i",diff,old[i],ne[i]);
            GB_print_error();
            continue;
        }
        sum[diff+1] ++;
    }
    {
        int msum = 0;
        for (i=0;i<seq_len;i++){
            msum += old[i];
            msum += ne[i];
        }
        msum /= 2;
    }
    double prob = 0;
    {
        int asum = 0;
        for (i=0;i<3;i++) asum += sum[i];
        double freq[3];
        double log_freq[3];
        for (i=0;i<3;i++){
            freq[i] = sum[i] / double(asum); // relative frequencies of -1, 0, 1
            if (sum[i] >0){
                log_freq[i] = log(freq[i]);
            }else{
                log_freq[i] = -1e100; // minus infinit
            }
        }

        int max = seq_len;  // bootstrap can select seq_len ones maximum
        double log_fak_seq_len = GB_log_fak(seq_len);
        double log_eps = log(1e-11);

        //printf("********* %i %i %i     max %i\n",sum[0],sum[1],sum[2],max);
        // loop over all delta_mutations, begin in the middle
        for ( int tsum_add = 1; tsum_add >= -1; tsum_add -= 2){
            int tsum = sum[2]-sum[0];
            if (tsum <=0) tsum = 1;
            for (; tsum < max && tsum > 0; tsum += tsum_add){       // sum of mutations in bootstrap sample, loop over all possibilities
                if (tsum_add < 0 && tsum == sum[2]-sum[0]) continue; // don't double count tsum



                int max_minus = max;        // we need tsum + n_minus ones, we cannot have more than max_minux minus, reduce also
                for (int minus_add = 1; minus_add>=-1;minus_add-=2){
                    int first_minus = 1;
                    for (int n_minus = sum[0]; n_minus<max_minus && n_minus>=0; first_minus = 0, n_minus+=minus_add){
                        if (minus_add < 0 && first_minus) continue; // dont double count center
                        int n_zeros = seq_len - n_minus * 2 - tsum; // number of minus
                        int n_plus = tsum + n_minus; // number of plus ones  (n_ones + n_minus + n_zeros = seq_len)

                        double log_a =
                            n_minus * log_freq[0] +
                            n_zeros * log_freq[1] +
                            n_plus  * log_freq[2] +
                            log_fak_seq_len - GB_log_fak(n_minus) - GB_log_fak(n_zeros) - GB_log_fak(n_plus);

                        if (log_a < log_eps){
                            if (first_minus && minus_add>0) goto end;
                            break; // cannot go with so many minus, try next
                        }
                        double a = exp(log_a);
                        //printf("\t\t%i %i %6f %e\n",tsum,n_minus,log_a,a);
                        prob += a;
                    }
                }
            }
        end:;
        }
    }
    //printf("&&&&&&&& result %e\n",prob);
    return prob;
}

void ap_calc_bootstrap_remark(AP_tree_nlen *son_node,AP_BL_MODE mode){
    if (!son_node->is_leaf){
        int seq_len = son_node->sequence->root->filter->real_len;
        float one = ap_calc_bootstrap_remark_sub(seq_len,
                                                 &AP_sequence::static_mutation_per_site[0][0],
                                                 &AP_sequence::static_mutation_per_site[1][0]);
        float two = ap_calc_bootstrap_remark_sub(seq_len,
                                                 &AP_sequence::static_mutation_per_site[0][0],
                                                 &AP_sequence::static_mutation_per_site[2][0]);
        if ((mode & AP_BL_BOOTSTRAP_ESTIMATE) == AP_BL_BOOTSTRAP_ESTIMATE){
            one = one * two;    // assume independent bootstrap values for both nnis
        }else{
            if (two<one) one = two; // dependent bootstrap values, take minimum (safe)
        }
        const char *text = 0;
        if (one < 1.0){ // was: < 0.99
            text = GBS_global_string("%i%%",int(100.0 * one + 0.5));
        }
        else {
            text = "100%";
        }
        delete son_node->remark_branch;
        delete son_node->Brother()->remark_branch;
        son_node->remark_branch = GB_strdup(text);
        son_node->Brother()->remark_branch = GB_strdup(text);
    }
    delete AP_sequence::static_mutation_per_site[0];
    delete AP_sequence::static_mutation_per_site[1];
    delete AP_sequence::static_mutation_per_site[2];

    AP_sequence::static_mutation_per_site[0] = 0;
    AP_sequence::static_mutation_per_site[1] = 0;
    AP_sequence::static_mutation_per_site[2] = 0;

}


void ap_calc_leaf_branch_length(AP_tree_nlen *leaf){
    AP_FLOAT Seq_len = leaf->sequence->real_len();
    if (Seq_len <=1.0) Seq_len = 1.0;

    AP_FLOAT parsbest = rootNode()->costs();
    ap_main->push();
    leaf->remove();
    AP_FLOAT Blen     = parsbest - rootNode()->costs();
    ap_main->pop();
    double   blen     = Blen/Seq_len;

#if defined(DEBUG)
    // printf("Blen=%f name=%s\n", Blen, leaf->name);
#endif // DEBUG


    if (!leaf->father->father){ // at root
        leaf->father->leftlen = blen*.5;
        leaf->father->rightlen = blen*.5;
    }else{
        if (leaf->father->leftson == leaf){
            leaf->father->leftlen = blen;
        }else{
            leaf->father->rightlen = blen;
        }
    }
}




void ap_calc_branch_lengths(AP_tree_nlen */*root*/, AP_tree_nlen *son, double /*parsbest*/, double blen){
    static double s_new = 0.0;
    static double s_old = 0.0;

    AP_FLOAT seq_len = son->sequence->real_len();
    if (seq_len <=1.0) seq_len = 1.0;
    blen *= 0.5 / seq_len * 2.0;        // doubled counted sum * corr

    AP_tree_nlen *fathr = son->Father();
    double old_len = 0.0;
    if (!fathr->father){    // at root
        old_len = fathr->leftlen + fathr->rightlen;
        fathr->leftlen = blen *.5;
        fathr->rightlen = blen *.5;
    }else{
        if (fathr->leftson == son){
            old_len = fathr->leftlen;
            fathr->leftlen = blen;
        }else{
            old_len = fathr->rightlen;
            fathr->rightlen = blen;
        }
    }
    if (old_len< 1e-5) old_len = 1e-5;
    s_new += blen;
    s_old += old_len;
    //printf("slen %5i Old\t%f, new\t%f, :\t%f\t%f\n",int(seq_len),old_len,blen,blen/old_len,s_new/s_old);

    if (son->leftson->is_leaf){
        ap_calc_leaf_branch_length((AP_tree_nlen*)son->leftson);
    }

    if (son->rightson->is_leaf){
        ap_calc_leaf_branch_length((AP_tree_nlen*)son->rightson);
    }
}
const double ap_undef_bl = 10.5;

void ap_check_leaf_bl(AP_tree_nlen *node){
    if (node->is_leaf){
        if (!node->father->father){
            if (node->father->leftlen + node->father->rightlen == ap_undef_bl){
                ap_calc_leaf_branch_length(node);
            }
        }else if (node->father->leftson == node){
            if (node->father->leftlen == ap_undef_bl){
                ap_calc_leaf_branch_length(node);
            }
        }else{
            if (node->father->rightlen == ap_undef_bl){
                ap_calc_leaf_branch_length(node);
            }
        }
        return;
    }else{
        if (node->leftlen == ap_undef_bl)   ap_calc_leaf_branch_length((AP_tree_nlen *)node->leftson);
        if (node->rightlen== ap_undef_bl)   ap_calc_leaf_branch_length((AP_tree_nlen *)node->rightson);
    }
}

AP_FLOAT AP_tree_edge::nni_rek(AP_BOOL openclosestatus, int &Abort, int deep, GB_BOOL skip_hidden,
                               AP_BL_MODE mode, AP_tree_nlen *skipNode)
{
    if (!rootNode())        return 0.0;
    if (rootNode()->is_leaf)    return rootNode()->costs();

    AP_tree_edge *oldRootEdge = rootEdge();

    if (openclosestatus) aw_openstatus("Calculating NNI");
    if (deep>=0) set_root();

    AP_FLOAT old_parsimony = rootNode()->costs();
    AP_FLOAT new_parsimony = old_parsimony;

    buildChain(deep,skip_hidden,0,skipNode);
    long cs = sizeofChain();
    AP_tree_edge *follow;
    long count = 0;
    { // set all branch lengths to undef
        for (follow = this;follow; follow = follow->next){
            follow->node[0]->leftlen = ap_undef_bl;
            follow->node[0]->rightlen = ap_undef_bl;
            follow->node[1]->leftlen = ap_undef_bl;
            follow->node[1]->rightlen = ap_undef_bl;
            follow->node[0]->father->leftlen = ap_undef_bl;
            follow->node[0]->father->rightlen = ap_undef_bl;
        }
        rootNode()->leftlen = ap_undef_bl *.5;
        rootNode()->rightlen = ap_undef_bl *.5;
    }
    for (follow = this;follow; follow = follow->next){
        if (aw_status(count++/(double)cs)) { Abort = 1; break;}
        AP_tree_nlen *son = follow->sonNode();
        AP_tree_nlen *fath = son;

        if (follow->otherNode(fath)==fath->Father()) fath = fath->Father();
        if (fath->father){
            if (fath->father->father){
                fath->set_root();
                new_parsimony = rootNode()->costs();
            }
        }
        if (mode & AP_BL_BOOTSTRAP_LIMIT){
            if (fath->father){
                son->set_root();
                new_parsimony = rootNode()->costs();
            }
            ap_init_bootstrap_remark(son);
        }
        new_parsimony = follow->nni(new_parsimony, mode);
        if (mode & AP_BL_BOOTSTRAP_LIMIT){
            ap_calc_bootstrap_remark(son,mode);
        }
    }
    for (follow = this;follow; follow = follow->next){
        ap_check_leaf_bl(follow->node[0]);
        ap_check_leaf_bl(follow->node[1]);
    }
    oldRootEdge->set_root();
    new_parsimony = rootNode()->costs();
    if (openclosestatus) aw_closestatus();

    return new_parsimony;
}

int AP_tree_edge::dumpNNI = 0;
int AP_tree_edge::distInsertBorder;
int AP_tree_edge::basesChanged;
int AP_tree_edge::speciesInTree;
int AP_tree_edge::nodesInTree;

AP_FLOAT AP_tree_edge::nni(AP_FLOAT pars_one, AP_BL_MODE mode)
{
    AP_tree_nlen *root = (AP_tree_nlen *) (*ap_main->tree_root);

    if (node[0]->is_leaf || node[1]->is_leaf) { // a son at root
#if 0
        // calculate branch lengths at root
        if (mode&AP_BL_BL_ONLY){
            AP_tree_nlen *tip,*brother;

            if (node[0]->is_leaf){
                tip = node[0]; brother = node[1];
            }else{
                tip = node[1]; brother = node[0];
            }

            AP_FLOAT    Blen = pars_one - brother->costs();
            AP_FLOAT    Seq_len = tip->sequence->real_len();

            node[0]->father->leftlen = node[0]->father->rightlen = Blen/Seq_len*.5;
        }
#endif
        return pars_one;
    }

    AP_tree_edge_data oldData = data;

    AP_FLOAT    parsbest = pars_one,
        pars_two,
        pars_three;
    AP_tree_nlen *son = sonNode();
    int     betterValueFound = 0;
    {               // ******** original tree
        if ( (mode & AP_BL_BOOTSTRAP_LIMIT)){
            root->costs();
            char *ms = AP_sequence::static_mutation_per_site[0];
            AP_sequence::mutation_per_site = ms;
            son->unhash_sequence();
            son->father->unhash_sequence();
            ap_assert(!son->father->father);
            AP_tree_nlen *brother = son->Brother();
            brother->unhash_sequence();
            pars_one = 0.0;
        }
        if (pars_one==0.0) pars_one = root->costs();
    }
    {               // ********* first nni
        ap_main->push();
        son->swap_assymetric(AP_LEFT);
        char *ms = AP_sequence::static_mutation_per_site[1];
        AP_sequence::mutation_per_site = ms;
        pars_two = root->costs();

        if (pars_two <= parsbest){
            if ((mode & AP_BL_NNI_ONLY)==0) ap_main->pop();
            else                ap_main->clear();
            parsbest = pars_two;
            betterValueFound = (int)(pars_one-pars_two);
        }else{
            ap_main->pop();
        }
    }
    {               // ********** second nni
        ap_main->push();
        son->swap_assymetric(AP_RIGHT);
        char *ms = AP_sequence::static_mutation_per_site[2];
        AP_sequence::mutation_per_site = ms;
        pars_three = root->costs();

        if (pars_three <= parsbest){
            if ((mode & AP_BL_NNI_ONLY)==0) ap_main->pop();
            else                ap_main->clear();
            parsbest = pars_three;
            betterValueFound = (int)(pars_one-pars_three);
        }else{
            ap_main->pop();
        }
        AP_sequence::mutation_per_site = 0;
    }

    if (mode & AP_BL_BL_ONLY){  // ************* calculate branch lengths **************
        AP_FLOAT blen = (pars_one + pars_two + pars_three) - (3.0 * parsbest) ;
        if (blen <0) blen = 0;
        ap_calc_branch_lengths(root,son,parsbest, blen);
    }

    // zu Auswertungszwecken doch unsortiert uebernehmen:

    data.parsValue[0] = pars_one;
    data.parsValue[1] = pars_two;
    data.parsValue[2] = pars_three;


    if (dumpNNI)
    {
        if (dumpNNI==2)
        {
            fprintf(stderr,"Fatal! NNI called between optimize and statistics!\n");
            exit(1);
        }
        AP_tree_nlen *node0 = this->node[0];
        AP_tree_nlen *node1 = this->node[1];
        if (node0->gr.leave_sum > node1->gr.leave_sum){ //node0 is final son
            node0 = node1;
        }
        static int num = 0;
        delete node0->remark_branch;
        node0->remark_branch = (char *)calloc(sizeof(char),100);
        sprintf(node0->remark_branch, "%i %4.0f:%4.0f:%4.0f     %4.0f:%4.0f:%4.0f\n",num++,
                oldData.parsValue[0],oldData.parsValue[1],oldData.parsValue[2],
                data.parsValue[0],data.parsValue[1],data.parsValue[2]);


        cout
            << setw(4) << distInsertBorder
            << setw(6) << basesChanged
            << setw(4) << distanceToBorder()
            << setw(4) << data.distance
            << setw(4) << betterValueFound
            << setw(8) << oldData.parsValue[0]
            << setw(8) << oldData.parsValue[1]
            << setw(8) << oldData.parsValue[2]
            << setw(8) << data.parsValue[0]
            << setw(8) << data.parsValue[1]
            << setw(8) << data.parsValue[2]
            << '\n';
    }

    return parsbest;
}

/**************************
    operator <<
**************************/

ostream& operator<<(ostream& out, const AP_tree_edge& e)
{
    static int notTooDeep;

    out << ' ';

    if (notTooDeep || &e==NULL)
    {
        out << e;
    }
    else
    {
        notTooDeep = 1;
        out << "AP_tree_edge(" << e
            << ", node[0]=" << *(e.node[0])
            << ", node[1]=" << *(e.node[1])
            << ")";
        notTooDeep = 0;
    }

    return out << ' ';
}

void AP_tree_edge::mixTree(int cnt)
{
    buildChain(-1);

    while (cnt--)
    {
        AP_tree_edge *follow = this;

        while (follow)
        {
            if (rand()%2)   follow->sonNode()->swap_assymetric(AP_LEFT);
            else        follow->sonNode()->swap_assymetric(AP_RIGHT);

            follow = follow->next;
        }
    }
}
