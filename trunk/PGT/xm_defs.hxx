//  ================================================================ //
//                                                                   //
//    File      : xm_defs.hxx                                        //
//    Purpose   :                                                    //
//                                                                   //
//    Coded by Ralf Westram (coder@reallysoft.de) in November 2005   //
//    Institute of Microbiology (Technical University Munich)        //
//    http://www.arb-home.de/                                        //
//                                                                   //
//  ================================================================ //

#ifndef XM_DEFS_HXX
#define XM_DEFS_HXX

inline XmString PGT_XmStringCreateLocalized(const char *s) {
    return XmStringCreateLocalized(const_cast<char*>(s));
}

#else
#error xm_defs.hxx included twice
#endif // XM_DEFS_HXX

