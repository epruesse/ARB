/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMGENEGENBANK_H
#define GAGENOMGENEGENBANK_H

#include "GAGenomGene.h"
#include "GAGenomGeneLocationGenBank.h"

namespace gellisary{

    class GAGenomGeneGenBank : public GAGenomGene{
    private:
        GAGenomGeneLocationGenBank location;

    public:

        GAGenomGeneGenBank(){}
        virtual ~GAGenomGeneGenBank(){}
        virtual void parse();
        GAGenomGeneLocationGenBank * getLocation();
    };

};

#endif // GAGENOMGENEGENBANK_H
