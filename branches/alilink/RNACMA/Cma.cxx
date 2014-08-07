/*
 * Cma.cpp
 *
 *  Created on: Dec 16, 2009
 *      Author: Breno Faria
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */

#include <iostream>
#include <iomanip>
#include <time.h>

#include "Cma.h"

//>--------------------------Global definitions--------------------------------

// Some local function prototypes.
void unifyCluster(int cluster1, int cluster2, VectorXi & result);

//>-----------------------local helper functions-------------------------------

/**
 * Helper function for Cma::computeClusters. Compares MITuples by
 * mutual information value.
 *
 * @param first: the first MITuple
 *
 * @param second: the second MITuple
 *
 * @returns whether the first element should be placed before the second.
 */
static bool compareMITuples(MITuple first, MITuple second) {
    return first.MI > second.MI;
}

/**
 * Helper function for Cma::computeClusters. Unites two clusters.
 *
 * @param cluster1: the index of the first cluster.
 *
 * @param cluster2: the index of the second cluster.
 */
void unifyCluster(int cluster1, int cluster2, VectorXi & result) {
    size_t size = result.size();
    for (size_t i = 0; i < size; ++i) {
        if (result[i] == cluster2) {
            result[i] = cluster1;
        }
    }
}

//>---------------------------Public methods-----------------------------------

/**
 * Constructor.
 *
 * @param seq_num: the number of sequences.
 *
 * @param alph: the vector containing the alphabet. Notice that letters of the
 *              alphabet might be longer than one character. This is not used
 *              so far, but limiting the length to 1 seems wrong...
 */
Cma::Cma(vector<string> & alph, int seq_num) {

    initAlphabet(alph);
    num_of_seqs = seq_num;

}

/**
 * Destructor.
 */
Cma::~Cma() {
}

/**
 * Computes the mutual information of position pairs in a set of
 * sequences.
 * Mutual information can be defined as:
 *
 * MI(pos1, pos2) = H(pos1) + H(pos2) - H(pos1, pos2),
 *
 * i.e. the sum of entropies at two positions minus the joint entropy
 * of these positions.
 *
 * @param seq: the multiple sequence alignment.
 *
 * @returns a matrix with the MI values.
 */
MatrixXd Cma::computeMutualInformation(VecVecType & seq) {

    JointEntropy = computeJointEntropy(seq);

    JointEntropy = JointEntropy + JointEntropy.adjoint() - JointEntropy.diagonal().asDiagonal();

    entropy = JointEntropy.diagonal();

    cout << "computing Mutual Information... ";
    flush(cout);
    clock_t start = clock();

    VectorXd Ones = VectorXd::Ones(entropy.size());
    MatrixXd Hx = (entropy * Ones.transpose()).transpose();
    MatrixXd Hy = Hx.transpose();

    MutualInformation = Hx + Hy - JointEntropy;

    cout << "completed in " << ((double) clock() - start) / CLOCKS_PER_SEC
         << "s." << endl;

    return MutualInformation;

}

/**
 * Computes the noiseless mutual information, or MIp (see classdoc), which is
 * defined as:
 *
 * MIp(pos1, pos2) = MI(pos1, pos2) - (mMI(pos1) * mMI(pos2)) / mMI,
 *
 * where MIp is the noiseless mutual information between positions 1 and 2 and
 * MI similarly. mMI(x) is the mean mutual information of position x, which is
 * defined as:
 *
 * mMI(pos1) = 1 / m * \sum_{i \in positions} MI(pos1, i), where m is the
 * length of the sequences.
 *
 * mMI is the overall mean mutual information, defined as:
 *
 * mMI = 2 / m * \sum_{i \in positions} \sum_{j in positions} MI(i, j), where
 * again m is the length of the sequences.
 *
 * @param seq: the multiple sequence alignment.
 *
 * @returns the noiseless mutual information.
 */
MatrixXd Cma::computeMutualInformationP(VecVecType & seq) {

    if (MutualInformation.size() == 0) {
        computeMutualInformation(seq);
    }

    cout << "computing noiseless Mutual Information (MIp)... ";
    flush(cout);
    clock_t start = clock();
    /*
     * We must remove negative MI values (the negative values are artificially
     * generated, see comments in computeJointEntropy).
     */
    for (int i = 0; i < MutualInformation.rows(); ++i) {
        for (int j = 0; j < MutualInformation.cols(); ++j) {
            if (MutualInformation(i, j) < 0.) {
                MutualInformation(i, j) = 0.;
            }
        }
    }

    VectorXd mMIs = computeMeanMutualInformationVector();
    double mMI = computeOveralMeanMutualInformation();

    MutualInformationP = MutualInformation - ((mMIs * mMIs.transpose()) / mMI);

    cout << "completed in " << ((double) clock() - start) / CLOCKS_PER_SEC
         << "s." << endl;

    return MutualInformationP;

}

/**
 * Given a MI-matrix, computes the sorted list of the highest MI values.
 *
 * @param mutualInformation: a matrix with MI (or MIp) measures.
 *
 * @returns the list of tuples of positions sorted by their MI value.
 */
list<MITuple> Cma::compute_mituples(MatrixXd mutualInformation) {
    size_t size = mutualInformation.cols();

    list<MITuple> mituples;

    // populate the mituples list with all entries in the upper triangular matrix
    // of mutualInformation.
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = i + 1; j < size; ++j) {
            MITuple curr;
            curr.MI = mutualInformation(i, j);
            curr.pos1 = i;
            curr.pos2 = j;
            mituples.push_back(curr);
        }
    }

    // sort mituples by MI-value
    mituples.sort(compareMITuples);

    return mituples;
}

/**
 * Computes the clusters of positions that are most correlated, up to
 * threshold.
 *
 * @param mituples: the mutualInformation matrix, as generated by
 *                  Cma::computeMutualInformation.
 *
 * @param size: the original size of the alignment.
 *
 * @param threshold: the mutual information threshold. If two positions have
 *                   mi < threshold, they won't be united in one cluster.
 *
 * @returns a vector with the cluster indices for each position.
 */
VectorXi Cma::computeClusters(list<MITuple> mituples, size_t size,
                              double threshold) {

    cout << "computing clusters of correlated positions... ";
    flush(cout);

    VectorXi result = VectorXi::Zero(size);

    //compute the clusters.
    int cluster = 1;
    int rest = int(size);
    for (list<MITuple>::iterator it = mituples.begin(); it != mituples.end(); ++it) {
        double result1 = abs(result[it->pos1]);
        double result2 = abs(result[it->pos2]);
        if (it->MI <= threshold || rest == 0) {
            break;
        }
        if (result1 < epsilon and result2 < epsilon) {
            result[it->pos1] = cluster;
            result[it->pos2] = cluster++;
            rest -= 2;
        }
        else if (result1 > epsilon and result2 < epsilon) {
            result[it->pos2] = result[it->pos1];
            rest -= 1;
        }
        else if (result1 < epsilon and result2 > epsilon) {
            result[it->pos1] = result[it->pos2];
            rest -= 1;
        }
        else if (result1 > epsilon and result2 > epsilon && result1 - result2 > epsilon) {
            unifyCluster(result[it->pos1], result[it->pos2], result);
        }
    }

    clusters = result;

    cout << "done." << endl;

    return result;
}

//>-------------------------getters and setters--------------------------------

MatrixXd Cma::getEntropy() {

    return entropy;

}

MatrixXd Cma::getJointEntropy() {

    return JointEntropy;

}

MatrixXd Cma::getMI() {

    return MutualInformation;

}

MatrixXd Cma::getMIp() {

    return MutualInformationP;

}

VectorXi Cma::getClusters() {

    return clusters;

}

//>----------------------------private methods---------------------------------

/**
 * Here we define the alphabet.
 */
void Cma::initAlphabet(vector<string> alph) {

    alphabet = vector<string> (alph);

    int i = 0;
    for (vector<string>::iterator it = alph.begin(); it != alph.end(); ++it) {
        alphabet_map[*it] = i;
        i++;
    }
    for (vector<string>::iterator it = alph.begin(); it != alph.end(); ++it) {
        for (vector<string>::iterator it2 = alph.begin(); it2 != alph.end(); ++it2) {
            alphabet_map[*it + *it2] = i;
            i++;
        }
    }
    alphabet_map["total"] = i;
}

/**
 * Computes an approximation of the joint entropy for each pair of positions.
 *
 * The joint entropy of two positions in a set of aligned sequences
 * is defined as:
 * - ( p(A,A) * log2(p(A,A)) + p(A,C) * log2(p(A,C)) + ... +
 *     p(C,A) * log2(p(C,A)) + p(C,C) * log2(p(C,A)) + ... +
 *     ... +
 *     p(T,A) * log2(p(T,A)) + ... + p(T) * log2(p(T,T)) ),
 * where p(x,y) is the probability of observing base x at position 1 and
 * base y at position 2.
 *
 * @param seqs: the aligned sequences
 *
 * @returns a square matrix with the joint entropy values for
 *          each pair of positions.
 */
MatrixXd Cma::computeJointEntropy(const VecVecType & seqs) {

    MapMatrixType hist = buildJointHistogram(seqs);
    return Cma::computeJointEntropy(hist);

}

/**
 * Computes an approximation of the joint entropy for each pair of positions.
 * The measure also includes an heuristic to penalise mismatches in occurrences
 * (see the documentation of this project for further explanations).
 *
 * @param hist: the histogram of labels for each position in the sequence.
 *
 * @returns a square matrix with the joint entropy values for
 *          each pair of positions.
 */
MatrixXd Cma::computeJointEntropy(MapMatrixType & hist) {

    cout << "computing joint entropy (this may take a while)... ";
    flush(cout);
    clock_t start = clock();

    int hist_size = int(hist.size());
    MatrixXd result(hist_size, hist_size);
    result.setZero(hist_size, hist_size);
    for (unsigned int i = 0; i < (unsigned int) hist_size; ++i) {

        //progress "bar"
        if (hist_size > 30 and i % (hist_size / 30) == 0) {
            cout << "(" << setw(6) << setiosflags(ios::fixed)
                 << setprecision(2) << i / float(hist_size) * 100 << "%)";
            flush(cout);
        }

        for (unsigned int j = i; j < hist[i].size(); ++j) {
            int total_i_j = hist[i][j][alphabet_map.at("total")];
            int total_i_i = hist[i][i][alphabet_map.at("total")];
            int total_j_j = hist[j][j][alphabet_map.at("total")];
            int mismatches = total_i_i + total_j_j - 2 * total_i_j;
            if (total_i_j != 0) {
                double result_i_j = 0.;
                for (map<int, int>::iterator it = hist[i][j].begin(); it
                         != hist[i][j].end(); ++it) {
                    double pair = double(it->second) + Cma::epsilon;
                    result_i_j += (pair / total_i_j) * log2(pair / total_i_j);
                }

                result(i, j) = -result_i_j + double(mismatches) / (total_i_i
                                                                   + total_j_j) * Cma::penalty;

            } else {
                /**
                 * We will only fall into this case if we have two positions which
                 * never occur simultaneously. In this case we cannot say anything about
                 * the MI.
                 * To guarantee that this joint entropy value will not come in our way in
                 * the MI calculation we set it artificially to 5.0, which is 1 greater than
                 * max(H(X) + H(Y)).
                 */
                result(i, j) = 5.;
            }

        }
        //progress "bar"
        if (hist_size > 30 and i % (hist_size / 30) == 0) {
            cout << "\b\b\b\b\b\b\b\b\b";
        }
    }
    flush(cout);
    cout << "completed in " << ((double) clock() - start) / CLOCKS_PER_SEC
         << "s." << endl;
    return result;

}

/**
 * Checks whether the key is valid (if it pertains to the alphabet).
 *
 * @param key: the base to be checked.
 *
 * @return whether the key is valid (if it pertains to the alphabet).
 */
bool Cma::isValid(string & key) {

    map<string, int>::iterator it = alphabet_map.find(key);
    return (it != alphabet_map.end());

}

/**
 * This method computes a matrix of maps, one for each pair of positions of the
 * aligned sequences.
 * Each map has an entry for each pair of occurring letters to count the joint
 * occurrences of these letters.
 *
 * @param alignedSequences: an array of string vectors, containing the sequences.
 *
 * @returns a matrix of maps (see method description)
 */
MapMatrixType Cma::buildJointHistogram(const VecVecType & alignedSequences) {

    cout << "building histogram (this may take a while)... ";
    flush(cout);
    clock_t start = clock();

    if (alignedSequences.empty()) {
        throw "Error: Cma::buildJointHistogram: parameter empty.";
    }

    // here we defined one dimension of the matrix as being the same as the
    // length of the sequences.
    MapMatrixType result(alignedSequences[0].size(), MapVecType(
                             alignedSequences[0].size()));

    int ii = 0;
    // for each sequence:
    for (VecVecType::const_iterator it = alignedSequences.begin(); it
             != alignedSequences.end(); ++it) {

        cout << "(" << setw(6) << setiosflags(ios::fixed) << setprecision(2)
             << ii / float(num_of_seqs) * 100 << "%)";
        flush(cout);

        // for each first position in the sequence
        for (size_t i = 0; i < it->size(); ++i) {
            string key1 = it->at(i);
            if (isValid(key1)) {

                //for each second position in the sequence
                for (size_t j = i; j < it->size(); ++j) {
                    string key2 = it->at(j);
                    if (isValid(key2)) {
                        result[i][j][alphabet_map.at(key1 + key2)] += 1;
                        result[i][j][alphabet_map.at("total")] += 1;
                    }
                }
            }
        }
        cout << "\b\b\b\b\b\b\b\b\b";
        ii++;
    }
    flush(cout);
    cout << "completed in " << ((double) clock() - start) / CLOCKS_PER_SEC
         << "s." << endl;

    return result;
}

/**
 * Computes the vector of mean mutual information values mMI.
 *
 * mMI(pos1) = 1 / m * \sum_{i \in positions} MI(pos1, i), where m is the
 * length of the sequences.
 *
 * @returns the vector of mean mutual information values mMI.
 */
VectorXd Cma::computeMeanMutualInformationVector() {

    return MutualInformation.rowwise().sum() / num_of_seqs;

}

/**
 * Computes the overall mean mutual information value.
 *
 * mMI = 2 / m * \sum_{i \in positions} \sum_{j in positions} MI(i, j), where
 * again m is the length of the sequences.
 *
 * @returns the overall mean mutual information value.
 */
double Cma::computeOveralMeanMutualInformation() {

    return 2. * MutualInformation.sum() / num_of_seqs / num_of_seqs;

}

