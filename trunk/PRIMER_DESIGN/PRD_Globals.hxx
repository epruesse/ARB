#ifndef PRD_GLOBALS_HXX
#define PRD_GLOBALS_HXX

#include <limits.h>              // for LONG_MAX

#ifndef MAX_LONG
#define MAX_LONG LONG_MAX
#endif

#define PRD_MAX_SEQUENCE_POS MAX_LONG
typedef long int PRD_Sequence_Pos;



class BaseInverter {
public:
  char BASE[128];

  BaseInverter()
  {
    for ( int i=0; i < 128; i++)
      BASE[i] = '\x00';
    
    BASE['A'] = 'T';
    BASE['T'] = 'A';
    BASE['U'] = 'A';
    BASE['C'] = 'G';
    BASE['G'] = 'C';

    BASE['R'] = 'Y';
    BASE['M'] = 'K';
    BASE['S'] = 'S';
    BASE['Y'] = 'R';
    BASE['K'] = 'M';
    BASE['W'] = 'W';
    BASE['V'] = 'B';
    BASE['B'] = 'V';
    BASE['D'] = 'H';
    BASE['H'] = 'D';
    BASE['N'] = 'N';
  }
};

static BaseInverter INVERT;


class ChildLookupTable {
public:
  int INDEX[128];

  ChildLookupTable ()
  {
    for ( int i=0; i < 128; i++ )
      INDEX[i] = -1;

    INDEX['C'] = 0;
    INDEX['G'] = 1;
    INDEX['A'] = 2;
    INDEX['T'] = 3;
    INDEX['U'] = 3;

    INDEX['Y'] = 4;
    INDEX['K'] = 4;
    INDEX['S'] = 4;
    INDEX['R'] = 4;
    INDEX['M'] = 4;
    INDEX['W'] = 4;
    INDEX['B'] = 4;
    INDEX['V'] = 4;
    INDEX['H'] = 4;
    INDEX['D'] = 4;
    INDEX['N'] = 4;
  }
};

static ChildLookupTable CHAR2CHILD;


class BitField {
public:
  unsigned int FIELD[128];

  BitField()
  {
    for ( int i=0; i < 128; i++)
      FIELD[i] = 0;

    FIELD['A'] =  1;
    FIELD['T'] =  2;
    FIELD['U'] =  2;
    FIELD['C'] =  4;
    FIELD['G'] =  8;

    FIELD['R'] =  9;
    FIELD['M'] =  5;
    FIELD['S'] = 12;
    FIELD['Y'] =  6;
    FIELD['K'] = 10;
    FIELD['W'] =  3;
    FIELD['V'] = 13;
    FIELD['B'] = 14;
    FIELD['D'] = 11;
    FIELD['H'] =  7;
    FIELD['N'] = 15;
  }
};

static BitField CHAR2BIT;

#else
#error PRD_Globals.hxx included twice
#endif // PRD_GLOBALS_HXX
