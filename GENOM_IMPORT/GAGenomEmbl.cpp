/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomEmbl.h"

#include "GAGenomUtilities.h"

#ifndef _CCP_IOSTREAM
#include <iostream>
#endif
#ifndef _CCP_IOMANIP
#include <iomanip>
#endif
#ifndef _CCP_FSTREAM
#include <fstream>
#endif
#ifndef _CCP_S STREAM
#include <sstream>
#endif

#include <perf_timer.h>

using namespace std;
using namespace gellisary;

gellisary::GAGenomEmbl::GAGenomEmbl(string * fname):GAGenom(fname)
{
    iter = 0;
}

void gellisary::GAGenomEmbl::parseFlatFile()
{
    string tmp_str;
    string del_str;
    string rep_str;
    vector<string> tmp_vector;
    vector<string> tmp_lines_vector;
    string t_str;
    int seq_len = 0;

    GAGenomReferenceEmbl *tmp_reference;

    bool dt = false;
    bool de = false;
    bool kw = false;
    bool oc = false;
    bool co_cc = false;
    bool rn = false;
    bool cc = false;
    bool co = false;
    bool ft = false;
    bool sq = false;

    char tmp_line[128];


#if defined(DEBUG)
    {
        PerfTimer flatFileReadTime("plain-reading complete flatfile");
        ifstream flatfile2(file_name.c_str());
        while(!flatfile2.eof())
        {
            flatfile2.getline(tmp_line,128);
        }
    }
    
    PerfTimer flatFileParse("parse flatfile");
#endif // DEBUG

    ifstream flatfile(file_name.c_str());
    while(!flatfile.eof())
    {
#if defined(DEBUG)
        flatFileParse.announceLoop();
#endif // DEBUG
        flatfile.getline(tmp_line,128);
        tmp_str = tmp_line;
        switch(tmp_str[0])
        {
            case 'I':
                if(tmp_str[1] == 'D')
                {
                    del_str = "ID";
                    GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&identification);
                }
                break;
            case 'A':
                if(tmp_str[1] == 'C')
                {
                    del_str = ";";
                    rep_str = " ";
                    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                    del_str = "AC";
                    GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&accession_number);
                }
                break;
            case 'S':
                switch(tmp_str[1])
                {
                    case 'V':
                        del_str = "SV";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&sequence_version);
                        break;
                    case 'Q':
                        del_str = "SQ";
                        GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
                        tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
                        int tmp_int_4 = 0;
                        for(int i = 1; i < (int)tmp_vector.size(); i = i + 2)
                        {
                            t_str = tmp_vector[i];
                            cout << t_str << endl;
                            tmp_int_4 = GAGenomUtilities::stringToInteger(&t_str);
                            if(i == 1)
                            {
                            	seq_len = tmp_int_4;
                            }
                            sequence_header.push_back(tmp_int_4);
                        }
                        sq = true;
                        break;
                }
                break;
            case 'D':
                switch(tmp_str[1])
                {
                    case 'E':
                        tmp_lines_vector.push_back(tmp_str);
                        de = true;
                        break;
                    case 'R':
                        rep_str = " ";
                        del_str = "DR";
                        GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                        del_str = "\r";
                        GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                        GAGenomUtilities::trimString(&tmp_str);
                        database_cross_reference.push_back(tmp_str);
                        break;
                    case 'T':
                        rep_str = " ";
                        del_str = "DT";
                        GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                        del_str = "\r";
                        GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                        GAGenomUtilities::trimString(&tmp_str);
                        if(dt)
                        {
                            date_of_last_update = tmp_str;
                            dt = false;
                        }
                        else
                        {
                            date_of_creation = tmp_str;
                            dt = true;
                        }
                        break;
                }
                break;
            case 'K':
                if(tmp_str[1] == 'W')
                {
                    del_str = "KW";
                    rep_str = " ";
                    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
                    GAGenomUtilities::trimString(&tmp_str);
                    GAGenomUtilities::onlyOneDelimerChar(&tmp_str,' ');
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
                    if(tmp_vector.size() > 0 && tmp_vector[0] != ".")
                    {
                        key_words.push_back(GAGenomUtilities::toOneString(&tmp_vector,true));
                    }
                    kw = true;
                }
                break;
            case 'O':
                switch(tmp_str[1])
                {
                    case 'S':
                        del_str = "OS";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&organism_species);
                        break;
                    case 'C':
                        tmp_lines_vector.push_back(tmp_str);
                        oc = true;
                        break;
                    case 'G':
                        del_str = "OG";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&organelle);
                        break;
                }
                break;
            case 'R':
                switch(tmp_str[1])
                {
                    case 'N':
                        if(rn)
                        {
                            if(!tmp_lines_vector.empty())
                            {
                                tmp_reference = new GAGenomReferenceEmbl;
                                for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                                {
                                    tmp_reference->update(&(tmp_lines_vector[m]));
                                }
                                tmp_reference->parse();
                                references.push_back(*tmp_reference);
                                tmp_lines_vector.clear();
                                delete(tmp_reference);
                            }
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else
                        {
                            tmp_lines_vector.push_back(tmp_str);
                            rn = true;
                        }
                        break;
                    case 'A':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'T':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'P':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'L':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'G':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'X':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                    case 'C':
                        if(rn)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        break;
                }
                break;
            case 'C':
                switch(tmp_str[1])
                {
                    case 'O':
                        tmp_lines_vector.push_back(tmp_str);
                        co = true;
                        co_cc = false;
                        break;
                    case 'C':
                        if(cc)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else if(co_cc)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else if(co)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else
                        {
                            t_str = tmp_str.substr(0,20);
                            if(t_str.find("join(") == string::npos)
                            {
                                tmp_lines_vector.push_back(tmp_str);
                                cc = true;
                                co = false;
                                co_cc = false;
                            }
                            else
                            {
                                tmp_lines_vector.push_back(tmp_str);
                                co = true;
                                co_cc = true;
                            }
                        }
                        break;
                }
                break;
            case 'F':
                if(tmp_str[1] == 'T')
                {
                    feature_table.update(&tmp_str);
                    ft = true;
                }
                break;
            case '/':
            cout << seq_len << "=" << (int) sequence.size() << endl;
            	if(seq_len == (int) sequence.size())
            	{
            		complete_file = true;
            		error_number = 0;
            		error_message = "All Okay!";
            	}
            	else
            	{
            		error_number = 1;
            		error_message = "Sequence string of genome is incomplete!";
            	}
                flatfile.close();
                break;
            case 'X':
                if(de)
                {
                    if(!tmp_lines_vector.empty())
                    {
                        t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                        del_str = "DE";
                        GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                    }
                    de = false;
                    tmp_lines_vector.clear();
                }
                if(kw)
                {
                    kw = false;
                }
                if(oc)
                {
                    if(!tmp_lines_vector.empty())
                    {
                        t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                        del_str = "OC";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        del_str = "\r";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        del_str = ".";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                        GAGenomUtilities::trimString(&t_str);
                        vector<string> tmp_s_vec;
                        tmp_s_vec = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,';',false);
                        organism_classification = tmp_s_vec;
                    }
                    oc = false;
                    tmp_lines_vector.clear();
                }
                if(rn)
                {
                    if(!tmp_lines_vector.empty())
                    {
                        tmp_reference = new GAGenomReferenceEmbl;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            tmp_reference->update(&(tmp_lines_vector[m]));
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                    }
                    tmp_lines_vector.clear();
                    rn = false;
                }
                if(cc)
                {
                    if(!tmp_lines_vector.empty())
                    {
                        t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                        del_str = "CC";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        del_str = "\r";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                        GAGenomUtilities::trimString(&t_str);
                        free_text_comment.push_back(t_str);
                    }
                    tmp_lines_vector.clear();
                    cc = false;
                }
                if(co)
                {
                    if(!tmp_lines_vector.empty())
                    {
                        t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                        if(co_cc)
                        {
                            del_str = "CC";
                        }
                        else
                        {
                            del_str = "CO";
                        }
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        del_str = "\r";
                        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                        GAGenomUtilities::trimString(&t_str);
                        contig = t_str;
                    }
                    tmp_lines_vector.clear();
                    co = false;
                    co_cc = false;
                }
                if(ft)
                {
                    feature_table.parse();
                    ft = false;
                }
                if(sq)
                {
                    parseSequence(&tmp_str);
                    sequence += tmp_str;
                    prepared = true;
                }
                break;
            case ' ':
                if(sq)
                {
                    parseSequence(&tmp_str);
                    sequence += tmp_str;
                    prepared = true;
                }
                break;
        }
    }

    prepared = true;
}

string * gellisary::GAGenomEmbl::getDateOfCreation()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &date_of_creation;
}

string * gellisary::GAGenomEmbl::getDateOfLastUpdate()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &date_of_last_update;
}

string * gellisary::GAGenomEmbl::getOrganelle()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &organelle;
}

vector<string> * gellisary::GAGenomEmbl::getDataCrossReference()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &database_cross_reference;
}
/*
  string * gellisary::GAGenomEmbl::getOrganismClassificationAsOneString()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &organism_classification_as_one_string;
  }
*/
GAGenomFeatureTableEmbl * gellisary::GAGenomEmbl::getFeatureTable()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &feature_table;
}

GAGenomReferenceEmbl * gellisary::GAGenomEmbl::getReference()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    if(iter < (int)references.size())
    {
        tmp_ref = references[iter];
        iter++;
        return &tmp_ref; // muss noch anschauen
    }
    else
    {
        return NULL;
    }
}
/*
  vector<string> * gellisary::GAGenomEmbl::getOrganismClassification()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &organism_classification;
  }
*/
string * gellisary::GAGenomEmbl::getDataCrossReferenceAsOneString()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    database_cross_reference_as_one_string = GAGenomUtilities::toOneString(&database_cross_reference,true);
    return &database_cross_reference_as_one_string;
}

vector<int> * gellisary::GAGenomEmbl::getSequenceHeader()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &sequence_header;
}

void gellisary::GAGenomEmbl::parseSequence(string * source_str)
{
    string target_str;
    string tst = *source_str;
    
    char tmp_char;
    //ostringstream ost;
    int i = 0;
    //vector<string> tmp_vector;
    for(i = 0; i < (int) tst.size(); i++)
    {
        tmp_char = tst[i];
        if((tmp_char != ' ') && (tmp_char != '\r') && (tmp_char != '\n') && (tmp_char != '0') && (tmp_char != '1') && (tmp_char != '2') && (tmp_char != '3') && (tmp_char != '4') && (tmp_char != '5') && (tmp_char != '6') && (tmp_char != '7') && (tmp_char != '8') && (tmp_char != '9'))
        {
//        	ost << tmp_char;
			target_str += tmp_char;
			
        }
        
    }
//    GAGenomUtilities::trimString(&target_str);
//    GAGenomUtilities::onlyOneDelimerChar(&target_str,' ');
//    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&target_str,' ',false);
//    target_str = GAGenomUtilities::toOneString(&tmp_vector,false);
	//target_str = ost.str();
	//cout << "Bla :-" << target_str << "-" << i << std::endl;
	//source_str->erase();
    *source_str = target_str;
}
