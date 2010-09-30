// ================================================================= //
//                                                                   //
//   File      : macke.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef MACKE_H
#define MACKE_H

class MackeReader {
    bool  firstRead;
    char *inName;

    FILE        *IFP1, *IFP2, *IFP3;
    FILE_BUFFER  fp1, fp2, fp3;
    
    void start_reading();
    void stop_reading();

public:
    MackeReader(const char *inName_)
        : firstRead(true), 
          inName(strdup(inName_))
    {}
    ~MackeReader() {
        stop_reading();
        free(inName);
    }

    char in();
};


#else
#error macke.h included twice
#endif // MACKE_H
