/*
 * AlignedSequenceLoader.h
 *
 * Interface to Arb's DB.
 * This class loads aligned sequences from Arb's DB and allows the other
 * code in this package to access it in a standard way.
 *
 *  Created on: Feb 15, 2010
 *      Author: Breno Faria
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */


#ifndef ALIGNEDSEQUENCELOADER_H
#define ALIGNEDSEQUENCELOADER_H

#ifndef CMA_H
#include "Cma.h"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class AlignedSequenceLoader : virtual Noncopyable {

private:
    /**
     * The aligned sequences (see Cma.h for the definition of VecVecType).
     */
    VecVecType *seqs;
    /**
     * The positions map between cleaned-up alignment and original one.
     */
    vector<size_t> *position_map;
    /**
     * The length of the multiple sequence alignment.
     */
    size_t MSA_len;
    /**
     * Cleans-up the MSA, removing positions with no base occurrence.
     */
    void cleanSeqs(size_t* occurrences, long len);

public:

    /**
     * Returns the MSA length.
     */
    size_t getMsaLen();

    /**
     * Returns the position map.
     */
    vector<size_t> * getPositionMap();

    /**
     * Returns the aligned sequences.
     */
    VecVecType* getSequences();

    /**
     * Constructor.
     */
    AlignedSequenceLoader();

    /**
     * Destructor.
     */
    virtual ~AlignedSequenceLoader();
};

#else
#error AlignedSequenceLoader.h included twice
#endif // ALIGNEDSEQUENCELOADER_H
