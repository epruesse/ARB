/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMFEATURETABLE_H
#define GAGENOMFEATURETABLE_H

#include "GAParser.h"

namespace gellisary {

    class GAGenomFeatureTable : public GAParser{
    protected:
        int number_of_genes;
        std::string tmp_key;
        //  std::vector<std::string> features;

    public:

        GAGenomFeatureTable();
        virtual ~GAGenomFeatureTable(){}
        virtual void parse()=0;
    };

};

#endif // GAGENOMFEATURETABLE_H
