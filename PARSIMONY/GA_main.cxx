#include <cstdlib>
#include <arbdb.h>
#include <arbdbt.h>
#include <cstring>
#include <cstdio>
#include <memory.h>
#include <iostream.h>
#include "AP_buffer.hxx"
#include "ap_main.hxx"
#include "ap_tree_nlen.hxx"
#include "GA_genetic.hxx"

void tree_init(AP_tree *tree0);
GA_genetic * GAgenetic;
void parsimony_func(AP_tree *);

void  buildRandomTreeRek(AP_tree *tree, AP_tree **list, int *num) {
    // builds a list of all species
    if (tree->is_leaf) {
        AP_tree_nlen *pntr = new AP_tree_nlen;
        pntr->copy((AP_tree_nlen*)tree);
        pntr->father = 0;
        list[*num] = (AP_tree *)pntr;
        (*num)++;
        return;
    }
    buildRandomTreeRek(tree->leftson, list, num);
    buildRandomTreeRek(tree->rightson, list, num);
    return;
}


AP_tree * buildRandomTree(AP_tree *root) {
    // function returns a random constructed tree
    // root is tree with species (needed to build a list of species)
    AP_tree **list;

    if (root->sequence_proto == 0) tree_init(root);

    AP_tree_nlen *ntree;
    AP_tree *tree1, *tree0;
    int num;
    int count = 0;

    root->arb_tree_leafsum();

    list = (AP_tree **)calloc(root->gr.leave_sum + 1, sizeof(AP_tree *));

    buildRandomTreeRek(root, list, &count);
    count--;
    while (count >1) {
        // choose two random numbers
        num = (int)random()%(count+1);
        tree0 = list[num];
        list[num] = list[count];
        count --;
        num = (int)random()%(count+1);
        tree1 = list[num];
        ntree = new AP_tree_nlen;
        ntree->leftson = tree0;
        ntree->rightson = tree1;
        ntree->sequence = 0;
        tree0->father = ntree;
        tree1->father = ntree;
        ntree->is_leaf = false;
        // ################## Laengenberechnung #################3
        ntree->leftlen = .5;
        ntree->rightlen = .5;
        list[num] = (AP_tree *)ntree;
    }
    tree0 = list[0];
    free(list);
    return tree0;
}

void kernighan_lin(AP_tree_nlen *tree) {
    if (tree == 0) new AP_ERR("kernighan_lin", "No tree !");
    // ruft kernighan auf
}

AP_tree_nlen *crossover(AP_tree_nlen *tree0, AP_tree_nlen *tree1) {
    int size1, size0;
    AP_CO_LIST *list0, *list1;

    if (tree0 == 0 || tree1 == 0) {
        new AP_ERR("crossover", "Needs two tress as argument");
        return 0;
    }
    list0 = tree0->createList(&size0);
    list1 = tree1->createList(&size1);

    fprintf(GAgenetic->fout, "\ncrossover tree %d %d size %d %d",
            tree0, tree1, size0, size1);

    // ruft crossover auf
    return tree0;
}

int randomCluster() {
    int maxcluster = GAgenetic->getMaxCluster();
    int cluster;
    cluster = (int)random()%maxcluster;
    cout << cluster << "clust\n";
    return cluster;
}
AP_ERR * make_start_population(GBDATA *gbmain, AP_tree *tree) {
    // makes random start population
    // (at least two trees in each cluster)
    static int msp = 0;
    msp ++;
    if (msp > 1) return new AP_ERR("make_start_population", "Only call it once !");
    int name=0, i = 0, maxcluster;

    AP_tree_nlen* rtree;

    if (GAgenetic == 0) {
        GAgenetic = new GA_genetic;
        GAgenetic->init(gbmain);
    }

    maxcluster = GAgenetic->getMaxCluster();
    while (i<maxcluster) {
        rtree = (AP_tree_nlen *)buildRandomTree(tree);
        rtree->parsimony_rek();
        GAgenetic->put_start_tree((AP_tree *)rtree, name, i);
        name ++;
        fprintf(GAgenetic->fout, "\ncluster %d put Starttree %d", i, name-1);
        rtree = (AP_tree_nlen *)buildRandomTree(tree);
        rtree->parsimony_rek();
        GAgenetic->put_start_tree((AP_tree *)rtree, name, i);
        name ++;
        fprintf(GAgenetic->fout, "\nCluster %d put Starttree %d", i, name-1);
        i ++;
    }
    return 0;
}

void quit_genetic() {
    fclose(GAgenetic->fout);
}

void start_genetic(GBDATA *gbmain) {
    //
    // the genetic algorithm is implemented here
    //

    GA_tree * starttree;
    GA_job *job;
    int cluster;


    if (GAgenetic == 0) {
        GAgenetic = new GA_genetic;
        GAgenetic->init(gbmain);
    }
    fprintf(GAgenetic->fout, "\n**** Genetic ALGORITHM *****\n");
    make_start_population(gbmain, ap_main->tree_root);

    //
    // get starttree and optimize it
    //

    int i = 0;

    while (i<GAgenetic->getMaxCluster()) {
        cluster = i;
        while ((starttree = GAgenetic->get_start_tree(cluster)) != 0) {
            if (starttree != 0) {
                kernighan_lin(starttree->tree);
                GAgenetic->put_optimized(starttree, cluster);
                fprintf(GAgenetic->fout, "\nStarttree %d optimized in cluster %d",
                        starttree->id,
                        cluster);
                delete starttree;
            }
            else {
                fprintf(GAgenetic->fout, "\nNo starttree found in cluster %d", cluster);
            }
        }
        i ++;
    }

    //
    // get job and do it
    //
    i = 0;
    while (i++ <20) {
        cluster = randomCluster();
        job = GAgenetic->get_job(cluster);
        if (job != 0) {
            switch (job->mode) {
                case GA_CROSSOVER: {
                    GA_tree * gaTree = new GA_tree;
                    gaTree->tree = crossover(job->tree0->tree, job->tree1->tree);

                    GB_push_transaction(gb_main);
                    char *use = GBT_get_default_alignment(gb_main);
                    gaTree->tree->load_sequences_rek(0, use);
                    GB_pop_transaction(gb_main);

                    parsimony_func(gaTree->tree);

                    gaTree->criteria = gaTree->tree->mutation_rate;
                    gaTree->id = -1;
                    GAgenetic->put_optimized(gaTree, job->cluster0);
                    delete gaTree;
                    delete use;
                    break; }
                case GA_KERNIGHAN:
                    kernighan_lin(job->tree0->tree);
                    break;
                case GA_NNI:
                    break;
                case GA_CREATEJOBS:
                    break;
                case GA_NONE:
                default:
                    break;
            }
            fprintf(GAgenetic->fout, "\njob %d in cluster %d : %d executed, mode %d"
                    , job, job->cluster0, job->cluster1, job->mode);
            GAgenetic->put_optimized(job->tree0, cluster);
        }
        else {
            fprintf(GAgenetic->fout, "\nno job found");
        }
    }
}
