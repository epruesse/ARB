
#ifndef CHIP_HXX
#define CHIP_HXX

#ifdef NDEBUG
#define chip_assert(bed)
#else
#define chip_assert(bed) do { if ((bed)==false) { cerr << "Assertion '" << #bed << "' failed in " << __LINE__ << "\n"; exit(1); } } while(0)
#endif

#else
#error chip.hxx included twice
#endif // CHIP_HXX

