/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomDDBJ.h"
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

using namespace std;
using namespace gellisary;

GAGenomDDBJ::GAGenomDDBJ(string * fname):GAGenom(fname)
{
    iter = 0;
}

void gellisary::GAGenomDDBJ::parseFlatFile()
{
    ifstream flatfile(file_name.c_str());
    string tmp_str;
    string del_str;
    string rep_str;
    vector<string> tmp_vector;
    vector<string> tmp_lines_vector;
    string t_str;
    string t2_str;
    string t3_str;
    vector<string> t2_vector;
    vector<string> t3_vector;

    GAGenomReferenceDDBJ *tmp_reference;

    bool de = false;
    bool ke = false;
    bool re = false;
    bool so = false;
    bool com = false;
    bool con = false;
    bool fe = false;
    bool ori = false;
    bool org = false;
    string tmp_str2;

    char tmp_line[128];

    while(!flatfile.eof())
    {
        flatfile.getline(tmp_line,128);
        tmp_str = tmp_line;
        tmp_str2 = tmp_str.substr(0,12);
        if(tmp_str2[3] != ' ')
        {
            switch(tmp_str[0])
            {
                case 'L':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'O')
                    {
                        del_str = "LOCUS";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&identification);
                    }
                    break;
                case 'A':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'C')
                    {
                        del_str = "ACCESSION";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&accession_number);
                    }
                    break;
                case 'V':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'E')
                    {
                        del_str = "VERSION";
                        GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&sequence_version);
                    }
                    break;
                case 'B':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'A')
                    {
                        del_str = "BASE COUNT";
                        GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
                        tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
                        for(int i = 0; i < (int) tmp_vector.size(); i = i + 2)
                        {
                            t_str = tmp_vector[i];
                            sequence_header.push_back(GAGenomUtilities::stringToInteger(&t_str));
                        }
                    }
                    break;
                case 'D':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'E')
                    {
                        tmp_lines_vector.push_back(tmp_str);
                        de = true;
                    }
                    break;
                case 'K':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'E')
                    {
                        del_str = "KEYWORDS";
                        GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
                        tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
                        if(tmp_vector.size() > 0 && tmp_vector[0] != ".")
                        {
                            key_words.push_back(GAGenomUtilities::toOneString(&tmp_vector,true));
                        }
                        ke = true;
                    }
                    break;
                case 'S':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }

                    if(tmp_str[1] == 'O')
                    {
                        tmp_lines_vector.push_back(tmp_str);
                        so = true;
                        org = false;
                    }
                    break;
                case 'R':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'E')
                    {
                        if(re)
                        {
                            if(!tmp_lines_vector.empty())
                            {
                                tmp_reference = new GAGenomReferenceDDBJ;
                                for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                                {
                                    tmp_reference->update(&(tmp_lines_vector[m]));
                                }
                                tmp_reference->parse();
                                references.push_back(*tmp_reference);
                                delete(tmp_reference);
                            }
                            tmp_lines_vector.clear();
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else
                        {
                            tmp_lines_vector.push_back(tmp_str);
                            re = true;
                        }
                    }
                    break;
                case 'C':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    switch(tmp_str[2])
                    {
                        case 'N':
                            tmp_lines_vector.push_back(tmp_str);
                            con = true;
                            break;
                        case 'M':
                            tmp_lines_vector.push_back(tmp_str);
                            com = true;
                            break;
                    }
                    break;
                case 'F':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'E')
                    {
                        feature_table.update(&tmp_str);
                        fe = true;
                    }
                    break;
                case 'O':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(so)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            del_str = "\r";
                            t2_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            GAGenomUtilities::replaceByWhiteSpaceCleanly(&t2_str,&del_str);
                            if(t2_str[0] == ' ')
                            {
                                GAGenomUtilities::trimString(&t2_str);
                            }
                            GAGenomUtilities::onlyOneDelimerChar(&t2_str,' ');
                            t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t2_str,' ',false);
                            if(t2_vector[0] == "ORGANISM")
                            {
                                for(int l = 1; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                            else
                            {
                                for(int l = 0; l < (int) t2_vector.size(); l++)
                                {
                                    organism_classification.push_back(t2_vector[l]);
                                }
                            }
                        }
                        so = false;
                        org = false;
                        tmp_lines_vector.clear();
                    }
                    if(re)
                    {
                        tmp_reference = new GAGenomReferenceDDBJ;
                        for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
                        {
                            t3_str = tmp_lines_vector[m];
                            tmp_reference->update(&t3_str);
                        }
                        tmp_reference->parse();
                        references.push_back(*tmp_reference);
                        delete(tmp_reference);
                        tmp_lines_vector.clear();
                        re = false;
                    }
                    if(com)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                            del_str = "COMMENT";
                            rep_str = " ";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            del_str = "\r";
                            GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
                            GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
                            GAGenomUtilities::trimString(&t_str);
                            free_text_comment.push_back(t_str);
                        }
                        tmp_lines_vector.clear();
                        com = false;
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                        ori = false;
                    }
                    if(tmp_str[1] == 'R')
                    {
                        tmp_lines_vector.push_back(tmp_str);
                        ori = true;
                    }
                    break;
                case '/':
                    if(ori)
                    {
                        ori = false;
                    }
                    flatfile.close();
                    complete_file = true;
                    break;
                case ' ':
                    if(de)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "DEFINITION";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
                        }
                        de = false;
                        tmp_lines_vector.clear();
                    }
                    if(ke)
                    {
                        ke = false;
                    }
                    if(fe)
                    {
                        feature_table.update(&tmp_str);
                    }
                    if(so)
                    {
                        if(org)
                        {
                            tmp_lines_vector.push_back(tmp_str);
                        }
                        else
                        {
                            if(tmp_str[2] == 'O')
                            {
                                vector<string> t3_vec;
                                string t3_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
                                GAGenomUtilities::onlyOneDelimerChar(&t3_str,' ');
                                if(t3_str[0] == ' ')
                                {
                                    GAGenomUtilities::trimString(&t3_str);
                                }
                                t2_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t3_str,' ',true);
                                if(t2_vector[0] == "SOURCE")
                                {
                                    for(int i = 1; i < (int)t2_vector.size(); i++)
                                    {
                                        t3_vec.push_back(t2_vector[i]);
                                    }
                                    t3_str = GAGenomUtilities::toOneString(&t3_vec,true);
                                    organism_species = t3_str;
                                }
                                org = true;
                                tmp_lines_vector.clear();
                                tmp_lines_vector.push_back(tmp_str);
                            }
                            else
                            {
                                tmp_lines_vector.push_back(tmp_str);
                            }
                        }
                    }
                    if(re)
                    {
                        tmp_lines_vector.push_back(tmp_str);
                    }
                    if(com)
                    {
                        tmp_lines_vector.push_back(tmp_str);
                    }
                    if(con)
                    {
                        if(!tmp_lines_vector.empty())
                        {
                            t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
                            del_str = "CONTIG";
                            GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
                        }
                        tmp_lines_vector.clear();
                        con = false;
                    }
                    if(fe)
                    {
                        feature_table.parse();
                        fe = false;
                    }
                    if(ori)
                    {
                        parseSequence(&tmp_str);
                        sequence += tmp_str;
                    }
                    break;
            }
        }
        else
        {
            if(fe)
            {
                feature_table.update(&tmp_str);
            }
            else
            {
                tmp_lines_vector.push_back(tmp_str);
            }
        }
    }
    prepared = true;
}

/*string * gellisary::GAGenomDDBJ::getDateOfCreation()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &date_of_creation;
  }*/

/*string * gellisary::GAGenomDDBJ::getDateOfLastUpdate()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &date_of_last_update;
  }*/

/*string * gellisary::GAGenomDDBJ::getOrganelle()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &organelle;
  }*/

/*vector<string> * gellisary::GAGenomDDBJ::getDataCrossReference()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &database_cross_reference;
  }*/

/*string * gellisary::GAGenomDDBJ::getOrganismClassificationAsOneString()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &organism_classification_as_one_string;
  }*/

GAGenomFeatureTableDDBJ * gellisary::GAGenomDDBJ::getFeatureTable()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &feature_table;
}

GAGenomReferenceDDBJ * gellisary::GAGenomDDBJ::getReference()
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

/*vector<string> * gellisary::GAGenomDDBJ::getOrganismClassification()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &organism_classification;
  }*/

/*string * gellisary::GAGenomDDBJ::getDataCrossReferenceAsOneString()
  {
  if(!prepared)
  {
  parseFlatFile();
  }
  return &database_cross_reference_as_one_string;
  }*/

vector<int> * gellisary::GAGenomDDBJ::getSequenceHeader()
{
    if(!prepared)
    {
        parseFlatFile();
    }
    return &sequence_header;
}

void gellisary::GAGenomDDBJ::parseSequence(string * source_str)
{
    string target_str;
    char tmp_char;
    int i = 0;
    vector<string> tmp_vector;
    for(i = 0; i < (int) source_str->size(); i++)
    {
        tmp_char = source_str->operator[](i);
        if((tmp_char == 't') || (tmp_char == 'g') || (tmp_char == 'c') || (tmp_char == 'a') || (tmp_char == 'T') || (tmp_char == 'G') || (tmp_char == 'C') || (tmp_char == 'A'))
        {
            target_str += tmp_char;
        }
    }
    GAGenomUtilities::trimString(&target_str);
    GAGenomUtilities::onlyOneDelimerChar(&target_str,' ');
    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&target_str,' ',false);
    target_str = GAGenomUtilities::toOneString(&tmp_vector,false);
    *source_str = target_str;
}
