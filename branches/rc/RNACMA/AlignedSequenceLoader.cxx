/*
 * AlignedSequenceLoader.cxx
 *
 *  Created on: Feb 15, 2010
 *      Author: Breno Faria
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */


#include "AlignedSequenceLoader.h"

#include <arbdb.h>
#include <arbdbt.h>
#include "dbconn.h"

/**
 * Loads the marked sequences aligned from Arb's DB.
 * This loader only considers letters given by the following regular
 * expression: [ACGTUacgtu]
 */
AlignedSequenceLoader::AlignedSequenceLoader() {

    GBDATA *gb_main = runningDatabase();

    GB_ERROR error = GB_push_transaction(gb_main);
    if (error) {
        cout << "RNACMA-Error: " << error << "\n";
        exit(EXIT_FAILURE);
    }

    seqs = new VecVecType(0);

    char *al_name = GBT_get_default_alignment(gb_main);
    MSA_len = GBT_get_alignment_len(gb_main, al_name);

    size_t occurrences[MSA_len];
    for (size_t i = 0; i < MSA_len; i++)
        occurrences[i] = 0;

    cout << "loading marked species: ";
    flush( cout);
    for (GBDATA *gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species
             = GBT_next_marked_species(gb_species)) {

        GBDATA *gb_ali = GB_entry(gb_species, al_name);

        if (gb_ali) { // existing alignment for this species
            GBDATA *gb_data = GB_entry(gb_ali, "data");

            if (gb_data) {

                cout << GBT_read_name(gb_species) << " ";
                flush(cout);

                string sequence = GB_read_string(gb_data);

                string *seq_as_vec = new string[MSA_len];
                int     k          = 0;
                for (string::iterator i = sequence.begin(); i != sequence.end(); ++i) {
                    switch (*i) {
                        case 'A':
                        case 'a':
                            seq_as_vec[k] = "A";
                            occurrences[k] += 1;
                            break;
                        case 'C':
                        case 'c':
                            seq_as_vec[k] = "C";
                            occurrences[k] += 1;
                            break;
                        case 'G':
                        case 'g':
                            seq_as_vec[k] = "G";
                            occurrences[k] += 1;
                            break;
                        case 'T':
                        case 't':
                        case 'U':
                        case 'u':
                            seq_as_vec[k] = "T";
                            occurrences[k] += 1;
                            break;
                        default:
                            seq_as_vec[k] = "-";
                            break;
                    }
                    k++;
                }

                assert((size_t)k == MSA_len);
                vector<string> seq_vector(&seq_as_vec[0], &seq_as_vec[k]);
                delete [] seq_as_vec;

                seqs->push_back(seq_vector);
            }
        }

    }

    GB_pop_transaction(gb_main);

    cout << "done. Total number of species: " << seqs->size() << endl;
    flush(cout);

    cleanSeqs(occurrences, MSA_len);
}

/**
 * Returns the aligned seqs.
 *
 * @return the aligned seqs.
 */
VecVecType* AlignedSequenceLoader::getSequences() {
    return seqs;
}

/**
 * Destructor.
 */
AlignedSequenceLoader::~AlignedSequenceLoader() {
    delete seqs;
}

/**
 * Getter for the MSA_len.
 *
 * @return the MSA length.
 */
size_t AlignedSequenceLoader::getMsaLen() {
    return MSA_len;
}

/**
 * Getter for the position map. The position map maps positions in the clean
 * sequence to the original positions in the alignment.
 *
 * @return the position map.
 */
vector<size_t> * AlignedSequenceLoader::getPositionMap() {
    return position_map;
}

/**
 * This method cleans-up the empty positions of the MSA.
 *
 *
 * @param occurrences: a list gathering the number of occurrences of bases at
 *                     each position of the MSA.
 * @param len: the length of occurrences.
 */
void AlignedSequenceLoader::cleanSeqs(size_t *occurrences, long len) {

    cout << "cleaning-up sequences of empty positions... " << endl;
    flush( cout);

    size_t num_of_bases = 0;
    for (int i = 0; i < len; i++) {
        if (occurrences[i] != 0) {
            num_of_bases++;
        }
    }

    cout << "number of non-empty positions in MSA: " << num_of_bases
         << ". Filtered out " << len - num_of_bases << " positions." << endl;

    VecVecType *clean_seqs = new VecVecType(0);

    cout << "computing position map..." << endl;
    position_map = new vector<size_t> (num_of_bases, 0);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (occurrences[i] != 0) {
            position_map->at(j) = i;
            j++;
        }
    }

    for (VecVecType::iterator seq = seqs->begin(); seq != seqs->end(); ++seq) {
        //for every sequence
        vector<string> sequence(num_of_bases, "");
        int jj = 0;
        for (int i = 0; i < len; ++i) {
            if (occurrences[i] != 0) {
                sequence.at(jj) = seq->at(i);
                jj++;
            }
        }
        assert(sequence.size() == num_of_bases);
        clean_seqs->push_back(sequence);
    }

    seqs = clean_seqs;

    cout << "clean-up done." << endl;

}

