/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMFEATURETABLESOURCEDDBJ_H
#define GAGENOMFEATURETABLESOURCEDDBJ_H

#include "GAGenomFeatureTableSource.h"

namespace gellisary {

    class GAGenomFeatureTableSourceDDBJ : public GAGenomFeatureTableSource{
    public:

        //  GAGenomFeatureTableSourceDDBJ();
        virtual ~GAGenomFeatureTableSourceDDBJ(){}
        virtual void parse();
    };

};

#endif // GAGENOMFEATURETABLESOURCEDDBJ_H
