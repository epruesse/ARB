#ifndef PRD_GLOBALS_HXX
#define PRD_GLOBALS_HXX

#include <limits.h>              // for LONG_MAX

#ifndef MAX_LONG
#define MAX_LONG LONG_MAX
#endif

#define PRD_MAX_SEQUENCE_POS MAX_LONG
typedef long int PRD_Sequence_Pos;


//
// BaseInverter is used to invert bases while matching sequence
// backwards vs. the primertrees
//
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

//     INDEX['Y'] = -1;
//     INDEX['K'] = -1;
//     INDEX['S'] = -1;
//     INDEX['R'] = -1;
//     INDEX['M'] = -1;
//     INDEX['W'] = -1;
//     INDEX['B'] = -1;
//     INDEX['V'] = -1;
//     INDEX['H'] = -1;
//     INDEX['D'] = -1;
//     INDEX['N'] = -1;
  }
};

static ChildLookupTable CHAR2CHILD;


//
// 2^0 = 1 = A
// 2^1 = 2 = T/U
// 2^2 = 4 = C
// 2^3 = 8 = G
//
class BitField {
public:
  unsigned int FIELD[128]; // bitoperations are done as unsigned int :(

  BitField()
  {
    for ( int i=0; i < 128; i++)
      FIELD[i] = 0;

    FIELD['A'] =  1;
    FIELD['T'] =  2;
    FIELD['U'] =  2;
    FIELD['C'] =  4;
    FIELD['G'] =  8;

    FIELD['R'] =  9; // A  G
    FIELD['M'] =  5; // A C
    FIELD['S'] = 12; //   CG
    FIELD['Y'] =  6; //  TC
    FIELD['K'] = 10; //  T G
    FIELD['W'] =  3; // AT
    FIELD['V'] = 13; // A CG
    FIELD['B'] = 14; //  TCG
    FIELD['D'] = 11; // AT G
    FIELD['H'] =  7; // ATC
    FIELD['N'] = 15; // ATCG
  }
};

static BitField CHAR2BIT;

#else
#error PRD_Globals.hxx included twice
#endif // PRD_GLOBALS_HXX
