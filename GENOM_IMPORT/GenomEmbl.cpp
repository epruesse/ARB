
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include "GenomEmbl.h"
#include "GenomUtilities.h"
// #include "GenomReferenceEmbl.h"
// #include "GenomFeatureTableEmbl.h"

using namespace std;

GenomEmbl::GenomEmbl(string file)
{
	prepared = false;
	actual_sq = 0;
	actual_dr = 0;
	actual_rr = 0;
	actual_cc = 0;
	actual_sdl = 0;
	actual_kw = 0;
	actual_oc = 0;
	embl_flatfile = file;
}

GenomEmbl::~GenomEmbl(void)
{

}

string* GenomEmbl::getID(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &identification;
}

string* GenomEmbl::getAC(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &accession_number;
}

string* GenomEmbl::getSV(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &sequence_version;
}

string* GenomEmbl::getDTCreation(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &date_of_creation;
}

string* GenomEmbl::getDTLastUpdate(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &date_of_last_update;
}

string* GenomEmbl::getDE(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &description;
}

string* GenomEmbl::getKW(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
  key_words_to_string = toOneString(key_words);
  return &key_words_to_string;
  /*if(key_words.size() < actual_kw)
	{
		return &key_words[actual_kw++];
	}
	else
	{
		return NULL;	// or null
	}*/
}

string* GenomEmbl::getOS(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &organism_species;
}

string* GenomEmbl::getOC(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
  organism_classification_to_string = toOneString(organism_classification);
  return &organism_classification_to_string;
/*  if(organism_classification.size() < actual_oc)
	{
		return &organism_classification[actual_oc++];
	}
	else
	{
		return NULL;	// or null
	}*/
}

string* GenomEmbl::getOG(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &or_garnelle;
}

GenomReferenceEmbl* GenomEmbl::getReference(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
  if((int)references.size() > actual_rr)
	{
		return &references[actual_rr++];
	}
	else
	{
		return NULL;	// or null
	}
}

string* GenomEmbl::getDR(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
  database_cross_reference_to_string = toOneString(database_cross_reference);
  return &database_cross_reference_to_string;
/*	if(database_cross_reference.size() < actual_dr)
	{
		return &database_cross_reference[actual_dr++];
	}
	else
	{
		return NULL;	// or null
	}*/
}

string* GenomEmbl::getCO(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &constructed;
}

string* GenomEmbl::getCC(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
  free_text_comments_to_string = toOneString(free_text_comments);
  return &free_text_comments_to_string;
/*	if(free_text_comments.size() < actual_cc)
	{
		return &free_text_comments[actual_cc++];
	}
	else
	{
		return NULL;	// or null
	}*/
}

long GenomEmbl::getSQ(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	if((int)sequence_header.size() > actual_sq)
	{
    if(sequence_header[actual_sq] == 0)
    {
      actual_sq++;
      return 0;
    }
    else
    {
      return sequence_header[actual_sq++];
    }
	}
	else
	{
		return 0;	// or null
	}
}

string* GenomEmbl::get(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	if((int)sequence_data_line.size() > actual_sdl)
	{
		return &sequence_data_line[actual_sdl++];
	}
	else
	{
		return NULL;	// or null
	}
}

/*string* GenomEmbl::getFT(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &
}*/

GenomFeatureTableEmbl* GenomEmbl::getFeatureTable(void)
{
	if(!prepared)
	{
		prepareFlatFile();
	}
	return &feature_table;
}

int GenomEmbl::prepareFlatFile(void)
{
	ifstream flatfile(embl_flatfile.c_str());
  char tmp[100];
  string tstr0;
  string tstr1;
  string fea;
  vector<string> words;
  vector<string> lines;

  GenomReferenceEmbl *tref;
  bool dt = false;
  bool de = false;
 // bool dr = false;
  bool kw = false;
  bool oc = false;
  bool co_cc = false;
  bool rn = false;
  bool cc = false;
  bool co = false;
  bool ft = false;
  bool sq = false;
  //int d =0;

/*############################################################*/
//cout << "Vor Parse-Switch und -While" << endl;
/*############################################################*/
  while(!flatfile.eof())
  {
  flatfile.getline(tmp,100);
  tstr0 = tmp;
//cout << d++ << endl;
  switch(tmp[0])
  {
    case 'I':
/*############################################################*/
//cout << "I gefunden" << endl;
/*############################################################*/
      if(tmp[1] == 'D')
      {
        fea = "ID";
        eliminateFeatureTableSign(&tstr0,&fea, false);
        words = findAndSeparateWordsBy(&tstr0,' ',true);
        identification = toOneString(words);
      }
      break;
    case 'A':
/*############################################################*/
//cout << "A gefunden" << endl;
/*############################################################*/
      if(tmp[1] == 'C')
      {
        fea = "AC";
        eliminateFeatureTableSign(&tstr0,&fea);
        words = findAndSeparateWordsBy(&tstr0,' ',true);
        accession_number = toOneString(words);
      }
      break;
    case 'S':
/*############################################################*/
//cout << "S gefunden" << endl;
/*############################################################*/
      switch(tmp[1])
      {
        case 'V':
/*############################################################*/
//cout << "-V gefunden" << endl;
/*############################################################*/
          fea = "SV";
          eliminateFeatureTableSign(&tstr0,&fea);
          words = findAndSeparateWordsBy(&tstr0,' ',true);
          sequence_version = toOneString(words);
          break;
        case 'Q':
/*############################################################*/
//cout << "-Q gefunden" << endl;
/*############################################################*/
          fea = "SQ";
          eliminateFeatureTableSign(&tstr0,&fea);
          //cout << "sq 0001" << endl;
          words = findAndSeparateWordsBy(&tstr0,' ',true);
          //cout << "sq 0002" << endl;
          for(int i = 1; i < 13; i = i + 2)
          {
            //cout << "sq 0003" << endl;
//            cout << "sq 0003 " << words.at(i) << endl;
//            cout << "sq 0004 " << words.at(i).c_str() << endl;
//            cout << "sq 0005 " << atol(words.at(i).c_str()) << endl;

//             sequence_header.push_back(atol(words.at(i).c_str()));
#warning read here
              // hallo artem - vector::at() kann der Compiler leider nicht
              // ich hab es an 3 Stellen wie folgt geaendert.
              sequence_header.push_back(atol(words[i].c_str()));

//            cout << "sq 0004" << endl;
          }
  //        cout << "sq 0005" << endl;
          sq = true;
    //      cout << "sq 0006" << endl;
          break;
      }
      break;
    case 'D':
/*############################################################*/
//cout << "D gefunden" << endl;
/*############################################################*/
      switch(tmp[1])
      {
        case 'T':
/*############################################################*/
//cout << "-T gefunden" << endl;
/*############################################################*/
          fea = "DT";
          eliminateFeatureTableSign(&tstr0,&fea);
          tstr0 = trimString(tstr0);
          if(!dt)
          {
            date_of_creation = tstr0;
            dt = true;
          }
          else
          {
            date_of_last_update = tstr0;
            dt = false;
          }
          break;
        case 'E':
/*############################################################*/
//cout << "-E gefunden" << endl;
/*############################################################*/
          lines.push_back(tstr0);
          de = true;
          break;
        case 'R':
/*############################################################*/
//cout << "-R gefunden" << endl;
/*############################################################*/
          fea = "DR";
          eliminateFeatureTableSign(&tstr0,&fea,false);
          words = findAndSeparateWordsBy(&tstr0,' ',true);
          database_cross_reference.push_back(toOneString(words));
          //dr = true;
          break;
      }
      break;
    case 'K':
/*############################################################*/
//cout << "K gefunden" << endl;
/*############################################################*/
      if(tmp[1] == 'W')
      {
        fea = "KW";
        eliminateFeatureTableSign(&tstr0,&fea);
        words = findAndSeparateWordsBy(&tstr0,' ',true);
//         if(words.size() > 0 && words.at(0) != ".")
        if(words.size() > 0 && words[0] != ".")
        {
          key_words.push_back(toOneString(words));
        }
        kw = true;
      }
      break;
    case 'O':
/*############################################################*/
//cout << "O gefunden" << endl;
/*############################################################*/
      switch(tmp[1])
      {
        case 'S':
/*############################################################*/
//cout << "-S gefunden" << endl;
/*############################################################*/
          fea = "OS";
          eliminateFeatureTableSign(&tstr0,&fea);
          words = findAndSeparateWordsBy(&tstr0,' ',true);
          organism_species = toOneString(words);
          break;
        case 'C':
/*############################################################*/
//cout << "-C gefunden" << endl;
/*############################################################*/
          lines.push_back(tstr0);
          oc = true;
          break;
        case 'G':
/*############################################################*/
cout << "-G gefunden" << endl;
/*############################################################*/
          fea = "OG";
          eliminateFeatureTableSign(&tstr0,&fea);
          words = findAndSeparateWordsBy(&tstr0,' ',true);
          or_garnelle = toOneString(words);
          break;
      }
      break;
    case 'R':
/*############################################################*/
//cout << "R gefunden" << endl;
/*############################################################*/
      switch(tmp[1])
      {
        case 'N':
/*############################################################*/
//cout << "-N gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            if(!lines.empty())
            {
              tref = new GenomReferenceEmbl;
              for(int m = 0; m < (int)lines.size(); m++)
              {
                tref->updateReference(&(lines[m]));
              }
              tref->prepareReference();
              references.push_back(*tref);
              lines.clear();
              delete(tref);
            }
            lines.push_back(tstr0);
          }
          else
          {
            lines.push_back(tstr0);
            rn = true;
          }
          break;
        case 'C':
/*############################################################*/
//cout << "-C gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'P':
/*############################################################*/
//cout << "-P gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'X':
/*############################################################*/
//cout << "-X gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'G':
/*############################################################*/
//cout << "-G gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'A':
/*############################################################*/
//cout << "-A gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'T':
/*############################################################*/
//cout << "-T gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
        case 'L':
/*############################################################*/
//cout << "-L gefunden" << endl;
/*############################################################*/
          if(rn)
          {
            lines.push_back(tstr0);
          }
          break;
      }
      break;
    case 'C':
/*############################################################*/
//cout << "C gefunden" << endl;
/*############################################################*/
      switch(tmp[1])
      {
        case 'O':
/*############################################################*/
//cout << "-O gefunden" << endl;
/*############################################################*/
          lines.push_back(tstr0);
          co = true;
      	  co_cc = false;
          break;
        case 'C':
/*############################################################*/
//cout << "-C gefunden" << endl;
/*############################################################*/
          if(cc)
          {
            lines.push_back(tstr0);
          }
          else if(co_cc)
          {
            lines.push_back(tstr0);
          }
          else if(co)
          {
            lines.push_back(tstr0);
          }
          else
          {
            tstr1 = tstr0.substr(0,20);
            if(tstr1.find("join(") == string::npos)
            {
              lines.push_back(tstr0);
              cc = true;
              co = false;
		          co_cc = false;
            }
            else
            {
              lines.push_back(tstr0);
              co = true;
		          co_cc = true;
            }
          }
          break;
      }
      break;
    case 'F':
/*############################################################*/
//cout << "F gefunden" << endl;
/*############################################################*/
      if(tmp[1] == 'T')
      {
        feature_table.updateContain(&tstr0);
        ft = true;
      }
      break;
    case 'X':
/*############################################################*/
//cout << "X gefunden" << endl;
/*############################################################*/
      if(de)
      {
        if(!lines.empty())
        {
          tstr1 = toOneString(lines);
          fea = "DE";
          //cout << "DE Vorstring : " << tstr1 << endl;
          eliminateFeatureTableSign(&tstr1,&fea);
          words = findAndSeparateWordsBy(&tstr1,' ',true);
          description = toOneString(words);
        }
        de = false;
        lines.clear();
      }
      /* if(dr)
      {
        fea = "DR";
        eliminateFeatureTableSign(&tstr0,&fea,false);
        words = findAndSeparateWordsBy(&tstr0,' ',true);
        database_cross_reference.push_back(toOneString(words));
        dr = false;
      }*/
      if(kw)
      {
        kw = false;
      }
      if(oc)
      {
        if(!lines.empty())
        {
          tstr1 = toOneString(lines);
          fea = "OC";
          eliminateFeatureTableSign(&tstr1,&fea);
          words = findAndSeparateWordsBy(&tstr1,' ',true);
          organism_classification = words;
        }
        oc = false;
        lines.clear();
      }
      if(rn)
      {
        if(!lines.empty())
        {
          tref = new GenomReferenceEmbl;
          for(int m = 0; m < (int)lines.size(); m++)
          {
            tref->updateReference(&(lines[m]));
          }
          tref->prepareReference();
          references.push_back(*tref);
          delete(tref);
        }
        lines.clear();
        rn = false;
      }
      if(cc)
      {
        if(!lines.empty())
        {
          tstr1 = toOneString(lines);
          fea = "CC";
          eliminateFeatureTableSign(&tstr1,&fea,false,false);
          words = findAndSeparateWordsBy(&tstr1,' ',true);
          free_text_comments.push_back(toOneString(words));
        }
        lines.clear();
        cc = false;
      }
      if(co)
      {
        if(!lines.empty())
        {
          tstr1 = toOneString(lines);
	        if(co_cc)
      	  {
            fea = "CC";
        	}
          else
          {
            fea = "CO";
          }
          eliminateFeatureTableSign(&tstr1,&fea,false,false);
          words = findAndSeparateWordsBy(&tstr1,' ',true);
          constructed = toOneString(words);
        }
        lines.clear();
        co = false;
      	co_cc = false;
      }
      if(ft)
      {
//        cout << "Hier_0" << endl;
        feature_table.prepareFeatureTable();
//        cout << "Hier_1" << endl;
        ft = false;
      }
      if(sq)
      {
        cout << "sq bye by" << endl;
        prepared = true;
        flatfile.close();
        return 0;
      }
      break;
    case ' ':
      if(sq)
      {
        cout << "sq bye by" << endl;
        prepared = true;
        flatfile.close();
        return 0;
      }
      break;
  }
  }
/*############################################################*/
//cout << "Nach Parse-Switch und -While" << endl;
/*############################################################*/
	prepared = true;
  return 0;
}
