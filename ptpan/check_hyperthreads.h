/*!
 * \file check_hyperthreads.h
 *
 * \date 01.08.2011
 * \author Tilo Eissler
 *
 * This header provides a simple function to determine the number
 * of hyperthreads on the system if hwloc is available.
 *
 * Linking against hwloc may be necessary. If not available, only the
 * number of processing units can be determined which may be suboptimal!
 */

#ifndef ARB_CHECK_HYPERTHREADS_H_
#define ARB_CHECK_HYPERTHREADS_H_

#ifdef DEBUG
#include <iostream>
#endif

#ifdef HWLOC_AVAILABLE
#include <hwloc.h>
#else
#warning Falling back to Boost::thread which may lead to SMT/Hyperthreaded cores being counted as well!
#warning Installing hwloc-library is recommended!
#include <boost/thread.hpp>
#endif

namespace arb {
namespace toolbox {

/*!
 * \brief Return number of <cores,logical> threads
 *
 * If "cores < logical" the CPU supports hyperthreading
 *
 * If detection fails, <0,0> is returned.
 *
 * You may need to link against 'hwloc' if available!
 *
 * \param std::pair<int,int>
 */
inline std::pair<int, int> getHardwareThreads() {
    int logical = 0;
    int cores = 0;

#ifdef HWLOC_AVAILABLE

    // Topology object
    hwloc_topology_t topology;
    // Allocate and initialize topology object.
    hwloc_topology_init(&topology);

    /* ... Optionally, put detection configuration here to e.g. ignore some
     objects types, define a synthetic topology, etc....  The default is
     to detect all the objects of the machine that the caller is allowed
     to access.
     See Configure Topology Detection.  */

    // Perform the topology detection.
    hwloc_topology_load(topology);
    // get the number of cores
    cores = (int) hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    // get the number of processing units (incl. SMT/hyperthreading)
    logical = (int) hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

    // Destroy topology object.
    hwloc_topology_destroy(topology);
#ifdef DEBUG
    std::cout << "cpu cores: " << cores << std::endl;
    std::cout << "cpu processing units: " << logical << std::endl;
#endif
#else
    cores = boost::thread::hardware_concurrency();
    logical = cores;
#endif

    return std::make_pair(cores, logical);
}

/*!
 * \brief Return true if hyperthreads are available on the system
 *
 * if in doubt, return false
 *
 * \param bool
 */
inline bool hasHyperThreads() {
    // Detect hyper-threads
    bool hyperThreads = false;

    std::pair<int, int> threads = getHardwareThreads();
    int cores = threads.first;
    int logical = threads.second;
    if (cores < logical) {
        hyperThreads = true;
    }
#ifdef DEBUG
    std::cout << "hyper-threads: " << (hyperThreads ? "true" : "false")
            << std::endl;
#endif
    // if in doubt, return false;
    return hyperThreads;
}

} // end namespace toolbox
} // end namespace arb

#endif /* ARB_CHECK_HYPERTHREADS_H_ */
