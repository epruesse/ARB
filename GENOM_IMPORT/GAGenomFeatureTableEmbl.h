/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMFEATURETABLEEMBL_H
#define GAGENOMFEATURETABLEEMBL_H

#include "GAGenomFeatureTable.h"
#include "GAGenomFeatureTableSourceEmbl.h"
#include "GAGenomGeneEmbl.h"

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

#ifndef _CPP_MAP
#include <map>
#endif

namespace gellisary {

    class GAGenomFeatureTableEmbl : public GAGenomFeatureTable{
    private:
        GAGenomFeatureTableSourceEmbl source;
        std::map<std::string,gellisary::GAGenomGeneEmbl> genes;
        std::vector<std::string> features;
        std::vector<int> number_of_features;
        std::map<std::string,gellisary::GAGenomGeneEmbl>::iterator iter;
        gellisary::GAGenomGeneEmbl tmp_genes;

        int nameToNumberOfFeature(std::string *);

    public:

        GAGenomFeatureTableEmbl();
        virtual ~GAGenomFeatureTableEmbl(){}
        virtual void parse();
        GAGenomFeatureTableSourceEmbl * getFeatureTableSource();
        std::string * getGeneName();
        GAGenomGeneEmbl * getGeneByName(std::string *);
        void setIterator();
    };

};

#endif // GAGENOMFEATURETABLEEMBL_H
