/*
 * Analyser.cpp
 *
 * Responsible for interacting with the Cma code and Arb.
 *
 *  Created on: Feb 15, 2010
 *      Author: breno
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */

#include "Analyser.h"
#include <time.h>
#include <iostream>
#include <iterator>
#include "dbconn.h"

/**
 * Constructor
 */
Analyser::Analyser() {

    //Definition of the alphabet
    vector<string> alphabet(0);
    alphabet.push_back("A");
    alphabet.push_back("C");
    alphabet.push_back("G");
    alphabet.push_back("T");

    loader = new AlignedSequenceLoader;
    VecVecType *seqs = loader->getSequences();

    cma = new Cma(alphabet, seqs->size());
}

/**
 * Destructor.
 */
Analyser::~Analyser() {
    delete loader;
    delete cma;
}

/**
 * Getter for the Cma object.
 *
 * @return the Cma object.
 */
Cma* Analyser::getCma() {
    return cma;
}

/**
 * Returns the AlignedSequenceLoader object.
 *
 * @return the AlignedSequenceLoader object.
 */
AlignedSequenceLoader* Analyser::getLoader() {
    return loader;
}

/**
 * Saves the clusters to the DB as SAI.
 *
 * @param clusters: the cluster indices.
 * @param threshold: the clustering threshold used.
 *
 * @return an GB_ERROR if some DB transaction fails.
 */
GB_ERROR Analyser::saveSAI(vector<size_t> clusters, double threshold) {

    GBDATA   *gb_main = runningDatabase();
    GB_ERROR  error   = GB_push_transaction(gb_main);
    if (error) {
        cout << "ERROR 1" << endl;
    }

    char *al_name = GBT_get_default_alignment(gb_main);

    vector<char> clusts = normalizeClusters(clusters);
    // build result string
    stringstream ss;
    copy(clusts.begin(), clusts.end(), ostream_iterator<char> (ss, ""));
    string result = ss.str();

    //save
    GBDATA *gb_sai = GBT_find_or_create_SAI(gb_main, getSAIname());
    GBDATA *gb_data = GBT_add_data(gb_sai, al_name, "data", GB_STRING);
    error = GB_write_string(gb_data, result.c_str());
    if (error) {
        cout << "RNACMA-Error: " << error << "\n";
        exit(EXIT_FAILURE);
    }

    GBDATA *gb_options = GBT_add_data(gb_sai, al_name, "_TYPE", GB_STRING);
    stringstream options;
    options << "CMA_CLUSTERING: [threshold: " << threshold << "]";
    error = GB_write_string(gb_options, options.str().c_str());

    GB_commit_transaction(gb_main);

    return error;
}

/**
 * Gives clusters a reasonable name. The clustering algorithm may return
 * clusters with indices 123 even though there are only 50 clusters.
 * Here we normalise the cluster names, in this example we would have
 * only clusters from 0..50 as result.
 *
 * @param clusters: the result of the clustering algorithm.
 *
 * @return the new cluster names (note that the cluster index is a char
 *         because we have to be able to show it in the SAI, where we
 *         are allowed to use only one character for each position).
 */
vector<char> Analyser::normalizeClusters(vector<size_t> clusters) {

    vector<char> result;
    map<unsigned int, char> rename_map;
    rename_map[0] = '-';
    char classes = '0';

    for (vector<size_t>::iterator it = clusters.begin(); it != clusters.end(); ++it) {
        if (rename_map.find(*it) == rename_map.end()) {
            rename_map[*it] = classes++;
        }
        result.push_back(rename_map[*it]);
    }
    return result;

}

//--------------------------------

int main(void) {
    cout
        << "arb_rnacma -- correlated mutation analysis" << endl
        << "              computes clusters of correlated positions" << endl
        << "(C) 2010 Lehrstuhl fuer Mikrobiologie, TU Muenchen" << endl
        << "Written 2009/2010 by Breno Faria" << endl
        << endl
        << "arb_rnacma uses the eigen C++ library (http://eigen.tuxfamily.org/)" << endl
        << "eigen is copyrighted by LGPL3" << endl
        << endl;

    Analyser *a   = new Analyser;
    Cma      *cma = a->getCma();

    cma->computeMutualInformationP(*(a->getLoader()->getSequences()));

    list<MITuple> mituple = cma->compute_mituples(cma->getMIp());

    cout << endl
         << "The highest MI value was: " << mituple.begin()->MI
         << " at position (" << mituple.begin()->pos1 << ", "
         << mituple.begin()->pos2 << ")." << endl
         << "(Note: pairs with MI-values down to the threshold will be joined in one cluster)" << endl;

    while (true) {
        cout << endl
             << "Press Ctrl-d to quit or" << endl
             << "choose a threshold value for the clustering process: ";


        string input;
        cin >> input;

        if (input.empty()) {
            cout << endl << "quit" << endl;
            break;
        }

        double threshold = strtod(input.c_str(), NULL);
        cout << "Building clusters with threshold = " << threshold << endl;

        VectorXi cl = cma->computeClusters(mituple, size_t(cma->getMIp().cols()), threshold);
        vector<size_t> *clusters = new vector<size_t> (0, 0);
        AlignedSequenceLoader *l = a->getLoader();
        size_t i = 0;
        size_t j = 0;
        for (vector<size_t>::iterator it = l->getPositionMap()->begin(); it
                 != l->getPositionMap()->end(); ++it) {
            while (i < *it) {
                clusters->push_back(0);
                i++;
            }
            clusters->push_back(cl[j]);
            j++;
            i++;
        }

        GB_ERROR e = a->saveSAI(*clusters, threshold);

        if (e) {
            cout << "Error" << endl;
        }

        cout << "Saved results to SAI '" << Analyser::getSAIname() << "'" << endl
             << "(To analyse the results, open the editor and visualise the clusters using the SAI annotations)" << endl;
    }

    delete a;

}
