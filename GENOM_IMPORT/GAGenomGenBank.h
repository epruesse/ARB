/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMGENBANK_H
#define GAGENOMGENBANK_H

#include "GAGenom.h"
#include "GAGenomReferenceGenBank.h"
#include "GAGenomFeatureTableGenBank.h"

namespace gellisary{

    class GAGenomGenBank : public GAGenom{
    private:
        std::vector<int> sequence_header;                   // embl:SQ  genbank,ddgj:BASE COUNT
        std::vector<gellisary::GAGenomReferenceGenBank> references;
        gellisary::GAGenomFeatureTableGenBank feature_table;
        int iter;
        GAGenomReferenceGenBank tmp_ref;
        std::string consortium;

    public:

        GAGenomGenBank(std::string *);
        virtual ~GAGenomGenBank(){}
        virtual void parseFlatFile();
        std::vector<int> * getSequenceHeader();
        gellisary::GAGenomFeatureTableGenBank * getFeatureTable();
        gellisary::GAGenomReferenceGenBank * getReference();
        virtual void parseSequence(std::string *);
    };
};

#endif // GAGENOMGENBANK_H
