/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMFEATURETABLESOURCEGENBANK_H
#define GAGENOMFEATURETABLESOURCEGENBANK_H

#include "GAGenomFeatureTableSource.h"

namespace gellisary{

    class GAGenomFeatureTableSourceGenBank : public GAGenomFeatureTableSource{
    public:

        //  GAGenomFeatureTableSourceGenBank();
        virtual ~GAGenomFeatureTableSourceGenBank(){}
        virtual void parse();
    };

};

#endif // GAGENOMFEATURETABLESOURCEGENBANK_H
