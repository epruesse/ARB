/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMREFERENCEEMBL_H
#define GAGENOMREFERENCEEMBL_H

#ifndef GAGENOMREFERENCE_H
#include "GAGenomReference.h"
#endif

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

namespace gellisary{

    class GAGenomReferenceEmbl : public GAGenomReference{
    private:
        std::vector<std::string> reference_cross_reference; // embl:RX  genbank,ddgj:MEDLINE,PUBMED
        std::string reference_cross_reference_as_string;
        std::string reference_group;                        // embl:RG  genbank,ddgj:?
        //  std::string reference_location;                     // embl:RL  genbank,ddgj:JOURNAL

    public:

        //  GAGenomReferenceEmbl();
        virtual ~GAGenomReferenceEmbl(){}
        virtual void parse();
        std::string * getGroup();
        //  std::string * getLocation();
        std::vector<std::string> * getCrossReference();
        std::string * getCrossReferenceAsString();
    };

};

#endif // GAGENOMREFERENCEEMBL_H
