// =============================================================== //
//                                                                 //
//   File      : sec_gc.hxx                                        //
//   Purpose   : GC definitions                                    //
//   Time-stamp: <Thu Sep/06/2007 12:39 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_GC_HXX
#define SEC_GC_HXX

enum {
    SEC_GC_LOOP       = 0, SEC_GC_FIRST_FONT = SEC_GC_LOOP, SEC_GC_FIRST_DATA = SEC_GC_LOOP,
    SEC_GC_HELIX,
    SEC_GC_NHELIX,                                          SEC_GC_LAST_DATA  = SEC_GC_NHELIX,
    SEC_GC_DEFAULT,        SEC_GC_ERROR = SEC_GC_DEFAULT,
    SEC_GC_BONDS,
    SEC_GC_ECOLI,          SEC_GC_LAST_FONT  = SEC_GC_ECOLI,
    SEC_GC_HELIX_NO,

    SEC_GC_CBACK_0,	// Ranges for SAI visualization
    SEC_GC_CBACK_1,
    SEC_GC_CBACK_2,
    SEC_GC_CBACK_3,
    SEC_GC_CBACK_4,
    SEC_GC_CBACK_5,
    SEC_GC_CBACK_6,
    SEC_GC_CBACK_7,
    SEC_GC_CBACK_8,
    SEC_GC_CBACK_9,

    SEC_GC_CURSOR,
    SEC_GC_MBACK,               //mismatches

    SEC_GC_SBACK_0,             // User 1
    SEC_GC_SBACK_1,             // User 2
    SEC_GC_SBACK_2,             // Probe
    SEC_GC_SBACK_3,             // Primer (local)
    SEC_GC_SBACK_4,             // Primer (region)
    SEC_GC_SBACK_5,             // Primer (global)
    SEC_GC_SBACK_6,             // Signature (local)
    SEC_GC_SBACK_7,             // Signature (region)
    SEC_GC_SBACK_8,             // Signature (global)

    SEC_SKELE_HELIX,  //skeleton helix color
    SEC_SKELE_LOOP,  //skeleton loop color
    SEC_SKELE_NHELIX,  //skeleton non-pairing helix color

    SEC_GC_MAX
}; // AW_gc

#define SEC_GC_DATA_COUNT (SEC_GC_LAST_DATA-SEC_GC_FIRST_DATA+1)
#define SEC_GC_FONT_COUNT (SEC_GC_LAST_FONT-SEC_GC_FIRST_FONT+1)

#else
#error sec_gc.hxx included twice
#endif // SEC_GC_HXX
