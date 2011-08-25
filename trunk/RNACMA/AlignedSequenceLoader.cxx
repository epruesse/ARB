/*
 * AlignedSequenceLoader.cxx
 *
 *  Created on: Feb 15, 2010
 *      Author: Breno Faria
 */


#include "AlignedSequenceLoader.h"

#include <arbdb.h>
#include <arbdbt.h>
#include "dbconn.h"

#define AL_DEBUG false

/**
 * Loads the marked sequences aligned from Arb's DB.
 * This loader only considers letters given by the following regular
 * expression: [ACGTUacgtu]
 */
AlignedSequenceLoader::AlignedSequenceLoader() {

//     GBDATA *gb_main = GB_open(":", "rwt"); // @@@
//     if (!gb_main) {
//         GB_print_error();
//         exit(-1);
//     }

    GBDATA *gb_main = runningDatabase();

    GB_ERROR error = GB_push_transaction(gb_main);
    if (error)
        exit(1);

    seqs = new VecVecType(0);

    char *al_name = GBT_get_default_alignment(gb_main);
    MSA_len = GBT_get_alignment_len(gb_main, al_name);

    size_t occurrences[MSA_len];
    for (int i = 0; i < MSA_len; i++)
        occurrences[i] = 0;

    cout << "loading marked species: ";
    flush( cout);
    long temp;
    for (GBDATA *gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species
             = GBT_next_marked_species(gb_species)) {

        GBDATA *gb_ali = GB_entry(gb_species, al_name);

        if (gb_ali) { // existing alignment for this species
            GBDATA *gb_data = GB_entry(gb_ali, "data");

            if (gb_data) {

                cout << GBT_read_name(gb_species) << " ";
                flush(cout);

                string sequence = GB_read_string(gb_data);

                string seq_as_vec[MSA_len];
                int k = 0;
                for (string::iterator i = sequence.begin(); i != sequence.end(); ++i) {
#if AL_DEBUG
                    cout << *i;
#endif
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
#if AL_DEBUG
                cout << endl << "len::" << /*seq_as_vec.size()*/k << endl;
                cout << "seq::" << seq_as_vec[0] << endl;
#endif

                assert(k == MSA_len);
                vector<string> seq_vector(&seq_as_vec[0], &seq_as_vec[k]);

                seqs->push_back(seq_vector);
#if AL_DEBUG
                for (vector<string>::iterator i = seqs->at(0).begin(); i
                         != seqs->at(0).end(); ++i) {
                    cout << *i;
                }
#endif

            }
        }

    }

    GB_pop_transaction(gb_main);

    cout << "done. Total number of species: " << seqs->size() << endl;
    flush(cout);

    cleanSeqs(occurrences, MSA_len);

#if AL_DEBUG
    cout << "popped" << endl;
#endif

    // GB_close(gb_main);
}

/**
 * Dummy - used for Debug.
 */
AlignedSequenceLoader::AlignedSequenceLoader(bool dummy) {

    seqs = new VecVecType(4);
    seqs->at(0) = vector<string> (0, "");
    seqs->at(0).push_back("A");
    seqs->at(0).push_back("-");
    seqs->at(0).push_back("A");
    seqs->at(0).push_back("T");
    seqs->at(1) = vector<string> (0, "");
    seqs->at(1).push_back("T");
    seqs->at(1).push_back("-");
    seqs->at(1).push_back("C");
    seqs->at(1).push_back("A");
    seqs->at(2) = vector<string> (0, "");
    seqs->at(2).push_back("C");
    seqs->at(2).push_back("C");
    seqs->at(2).push_back("-");
    seqs->at(2).push_back("A");
    seqs->at(3) = vector<string> (0, "");
    seqs->at(3).push_back("A");
    seqs->at(3).push_back("G");
    seqs->at(3).push_back("-");
    seqs->at(3).push_back("G");

    int occurrences[] = { 1, 1, 1, 1 };
    cleanSeqs((size_t*) occurrences, 4);

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

#if AL_DEBUG
    for (j = 0; j < num_of_bases; j++) {
        cout << ":" << position_map->at(j);
    }
    cout << endl;
#endif

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

#if AL_DEBUG
    for (VecVecType::iterator i = clean_seqs->begin(); i != clean_seqs->end(); ++i) {
        cout << "new sequence" << endl;
        for (vector<string>::iterator j = i->begin(); j != i->end(); j++) {
            cout << *j;
        }
        cout << endl;
    }
#endif

    seqs = clean_seqs;

#if AL_DEBUG
    cout << "cleaned-up seqs:" << endl;
    for (VecVecType::iterator i = seqs->begin(); i != seqs->end(); ++i) {
        cout << "new sequence" << endl;
        for (vector<string>::iterator j = i->begin(); j != i->end(); j++) {
            cout << *j;
        }
        cout << endl;
    }
#endif

    cout << "clean-up done." << endl;

}

