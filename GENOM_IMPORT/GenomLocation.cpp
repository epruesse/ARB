#include "GenomLocation.h"

int GenomLocation::tmp = 0;

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
  cout << "Hier " << tmp++ << " String : -" << location << "-" << endl;
  count_levels = (level_pointer+1) / 2;
  GenomLocation tlocation;

  char_pointer = 0;
  end_pointer = ls-1;


  while(fore)
  {
    cout << "Hier a" << endl;
    t = location.at(char_pointer);
    switch(t)
    {
      case 'o':
          cout << "Hier b" << endl;
          order = true;
          normal = false;
          char_pointer += 5;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2]-char_pointer));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
            }
          }
          fore = false;
          break;
      case 'j':
          cout << "Hier c" << endl;
          join = true;
          normal = false;
          char_pointer += 4;
          for(int j = 0; j < count_levels; j++)
          {
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2]-char_pointer));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
            }
          }
          fore = false;
          break;
      case 'c':
          cout << "Hier d" << endl;
          complement = true;
          normal = false;
          char_pointer += 10;
          for(int j = 0; j < count_levels; j++)
          {
            cout << "Hier n" << endl;
            if(levels[j*2] == char_pointer)
            {
              substring = location.substr((char_pointer+1),(levels[j*2]-char_pointer));
              cout << "Hier p Substring : -" << substring << "-" <<  endl;
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
            }
          }
          fore = false;
          break;
      case '.':
          cout << "Hier e" << endl;
          if(until)
          {
            range = true;
            normal = false;
            if(!has_ref)
            {
              substring = location.substr(0,(char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
            }
            else
            {
              pointer = true;
              substring = location.substr(0,ref_pointer);
              reference = substring;
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-2));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
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
          cout << "Hier f" << endl;
          roof = true;
          normal = false;
          if(!has_ref)
          {
            substring = location.substr(0,char_pointer);
            tlocation.parseLocation(substring);
            locations.push_back(tlocation);
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation.parseLocation(substring);
            locations.push_back(tlocation);
          }
          else
          {
            pointer = true;
            substring = location.substr(0,ref_pointer);
            reference = substring;
            substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
            tlocation.parseLocation(substring);
            locations.push_back(tlocation);
            substring = location.substr((char_pointer+1),(ls-char_pointer-1));
            tlocation.parseLocation(substring);
            locations.push_back(tlocation);
            has_ref = false;
          }
          fore = false;
          char_pointer++;
          break;
      case '<':
          cout << "Hier g" << endl;
          if(!has_sparator)
          {
            if(until)
            {
              point = true;
              substring = location.substr(0,(char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
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
          cout << "Hier h" << endl;
          if(!has_sparator)
          {
            if(until)
            {
              point = true;
              substring = location.substr(0,(char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
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
          cout << "Hier i" << endl;
          char_pointer++;
          break;
      case ':':
          cout << "Hier j" << endl;
          has_ref = true;
          ref_pointer = char_pointer;
          char_pointer++;
          break;
      default:
          cout << "Hier k" << endl;
          if(until)
          {
            point = true;
            value = false;
            normal = false;
            if(!has_ref)
            {
              substring = location.substr(0,(char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr(char_pointer,(ls-char_pointer));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
            }
            else
            {
              pointer = true;
              substring = location.substr(0,ref_pointer);
              reference = substring;
              substring = location.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              substring = location.substr((char_pointer+1),(ls-char_pointer-1));
              tlocation.parseLocation(substring);
              locations.push_back(tlocation);
              has_ref = false;
            }
            until = false;
            fore = false;
          }
          if(!has_ref_atall && !has_sep_atall && !has_bs_atall)
          {
            normal = true;
            substring = location.substr(char_pointer,ls);
            value = atol(substring.c_str());
            fore = false;
          }
          char_pointer++;
          break;
    }
    cout << "Hier l" << endl;
    /*if(char_pointer == end_pointer)
    {
      fore = false;
    }*/
  }
  cout << "Hier m" << endl;
  tmp--;
}

