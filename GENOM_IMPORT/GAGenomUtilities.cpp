/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomUtilities.h"
#include <cstdio>

using namespace std;
using namespace gellisary;

/*
 * Diese Funktion sucht in 'source_str' nach einem 'delimer_str' und ersetzt ihn durch 'replacing_str'
 * Das ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::replaceByString(string * source_str, string * delimer_str, string * replacing_str)
{
    int delimer_str_pos = 0;
    string target_str;
    string tmp_str;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if((source_str->c_str())[i] == (delimer_str->c_str())[delimer_str_pos])
        {
            delimer_str_pos++;
            if(delimer_str_pos == (int) delimer_str->size())
            {
                delimer_str_pos = 0;
                if(tmp_str.size() > 0)
                {
                    tmp_str = "";
                }
                for(int j = 0; j < (int) replacing_str->size(); j++)
                {
                    target_str += (replacing_str->c_str())[j];
                }
            }
            else
            {
                tmp_str += (*source_str)[i];
            }
        }
        else
        {
            delimer_str_pos = 0;
            if(tmp_str.size() > 0)
            {
                for(int j = 0; j < (int) tmp_str.size(); j++)
                {
                    target_str += tmp_str[j];
                }
                tmp_str = "";
            }
            target_str += (source_str->c_str())[i];
        }
    }
    *source_str = target_str;
}

/*
 * Diese Funktionen suchen in 'source_str' nach einem 'delimer_str' und nach einem '\r' und ersetzen ihn durch ' '
 * (Leerzeichen). Das ergebnis wird in 'target_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::preparePropertyString(string * source_str, string * delimer_str, string * target_str)
{
    vector<string> tmp_vector;
    GAGenomUtilities::replaceByWhiteSpaceCleanly(source_str,delimer_str);
    tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(source_str,' ',true);
    *target_str = GAGenomUtilities::toOneString(&tmp_vector,true);
}

void gellisary::GAGenomUtilities::replaceByWhiteSpaceCleanly(string * source_str, string * delimer_str)
{
    string del_str = "\r";
    string rep_str = " ";
    GAGenomUtilities::replaceByString(source_str,delimer_str,&rep_str);
    GAGenomUtilities::replaceByString(source_str,&del_str,&rep_str);
    GAGenomUtilities::onlyOneDelimerChar(source_str,' ');
}

/*
 * Wenn die Methode oben 'replaceByString()' einwandfrei funktioniert, dann
 * kann die Methode 'eliminateFeatureTableSignInEmbl()' getroest geloescht werden.
 */
void gellisary::GAGenomUtilities::eliminateFeatureTableSignInEmbl(string * source_str, string * delimer_str)
{
    char char_source_str[source_str->size()+1];
    strcpy(char_source_str,source_str->c_str());
    char char_delimer_str[delimer_str->size()+1];
    strcpy(char_delimer_str,delimer_str->c_str());
    bool feature = true;
    bool first = true;
    for(int i = 0; i < (int)strlen(char_source_str); i++)
    {
        if(char_source_str[i] == (delimer_str->c_str())[0])
        {
            if(first)
            {
                feature = true;
                first = false;
            }
            else
            {
                if(feature)
                {
                    char_source_str[i] = ' ';
                    char_source_str[i-1] = ' ';
                    first = true;
                    feature = false;
                }
            }
        }
        else if(char_source_str[i] == (delimer_str->c_str())[1])
        {
            if(feature)
            {
                char_source_str[i] = ' ';
                char_source_str[i-1] = ' ';
                feature = false;
            }
        }
        else
        {
            feature = false;
            first = true;
        }
    }
    *source_str = char_source_str;
}

void gellisary::GAGenomUtilities::eliminateSign(std::string * source_str, char delimer_char)
{
    char char_source_str[source_str->size()+1];
    strcpy(char_source_str,source_str->c_str());
    for(int i = 0; i < (int)strlen(char_source_str); i++)
    {
        if(char_source_str[i] == delimer_char)
        {
            char_source_str[i] = ' ';
        }
    }
    *source_str = char_source_str;
}

/*
 * Diese Funktion macht aus einem String-Vector einen string und gibt ihn zurück.
 * withSpace - wenn true, dann wird zwischen strings ein Leerzeichen reigeschoben.
 * Ansonsten ohne.
 */

string gellisary::GAGenomUtilities::toOneString(vector<string> * source_vector, bool withSpace)
{
    string target_str;
    vector<string> tmp_vector;
    tmp_vector = *source_vector;
    int vector_size = (int)tmp_vector.size();
    for(int i = 0; i < vector_size; i++)
    {
        if(i != 0)
        {
            if(withSpace)
            {
                target_str.append(" ");
            }
        }
        target_str.append(tmp_vector[i]);
    }
    return target_str;
}

/*
 * Diese Funktion sucht in 'source_str' nach einem 'delimer_char' und ersetzt beliebige Anzahl an 'delimer_char'
 * durch ein 'delimer_char'.
 * Das ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::onlyOneDelimerChar(string * source_str, char delimer_char)
{
    bool current_stat = false;
    string target;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if((source_str->c_str())[i] == delimer_char)
        {
            if(i == 0)
            {
                target += delimer_char;
                current_stat = true;
            }
            else
            {
                if(!current_stat)
                {
                    target += delimer_char;
                    current_stat = true;
                }
            }
        }
        else
        {
            /*if(current_stat)
              {
              current_stat = false;
              target += ' ';
              target += (source_str->c_str())[i];
              }
              else
              {*/
            target += (source_str->c_str())[i];
            current_stat = false;
            //}
        }
    }
    *source_str = target;
}

/*
 * Diese Funktion spaltet 'source_str' anhand eines 'delimer_char' und gibt das Ergebnis als String-Vector zurück.
 * withInnerSpace - wenn true, dann werden strings innerhalb der Doppelhochkommas nicht durchsucht, sondern als
 * ein String behandelt.
 */

vector<string> gellisary::GAGenomUtilities::findAndSeparateWordsByChar(string * source_str, char sep_char, bool withInnerString)
{
    GAGenomUtilities::onlyOneDelimerChar(source_str,sep_char);
    vector<string> target_vector;
    bool inInnerString = false;
    string tmp_str;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if(withInnerString)
        {
            if(inInnerString)
            {
                if((source_str->c_str())[i] == '"')
                {
                    inInnerString = false;
                    target_vector.push_back(tmp_str);
                    tmp_str = "";
                }
                else
                {
                    tmp_str += (source_str->c_str())[i];
                }
            }
            else
            {
                if((source_str->c_str())[i] == '"')
                {
                    inInnerString = true;
                    if(i > 0)
                    {
                        target_vector.push_back(tmp_str);
                        tmp_str = "";
                    }
                }
                else
                {
                    if((source_str->c_str())[i] == sep_char)
                    {
                        if(i > 0)
                        {
                            target_vector.push_back(tmp_str);
                            tmp_str = "";
                        }
                    }
                    else
                    {
                        tmp_str += (source_str->c_str())[i];
                    }
                }
            }
        }
        else
        {
            if((source_str->c_str())[i] == sep_char)
            {
                if(i > 0)
                {
                    target_vector.push_back(tmp_str);
                    tmp_str = "";
                }
            }
            else
            {
                tmp_str += (source_str->c_str())[i];
            }
        }
    }
    if(((int) tmp_str.size()) > 0)
    {
        target_vector.push_back(tmp_str);
    }
    return target_vector;
}

/*
 * Diese Funktion spaltet 'source_str' anhand eines 'delimer_sep_str' und gibt das Ergebnis als String-Vector zurück.
 * withInnerSpace - wenn true, dann werden strings innerhalb der Doppelhochkommas nicht durchsucht, sondern als
 * ein String behandelt.
 */

vector<string> gellisary::GAGenomUtilities::findAndSeparateWordsByString(string * source_str, string * sep_str, bool withInnerString)
{
    vector<string> target_vector;
    if(withInnerString)
    {
        bool isBegin = false;
        bool isEnd = false;
        vector<string> tmp_target_str;
        vector<string> tmp_target_str2;
        GAGenomUtilities::trimString(source_str);
        if((source_str->c_str())[0] == '"')
        {
            isBegin = true;
        }
        if((source_str->c_str())[((int) source_str->size())-1] == '"')
        {
            isEnd = true;
        }
        tmp_target_str = findAndSeparateWordsByChar(source_str,'"',false);
        if(tmp_target_str.size() > 0)
        {
            if((tmp_target_str.size() % 2) == 0)    //  gerade
            {
                if(isBegin && !isEnd)
                {
                    for(int i = 0; i < (int) tmp_target_str.size(); i++)
                    {
                        if((i % 2) == 0)
                        {
                            target_vector.push_back(tmp_target_str[i]);
                        }
                        else
                        {
                            string tmp_str = tmp_target_str[i];
                            tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
                            for(int j = 0; j < (int) tmp_target_str2.size(); j++)
                            {
                                target_vector.push_back(tmp_target_str2[j]);
                            }
                            tmp_target_str2.clear();
                        }
                    }
                }
                else if(!isBegin && isEnd)
                {
                    for(int i = 0; i < (int) tmp_target_str.size(); i++)
                    {
                        if((i % 2) == 1)
                        {
                            target_vector.push_back(tmp_target_str[i]);
                        }
                        else
                        {
                            string tmp_str = tmp_target_str[i];
                            tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
                            for(int j = 0; j < (int) tmp_target_str2.size(); j++)
                            {
                                target_vector.push_back(tmp_target_str2[j]);
                            }
                            tmp_target_str2.clear();
                        }
                    }
                }
            }
            else    //  ungerade
            {
                if(isBegin && isEnd)
                {
                    for(int i = 0; i < (int) tmp_target_str.size(); i++)
                    {
                        if((i % 2) == 0)
                        {
                            target_vector.push_back(tmp_target_str[i]);
                        }
                        else
                        {
                            string tmp_str = tmp_target_str[i];
                            tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
                            for(int j = 0; j < (int) tmp_target_str2.size(); j++)
                            {
                                target_vector.push_back(tmp_target_str2[j]);
                            }
                            tmp_target_str2.clear();
                        }
                    }
                }
                else if(!isBegin && !isEnd)
                {
                    if(((int) tmp_target_str.size()) > 1)
                    {
                        for(int i = 0; i < (int) tmp_target_str.size(); i++)
                        {
                            if((i % 2) == 1)
                            {
                                target_vector.push_back(tmp_target_str[i]);
                            }
                            else
                            {
                                string tmp_str = tmp_target_str[i];
                                tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
                                for(int j = 0; j < (int) tmp_target_str2.size(); j++)
                                {
                                    target_vector.push_back(tmp_target_str2[j]);
                                }
                                tmp_target_str2.clear();
                            }
                        }
                    }
                    else
                    {
                        target_vector.push_back(tmp_target_str[0]);
                    }
                }
            }
        }
    }
    else
    {
        string tmp_rep_str("\r");
        GAGenomUtilities::replaceByString(source_str,sep_str,&tmp_rep_str);
        target_vector = findAndSeparateWordsByChar(source_str,'\r',false);
    }
    return target_vector;
}

/*
 * Diese Funktion eliminiert in 'source_str' alle Leerzeichen am Anfang und am Ende.
 * Das Ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::trimString2(string * source_str)
{
    GAGenomUtilities::trimStringByChar2(source_str, ' ');
}

/*
 * Diese Funktion eliminiert in 'source_str' alle Leerzeichen am Anfang und am Ende.
 * Das Ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::trimString(string * source_str)
{
    GAGenomUtilities::trimStringByChar(source_str, ' ');
}
/*
  void gellisary::GAGenomUtilities::trimStringByChar(string * source_str, char trim_char)
  {
  string source = *source_str;
  int begin = -1;
  int end = -1;
  char tmp_char;
  int source_size = source.size();
  int pos[source_size];
  int current_pos = -1;
  bool other_char = false;

  for(int i = 0; i < source_size; i++)
  {
  tmp_char = source[i];
  if(tmp_char == trim_char)
  {
  current_pos++;
  pos[current_pos] = i;
  }
  else
  {
  other_char = true;
  }
  }
  if(!other_char)
  {
  *source_str = "";
  }
  else if(current_pos > -1)
  {
  if(pos[0] == 0) // soll vom Anfang 'getrimt' werden
  {
  begin = 0;
  for(int j = 1; j <= current_pos; j++)
  {
  if((pos[j] - begin) == 1)
  {
  begin = pos[j];
  }
  else
  {
  break;
  }
  }
  }
  if(pos[current_pos] == (source_size-1)) // soll vom Ende 'getrimt' werden
  {
  end = pos[current_pos];

  for(int k = current_pos; k >= 0; k--)
  {
  if((end - pos[k]) == 1)
  {
  end = pos[k];
  }
  else
  {
  break;
  }
  }
  }
  *source_str = source.substr(begin,(end-begin-1));
  }
  }*/

/*
 * Unterscheid zwischen trimStringByChar2() und trimStringByChar():
 * Leider sind beider Funktionen fehlerhaft:
 * trimStringByChar(): wenn ein String nicht mit 'trim_char' beginnt, dann wird das erste Zeichen trotzdem
 * gelöscht, ungeachtet dessen, ob es dem 'trim_char' gleicht oder nicht.
 * trimStringByChar2(): löst das Problem von trimStringByChar(), aber sie belässt ein 'trim_char' irgendwo
 * ein 'trim_char'
 */

/*
 * Diese Funktion eliminiert in 'source_str' alle 'trim_char' am Anfang und am Ende.
 * Das Ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::trimStringByChar(string * source_str, char trim_char)
{
    string target;
    int begin = -1;
    int end = -1;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if((source_str->c_str())[i] == trim_char)
        {
            begin = i;
        }
        else
        {
            break;
        }
    }
    for(int j = (((int) source_str->size())-1); j >= 0; j--)
    {
        if((source_str->c_str())[j] == trim_char)
        {
            end = j;
        }
        else
        {
            break;
        }
    }
    if(begin == -1)
    {
        begin = 0;
    }
    if(end == -1)
    {
        end = (int) source_str->size();
    }
    for(int k = (begin+1); k < end; k++)
    {
        target.push_back((source_str->c_str())[k]);
    }
    *source_str = target;
}

/*
 * Diese Funktion eliminiert in 'source_str' alle 'trim_char' am Anfang und am Ende.
 * Das Ergebnis wird in 'source_str' reingeschrieben.
 */

void gellisary::GAGenomUtilities::trimStringByChar2(string * source_str, char trim_char)
{
    string target;
    int begin = -1;
    int end = -1;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if((source_str->c_str())[i] == trim_char)
        {
            begin = i;
        }
        else
        {
            break;
        }
    }
    for(int j = (((int) source_str->size())-1); j >= 0; j--)
    {
        if((source_str->c_str())[j] == trim_char)
        {
            end = j;
        }
        else
        {
            break;
        }
    }
    if(begin == -1)
    {
        begin = 0;
    }
    if(end == -1)
    {
        end = (int) source_str->size();
    }
    for(int k = begin; k < end; k++)
    {
        target.push_back((source_str->c_str())[k]);
    }
    *source_str = target;
}

/*
 * Diese Funktion konvertiert ein 'sosurce_str' in ein integer.
 */

int gellisary::GAGenomUtilities::stringToInteger(string * source_str)
{
    return atoi(source_str->c_str());

    //  int target_int = 0;
    //  int tmp_int = 0;
    //  for(int i = (((int) source_str->size()) - 1); i >= 0; i--)
    //  {
    //      switch((source_str->c_str())[i])
    //      {
    //          case '0':
    //              tmp_int = 0;
    //              break;
    //          case '1':
    //              tmp_int = 1;
    //              break;
    //          case '2':
    //              tmp_int = 2;
    //              break;
    //          case '3':
    //              tmp_int = 3;
    //              break;
    //          case '4':
    //              tmp_int = 4;
    //              break;
    //          case '5':
    //              tmp_int = 5;
    //              break;
    //          case '6':
    //              tmp_int = 6;
    //              break;
    //          case '7':
    //              tmp_int = 7;
    //              break;
    //          case '8':
    //              tmp_int = 8;
    //              break;
    //          case '9':
    //              tmp_int = 9;
    //              break;
    //          default:
    //              break;
    //      }
    //      target_int += (tmp_int * (int)pow((double)10,i));
    //  }
    //  return target_int;
}

/*
 * Diese Funktion konvertiert ein integer in ein string.
 */

string gellisary::GAGenomUtilities::integerToString(int source_int)
{
    char buffer[50];
    sprintf(buffer, "%i", source_int);
    return string(buffer);

    //  string target_str;
    //  bool before = false;
    //  long source_double = (long) source_int; // nicht unbedingt notwendig trotzdem ...
    //  for(long i = 9; i >= 0; i++)
    //  {
    //             if((source_double >= pow(10,i)) && (source_double < pow((double)10,(i+1))))
    //          {
    //          target_str += (char)(((int)(source_double / pow((double)10,i)))+48);
    //          source_double = source_double % ((long)pow((double)10,i));
    //          before = true;
    //      }
    //      else
    //      {
    //          if(before)
    //          {
    //              target_str += '0';
    //          }
    //      }
    //  }
    //  return target_str;
}

/*
 * Diese Funktion generiert aus einem 'source_str' und einem 'gene_type' eine ID für Genes
 */

string gellisary::GAGenomUtilities::generateGeneID(string * source_str, int gene_type)
{
    string target_str("a");
    bool next = true;
    int pointer = 0;
    bool point = false;
    while(next)
    {
        switch((source_str->c_str())[pointer])
        {
            case 'c':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("c");
                pointer = pointer + 10;
                break;
            case 'o':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("o");
                pointer = pointer + 5;
                break;
            case 'j':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("j");
                pointer = pointer + 4;
                break;
            case ',':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("h");
                pointer = pointer + 1;
                break;
            case '^':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("e");
                pointer = pointer + 1;
                break;
            case '<':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("f");
                pointer = pointer + 1;
                break;
            case '>':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str.append("g");
                pointer = pointer + 1;
                break;
            case '.':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                else
                {
                    point = true;
                }
                pointer = pointer + 1;
                break;
            case '(': case ')':
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                pointer = pointer + 1;
                break;
            default:
                if(point)
                {
                    target_str.append("d");
                    point = false;
                }
                target_str += ((source_str->c_str())[pointer]);
                pointer = pointer + 1;
                break;
        }
        if(pointer == (int) source_str->size())
        {
            next = false;
        }
    }
    target_str.append("y");
    target_str.append(GAGenomUtilities::integerToString(gene_type));
    target_str.append("z");
    return target_str;
}

/*
 * Diese Funktion parst 'location'-Angabe von source aus feature table.
 * Und gibt alle integer-werte in einem Ineger-Vector zurück.
 */

vector<int> gellisary::GAGenomUtilities::parseSourceLocation(string * source_str)
{
    vector<int> target_vector;
    GAGenomUtilities::trimString(source_str);
    bool point = false;
    string tmp_str;
    for(int i = 0; i < (int) source_str->size(); i++)
    {
        if((source_str->c_str())[i] == '.')
        {
            if(!point)
            {
                point = true;
                if(tmp_str.size() > 0)
                {
                    target_vector.push_back(GAGenomUtilities::stringToInteger(&tmp_str));
                    tmp_str = "";
                }
            }
        }
        else
        {
            tmp_str.push_back((source_str->c_str())[i]);
        }
    }
    if(tmp_str.size() > 0)
    {
        target_vector.push_back(GAGenomUtilities::stringToInteger(&tmp_str));
        tmp_str = "";
    }
    return target_vector;
}

/*
 * Diese Funktion prüft, ob der 'source_str' ein neuer Gene ist.
 */

bool gellisary::GAGenomUtilities::isNewGene(string * source_str)
{
    string tmp_str = source_str->substr(4,16);
    GAGenomUtilities::trimString(&tmp_str);
    if(tmp_str.size() > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
    /*
      if(tmp_str[0] == ' ')
      {
      std::cout << "New Gene 003" << std::endl;
      if(tmp_str[2] == ' ')
      {
      std::cout << "New Gene 004 Falsch" << std::endl;
      return false;
      }
      else
      {
      std::cout << "New Gene 005 Wahr" << std::endl;
      return true;
      }
      }
      else
      {
      std::cout << "New Gene 006 Wahr" << std::endl;
      return true;
      }*/
}

/*
 * Diese Funktion prüft, ob der 'source_str' 'source' feature von feature Table ist.
 */

bool gellisary::GAGenomUtilities::isSource(string * source_str)
{
    string tmp_str = source_str->substr(4,16);
    GAGenomUtilities::trimString(&tmp_str);
    if(tmp_str.find("source") != string::npos)
    {
        return true;
    }
    else
    {
        return false;
    }
}
