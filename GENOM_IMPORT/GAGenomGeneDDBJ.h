/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMGENEDDBJ_H
#define GAGENOMGENEDDBJ_H

#include "GAGenomGene.h"
#include "GAGenomGeneLocationDDBJ.h"

namespace gellisary{

    class GAGenomGeneDDBJ : public GAGenomGene{
    private:
        GAGenomGeneLocationDDBJ location;

    public:

        GAGenomGeneDDBJ(){}
        virtual ~GAGenomGeneDDBJ(){}
        virtual void parse();
        GAGenomGeneLocationDDBJ * getLocation();
    };

};

#endif // GAGENOMGENEDDBJ_H
