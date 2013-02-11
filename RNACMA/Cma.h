/*
 * Cma.h
 *
 * CMA stands for correlated mutation analysis. This class implements
 * information-theoretical measures to compute correlations between
 * positions in a set of aligned sequences.
 * The correlation values computed are Mutual Information (MI) and noiseless
 * Mutual Information (MIp). MIp is a correlation measure proposed in the
 * paper "Mutual information without the influence of phylogeny or entropy
 * dramatically improves residue contact prediction" by S. D. Dunn et al.,
 * Bioinformatics Vol. 24 no. 3, 333-340, 2008.
 * The clustering of correlated positions follows a simple heuristic, which
 * can be found in Thomas Runkler, "Information Mining", chapter 5, 53-58,
 * 2000.
 *
 *  Created on: Dec 16, 2009
 *      Author: Breno Faria
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */

 
#ifndef CMA_H
#define CMA_H

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif

#include <eigen/Eigen/Eigen>

#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif

using namespace std; // @@@ wtf! this really is a nono
using Eigen::VectorXd;
using Eigen::VectorXi;
using Eigen::MatrixXd;

// Vector of maps to store the histogram of sequences.
typedef vector<map<int, int> > MapVecType;
// Vector of vectors to store the aligned sequences (each one represented by a vector)
typedef vector<vector<string> > VecVecType; // @@@ string always is length 1. this should be vector<vector<char>> or vector<string>
// Matrix of maps to store the joint histogram of sequences.
typedef vector<MapVecType> MapMatrixType;

// Struct representing the tuple (pos1, pos2, mutualInformation(pos1,pos2)).
struct MITuple {
    int pos1;
    int pos2;
    double MI;
};

class Cma {

private:
    /**
     * This is used for numerical stability and to compare floating point numbers.
     */
    static CONSTEXPR double epsilon = 1.0e-9;
    /**
     * See documentation of project, p. 7.
     */
    static CONSTEXPR double penalty = 5.;
    /**
     * The alphabet considered.
     */
    vector<string> alphabet; // @@@ string always is length 1. this should be vector<char>
    /**
     * A map from alphabet letters to integers. This speeds things considerably up,
     * and we use less memory.
     */
    map<string, int> alphabet_map; // @@@ string is length 1 most of the time, disregarding alphabet_map["total"] which is used as global counter. this should be vector<char>
    /**
     * The number of sequences in the MSA.
     */
    int num_of_seqs;
    /**
     * A vector with the entropies for eac position.
     */
    VectorXd entropy;
    /**
     * A matrix with all joint entropies.
     */
    MatrixXd JointEntropy;
    /**
     * A matrix with the MI values for each pair of positions.
     */
    MatrixXd MutualInformation;
    /**
     * A matrix with the MIp values for each pair of positions.
     * See Dunn et al. "Mutual Information without the influence
     * of phylogeny or entropy dramatically improves residue contact
     * prediction", Bioinformatics Vol.24 no.3 333-340, 2008
     */
    MatrixXd MutualInformationP;
    /**
     * A vector with the cluster indices.
     */
    VectorXi clusters;

    /**
     * Initialises the alphabet.
     */
    void initAlphabet(vector<string> alph);
    /**
     * Builds the joint histogram of every pair of positions.
     */
    MapMatrixType buildJointHistogram(const VecVecType & alignedSequences);
    /**
     * Compute the joint entropy.
     */
    MatrixXd computeJointEntropy(MapMatrixType & hist);
    MatrixXd computeJointEntropy(const VecVecType & seq);
    /**
     * Used to compute the MIp.
     */
    VectorXd computeMeanMutualInformationVector();
    double computeOveralMeanMutualInformation();
    /**
     * Checks whether a letter pertains to the alphabet.
     */
    bool isValid(string & key1);


public:

    /**
     * Computes the MI.
     */
    MatrixXd computeMutualInformation(VecVecType & seq);
    /**
     * Computes the MIp.
     */
    MatrixXd computeMutualInformationP(VecVecType & seq);
    /**
     * Computes the clusters.
     */
    VectorXi computeClusters(list<MITuple> mituples, size_t size, double threshold);
    /**
     * Transforms the upper triangular matrix of a symmetric matrix into a
     * sortable list of MITuples.
     */
    list<MITuple> compute_mituples(MatrixXd mutualInformation);

    /**
     * Getters for the fields.
     */
    MatrixXd getMI();
    MatrixXd getMIp();
    MatrixXd getEntropy();
    MatrixXd getJointEntropy();
    VectorXi getClusters();

    /**
     * Birth and death...
     */
    Cma(vector<string> & alph, int seq_number);
    virtual ~Cma();
};

#else
#error Cma.h included twice
#endif // CMA_H
