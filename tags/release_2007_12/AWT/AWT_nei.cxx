#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <arbdb.h>
#include <arbdbt.h>
#include "awt_tree.hxx"
#include "awt_nei.hxx"

#define CHECK_NAN(x) if ( (!(x>=0.0)) && (!(x<0.0))  ) *(int *)0=0;

PH_NEIGHBOUR_DIST::PH_NEIGHBOUR_DIST(void)
{
    memset((char *)this,0,sizeof(PH_NEIGHBOUR_DIST));
}


AP_FLOAT PH_NEIGHBOURJOINING::get_max_di(AP_FLOAT **m)  // O(n*2)
{
    long i,j;
    AP_FLOAT max = 0.0;
    for (i=0;i<size; i++){
        for (j=0;j<i;j++){
            if (m[i][j] > max) max = m[i][j];
        }
    }
    return max;
}

void PH_NEIGHBOURJOINING::remove_taxa_from_dist_list(long i)    // O(n/2)
{
    long a,j;
    register PH_NEIGHBOUR_DIST *nd;
    for (a=0;a<swap_size;a++) {
        j= swap_tab[a];
        if (i==j) continue;
        if (j<i) {
            nd = &(dist_matrix[i][j]);
        }else{
            nd = &(dist_matrix[j][i]);
        }
        nd->remove();
        net_divergence[j] -= nd->val;   // corr net divergence
    }
}
void PH_NEIGHBOURJOINING::add_taxa_to_dist_list(long i) // O(n/2)
{
    long a;
    register long pos,j;
    register PH_NEIGHBOUR_DIST *nd;
    AP_FLOAT my_nd = 0.0;
    for (a=0;a<swap_size;a++) {
        j= swap_tab[a];
        if (i==j) continue;
        if (j<i) {
            nd = &(dist_matrix[i][j]);
        }else{
            nd = &(dist_matrix[j][i]);
        }
        if (nd->previous) GB_CORE;
        pos = (int)(nd->val*dist_list_corr);
        if (pos>= dist_list_size) {
            pos = dist_list_size-1;
        }else if (pos<0)
            pos = 0;
        nd->add(&(dist_list[pos]));

        net_divergence[j] += nd->val;   // corr net divergence
        my_nd += nd->val;
    }
    net_divergence[i] = my_nd;
}

AP_FLOAT PH_NEIGHBOURJOINING::get_max_net_divergence(void)  // O(n/2)
{
    long a,i;
    AP_FLOAT max = 0.0;
    for (a=0;a<swap_size;a++){
        i = swap_tab[a];
        if (net_divergence[i] > max) max = net_divergence[i];
    }
    return max;
}

void PH_NEIGHBOURJOINING::remove_taxa_from_swap_tab(long i) // O(n/2)
{
    long a;
    long *source,*dest;
    source = dest = swap_tab;
    for (a=0;a<swap_size;a++){
        if (swap_tab[a] == i){
            source++;
        }else{
            *(dest++) = *(source++);
        }
    }
    swap_size --;
}

PH_NEIGHBOURJOINING::PH_NEIGHBOURJOINING(AP_FLOAT **m, long isize)
{
    long i,j;

    size = isize;
    swap_size = size;       // init swap tab
    swap_tab = new long[size];
    for (i=0;i<swap_size;i++) swap_tab[i] = i;

    net_divergence = (AP_FLOAT *)calloc(sizeof(AP_FLOAT),(size_t)size);


    dist_list_size = size;      // hope te be the best
    dist_list = new PH_NEIGHBOUR_DIST[dist_list_size];// the roots, no elems
    dist_list_corr = (dist_list_size-2.0)/get_max_di(m);

    dist_matrix = new PH_NEIGHBOUR_DIST*[size];
    for (i=0;i<size;i++) {
        dist_matrix[i] = new PH_NEIGHBOUR_DIST[i];
        for (j=0;j<i;j++){
            dist_matrix[i][j].val = m[i][j];
            dist_matrix[i][j].i = i;
            dist_matrix[i][j].j = j;
        }
    }
    for (i=0;i<size;i++){
        swap_size = i;      // to calculate the correct net divergence
        add_taxa_to_dist_list(i);   // add to dist list and add n.d.
    }
    swap_size = size;
}

PH_NEIGHBOURJOINING::~PH_NEIGHBOURJOINING(void)
{
//     long i;
//     for (i=0;i<size;i++) {
//         free(dist_matrix[i]);
//         dist_matrix[i] = 0;
//     }
    delete [] dist_matrix;

//     free((char *)dist_list);
    delete [] dist_list;
    free((char *)net_divergence);
    delete [] swap_tab;
}

void PH_NEIGHBOURJOINING::get_min_ij(long& mini, long& minj)    // O(n*n/speedup)
{
    AP_FLOAT maxri = get_max_net_divergence();  // O(n/2)
    register PH_NEIGHBOUR_DIST *dl;
    register long stat = 0;
    register AP_FLOAT x;
    AP_FLOAT minval;
    minval = 100000.0;
    AP_FLOAT N_1 = 1.0/(swap_size-2.0);
    maxri = maxri*2.0*N_1;
    long pos;
    get_last_ij(mini,minj);

    for (pos=0;pos<dist_list_size;pos++){
        if (minval < pos/dist_list_corr - maxri) break;
        // no way to get a better minimum
        dl = dist_list[pos].next;   // first entry does not contain information
        for (;dl;dl=dl->next){
            x = (net_divergence[dl->i] + net_divergence[dl->j])*N_1;
            if (dl->val-x<minval) {
                minval = dl->val -x;
                minj = dl->i;
                mini = dl->j;
            }
            stat++;
        }
    }


    //printf("stat %li of %li   mini %li minj %li\n",
    //  stat,swap_size*(swap_size-1)/2,mini,minj);
}

void PH_NEIGHBOURJOINING::join_nodes(long i,long j,AP_FLOAT &leftl,AP_FLOAT& rightl)
{
    register PH_NEIGHBOUR_DIST **d = dist_matrix;
    register AP_FLOAT dji;

    AP_FLOAT dist = get_dist(i,j);

    leftl = dist*.5 + (net_divergence[i] - net_divergence[j])*.5/
        (swap_size - 2.0);
    rightl = dist - leftl;

    remove_taxa_from_dist_list(j);
    remove_taxa_from_swap_tab(j);
    remove_taxa_from_dist_list(i);

    long a,k;
    dji = d[j][i].val;
    for (a=0;a<swap_size;a++) {
        k = swap_tab[a];
        if (k==i) continue; // k == j not possible
        if (k>i) {
            if (k>j) {
                d[k][i].val = .5*(d[k][i].val + d[k][j].val - dji);
            }else{
                d[k][i].val = .5*(d[k][i].val + d[j][k].val - dji);
            }
        }else{
            d[i][k].val = 0.5 * (d[i][k].val + d[j][k].val - dji);

        }
    }
    add_taxa_to_dist_list(i);
}

void PH_NEIGHBOURJOINING::get_last_ij(long& i, long& j)
{
    i = swap_tab[0];
    j = swap_tab[1];
}

AP_FLOAT PH_NEIGHBOURJOINING::get_dist(long i, long j)
{
    return dist_matrix[j][i].val;
}

GBT_TREE *neighbourjoining(char **names, AP_FLOAT **m, long size, size_t structure_size)
{
    // structure_size >= sizeof(GBT_TREE);
    // lower triangular matrix
    // size: size of matrix


    PH_NEIGHBOURJOINING *nj = new PH_NEIGHBOURJOINING(m,size);
    long i;
    long a,b;
    GBT_TREE **nodes;
    AP_FLOAT ll,rl;
    nodes = (GBT_TREE **)calloc(sizeof(GBT_TREE *),(size_t)size);
    for (i=0;i<size;i++) {
        nodes[i] = (GBT_TREE *)calloc(structure_size,1);
        nodes[i]->name = strdup(names[i]);
        nodes[i]->is_leaf = AP_TRUE;
    }

    for (i=0;i<size-2;i++) {
        nj->get_min_ij(a,b);
        nj->join_nodes(a,b,ll,rl);
        GBT_TREE *father = (GBT_TREE *)calloc(structure_size,1);
        father->leftson = nodes[a];
        father->rightson = nodes[b];
        father->leftlen = ll;
        father->rightlen = rl;
        nodes[a]->father = father;
        nodes[b]->father = father;
        nodes[a] = father;
    }
    nj->get_last_ij(a,b);
    AP_FLOAT dist = nj->get_dist(a,b);
    ll = dist*0.5;
    rl = dist*0.5;

    GBT_TREE *father = (GBT_TREE *)calloc(structure_size,1);
    father->leftson = nodes[a];
    father->rightson = nodes[b];
    father->leftlen = ll;
    father->rightlen = rl;
    nodes[a]->father = father;
    nodes[b]->father = father;

    delete nj;
    free((char*)nodes);
    return father;
}
