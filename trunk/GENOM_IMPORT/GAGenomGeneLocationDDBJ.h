/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMGENELOCATIONDDBJ_H
#define GAGENOMGENELOCATIONDDBJ_H

#include "GAGenomGeneLocation.h"

namespace gellisary {

    class GAGenomGeneLocationDDBJ : public gellisary::GAGenomGeneLocation{
    private:
        std::string reference;
        bool pointer;
        std::vector<GAGenomGeneLocationDDBJ> locations;
        int current_value;
        std::vector<std::string> getParts(std::string *, int *);
        //  gellisary::GAGenomGeneLocationDDBJ tmp_loc;

    public:

        GAGenomGeneLocationDDBJ(std::string *);
        GAGenomGeneLocationDDBJ(){}
        virtual ~GAGenomGeneLocationDDBJ(){}
        virtual void parse();
        bool isReference();
        void setReference(std::string *);
        std::string * getReference();
        bool hasMoreValues();
        GAGenomGeneLocationDDBJ * getNextValue();
        void setValue(GAGenomGeneLocationDDBJ *);
        std::vector<GAGenomGeneLocationDDBJ> * getLocations();
    };

};

#endif // GAGENOMGENELOCATIONDDBJ_H
