/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomFeatureTableSourceEmbl.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomFeatureTableSourceEmbl::parse()
{
    string tmp_str;
    string del_str;
    string rep_str;
    vector<string> tmp_vector;
    tmp_str = GAGenomUtilities::toOneString(&row_lines,true);
//    cout << "Source 0 : -" << tmp_str << "-" << endl;
    del_str = "\r";
    rep_str = " ";
    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
//    cout << "Source 1 : -" << tmp_str << "-" << endl;
    del_str = "FT";
    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
//    cout << "Source 2 : -" << tmp_str << "-" << endl;
    del_str = "=";
    GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
//    cout << "Source 3 : -" << tmp_str << "-" << endl;
    if(tmp_str[0] == ' ')
    {
    	GAGenomUtilities::trimString(&tmp_str);
    }
    else
    {
    	GAGenomUtilities::trimString2(&tmp_str);
    }
//    cout << "Source 4 : -" << tmp_str << "-" << endl;
    GAGenomUtilities::onlyOneDelimerChar(&tmp_str,' ');
//    cout << "Source 5 : -" << tmp_str << "-" << endl;
    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
    string t_str;
    int i = 0;
    for(i = 0; i < (int) tmp_vector.size(); i++)
    {
        t_str = tmp_vector[i];
//        cout << "Source 6 : -" << t_str << "-" << endl;
        if(t_str[0] == ' ')
	    {
    		GAGenomUtilities::trimString(&t_str);
	    }
    	else
	    {
    		GAGenomUtilities::trimString2(&t_str);
	    }
//        cout << "Source 7 : -" << t_str << "-" << endl;
        if(t_str == "source")
        {
            t_str = tmp_vector[i+1];
            vector<int> locations = GAGenomUtilities::parseSourceLocation(&t_str);
            if(locations.size() > 0)
            {
                source_begin = locations[0];
                source_end = locations[1];
            }
            break;
        }
    }
    string tt_str;
    bool qual = false;
    for(i+=2; i < (int) tmp_vector.size(); i++)
    {
        t_str = tmp_vector[i];
        if(t_str == "")
        {
        	continue;
        }
//        cout << "Source 8 : -" << t_str << "-" << endl;
        if(t_str[0] == ' ')
	    {
    		GAGenomUtilities::trimString(&t_str);
	    }
    	else
	    {
    		GAGenomUtilities::trimString2(&t_str);
	    }
//        cout << "Source 9 : -" << t_str << "-" << endl;
        if(t_str[0] == '/')
        {
//            cout << "Source 10 : -" << t_str << "-" << endl;
            if(qual)
            {
//                cout << "Source 11 : -" << t_str << "-" << endl;
                qualifiers[tt_str] = "none";
                tt_str = t_str;
                del_str = "/";
                rep_str = " ";
                GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
//                cout << "Source 12 : -" << tt_str << "-" << endl;
                if(tt_str[0] == ' ')
			    {
	    			GAGenomUtilities::trimString(&tt_str);
		    	}
		    	else
			    {
		    		GAGenomUtilities::trimString2(&tt_str);
			    }
//			    cout << "Source 13 : -" << tt_str << "-" << endl;
            }
            else
            {
//                cout << "Source 14 : -" << t_str << "-" << endl;
                qual = true;
                tt_str = t_str;
                del_str = "/";
                rep_str = " ";
                GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
//                cout << "Source 15 : -" << tt_str << "-" << endl;
                if(tt_str[0] == ' ')
			    {
	    			GAGenomUtilities::trimString(&tt_str);
		    	}
		    	else
			    {
		    		GAGenomUtilities::trimString2(&tt_str);
			    }
//			    cout << "Source 16 : -" << tt_str << "-" << endl;
            }
        }
        else
        {
//            cout << "Source 17 : -" << t_str << "-" << endl;
            if(!tt_str.empty())
            {
//                cout << "Source 18 : -" << t_str << "-" << endl;
                del_str = "\"";
                rep_str = " ";
                GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
//                cout << "Source 19 : -" << tt_str << "-" << endl;
                if(t_str[0] == ' ')
			    {
	    			GAGenomUtilities::trimString(&t_str);
		    	}
		    	else
			    {
		    		GAGenomUtilities::trimString2(&t_str);
			    }
//			    cout << "Source 20 : -" << tt_str << "-" << endl;
                qualifiers[tt_str] = t_str;
                qual = false;
            }
        }
    }
    prepared = true;
}
