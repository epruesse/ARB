/**
 * Analyser.h
 *
 * This class is the main entry point to this package.
 * The analyser object is responsible for interacting with the Cma code and Arb.
 *
 * The alphabet used is also defined in this class. Please check the implementation
 * of the constructor.
 *
 * If you are looking for information on Cma computations, please check the
 * comments in the Cma class.
 *
 *  Created on: Feb 15, 2010
 *      Author: Breno Faria
 *
 *  Institute of Microbiology (Technical University Munich)
 *  http://www.arb-home.de/ 
 */

#ifndef ANALYSER_H
#define ANALYSER_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef ALIGNEDSEQUENCELOADER_H
#include "AlignedSequenceLoader.h"
#endif
#ifndef CMA_H
#include "Cma.h"
#endif

class Analyser : virtual Noncopyable {

private:
    /**
     * The sequence loader. This object is an interface to Arb's DB.
     */
    AlignedSequenceLoader *loader;

    /**
     * The correlated mutation analysis computation is done by this object.
     */
    Cma *cma;

public:
    /**
     * Returns a reference to the Cma object.
     */
    Cma* getCma();

    /**
     * Saves the clustering result as SAI in the ARB DB.
     */
    GB_ERROR saveSAI(vector<size_t> clusters, double threshold);
    static const char *getSAIname() { return "cma_clusters"; }

    /**
     * Gives the clusters sensible names.
     */
    vector<char> normalizeClusters(vector<size_t> clusters);

    /**
     * Returns reference to the loader object.
     */
    AlignedSequenceLoader* getLoader();

    /**
     * Constructor.
     */
    Analyser();

    /**
     * Desctructor.
     */
    virtual ~Analyser();

};

#else
#error Analyser.h included twice
#endif // ANALYSER_H
