/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomReferenceGenBank.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomReferenceGenBank::parse()
{
    string tmp_str;
    string tmp_str2;
    string tmp_str3;
    string del_str;
    string rep_str;
    vector<string> tmp_vector;
    vector<string> tmp_lines_vector;
    string t_str;
    bool ra = false;
    bool rt = false;
    bool rx = false;
    bool rl = false;
    bool rc = false;
    bool cs = false;
    for(int i = 0; i < (int) row_lines.size(); i++)
    {
        tmp_str = row_lines[i];
        tmp_str2 = tmp_str.substr(0,12);
        tmp_str3 = tmp_str.substr(12);
        if(tmp_str2[4] != ' ')
        {
            if(tmp_str2[0] == ' ')
            {
                GAGenomUtilities::trimString(&tmp_str2);
            }
            else
            {
                GAGenomUtilities::trimString2(&tmp_str2);
            }
            if(tmp_str3[0] == ' ')
            {
                GAGenomUtilities::trimString(&tmp_str3);
            }
            else
            {
                GAGenomUtilities::trimString2(&tmp_str3);
            }
            if(tmp_str2 == "REFERENCE")
            {
                if(rl)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_location = t_str;
                    tmp_lines_vector.clear();
                    rl = false;
                }
                if(rt)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\"";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_title = t_str;
                    tmp_lines_vector.clear();
                    rt = false;
                }
                if(rx)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        reference_cross_reference.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rx = false;
                }
                if(ra)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        del_str = ",";
                        GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
                        if(tt_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tt_str);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tt_str);
                        }
                        reference_authors.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    ra = false;
                }
                bool hasPosition = false;
                for(int l = 0; l < (int) tmp_str3.size(); l++)
                {
                    if(tmp_str3[l] == 'b')
                    {
                        hasPosition = true;
                    }
                }
                del_str = "\r";
                rep_str = " ";
                GAGenomUtilities::replaceByString(&tmp_str3,&del_str,&rep_str);
                if(tmp_str[0] == ' ')
                {
                    GAGenomUtilities::trimString(&tmp_str3);
                }
                else
                {
                    GAGenomUtilities::trimString2(&tmp_str3);
                }
                if(hasPosition)
                {
                    GAGenomUtilities::onlyOneDelimerChar(&tmp_str3,' ');
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str3,' ',false);
                    if(tmp_vector.size() > 0)
                    {
                        del_str = "(";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&tmp_str3,&del_str,&rep_str);
                        del_str = ")";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&tmp_str3,&del_str,&rep_str);
                        del_str = "bases";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&tmp_str3,&del_str,&rep_str);
                        del_str = "to";
                        rep_str = " ";
                        GAGenomUtilities::replaceByString(&tmp_str3,&del_str,&rep_str);
                        GAGenomUtilities::onlyOneDelimerChar(&tmp_str3,' ');
                        if(t_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tmp_str3);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tmp_str3);
                        }
                        vector<string> t_vec = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str3,' ',false);
                        if(t_vec.size() > 2)
                        {
                            t_str = t_vec[1];
                            reference_position_begin.push_back(GAGenomUtilities::stringToInteger(&t_str));
                            t_str = t_vec[2];
                            reference_position_end.push_back(GAGenomUtilities::stringToInteger(&t_str));
                        }
                        t_str = t_vec[0];
                        reference_number = GAGenomUtilities::stringToInteger(&t_str);
                    }
                }
                else
                {
                    reference_number = GAGenomUtilities::stringToInteger(&tmp_str3);
                }
            }
            else if(tmp_str2 == "JOURNAL")
            {
                if(rt)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\"";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_title = t_str;
                    tmp_lines_vector.clear();
                    rt = false;
                }
                if(rx)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        reference_cross_reference.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rx = false;
                }
                /*if(rg)
                  {
                  del_str = "RG";
                  rep_str = " ";
                  string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  del_str = "\r";
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                  GAGenomUtilities::trimString(&t_str);
                  tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                  vector<string> tt_vector;
                  for(int j = 0; j < (int)tmp_vector.size(); j++)
                  {
                  string tt_str = tmp_vector[j];
                  GAGenomUtilities::trimString(&tt_str);
                  tt_vector.push_back(tt_str);
                  }
                  reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
                  tmp_vector.clear();
                  tmp_lines_vector.clear();
                  rg = false;
                  }*/
                if(ra)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        del_str = ",";
                        GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
                        if(tt_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tt_str);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tt_str);
                        }
                        reference_authors.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    ra = false;
                }
                if(rc)
                {
                    del_str = "RC";
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    GAGenomUtilities::trimString(&t_str);
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    vector<string> tt_vector;
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        GAGenomUtilities::trimString(&tt_str);
                        tt_vector.push_back(tt_str);
                    }
                    reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rc = false;
                }
                rl = true;
                tmp_lines_vector.push_back(tmp_str3);
            }
            else if(tmp_str2 == "AUTHORS")
            {
                if(rl)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_location = t_str;
                    tmp_lines_vector.clear();
                    rl = false;
                }
                /*if(rg)
                  {
                  del_str = "RG";
                  rep_str = " ";
                  string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  del_str = "\r";
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                  GAGenomUtilities::trimString(&t_str);
                  tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                  vector<string> tt_vector;
                  for(int j = 0; j < (int)tmp_vector.size(); j++)
                  {
                  string tt_str = tmp_vector[j];
                  GAGenomUtilities::trimString(&tt_str);
                  tt_vector.push_back(tt_str);
                  }
                  reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
                  tmp_vector.clear();
                  tmp_lines_vector.clear();
                  rg = false;
                  }*/
                if(rt)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\"";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_title = t_str;
                    tmp_lines_vector.clear();
                    rt = false;
                }
                if(rx)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        reference_cross_reference.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rx = false;
                }
                if(rc)
                {
                    del_str = "RC";
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    GAGenomUtilities::trimString(&t_str);
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    vector<string> tt_vector;
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        GAGenomUtilities::trimString(&tt_str);
                        tt_vector.push_back(tt_str);
                    }
                    reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rc = false;
                }
                ra = true;
                tmp_lines_vector.push_back(tmp_str3);
            }
            else if(tmp_str2 == "TITLE")
            {
                if(rl)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_location = t_str;
                    tmp_lines_vector.clear();
                    rl = false;
                }
                /*
                  if(rg)
                  {
                  del_str = "RG";
                  rep_str = " ";
                  string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  del_str = "\r";
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                  GAGenomUtilities::trimString(&t_str);
                  tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                  vector<string> tt_vector;
                  for(int j = 0; j < (int)tmp_vector.size(); j++)
                  {
                  string tt_str = tmp_vector[j];
                  GAGenomUtilities::trimString(&tt_str);
                  tt_vector.push_back(tt_str);
                  }
                  reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
                  tmp_vector.clear();
                  tmp_lines_vector.clear();
                  rg = false;
                  }*/
                if(rx)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        reference_cross_reference.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rx = false;
                }
                if(ra)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        del_str = ",";
                        GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
                        if(tt_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tt_str);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tt_str);
                        }
                        reference_authors.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    ra = false;
                }
                /*if(cs)
                {
                	del_str = "CONSRTM";
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    GAGenomUtilities::trimString(&t_str);
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    vector<string> tt_vector;
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        GAGenomUtilities::trimString(&tt_str);
                        tt_vector.push_back(tt_str);
                    }
                    reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                	cs = false;
                }*/
                if(rc)
                {
                    del_str = "RC";
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    GAGenomUtilities::trimString(&t_str);
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    vector<string> tt_vector;
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        GAGenomUtilities::trimString(&tt_str);
                        tt_vector.push_back(tt_str);
                    }
                    reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rc = false;
                }
                rt = true;
                tmp_lines_vector.push_back(tmp_str3);
            }
            else if(tmp_str2 == "CONSRTM")
            {
				/*cs = true;
                tmp_lines_vector.push_back(tmp_str3);*/
            }
            else if((tmp_str2 == "PUBMED") || (tmp_str2 == "MEDLINE"))
            {
                if(rt)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\"";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_title = t_str;
                    tmp_lines_vector.clear();
                    rt = false;
                }
                /*if(rg)
                  {
                  del_str = "RG";
                  rep_str = " ";
                  string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  del_str = "\r";
                  GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                  GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                  GAGenomUtilities::trimString(&t_str);
                  tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                  vector<string> tt_vector;
                  for(int j = 0; j < (int)tmp_vector.size(); j++)
                  {
                  string tt_str = tmp_vector[j];
                  GAGenomUtilities::trimString(&tt_str);
                  tt_vector.push_back(tt_str);
                  }
                  reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
                  tmp_vector.clear();
                  tmp_lines_vector.clear();
                  rg = false;
                  }*/
                if(rl)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_location = t_str;
                    tmp_lines_vector.clear();
                    rl = false;
                }
                if(rc)
                {
                    del_str = "RC";
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    GAGenomUtilities::trimString(&t_str);
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    vector<string> tt_vector;
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        GAGenomUtilities::trimString(&tt_str);
                        tt_vector.push_back(tt_str);
                    }
                    reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rc = false;
                }
                if(ra)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        del_str = ",";
                        GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
                        if(tt_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tt_str);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tt_str);
                        }
                        reference_authors.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    ra = false;
                }
                rx = true;
                tmp_lines_vector.push_back(tmp_str);
            }
            else
            {
                if(ra)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        del_str = ",";
                        GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
                        if(tt_str[0] == ' ')
                        {
                            GAGenomUtilities::trimString(&tt_str);
                        }
                        else
                        {
                            GAGenomUtilities::trimString2(&tt_str);
                        }
                        reference_authors.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    ra = false;
                }
                if(rl)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_location = t_str;
                    tmp_lines_vector.clear();
                    rl = false;
                }
                if(rt)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = "\"";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    reference_title = t_str;
                    tmp_lines_vector.clear();
                    rt = false;
                }
                if(rx)
                {
                    rep_str = " ";
                    string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                    del_str = "\r";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    del_str = ";";
                    GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                    GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                    if(t_str[0] == ' ')
                    {
                        GAGenomUtilities::trimString(&t_str);
                    }
                    else
                    {
                        GAGenomUtilities::trimString2(&t_str);
                    }
                    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
                    for(int j = 0; j < (int)tmp_vector.size(); j++)
                    {
                        string tt_str = tmp_vector[j];
                        reference_cross_reference.push_back(tt_str);
                    }
                    tmp_vector.clear();
                    tmp_lines_vector.clear();
                    rx = false;
                }
            }
        }
        else
        {
            tmp_lines_vector.push_back(tmp_str3);
        }
    }
    if(ra)
    {
        rep_str = " ";
        string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
        del_str = "\r";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
        if(t_str[0] == ' ')
        {
            GAGenomUtilities::trimString(&t_str);
        }
        else
        {
            GAGenomUtilities::trimString2(&t_str);
        }
        tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
        for(int j = 0; j < (int)tmp_vector.size(); j++)
        {
            string tt_str = tmp_vector[j];
            del_str = ",";
            GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
            if(tt_str[0] == ' ')
            {
                GAGenomUtilities::trimString(&tt_str);
            }
            else
            {
                GAGenomUtilities::trimString2(&tt_str);
            }
            reference_authors.push_back(tt_str);
        }
        tmp_vector.clear();
        tmp_lines_vector.clear();
        ra = false;
    }
    if(rl)
    {
        rep_str = " ";
        string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
        del_str = "\r";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
        if(t_str[0] == ' ')
        {
            GAGenomUtilities::trimString(&t_str);
        }
        else
        {
            GAGenomUtilities::trimString2(&t_str);
        }
        reference_location = t_str;
        tmp_lines_vector.clear();
        rl = false;
    }
    if(rt)
    {
        rep_str = " ";
        string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
        del_str = "\r";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        del_str = "\"";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        del_str = ";";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
        if(t_str[0] == ' ')
        {
            GAGenomUtilities::trimString(&t_str);
        }
        else
        {
            GAGenomUtilities::trimString2(&t_str);
        }
        reference_title = t_str;
        tmp_lines_vector.clear();
        rt = false;
    }
    if(rx)
    {
        rep_str = " ";
        string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
        del_str = "\r";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        del_str = ";";
        GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
        GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
        if(t_str[0] == ' ')
        {
            GAGenomUtilities::trimString(&t_str);
        }
        else
        {
            GAGenomUtilities::trimString2(&t_str);
        }
        tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
        for(int j = 0; j < (int)tmp_vector.size(); j++)
        {
            string tt_str = tmp_vector[j];
            reference_cross_reference.push_back(tt_str);
        }
        tmp_vector.clear();
        tmp_lines_vector.clear();
        rx = false;
    }
    prepared = true;
}

string * gellisary::GAGenomReferenceGenBank::getJournal()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_journal;
}
vector<int> * gellisary::GAGenomReferenceGenBank::getPubMeds()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_pubmed;
}
string * gellisary::GAGenomReferenceGenBank::getCrossReferenceAsString()
{
    if(!prepared)
    {
        parse();
    }
    reference_cross_reference_as_string = GAGenomUtilities::toOneString(&reference_cross_reference,true);
    return &reference_cross_reference_as_string;
}

vector<string> * gellisary::GAGenomReferenceGenBank::getCrossReference()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_cross_reference;
}

vector<int> * gellisary::GAGenomReferenceGenBank::getMedLines()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_medline;
}
