/*!
 * \file check_hyperthreads.h
 *
 * \date 01.08.2011
 * \author Tilo Eissler
 *
 * This header provides a simple function to determine the number
 * of hyperthreads on the system.
 */

#ifndef ARB_CHECK_HYPERTHREADS_H_
#define ARB_CHECK_HYPERTHREADS_H_

#include <iostream>
#include <string>

namespace arb {
namespace toolbox {

/*!
 * \brief Load CPU ID information
 */
inline void cpuID(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
    __cpuid((int *)regs, (int)i);

#else
    asm volatile
    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
            : "a" (i), "c" (0));
    // ECX is set to zero for CPUID function 4
#endif
}

/*!
 * \brief Return number of <cores,logical> threads
 *
 * If "cores < logical" the CPU supports hyperthreading
 *
 * If detection fails, <0,0> is returned.
 *
 * \param std::pair<int,int>
 */
inline std::pair<int, int> getHardwareThreads() {
    unsigned regs[4];

    // Get vendor
    char vendor[12];
    cpuID(0, regs);
    ((unsigned *) vendor)[0] = regs[1]; // EBX
    ((unsigned *) vendor)[1] = regs[3]; // EDX
    ((unsigned *) vendor)[2] = regs[2]; // ECX
    std::string cpuVendor = std::string(vendor, 12);

    // Get CPU features
    cpuID(1, regs);
    unsigned cpuFeatures = regs[3]; // EDX

    int logical = 0;
    int cores = 0;
    if (cpuVendor == "GenuineIntel" && (cpuFeatures & (1 << 28))) { // HTT bit
        // Logical core count per CPU
        cpuID(1, regs);
        logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
#ifdef DEBUG
        std::cout << " logical cpus: " << logical << std::endl;
#endif
        cores = logical;

        if (cpuVendor == "GenuineIntel") {
            // Get DCP cache info
            cpuID(4, regs);
            cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1

        } else if (cpuVendor == "AuthenticAMD") {
            // Get NC: Number of CPU cores - 1
            cpuID(0x80000008, regs);
            cores = ((unsigned) (regs[2] & 0xff)) + 1; // ECX[7:0] + 1
        }
#ifdef DEBUG
        std::cout << "    cpu cores: " << cores << std::endl;
#endif
    }
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
