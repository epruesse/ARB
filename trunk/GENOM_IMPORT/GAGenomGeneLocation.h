/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef GAGENOMGENELOCATION_H
#define GAGENOMGENELOCATION_H

#include "GAParser.h"

namespace gellisary {

    class GAGenomGeneLocation : public GAParser{
    protected:
        bool range;
        bool complement;            //  complemnt(...)
        bool join;              //  join(...)
        bool order;             //  order(...)
        bool point;             //  x.y
        bool roof;              //  x^y
        bool single_value;
        bool collection;        //  x,y,...,z
        bool smaller_begin;     //  <x
        bool smaller_end;       //  x<
        bool bigger_begin;      //  >x
        bool bigger_end;        //  x>
        int value;
        std::string location_as_string;

    public:

        GAGenomGeneLocation(std::string *);
        GAGenomGeneLocation(){}
        virtual ~GAGenomGeneLocation(){}
        virtual void parse() = 0;
        bool isSingleValue();
        bool isRange();
        bool isJoin();
        bool isComplement();
        bool isOrder();
        bool isRoof();
        bool isPoint();
        bool isCollection();
        bool isSmallerBegin();
        bool isSmallerEnd();
        bool isBiggerBegin();
        bool isBiggerEnd();

        void setSingleValue(int);
        int getSingleValue();
    };

};

#endif // GAGENOMGENELOCATION_H
