#include "GenomLocation.h"

int GenomLocation::tmp = 0;

#define ALL

using namespace std;

GenomLocation::GenomLocation()
{
	value = 0;
	range = false;
	complement = false;
  coll = false;
	join = false;
	order = false;
	point = false;
	roof = false;
	smaller_begin = false;
	bigger_begin = false;
	smaller_end = false;
	bigger_end = false;
  normal = true;
  actual_value = 0;
  pointer = false;
}

bool GenomLocation::hasMore()
{
  if(actual_value < (int)locations.size())
  {
    return true;
  }
  else
  {
    return false;
  }
}

GenomLocation GenomLocation::getNextValue(void)
{
 	return locations[actual_value++];
}

bool GenomLocation::hasMoreValues()
{
	if(actual_value < (int)locations.size())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void GenomLocation::setValue(const GenomLocation& nValue)
{
	locations.push_back(nValue);
}

void GenomLocation::parseLocation(string & location)
{
  int ls = location.size();
  int my_stack[ls];
  int levels[ls];
  int level_pointer = -1;
  int stack_pointer = -1;
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
  bool has_comma = false;
  vector<string> tvector;
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
      case ',':
        has_comma = true;
        break;
    }
  }
  #ifdef ALL
  cout << "Hier " << tmp++ << " String : -" << location << "-" << endl;
  #endif
  count_levels = (level_pointer+1) / 2;
  GenomLocation *tlocation;

  char_pointer = 0;
  end_pointer = ls-1;
  int current_level = -1;


  while(fore)
  {
    #ifdef ALL
    cout << "Hier a" << endl;
    #endif
//    t = location.at(char_pointer);
    t = location[char_pointer];
    if(count_levels > 0)
    {
      if((char_pointer == levels[1]) && (t == ')'))
      {
        fore = false;
      }
    }
    switch(t)
    {
      case 'o':
          #ifdef ALL
          cout << "Hier b" << endl;
          #endif
          order = true;
          normal = false;
          char_pointer += 5;
          tlocation = new GenomLocation;
          current_level = -1;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2+1]-char_pointer-1));
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              current_level = j;
            }
          }
          delete(tlocation);
          if((current_level != -1) && (count_levels > 1))
          {
            if(current_level < count_levels)
            {
              if((levels[(current_level+1)*2+1] - levels[current_level*2+1]) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else if(((levels[1] - levels[current_level*2+1]) - (count_levels - current_level-1)) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else
              {
                fore = false;
              }
            }
            else
            {
              fore = false;
            }
          }
          else
          {
            fore = false;
          }
          break;
      case 'j':
          #ifdef ALL
          cout << "Hier c" << endl;
          #endif
          join = true;
          normal = false;
          char_pointer += 4;
          tlocation = new GenomLocation;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2+1]-char_pointer-1));
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
            }
          }
          delete(tlocation);
          if((current_level != -1) && (count_levels > 1))
          {
            if(current_level < count_levels)
            {
              if((levels[(current_level+1)*2+1] - levels[current_level*2+1]) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else if(((levels[1] - levels[current_level*2+1]) - (count_levels - current_level-1)) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else
              {
                fore = false;
              }
            }
            else
            {
              fore = false;
            }
          }
          else
          {
            fore = false;
          }
          break;
      case 'c':
          #ifdef ALL
          cout << "Hier d" << endl;
          #endif
          complement = true;
          normal = false;
          char_pointer += 10;
          #ifdef ALL
          cout << "Levels : " << count_levels << endl;
          #endif
          tlocation = new GenomLocation;
          for(int j = 0; j < count_levels; j++)
          {
            #ifdef ALL
            cout << "Hier n" << endl;
            cout << "Level " << (j+1) << " Begin : " << levels[j*2] << endl;
            cout << "Level " << (j+1) << " End : " << levels[j*2+1] << endl;
            #endif
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2+1]-char_pointer-1));
              #ifdef ALL
              cout << "Hier p Substring : -" << substring << "-" <<  endl;
              #endif
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
            }
          }
          delete(tlocation);
          if((current_level != -1) && (count_levels > 1))
          {
            if(current_level < count_levels)
            {
              if((levels[(current_level+1)*2+1] - levels[current_level*2+1]) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else if(((levels[1] - levels[current_level*2+1]) - (count_levels - current_level-1)) > 1)
              {
                char_pointer = levels[(current_level+1)*2]+1;
              }
              else
              {
                fore = false;
              }
            }
            else
            {
              fore = false;
            }
          }
          else
          {
            fore = false;
          }
          break;
      case '.':
          #ifdef ALL
          cout << "Hier e" << endl;
          #endif
          if(until)
          {
            #ifdef ALL
            cout << "Hier e_0" << endl;
            #endif
            if(!has_ref && !has_comma)
            {
              #ifdef ALL
              cout << "Hier e_1" << endl;
              #endif
              range = true;
              normal = false;
              substring = location.substr(0,(char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              fore = false;
            }
            else if(!has_comma)
            {
              #ifdef ALL
              cout << "Hier e_2" << endl;
              #endif
              pointer = true;
              range = true;
              normal = false;
              substring = location.substr(0,ref_pointer);
              reference = substring;
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-2));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              has_ref = false;
              fore = false;
            }
            until = false;
          }
          else
          {
            #ifdef ALL
            cout << "Hier e_3" << endl;
            #endif
            until = true;
          }
          char_pointer++;
          break;
      case '^':
          #ifdef ALL
          cout << "Hier f" << endl;
          #endif
          if(!has_ref && !has_comma)
          {
            #ifdef ALL
            cout << "Hier f_0" << endl;
            #endif
            roof = true;
            normal = false;
            substring = location.substr(0,char_pointer);
            tlocation = new GenomLocation;
            tlocation->parseLocation(substring);
            locations.push_back(*tlocation);
            delete(tlocation);
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation = new GenomLocation;
            tlocation->parseLocation(substring);
            locations.push_back(*tlocation);
            delete(tlocation);
            fore = false;
          }
          else if(!has_comma)
          {
            #ifdef ALL
            cout << "Hier f_1" << endl;
            #endif
            pointer = true;
            roof = true;
            normal = false;
            substring = location.substr(0,ref_pointer);
            reference = substring;
            substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
            tlocation = new GenomLocation;
            tlocation->parseLocation(substring);
            locations.push_back(*tlocation);
            delete(tlocation);
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation = new GenomLocation;
            tlocation->parseLocation(substring);
            locations.push_back(*tlocation);
            delete(tlocation);
            has_ref = false;
            fore = false;
          }
          char_pointer++;
          break;
      case '<':
          #ifdef ALL
          cout << "Hier g" << endl;
          #endif
          if(!has_sparator && !has_comma)
          {
            #ifdef ALL
            cout << "Hier g_0" << endl;
            #endif
            if(until)
            {
              #ifdef ALL
              cout << "Hier g_1" << endl;
              #endif
              point = true;
              substring = location.substr(0,(char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              until = false;
              fore = false;
            }
            else
            {
              if(char_pointer == 0)
              {
                smaller_begin = true;
                substring = location.substr((char_pointer+1),(ls-char_pointer-1));
                value = atol(substring.c_str());
                fore = false;
              }
              else if(char_pointer == end_pointer)
              {
                smaller_end = true;
                substring = location.substr(0,char_pointer);
                value = atol(substring.c_str());
                fore = false;
              }
            }
          }
          char_pointer++;
          break;
      case '>':
          #ifdef ALL
          cout << "Hier h" << endl;
          #endif
          if(!has_sparator && !has_comma)
          {
            if(until)
            {
              point = true;
              substring = location.substr(0,(char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              until = false;
              fore = false;
            }
            else
            {
              if(char_pointer == 0)
              {
                bigger_begin = true;
                substring = location.substr((char_pointer+1),(ls-char_pointer-1));
                value = atol(substring.c_str());
                fore = false;
              }
              else if(char_pointer == end_pointer)
              {
                bigger_end = true;
                substring = location.substr(0,char_pointer);
                value = atol(substring.c_str());
                fore = false;
              }
            }
          }
          char_pointer++;
          break;
      case ',':
          #ifdef ALL
          cout << "Hier i" << endl;
          #endif
          tvector = getParts(location);
          normal = false;
          if(has_sparator || has_ref_atall)
          {
            #ifdef ALL
            cout << "Hier i_0" << endl;
            #endif
            coll = true;
          }
          for(int k = 0; k < (int)tvector.size(); k++)
          {
            tlocation = new GenomLocation;
            tlocation->parseLocation(tvector[k]);
            locations.push_back(*tlocation);
            delete(tlocation);
          }
          tvector.clear();
          fore = false;
          char_pointer++;
          break;
      case ':':
          #ifdef ALL
          cout << "Hier j" << endl;
          #endif
          has_ref = true;
          ref_pointer = char_pointer;
          char_pointer++;
          break;
      default:
          #ifdef ALL
          cout << "Hier k" << endl;
          #endif
          if(until)
          {
            #ifdef ALL
            cout << "Hier k_0" << endl;
            #endif
            if(!has_ref && !has_comma)
            {
              #ifdef ALL
              cout << "Hier k_1" << endl;
              #endif
              point = true;
              value = false;
              normal = false;
              substring = location.substr(0,(char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              fore = false;
            }
            else if(!has_comma)
            {
              #ifdef ALL
              cout << "Hier k_2" << endl;
              #endif
              point = true;
              value = false;
              normal = false;
              pointer = true;
              substring = location.substr(0,ref_pointer);
              reference = substring;
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation = new GenomLocation;
              tlocation->parseLocation(substring);
              locations.push_back(*tlocation);
              delete(tlocation);
              has_ref = false;
              fore = false;
            }
            until = false;
            
          }
          if(!has_ref_atall && !has_sep_atall && !has_bs_atall)
          {
            #ifdef ALL
            cout << "Hier k_3" << endl;
            #endif
            normal = true;
            substring = location.substr(char_pointer,ls);
            value = atol(substring.c_str());
            fore = false;
          }
          char_pointer++;
          break;
    }
    #ifdef ALL
    cout << "Hier l" << endl;
    #endif
    /*if(char_pointer == end_pointer)
    {
      fore = false;
    }*/
  }
  #ifdef ALL
  cout << "Hier m" << endl;
  #endif
  tmp--;
}

vector<string> GenomLocation::getParts(const string& beginning)
{
  int ss = beginning.size();
  char tc;
  int count_comma = 0;
  int commas[ss];
  int ccounter = 0;
  int an = 0;
  vector<string> tvctor;
  string tstring;
  for(int i = 0; i < ss; i++)
  {
    tc = beginning[i];
    if(tc == ',')
    {
      count_comma++;
      commas[ccounter++] = i;
    }
  }
  if(count_comma > 0)
  {
    for(int i = 0; i < count_comma;i++)
    {
      if(i == 0)
      {
        an = 0;
      }
      else
      {
        an = commas[i-1]+1;
      }
      tstring = beginning.substr(an,(commas[i]-an));
      an = commas[i]+1;
      tvctor.push_back(tstring);
    }
    tstring = beginning.substr(an,(ss-1));
    tvctor.push_back(tstring);
  }
  return tvctor;
}

