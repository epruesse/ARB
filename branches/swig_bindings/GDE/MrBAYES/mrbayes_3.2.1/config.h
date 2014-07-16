// modified for ARB build integration by Ralf Westram
// variables possibly passed from Makefile: ARB_64 ARB_DARWIN

/* config.h.  Generated from config.h.in by configure.  */
/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://www.mrbayes.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "mrbayes"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "mrbayes 3.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "mrbayes"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.2"

/* Define to 1 if you have the `readline' library (-lreadline). */
#define HAVE_LIBREADLINE 1

/* Define to 1 if you have a recent version of readline */
#define COMPLETIONMATCHES 1

/* Define to 1 if you want to compile the parallel version */
/* #undef MPI_ENABLED */

/* Define to 1 if you want to compile SSE-enabled version */
#if defined(ARB_64)
#define SSE_ENABLED 1
#else
#undef SSE_ENABLED
#endif

/* Define one of the UNIX, MAC or WINDOWS */
#if defined(ARB_DARWIN)
#define MAC_VERSION 1
#undef UNIX_VERSION
#else
#define UNIX_VERSION 1
#undef MAC_VERSION
#endif
#undef WINDOWS_VERSION

/* Define to 1 if you want to compile the debug version */
/* #undef DEBUGOUTPUT */

/* Define if used on a 64bit cpu */
#if defined(ARB_64)
#define _64BIT 1
#else
#undef _64BIT
#endif


/* Define to use a log lookup for 4by4 nucleotide data (actually SLOWER than normal code on intel processors) */
/* #undef FAST_VERSION */

/* Define to enable beagle library */
/* #undef BEAGLE_ENABLED */

/* Define to enable threads in BEAGLE library */
/* #undef THREADS_ENABLED */

/* PAUL: "#if defined READLINE_LIBRARY" (sax?) */
