#include "GenomUtilities.h"




void eliminateFeatureTableSign(string * source_string, string * fts, bool semi, bool equal)
{
	char chars[source_string->size()+1];
	strcpy(chars,source_string->c_str());
	char b;
	char feac[fts->size()+1];
	strcpy(feac,fts->c_str());
	bool feature = true;
	bool first = true;
	for(int i = 0; i < (int)strlen(chars); i++)
	{
		b = chars[i];
		if(b == feac[0])
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
    				chars[i] = ' ';
    				chars[i-1] = ' ';
            first = true;
    				feature = false;
    			}
		    }
		}
		else if(b == feac[1])
		{
			if(feature)
			{
				chars[i] = ' ';
				chars[i-1] = ' ';
				feature = false;
			}
		}
		else if(b == 0xa)
		{
			chars[i] = ' ';
		}
    else if(b == '=')
    {
      if(equal)
      {
        chars[i] = ' ';
      }
    }
    else if(b == ';')
    {
      if(semi)
      {
        chars[i] = ' ';
      }
    }
		else
		{
			feature = false;
      first = true;
		}
	}
	*source_string = chars;
}

string parseSequenceDataLine(string & source)
{
  string target = source;
  vector<string> words = findAndSeparateWordsBy(&target,' ',true);
  int wsize = words.size();
  vector<string> twords;
  for(int i = 0; i < (wsize - 1); i++)
  {
      //     twords.push_back(words.at(i));
      twords.push_back(words[i]);
  }
  return toOneString(twords,false);
}

bool isNewGene(string & line)
{
	string tstr = line.substr(5,20);
	char c = tstr.at(0);
	if(c == ' ')
	{
		c = tstr.at(1);
		if(c == ' ')
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		return true;
	}
}

bool isSource(string & line)
{
	string tstr = line.substr(5,20);
	if(tstr.find("source") != string::npos)
	{
		return true;
	}
	else
	{
		return false;
	}
}

vector<long> parseSourceLocation(string & loc)
{
	char buff[loc.size()];
	char buff_one[512];
	char buff_two[512];
//  cout << "Location : " << loc << endl;
	strcpy(buff,loc.c_str());
	bool ein = false;
	int one = 0;
	int two = 0;
	vector<long> cipher;
	for(int i = 0; i < (int)strlen(buff); i++)
	{
		if(buff[i] == '.')
		{
			if(ein)
			{
				two = i;
				break;
			}
			else
			{
				ein = true;
				one = i;
			}
		}
	}
	if(one != two)
	{
		for(int i = 0; i < one; i++)
		{
			buff_one[i] = buff[i];
		}
		buff_one[one] = '\0';
		int j = 0;
		for(int i = two; i < (int)strlen(buff); i++)
		{
			buff_two[j++] = buff[i+1];
		}
		buff_two[j] = '\0';
		cipher.push_back(atol(buff_one));
		cipher.push_back(atol(buff_two));
	}
	return cipher;
}

string toOneString(vector<string> & str_source, bool withSpace)
{
	string target_string("");
	for(int i = 0; i < (int)str_source.size(); i++)
	{
		if(i != 0)
		{
      if(withSpace)
      {
        target_string += " ";
      }
		}
		target_string += str_source[i];
	}
	return target_string;
}

string trimString(string str_to_trim)
{
	char cstr[10240];
	strcpy(cstr,str_to_trim.c_str());
	int csize = strlen(cstr);
	int begin = 0;
	int end = 0;

	char buff[csize];
	int j = 0;
	for(int i = 0; i < csize; i++)
	{
		if(cstr[i] != ' ')
		{
			begin = i;
			break;
		}
	}
	for(int i = (csize-1); i >= 0; i--)
	{
		if(cstr[i] != ' ')
		{
			end = i;
			break;
		}
	}
	for(int i = begin; i <= end; i++)
	{
		buff[j++] = cstr[i];
	}
	buff[j] = '\0';
	string str(buff);
	return str;
}

vector<string> findAndSeparateWordsBy(string * origin, char separator, bool spaceInString)
{
//	eliminateFeatureTableSign(origin,fea);
	vector<string> words;
	char chars[origin->size()+1];
	strcpy(chars,origin->c_str());
//	char feac[1024];
//	strcpy(feac,fea->c_str());
	int cs = strlen(chars);
	char word[cs];
	char b;
	int other_count = 0;
	bool beforeEmpty = true;
	vector<string> buffer;
	bool sopen = false;
  //cout << "Der Text nochmal=" << *origin << endl;
  //cout << "Der Text nochmal als char*=" << chars << endl;

	// for_1 : spaltet die Zeichenkette in Wörter auf,
	//         wobei die Zeichenkette in Einfürungszeichen unverändert bleibt (wenn gewünscht).
	//				 Einführungszeichen werden eliminiert.
  for(int i = 0; i < cs; i++)
	{
   	b = chars[i];
    //cout << "Buchstabe " << i << " = " << b << endl;
		if(b != separator)
		{
			if(b == 0x22)
			{
				if(sopen)
				{
					word[other_count] = '\0';
					other_count = 0;
					string str(word);
					words.push_back(str);
					sopen = false;
				}
				else
				{
					if(other_count > 0)
					{
						word[other_count] = '\0';
						other_count = 0;
						string str(word);
						words.push_back(str);
					}
					other_count = 0;
					beforeEmpty = false;
					sopen = true;
				}
			}
			else
			{
				if(beforeEmpty)
				{
					other_count = 0;
					word[other_count++] = b;
					beforeEmpty = false;
				}
				else
				{
					word[other_count++] = b;
					if(i == (cs-1))
					{
						word[other_count] = '\0';
						string str(word);
						words.push_back(str);
					}
				}
			}
		}
		else if(sopen)
		{
			if(spaceInString)
			{
				word[other_count++] = b;
			}
		}
		else
		{
			if(other_count > 0)
			{
				word[other_count] = '\0';
				other_count = 0;
				string str(word);
				words.push_back(str);
				beforeEmpty = true;
			}
		}
	}// end of for_1
	if(separator == ',')
	{
		for(int i = 0; i < (int)words.size(); i++)
		{
			buffer.push_back(trimString(words[i]));
		}
		return buffer;
	}
	return words;
}

/*
string getFunction(string & loc_with_function)
{
	char buff[10240];
	srtcpy(buff,loc_with_function.c_str());
	int csize = strlen(buff);
	bool isComplement = false;
	bool isOrder = false;
	bool isJoin = false;
	char sign[1024];
	int signing = 0;
	for(int i = 0; i < csize; i++)
	{
		switch(buff[i])
		{
			case 'o':
					if(!isComplement && !isOrder && !isJoin)
					{
						sign[signing++] = 'o';
						sign[signing++] = i;
						isOrder = true;
					}
				break;
			case 'c':
				  if(!isComplement && !isOrder && !isJoin)
					{
						sign[signing++] = 'c';
						sign[signing++] = i;
						isComplement = true;
					}
				break;
			case 'j':
				  if(!isComplement && !isOrder && !isJoin)
					{
						sign[signing++] = 'j';
						sign[signing++] = i;
						isJoin = true;
					}
				break;
			case 'r':
					if(!isComplement && isOrder && !isJoin)
					{
						sign[signing++] = i;
						isOrder = false;
					}
				break;
			case 't':
				  if(isComplement && !isOrder && !isJoin)
					{
						sign[signing++] = i;
						isComplement = false;
					}
				break;
			case 'n':
				  if(!isComplement && !isOrder && isJoin)
					{
						sign[signing++] = i;
						isJoin = false;
					}
				break;
		}
	}
	sign[signing] = '\0';
	string str2(sign);
	return str2;
}*/

string generateID(string source, int itype)
{
//cout << "Hier_6.4.0" << endl;
  string target("a");
  string tstr;
	char c;
  int ss = source.size();
  int pointer = 0;
//  int level = 0;
//  int auf = 0;
  bool fore = true;
  bool point = false;
//  cout << "Hier_6.4.1" << endl;
  while(fore)
  {
    c = source.at(pointer);
//    cout << "Hier_6.4.2 =" << c << endl;
    switch(c)
    {
      case 'c':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "c";
        pointer = pointer + 10;
        break;
      case 'o':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "o";
        pointer = pointer + 5;
        break;
      case 'j':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "j";
        pointer = pointer + 4;
        break;
      case ',':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "h";
        pointer++;
        break;
      case '^':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "e";
        pointer++;
        break;
      case '<':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "f";
        pointer++;
        break;
      case '>':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + "g";
        pointer++;
        break;
      case '.':
        if(point)
        {
          target = target + "b";
          point = false;
        }
        else
        {
          point = true;
        }
        pointer++;
        break;
      case '(': case ')':
        if(point)
        {
          target = target + "d";
          point = false;
        }
        pointer++;
        break;
      default:
        if(point)
        {
          target = target + "d";
          point = false;
        }
        target = target + c;
        pointer++;
        break;
    }
//    cout << "Hier_6.4.3 pointer=" << pointer << endl;
    if(pointer == ss)
    {
//      cout << "Hier_6.4.5" << endl;
      fore = false;
    }
//    cout << "Hier_6.4.4" << endl;
  }
  target = target + "y";
  target = target + intToString(itype);
  target = target + "z";
//  cout << "Hier_6.4.20" << endl;
  return target;
}
// nur fuer die zahlen zwischen 0 und 99, fuer die groesseren zahlen
// muss man die funktion um die pruefung ergaenzen, ob die nullen
// drin stehen. z.B. 450032 ohne pruefung (und momentan) : 4532, mit pruefung : 450032
string intToString(int c)
{
  unsigned int a = (unsigned int) c;
  string b;
  /*if((a >= 1000000000) && (a <= 4294967296))
  {
    b += (char)(((int)(a / 1000000000))+48);
    a = a % 1000000000;
  }
  if((a >= 100000000) && (a < 1000000000))
  {
    b += (char)(((int)(a / 100000000))+48);
    a = a % 100000000;
  }
  if((a >= 10000000) && (a < 100000000))
  {
    b += (char)(((int)(a / 10000000))+48);
    a = a % 10000000;
  }
  if((a >= 1000000) && (a < 10000000))
  {
    b += (char)(((int)(a / 1000000))+48);
    a = a % 1000000;
  }
  if((a >= 100000) && (a < 1000000))
  {
    b += (char)(((int)(a / 100000))+48);
    a = a % 100000;
  }
  if((a >= 10000) && (a < 100000))
  {
    b += (char)(((int)(a / 10000))+48);
    a = a % 10000;
  }
  if((a >= 1000) && (a < 10000))
  {
    b += (char)(((int)(a / 1000))+48);
    a = a % 1000;
  }
  if((a >= 100) && (a < 1000))
  {
    b += (char)(((int)(a / 100))+48);
    a = a % 100;
  }*/
  if((a >= 10) && (a < 100))//
  {
    b += (char)(((int)(a / 10))+48);
    a = a % 10;
  }
  if((a >= 1) && (a < 10)) // one
  {
    b += (char)(((int)(a / 1))+48);
    a = a % 1;
  }
  else if(a==0) // zero
  {
    b += (char)48;
  }
  return b;
}

void trimByDoubleQuote(string & source)
{
	int be = source.find_first_of(0x22);
	int en = source.find_last_of(0x22);
	string target = source.substr(be+1,en-be-1);
	source = target;
}

void onlyOneSpace(string & source)
{
	string target;
	vector<string> targets;
	targets = findAndSeparateWordsBy(&source,' ',true);
	target = toOneString(targets);
	source = target;
}

/*
GenomLocation splitLocation(string & location)
{
  int ls = location.size();
  int my_stack[ls];
  int levels[ls];
  int level_pointer = -1;
  int stack_pointr = -1;
  char t;
  int count_levels;
  int char_pointer = -1;
  int end_pointer = -1;
  int ref_pointer = -1;
  bool fore = true;
  bool until = false;
  bool has_sparator = false;
  bool has_ref = false;
  bool has_ref_atall = false;
  bool has_sep_atall = false;
  bool has_bs_atall = false;
  string substring;

  for(int i = 0; i < ls; i++)
  {
    t = location.at(i);
    switch(t)
    {
      case '(':
        my_stack[++stack_pointer] = i;
        break;
      case ')':
        levels[++level_pointer] = my_stack[stack_pointer--];
        levels[++level_pointer] = i;
        break;
      case '.': case '^':
        has_sparator = true;
        has_sep_atall = true;
        break;
      case ':':
        has_ref_atall = true;
        break;
      case '<': case '>':
        has_bs_atall = true;
        break;
    }
  }
  count_levels = (level_pointer+1) / 2;
  GenomLocation tlocation;

  char_pointer = 0;
  end_pointer = ls-1;


  while(fore)
  {
    t = location.at(char_pointer);
    switch(t)
    {
      case 'o':
//          tlocation = new GenomLocation;
          tlocation.isOrder(true);
          tlocation.isSingleValue(false);
          char_pointer += 5;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substring((char_pointer+1),(levels[j*2]-char_pointer));
              tlocation.setValue(splitLocation(substring));
            }
          }
          fore = false;
          break;
      case 'j':
//          tlocation = new GenomLocation;
          tlocation.isJoin(true);
          tlocation.isSingleValue(false);
          char_pointer += 4;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2]-char_pointer));
              tlocation.setValue(splitLocation(substring));
            }
          }
          fore = false;
          break;
      case 'c':
//          tlocation = new GenomLocation;
          tlocation.isComplement(true);
          tlocation.isSingleValue(false);
          char_pointer += 10;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2]-char_pointer));
              tlocation.setValue(splitLocation(substring));
            }
          }
          fore = false;
          break;
      case '.':
//          tlocation = new GenomLocation;
          if(until)
          {
            tlocation.isRanged(true);
            tlocation.isSingleValue(false);
            if(!has_ref)
            {
              substring = location.substr(0,(char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.setValue(splitLocation(substring));
            }
            else
            {
              tlocation.isReference(true);
              substring = location.substr(0,ref_pointer);
              tlocation.setReference(substring);
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-2));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              has_ref = false;
            }
            until = false;
            fore = false;
          }
          else
          {
            until = true;
          }
          char_pointer++;
          break;
      case '^':
          tlocation.isRoof(true);
          tlocation.isSingleValue(false);
          if(!has_ref)
          {
//            tlocation = new GenomLocation;
            substring = location.substr(0,char_pointer);
            tlocation.setValue(splitLocation(substring));
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation.setValue(splitLocation(substring));
          }
          else
          {
//            tlocation = new GenomLocation;
            tlocation.isReference(true);
            substring = location.substr(0,ref_pointer);
            tlocation.setReference(substring);
            substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
            tlocation.setValue(splitLocation(substring));
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation.setValue(splitLocation(substring));
            has_ref = false;
          }
          fore = false;
          char_pointer++;
          break;
      case '<':
          if(!has_sparator)
          {
            if(until)
            {
//              tlocation = new GenomLocation;
              tlocation.isPoint(true);
              substring = location.substr(0,(char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.setValue(splitLocation(substring));
              until = false;
              fore = false;
            }
            else
            {
//              tlocation = new GenomLocation;
              if(char_pointer == 0)
              {
                tlocation.isSmallerBegin(true);
                substring = location.substr((char_pointer+1),(ls-char_pointer-1));
                tlocation.setSingleValue(atol(substring.c_str()));
                fore = false;
              }
              else if(char_pointer == end_pointer)
              {
                tlocation.isSmallerEnd(true);
                substring = location.substr(0,char_pointer);
                tlocation.setSingleValue(atol(substring.c_str()));
                fore = false;
              }
            }
          }
          char_pointer++;
          break;
      case '>':
          if(!has_sparator)
          {
            if(until)
            {
              tlocation = new GenomLocation;
              tlocation.isPoint(true);
              substring = location.substr(0,(char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.setValue(splitLocation(substring));
              until = false;
              fore = false;
            }
            else
            {
//              tlocation = new GenomLocation;
              if(char_pointer == 0)
              {
                tlocation.isBiggerBegin(true);
                substring = location.substr((char_pointer+1),(ls-char_pointer-1));
                tlocation.setSingleValue(atol(substring.c_str()));
                fore = false;
              }
              else if(char_pointer == end_pointer)
              {
                tlocation.isBiggerEnd(true);
                substring = location.substr(0,char_pointer);
                tlocation.setSingleValue(atol(substring.c_str()));
                fore = false;
              }
            }
          }
          char_pointer++;
          break;
      case ',':
          char_pointer++;
          break;
      case ':':
          has_ref = true;
          ref_pointer = char_pointer;
          char_pointer++;
          break;
      default:
          if(until)
          {
         //   tlocation = new GenomLocation;
            tlocation.isPoint(true);
            tlocation.isSingleValue(false);
            if(!has_ref)
            {
              substring = location.substr(0,(char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.setValue(splitLocation(substring));
            }
            else
            {
              tlocation.isReference(true);
              substring = location.substr(0,ref_pointer);
              tlocation.setReference(substring);
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
              tlocation.setValue(splitLocation(substring));
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.setValue(splitLocation(substring));
              has_ref = false;
            }
            until = false;
            fore = false;
          }
          if(!has_ref_atall && !has_sep_atall && !has_bs_atall)
          {
            tlocation.isSingleValue(true);
            substring = location.substr(char_pointer,ls);
            tlocation.setSingleValue(atol(substring.c_str()));
            fore = false;
          }
          char_pointer++;
          break;
    }*/
    /*if(char_pointer == end_pointer)
    {
      fore = false;
    }*/
 // }
//  return tlocation;

  /*int csize = strlen(buff);
	bool isComplement = false;
	bool isOrder = false;
	bool isJoin = false;

	char stack[1024];
	int sp = 0;
	for(int i = 0; i < csize; i++)
	{
		switch(buff[i])
		{
			case 'o':
					if(!isComplement && !isOrder && !isJoin)
					{
						isOrder = true;
						stack[sp++] = 'o';
					}
				break;
			case 'c':
				  if(!isComplement && !isOrder && !isJoin)
					{
						isComplement = true;
					}
				break;
			case 'j':
				  if(!isComplement && !isOrder && !isJoin)
					{
						isJoin = true;
					}
				break;
			case 'r':
					if(!isComplement && isOrder && !isJoin)
					{
						isOrder = false;
					}
				break;
			case 't':
				  if(isComplement && !isOrder && !isJoin)
					{
						isComplement = false;
					}
				break;
			case 'n':
				  if(!isComplement && !isOrder && isJoin)
					{
						isJoin = false;
					}
				break;
		}
	}*/
//}
