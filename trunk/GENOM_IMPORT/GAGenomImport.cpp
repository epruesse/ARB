/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include <cstdio>
#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>
#include <awtc_rename.hxx>
#include <adGene.h>

#ifndef _CCP_IOSTREAM
#include <iostream>
#endif
#ifndef _CCP_IOMANIP
#include <iomanip>
#endif
#ifndef _CCP_FSTREAM
#include <fstream>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif
#ifndef _CPP_STRING
#include <string>
#endif

#include "GAGenomImport.h"
#include "GAParser.h"
#include "GAGenomUtilities.h"

#include "GAGenomEmbl.h"
#include "GAGenomFeatureTableSourceEmbl.h"
#include "GAGenomFeatureTableEmbl.h"
#include "GAGenomGeneEmbl.h"
#include "GAGenomGeneLocationEmbl.h"

#include "GAGenomGenBank.h"
#include "GAGenomFeatureTableSourceGenBank.h"
#include "GAGenomFeatureTableGenBank.h"
#include "GAGenomGeneGenBank.h"
#include "GAGenomGeneLocationGenBank.h"

#include "GAGenomDDBJ.h"
#include "GAGenomFeatureTableSourceDDBJ.h"
#include "GAGenomFeatureTableDDBJ.h"
#include "GAGenomGeneDDBJ.h"
#include "GAGenomGeneLocationDDBJ.h"

namespace gellisary {
    GB_ERROR executeQuery(GBDATA *, const char *,const char *);

    void writeReferenceEmbl(GBDATA *, gellisary::GAGenomReferenceEmbl *);
    void writeFeatureTableEmbl(GBDATA *, gellisary::GAGenomFeatureTableEmbl *);
    void writeGeneEmbl(GBDATA *, gellisary::GAGenomGeneEmbl *);
    bool writeLocationEmbl(GBDATA *, gellisary::GAGenomGeneLocationEmbl *, std::vector<int> *);
    void writeSourceEmbl(GBDATA *, gellisary::GAGenomFeatureTableSourceEmbl *);

    void writeReferenceGenBank(GBDATA *, gellisary::GAGenomReferenceGenBank *);
    void writeFeatureTableGenBank(GBDATA *, gellisary::GAGenomFeatureTableGenBank *);
    void writeGeneGenBank(GBDATA *, gellisary::GAGenomGeneGenBank *);
    bool writeLocationGenBank(GBDATA *, gellisary::GAGenomGeneLocationGenBank *, std::vector<int> *);
    void writeSourceGenBank(GBDATA *, gellisary::GAGenomFeatureTableSourceGenBank *);

    void writeReferenceDDBJ(GBDATA *, gellisary::GAGenomReferenceDDBJ *);
    void writeFeatureTableDDBJ(GBDATA *, gellisary::GAGenomFeatureTableDDBJ *);
    void writeGeneDDBJ(GBDATA *, gellisary::GAGenomGeneDDBJ *);
    bool writeLocationDDBJ(GBDATA *, gellisary::GAGenomGeneLocationDDBJ *, std::vector<int> *);
    void writeSourceDDBJ(GBDATA *, gellisary::GAGenomFeatureTableSourceDDBJ *);

    void writeByte(GBDATA *, std::string *, int);
    void writeString(GBDATA *, std::string *, std::string *);
    void writeInteger(GBDATA *, std::string *, int);
    void writeGenomSequence(GBDATA *, std::string *, const char *);
};

using namespace std;

/*
 * Functions for Embl Format Begin
 */

void gellisary::writeReferenceEmbl(GBDATA * source_container, gellisary::GAGenomReferenceEmbl * reference)
{
    std::string r_nummer("reference_");
    int tmp_int = reference->getNumber();
    r_nummer.append((GAGenomUtilities::integerToString(tmp_int)));
    r_nummer.append("_");

    std::string * tmp_str_pnt;
    std::vector<int> * tmp_int_vec_1;
    std::vector<int> * tmp_int_vec_2;

    tmp_str_pnt = reference->getAuthorsAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("authors");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_int_vec_1 = reference->getPositionBegin();
    tmp_int_vec_2 = reference->getPositionEnd();
    if((tmp_int_vec_1->size() > 0) && (tmp_int_vec_2->size() > 0) && (tmp_int_vec_1->size() == tmp_int_vec_2->size()))
    {
        for(int i = 0; i < (int)tmp_int_vec_1->size(); i++)
        {
            std::string tmp1_str = r_nummer;
            std::string tmp2_str = r_nummer;
            tmp1_str.append("position_begin_");
            tmp1_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            tmp2_str.append("position_end_");
            tmp2_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            gellisary::writeInteger(source_container,&tmp1_str,(*tmp_int_vec_1)[i]);
            gellisary::writeInteger(source_container,&tmp2_str,(*tmp_int_vec_2)[i]);
        }
    }
    tmp_str_pnt = reference->getComment();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("comment");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getCrossReferenceAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("cross_reference");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getGroup();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("group");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getLocation();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("location");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getTitle();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("title");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
}

void gellisary::writeFeatureTableEmbl(GBDATA * source_container, gellisary::GAGenomFeatureTableEmbl * feature_table)
{
    gellisary::writeSourceEmbl(source_container,feature_table->getFeatureTableSource());
    gellisary::GAGenomGeneEmbl * tmp_gene;
    std::string * tmp_str;
    feature_table->setIterator();
    while((tmp_str = feature_table->getGeneName()) != NULL)
    {
        tmp_gene = feature_table->getGeneByName(tmp_str);
        gellisary::writeGeneEmbl(source_container,tmp_gene);
    }
}

void gellisary::writeGeneEmbl(GBDATA * source_container, gellisary::GAGenomGeneEmbl * gene)
{
    gellisary::GAGenomGeneEmbl tmp_gene = *gene;
    std::string tmp_str;
    tmp_gene.setIterator();
    int tmp_int;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    GBDATA * gene_container;
    tmp_str_pnt = tmp_gene.getNameOfGene();
    if(((*tmp_str_pnt) != "none") && ((*tmp_str_pnt) != ""))
    {
        tmp_str = "name";
        gene_container = GEN_create_gene(source_container, tmp_str_pnt->c_str());
        //      gene_container = GB_create_container(source_container, tmp_str_pnt->c_str());
        //      gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        tmp_int = tmp_gene.getGeneNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getGeneType();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "type";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        tmp_int = tmp_gene.getGeneTypeNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_type_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getLocationAsString();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "location";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        while((tmp_str_pnt = tmp_gene.getQualifierName()) != NULL)
        {
            if(*tmp_str_pnt != "translation")
            {
                tmp_str_pnt2 = tmp_gene.getQualifierValue(tmp_str_pnt);
                if(!(*tmp_str_pnt2).empty())
                {
                    gellisary::writeString(gene_container,tmp_str_pnt,tmp_str_pnt2);
                }
                else
                {
                    tmp_str = "yes";
                    gellisary::writeString(gene_container,tmp_str_pnt,&tmp_str);
                }
            }
        }

        GAGenomGeneLocationEmbl * tmp_location;
        GAGenomGeneLocationEmbl tmp_location2;
        tmp_location = tmp_gene.getLocation();
        string t3_str;
        int t3_int;
        std::vector<gellisary::GAGenomGeneLocationEmbl> * t_locs;
        std::vector<int> joined;
        int gene_length = 0;
        if(tmp_location->isSingleValue())
        {
            t3_int = tmp_location->getSingleValue();
            t3_str = "pos_begin";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "pos_end";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "gene_length";
            gellisary::writeInteger(gene_container,&t3_str,1);
        }
        else
        {
            t_locs = tmp_location->getLocations();

            for(int m = 0; m < (int)t_locs->size(); m++)
            {
                tmp_location2 = t_locs->operator[](m);
                if(gellisary::writeLocationEmbl(gene_container,&tmp_location2,&joined))
                {
                    t3_str = "complement";
                    gellisary::writeByte(gene_container,&t3_str,1);
                }
                else
                {
                    if(tmp_location->isComplement())
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,1);
                    }
                    else
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,0);
                    }
                }
            }
            if((int)joined.size() > 2)
            {
                t3_str = "pos_joined";
                gellisary::writeInteger(gene_container,&t3_str,((int)joined.size()/2));
                for(int k = 0; k < ((int) joined.size()/2); k++)
                {
                    if(k == 0)
                    {
                        t3_str = "pos_begin";
                        t3_int = joined[k];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end";
                        t3_int = joined[k+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                    else
                    {
                        t3_str = "pos_begin"+GAGenomUtilities::integerToString((k+1));
                        t3_int = joined[(k*2)];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end"+GAGenomUtilities::integerToString((k+1));
                        t3_int = joined[(k*2)+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                }
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
            else if((int)joined.size() > 0)
            {
                t3_str = "pos_begin";
                t3_int = joined[0];
                gene_length -= t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "pos_end";
                t3_int = joined[1];
                gene_length += t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
        }
    }
}

bool gellisary::writeLocationEmbl(GBDATA * source_container, gellisary::GAGenomGeneLocationEmbl * location, std::vector<int> * joined)
{
    GAGenomGeneLocationEmbl tmp1_location = *location;
    GAGenomGeneLocationEmbl tmp2_location;
    std::vector<gellisary::GAGenomGeneLocationEmbl> * locations;
    std::string tmp_str;
    int tmp_int;
    bool comp = false;
    if(tmp1_location.isSingleValue())
    {
        tmp_int = tmp1_location.getSingleValue();
        joined->push_back(tmp_int);
    }
    else
    {
        if(location->isComplement())
        {
            comp = true;
        }
        locations = tmp1_location.getLocations();
        for(int i = 0; i < (int) locations->size(); i++)
        {
            tmp2_location = locations->operator[](i);
            if(gellisary::writeLocationEmbl(source_container,&tmp2_location,joined))
            {
                comp = true;
            }
        }
    }
    return comp;
}

void gellisary::writeSourceEmbl(GBDATA * source_container, gellisary::GAGenomFeatureTableSourceEmbl * source)
{
    std::string tmp_str;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    source->setIterator();
    while((tmp_str_pnt = source->getNameOfQualifier()) != NULL)
    {
        tmp_str_pnt2 = source->getValueOfQualifier(tmp_str_pnt);
        if(!(*tmp_str_pnt2).empty())
        {
            gellisary::writeString(source_container,tmp_str_pnt,tmp_str_pnt2);
        }
        else
        {
            tmp_str = "yes";
            gellisary::writeString(source_container,tmp_str_pnt,&tmp_str);
        }
    }
}

/*
 * Functions for Embl Format End
 */

/************************************************* *
 * *********************************************** *
 * *********************************************** *
 * *********************************************** *
 * *************************************************/

/*
 * Functions for GenBank Format Begin
 */

void gellisary::writeReferenceGenBank(GBDATA * source_container, gellisary::GAGenomReferenceGenBank * reference)
{
    std::string r_nummer("reference_");
    int tmp_int = reference->getNumber();
    r_nummer.append((GAGenomUtilities::integerToString(tmp_int)));
    r_nummer.append("_");
    std::string * tmp_str_pnt;
    std::vector<int> * tmp_int_vec_1;
    std::vector<int> * tmp_int_vec_2;
    tmp_str_pnt = reference->getAuthorsAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("authors");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_int_vec_1 = reference->getPositionBegin();
    tmp_int_vec_2 = reference->getPositionEnd();
    if((tmp_int_vec_1->size() > 0) && (tmp_int_vec_2->size() > 0) && (tmp_int_vec_1->size() == tmp_int_vec_2->size()))
    {

        for(int i = 0; i < (int)tmp_int_vec_1->size(); i++)
        {
            std::string tmp1_str = r_nummer;
            std::string tmp2_str = r_nummer;
            tmp1_str.append("position_begin_");
            tmp1_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            tmp2_str.append("position_end_");
            tmp2_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            gellisary::writeInteger(source_container,&tmp1_str,(*tmp_int_vec_1)[i]);
            gellisary::writeInteger(source_container,&tmp2_str,(*tmp_int_vec_2)[i]);
        }
    }
    tmp_str_pnt = reference->getComment();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("comment");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getCrossReferenceAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("cross_reference");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    /*tmp_str_pnt = reference->getGroup();
      if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
      {
      std::string tmp_str = r_nummer;
      tmp_str.append("group");
      gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
      }*/
    tmp_str_pnt = reference->getLocation();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("location");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getTitle();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("title");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
}

void gellisary::writeFeatureTableGenBank(GBDATA * source_container, gellisary::GAGenomFeatureTableGenBank * feature_table)
{
    gellisary::writeSourceGenBank(source_container,feature_table->getFeatureTableSource());
    gellisary::GAGenomGeneGenBank * tmp_gene;
    std::string * tmp_str;
    feature_table->setIterator();
    while((tmp_str = feature_table->getGeneName()) != NULL)
    {
        tmp_gene = feature_table->getGeneByName(tmp_str);
        gellisary::writeGeneGenBank(source_container,tmp_gene);
    }
}

void gellisary::writeGeneGenBank(GBDATA * source_container, gellisary::GAGenomGeneGenBank * gene)
{
    gellisary::GAGenomGeneGenBank tmp_gene = *gene;
    std::string tmp_str;
    tmp_gene.setIterator();
    int tmp_int;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    GBDATA * gene_container;
    tmp_str_pnt = tmp_gene.getNameOfGene();
    if(((*tmp_str_pnt) != "none") && ((*tmp_str_pnt) != ""))
    {
        tmp_str = "name";
        gene_container = GEN_create_gene(source_container, tmp_str_pnt->c_str());
        //      gene_container = GB_create_container(source_container, tmp_str_pnt->c_str());
        //      gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        tmp_int = tmp_gene.getGeneNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getGeneType();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "type";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        tmp_int = tmp_gene.getGeneTypeNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_type_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getLocationAsString();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "location";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        while((tmp_str_pnt = tmp_gene.getQualifierName()) != NULL)
        {
            if(*tmp_str_pnt != "translation")
            {
                tmp_str_pnt2 = tmp_gene.getQualifierValue(tmp_str_pnt);
                if(!(*tmp_str_pnt2).empty())
                {
                    gellisary::writeString(gene_container,tmp_str_pnt,tmp_str_pnt2);
                }
                else
                {
                    tmp_str = "yes";
                    gellisary::writeString(gene_container,tmp_str_pnt,&tmp_str);
                }
            }
        }

        GAGenomGeneLocationGenBank * tmp_location;
        GAGenomGeneLocationGenBank tmp_location2;
        tmp_location = tmp_gene.getLocation();
        string t3_str;
        int t3_int;
        std::vector<gellisary::GAGenomGeneLocationGenBank> * t_locs;
        std::vector<int> joined;
        int gene_length = 0;
        if(tmp_location->isSingleValue())
        {
            t3_int = tmp_location->getSingleValue();
            t3_str = "pos_begin";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "pos_end";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "gene_length";
            gellisary::writeInteger(gene_container,&t3_str,1);
        }
        else
        {
            t_locs = tmp_location->getLocations();

            for(int m = 0; m < (int)t_locs->size(); m++)
            {
                tmp_location2 = t_locs->operator[](m);
                if(gellisary::writeLocationGenBank(gene_container,&tmp_location2,&joined))
                {
                    t3_str = "complement";
                    gellisary::writeByte(gene_container,&t3_str,1);
                }
                else
                {
                    if(tmp_location->isComplement())
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,1);
                    }
                    else
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,0);
                    }
                }
            }
            if((int)joined.size() > 2)
            {
                t3_str = "pos_joined";
                gellisary::writeInteger(gene_container,&t3_str,((int)joined.size()/2));
                for(int k = 0; k < (int) joined.size(); k+=2)
                {
                    if(k == 0)
                    {
                        t3_str = "pos_begin";
                        t3_int = joined[k];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end";
                        t3_int = joined[k+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                    else
                    {
                        t3_str = "pos_begin"+GAGenomUtilities::integerToString(k);
                        t3_int = joined[k];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end"+GAGenomUtilities::integerToString(k);
                        t3_int = joined[k+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                }
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
            else if((int)joined.size() > 0)
            {
                t3_str = "pos_begin";
                t3_int = joined[0];
                gene_length -= t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "pos_end";
                t3_int = joined[1];
                gene_length += t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
        }
    }
}

bool gellisary::writeLocationGenBank(GBDATA * source_container, gellisary::GAGenomGeneLocationGenBank * location, std::vector<int> * joined)
{
    GAGenomGeneLocationGenBank tmp1_location = *location;
    GAGenomGeneLocationGenBank tmp2_location;
    std::vector<gellisary::GAGenomGeneLocationGenBank> * locations;
    std::string tmp_str;
    int tmp_int;
    bool comp = false;
    if(tmp1_location.isSingleValue())
    {
        tmp_int = tmp1_location.getSingleValue();
        joined->push_back(tmp_int);
    }
    else
    {
        if(location->isComplement())
        {
            comp = true;
        }
        locations = tmp1_location.getLocations();
        for(int i = 0; i < (int) locations->size(); i++)
        {
            tmp2_location = locations->operator[](i);
            if(gellisary::writeLocationGenBank(source_container,&tmp2_location,joined))
            {
                comp = true;
            }
        }
    }
    return comp;
}

void gellisary::writeSourceGenBank(GBDATA * source_container, gellisary::GAGenomFeatureTableSourceGenBank * source)
{
    std::string tmp_str;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    source->setIterator();
    while((tmp_str_pnt = source->getNameOfQualifier()) != NULL)
    {
        tmp_str_pnt2 = source->getValueOfQualifier(tmp_str_pnt);
        if(!(*tmp_str_pnt2).empty())
        {
            gellisary::writeString(source_container,tmp_str_pnt,tmp_str_pnt2);
        }
        else
        {
            tmp_str = "yes";
            gellisary::writeString(source_container,tmp_str_pnt,&tmp_str);
        }
    }
}

/*
 * Functions for GenBankFormat End
 */

/************************************************* *
 * *********************************************** *
 * *********************************************** *
 * *********************************************** *
 * *************************************************/

/*
 * Functions for DDBJ Format Begin
 */

void gellisary::writeReferenceDDBJ(GBDATA * source_container, gellisary::GAGenomReferenceDDBJ * reference)
{
    std::string r_nummer("reference_");
    int tmp_int = reference->getNumber();
    r_nummer.append((GAGenomUtilities::integerToString(tmp_int)));
    r_nummer.append("_");
    std::string * tmp_str_pnt;
    std::vector<int> * tmp_int_vec_1;
    std::vector<int> * tmp_int_vec_2;
    tmp_str_pnt = reference->getAuthorsAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("authors");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_int_vec_1 = reference->getPositionBegin();
    tmp_int_vec_2 = reference->getPositionEnd();
    if((tmp_int_vec_1->size() > 0) && (tmp_int_vec_2->size() > 0) && (tmp_int_vec_1->size() == tmp_int_vec_2->size()))
    {

        for(int i = 0; i < (int)tmp_int_vec_1->size(); i++)
        {
            std::string tmp1_str = r_nummer;
            std::string tmp2_str = r_nummer;
            tmp1_str.append("position_begin_");
            tmp1_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            tmp2_str.append("position_end_");
            tmp2_str.append(gellisary::GAGenomUtilities::integerToString((i+1)));
            gellisary::writeInteger(source_container,&tmp1_str,(*tmp_int_vec_1)[i]);
            gellisary::writeInteger(source_container,&tmp2_str,(*tmp_int_vec_2)[i]);
        }
    }
    tmp_str_pnt = reference->getComment();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("comment");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getCrossReferenceAsString();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("cross_reference");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    /*tmp_str_pnt = reference->getGroup();
      if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
      {
      std::string tmp_str = r_nummer;
      tmp_str.append("group");
      gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
      }*/
    tmp_str_pnt = reference->getLocation();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("location");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
    tmp_str_pnt = reference->getTitle();
    if((*tmp_str_pnt != "none") && (!(*tmp_str_pnt).empty()))
    {
        std::string tmp_str = r_nummer;
        tmp_str.append("title");
        gellisary::writeString(source_container,&tmp_str,tmp_str_pnt);
    }
}

void gellisary::writeFeatureTableDDBJ(GBDATA * source_container, gellisary::GAGenomFeatureTableDDBJ * feature_table)
{
    gellisary::writeSourceDDBJ(source_container,feature_table->getFeatureTableSource());
    gellisary::GAGenomGeneDDBJ * tmp_gene;
    std::string * tmp_str;
    feature_table->setIterator();
    while((tmp_str = feature_table->getGeneName()) != NULL)
    {
        tmp_gene = feature_table->getGeneByName(tmp_str);
        gellisary::writeGeneDDBJ(source_container,tmp_gene);
    }
}

void gellisary::writeGeneDDBJ(GBDATA * source_container, gellisary::GAGenomGeneDDBJ * gene)
{
    gellisary::GAGenomGeneDDBJ tmp_gene = *gene;
    std::string tmp_str;
    tmp_gene.setIterator();
    int tmp_int;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    GBDATA * gene_container;
    tmp_str_pnt = tmp_gene.getNameOfGene();
    if(((*tmp_str_pnt) != "none") && ((*tmp_str_pnt) != ""))
    {
        tmp_str = "name";
        gene_container = GEN_create_gene(source_container, tmp_str_pnt->c_str());
        //      gene_container = GB_create_container(source_container, tmp_str_pnt->c_str());
        //      gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        tmp_int = tmp_gene.getGeneNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getGeneType();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "type";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        tmp_int = tmp_gene.getGeneTypeNumber(); // int
        if(tmp_int != -1)
        {
            tmp_str = "gene_type_number";
            gellisary::writeInteger(gene_container,&tmp_str,tmp_int);
        }
        tmp_str_pnt = tmp_gene.getLocationAsString();
        if(*tmp_str_pnt != "none")
        {
            tmp_str = "location";
            gellisary::writeString(gene_container,&tmp_str,tmp_str_pnt);
        }
        while((tmp_str_pnt = tmp_gene.getQualifierName()) != NULL)
        {
            if(*tmp_str_pnt != "translation")
            {
                tmp_str_pnt2 = tmp_gene.getQualifierValue(tmp_str_pnt);
                if(!(*tmp_str_pnt2).empty())
                {
                    gellisary::writeString(gene_container,tmp_str_pnt,tmp_str_pnt2);
                }
                else
                {
                    tmp_str = "yes";
                    gellisary::writeString(gene_container,tmp_str_pnt,&tmp_str);
                }
            }
        }

        GAGenomGeneLocationDDBJ * tmp_location;
        GAGenomGeneLocationDDBJ tmp_location2;
        tmp_location = tmp_gene.getLocation();
        string t3_str;
        int t3_int;
        std::vector<gellisary::GAGenomGeneLocationDDBJ> * t_locs;
        std::vector<int> joined;
        int gene_length = 0;
        if(tmp_location->isSingleValue())
        {
            t3_int = tmp_location->getSingleValue();
            t3_str = "pos_begin";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "pos_end";
            gellisary::writeInteger(gene_container,&t3_str,t3_int);
            t3_str = "gene_length";
            gellisary::writeInteger(gene_container,&t3_str,1);
        }
        else
        {
            t_locs = tmp_location->getLocations();

            for(int m = 0; m < (int)t_locs->size(); m++)
            {
                tmp_location2 = t_locs->operator[](m);
                if(gellisary::writeLocationDDBJ(gene_container,&tmp_location2,&joined))
                {
                    t3_str = "complement";
                    gellisary::writeByte(gene_container,&t3_str,1);
                }
                else
                {
                    if(tmp_location->isComplement())
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,1);
                    }
                    else
                    {
                        t3_str = "complement";
                        gellisary::writeByte(gene_container,&t3_str,0);
                    }
                }
            }
            if((int)joined.size() > 2)
            {
                t3_str = "pos_joined";
                gellisary::writeInteger(gene_container,&t3_str,((int)joined.size()/2));
                for(int k = 0; k < (int) joined.size(); k+=2)
                {
                    if(k == 0)
                    {
                        t3_str = "pos_begin";
                        t3_int = joined[k];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end";
                        t3_int = joined[k+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                    else
                    {
                        t3_str = "pos_begin"+GAGenomUtilities::integerToString(k);
                        t3_int = joined[k];
                        gene_length -= t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                        t3_str = "pos_end"+GAGenomUtilities::integerToString(k);
                        t3_int = joined[k+1];
                        gene_length += t3_int;
                        gellisary::writeInteger(gene_container,&t3_str,t3_int);
                    }
                }
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
            else if((int)joined.size() > 0)
            {
                t3_str = "pos_begin";
                t3_int = joined[0];
                gene_length -= t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "pos_end";
                t3_int = joined[1];
                gene_length += t3_int;
                gellisary::writeInteger(gene_container,&t3_str,t3_int);
                t3_str = "gene_length";
	            gellisary::writeInteger(gene_container,&t3_str,gene_length);
            }
        }
    }
}

bool gellisary::writeLocationDDBJ(GBDATA * source_container, gellisary::GAGenomGeneLocationDDBJ * location, std::vector<int> * joined)
{
    GAGenomGeneLocationDDBJ tmp1_location = *location;
    GAGenomGeneLocationDDBJ tmp2_location;
    std::vector<gellisary::GAGenomGeneLocationDDBJ> * locations;
    std::string tmp_str;
    int tmp_int;
    bool comp = false;
    if(tmp1_location.isSingleValue())
    {
        tmp_int = tmp1_location.getSingleValue();
        joined->push_back(tmp_int);
    }
    else
    {
        if(location->isComplement())
        {
            comp = true;
        }
        locations = tmp1_location.getLocations();
        for(int i = 0; i < (int) locations->size(); i++)
        {
            tmp2_location = locations->operator[](i);
            if(gellisary::writeLocationDDBJ(source_container,&tmp2_location,joined))
            {
                comp = true;
            }
        }
    }
    return comp;
}

void gellisary::writeSourceDDBJ(GBDATA * source_container, gellisary::GAGenomFeatureTableSourceDDBJ * source)
{
    std::string tmp_str;
    std::string * tmp_str_pnt;
    std::string * tmp_str_pnt2;
    source->setIterator();
    while((tmp_str_pnt = source->getNameOfQualifier()) != NULL)
    {
        tmp_str_pnt2 = source->getValueOfQualifier(tmp_str_pnt);
        if(!(*tmp_str_pnt2).empty())
        {
            gellisary::writeString(source_container,tmp_str_pnt,tmp_str_pnt2);
        }
        else
        {
            tmp_str = "yes";
            gellisary::writeString(source_container,tmp_str_pnt,&tmp_str);
        }
    }
}

/*
 * Functions for DDBJFormat End
 */

void gellisary::writeByte(GBDATA * source_container, std::string * target_field, int source_int)
{
    GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_BYTE);
    GB_write_byte(gb_field, source_int);
}

void gellisary::writeString(GBDATA * source_container, std::string * target_field, std::string * source_str)
{
    GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_STRING);
    GB_write_string(gb_field, source_str->c_str());
}

void gellisary::writeInteger(GBDATA * source_container, std::string * target_field, int source_int)
{
    GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_INT);
    GB_write_int(gb_field, source_int);
}

void gellisary::writeGenomSequence(GBDATA * source_container, std::string * source_str, const char * alignment_name)
{
    GB_write_string(GBT_add_data(source_container, alignment_name, "data", GB_STRING), source_str->c_str());
}

GB_ERROR gellisary::executeQuery(GBDATA * gb_main, const char * file_name, const char * ali_name)
{
    GB_ERROR error = NULL;
    int file_name_size = strlen(file_name);

    std::string extension;
    std::string flatfile_name;

    for(int i = (file_name_size-1); i >= 0; i--)
    {
        if(file_name[i] == '.')
        {
            for(int j = (i+1); j < file_name_size; j++)
            {
                extension += file_name[j];
            }
        }
    }
    for(int i = (file_name_size-1); i >= 0; i--)
    {
        if(file_name[i] == '/')
        {
            for(int j = (i+1); j < file_name_size; j++)
            {
                flatfile_name += file_name[j];
            }
            break;
        }
    }
    if((extension != "embl") && (extension != "gbk") && (extension != "ff"))
    {
        ifstream flatfile(file_name);
        char tmp_line[128];
        flatfile.getline(tmp_line,128);
        if(strlen(tmp_line) > 0)
        {
            if((tmp_line[0] == 'I') && (tmp_line[1] == 'D') && (tmp_line[2] == ' '))
            {
                extension = "embl";
            }
            else if((tmp_line[0] == 'L') && (tmp_line[1] == 'O') && (tmp_line[2] == 'C') && (tmp_line[3] == 'U') && (tmp_line[4] == 'S') && (tmp_line[5] == ' '))
            {
                extension = "gbk";
            }
        }
        flatfile.close();
    }

    if(extension == "embl")
    {
        string ffname = file_name;
        gellisary::GAGenomEmbl genomembl(&ffname);
        genomembl.parseFlatFile();
        if(genomembl.isFileComplete())
        {
            GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
            char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
            GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);

            std::vector<std::string> tmp_str_vector;
            std::vector<int> tmp_int_vector;
            std::string tmp_string;
            std::string * tmp_string_pnt;

            std::string field;
            field = "identification";
            tmp_string_pnt = genomembl.getIdentification();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "name_of_flatfile";
            tmp_string_pnt = &flatfile_name;
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "identification";
            tmp_string = "embl";
            field = "source_database";
            gellisary::writeString(gb_species,&field,&tmp_string);
            field = "accession_number";
            tmp_string_pnt = genomembl.getAccessionNumber();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "free_text_comment";
            tmp_string_pnt = genomembl.getCommentAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "contig";
            tmp_string_pnt = genomembl.getContig();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "database_cross_reference";
            tmp_string_pnt = genomembl.getDataCrossReferenceAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "date_of_creation";
            tmp_string_pnt = genomembl.getDateOfCreation();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "date_of_last_update";
            tmp_string_pnt = genomembl.getDateOfLastUpdate();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "description";
            tmp_string_pnt = genomembl.getDescription();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "key_words";
            tmp_string_pnt = genomembl.getKeyWordsAsString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "orgarnelle";
            tmp_string_pnt = genomembl.getOrganelle();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "full_name";
            tmp_string_pnt = genomembl.getOrganism();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "organism_classification";
            tmp_string_pnt = genomembl.getOrganismClassificationAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "sequence_version";
            tmp_string_pnt = genomembl.getSequenceVersion();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            gellisary::writeGenomSequence(gb_species,genomembl.getSequence(),ali_name);

            tmp_int_vector = *(genomembl.getSequenceHeader());
            tmp_str_vector.clear();
            tmp_str_vector.push_back("sequence_length");
            tmp_str_vector.push_back("sequence_a");
            tmp_str_vector.push_back("sequence_c");
            tmp_str_vector.push_back("sequence_g");
            tmp_str_vector.push_back("sequence_t");
            tmp_str_vector.push_back("sequence_other");
            int tmp_int;
            for(int i = 0; i < (int)tmp_int_vector.size(); i++)
            {
                field = tmp_str_vector[i];
                tmp_int = tmp_int_vector[i];
                if(tmp_int != 0)
                {
                    gellisary::writeInteger(gb_species,&field,tmp_int);
                }
            }
            gellisary::writeFeatureTableEmbl(gb_species,genomembl.getFeatureTable());
            GAGenomReferenceEmbl * tmp_reference;
            while((tmp_reference = genomembl.getReference()) != NULL)
            {
                gellisary::writeReferenceEmbl(gb_species,tmp_reference);
            }
            delete (tmp_reference);
        }
        else
        {
            GB_warning("The Embl flatfile is incomplete!");
        }
    }
    else if(extension == "gbk")
    {
        string ffname = file_name;
        gellisary::GAGenomGenBank genomgenbank(&ffname);
        genomgenbank.parseFlatFile();
        if(genomgenbank.isFileComplete())
        {
            GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
            char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
            GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);

            std::vector<std::string> tmp_str_vector;
            std::vector<int> tmp_int_vector;
            std::string tmp_string;
            std::string * tmp_string_pnt;
    
            std::string field;
            field = "identification";
            tmp_string_pnt = genomgenbank.getIdentification();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "name_of_flatfile";
            tmp_string_pnt = &flatfile_name;
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            tmp_string = "genbank";
            field = "source_database";
            gellisary::writeString(gb_species,&field,&tmp_string);
            field = "accession_number";
            tmp_string_pnt = genomgenbank.getAccessionNumber();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "free_text_comment";
            tmp_string_pnt = genomgenbank.getCommentAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "contig";
            tmp_string_pnt = genomgenbank.getContig();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            /*field = "database_cross_reference";
              tmp_string_pnt = genomgenbank.getDataCrossReferenceAsOneString();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            /*field = "date_of_creation";
              tmp_string_pnt = genomgenbank.getDateOfCreation();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            /*field = "date_of_last_update";
              tmp_string_pnt = genomgenbank.getDateOfLastUpdate();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            field = "description";
            tmp_string_pnt = genomgenbank.getDescription();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "key_words";
            tmp_string_pnt = genomgenbank.getKeyWordsAsString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            /*field = "orgarnelle";
              tmp_string_pnt = genomgenbank.getOrganelle();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            field = "full_name";
            tmp_string_pnt = genomgenbank.getOrganism();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "organism_classification";
            tmp_string_pnt = genomgenbank.getOrganismClassificationAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "sequence_version";
            tmp_string_pnt = genomgenbank.getSequenceVersion();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            gellisary::writeGenomSequence(gb_species,genomgenbank.getSequence(),ali_name);
    
            tmp_int_vector = *(genomgenbank.getSequenceHeader());
            tmp_str_vector.clear();
            tmp_str_vector.push_back("sequence_a");
            tmp_str_vector.push_back("sequence_c");
            tmp_str_vector.push_back("sequence_g");
            tmp_str_vector.push_back("sequence_t");
            int tmp_int;
            for(int i = 0; i < (int)tmp_int_vector.size(); i++)
            {
                field = tmp_str_vector[i];
                tmp_int = tmp_int_vector[i];
                if(tmp_int != 0)
                {
                    gellisary::writeInteger(gb_species,&field,tmp_int);
                }
            }
            field = "sequence_length";
            tmp_int = 0;
            for(int i = 0; i < (int)tmp_int_vector.size(); i++)
            {
                tmp_int += tmp_int_vector[i];
            }
            gellisary::writeInteger(gb_species,&field,tmp_int);
            gellisary::writeFeatureTableGenBank(gb_species,genomgenbank.getFeatureTable());
            GAGenomReferenceGenBank * tmp_reference;
            while((tmp_reference = genomgenbank.getReference()) != NULL)
            {
                gellisary::writeReferenceGenBank(gb_species,tmp_reference);
            }
            delete (tmp_reference);
        }
        else
        {
            GB_warning("The GenBank flatfile is incomplete!");
        }
    }
    else if(extension == "ff")
    {
        string ffname = file_name;
        gellisary::GAGenomDDBJ genomddbj(&ffname);
        genomddbj.parseFlatFile();
        if(genomddbj.isFileComplete())
        {
            GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
            char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
            GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);
    
            std::vector<std::string> tmp_str_vector;
            std::vector<int> tmp_int_vector;
            std::string tmp_string;
            std::string * tmp_string_pnt;
    
            std::string field;
            field = "identification";
            tmp_string_pnt = genomddbj.getIdentification();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "name_of_flatfile";
            tmp_string_pnt = &flatfile_name;
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            tmp_string = "ddbj";
            field = "source_database";
            gellisary::writeString(gb_species,&field,&tmp_string);
            field = "accession_number";
            tmp_string_pnt = genomddbj.getAccessionNumber();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "free_text_comment";
            tmp_string_pnt = genomddbj.getCommentAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "contig";
            tmp_string_pnt = genomddbj.getContig();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            /*field = "database_cross_reference";
              tmp_string_pnt = genomddbj.getDataCrossReferenceAsOneString();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            /*field = "date_of_creation";
              tmp_string_pnt = genomddbj.getDateOfCreation();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            /*field = "date_of_last_update";
              tmp_string_pnt = genomddbj.getDateOfLastUpdate();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            field = "description";
            tmp_string_pnt = genomddbj.getDescription();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "key_words";
            tmp_string_pnt = genomddbj.getKeyWordsAsString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            /*field = "orgarnelle";
              tmp_string_pnt = genomddbj.getOrganelle();
              if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
              {
              gellisary::writeString(gb_species,&field,tmp_string_pnt);
              }*/
            field = "full_name";
            tmp_string_pnt = genomddbj.getOrganism();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "organism_classification";
            tmp_string_pnt = genomddbj.getOrganismClassificationAsOneString();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            field = "sequence_version";
            tmp_string_pnt = genomddbj.getSequenceVersion();
            if((*tmp_string_pnt != "none") && (!(*tmp_string_pnt).empty()))
            {
                gellisary::writeString(gb_species,&field,tmp_string_pnt);
            }
            gellisary::writeGenomSequence(gb_species,genomddbj.getSequence(),ali_name);
    
            tmp_int_vector = *(genomddbj.getSequenceHeader());
            tmp_str_vector.clear();
            tmp_str_vector.push_back("sequence_a");
            tmp_str_vector.push_back("sequence_c");
            tmp_str_vector.push_back("sequence_g");
            tmp_str_vector.push_back("sequence_t");
            int tmp_int;
            for(int i = 0; i < (int)tmp_int_vector.size(); i++)
            {
                field = tmp_str_vector[i];
                tmp_int = tmp_int_vector[i];
                if(tmp_int != 0)
                {
                    gellisary::writeInteger(gb_species,&field,tmp_int);
                }
            }
            field = "sequence_length";
            tmp_int = 0;
            for(int i = 0; i < (int)tmp_int_vector.size(); i++)
            {
                tmp_int += tmp_int_vector[i];
            }
            gellisary::writeInteger(gb_species,&field,tmp_int);
            gellisary::writeFeatureTableDDBJ(gb_species,genomddbj.getFeatureTable());
            GAGenomReferenceDDBJ * tmp_reference;
            while((tmp_reference = genomddbj.getReference()) != NULL)
            {
                gellisary::writeReferenceDDBJ(gb_species,tmp_reference);
            }
            delete (tmp_reference);
        }
        else
        {
            GB_warning("The DDBJ flatfile : is incomplete!");
        }
    }
    return error;
}


GB_ERROR GI_importGenomeFile(GBDATA * gb_main, const char * file_name, const char * ali_name) {
    return gellisary::executeQuery(gb_main, file_name, ali_name);
}

