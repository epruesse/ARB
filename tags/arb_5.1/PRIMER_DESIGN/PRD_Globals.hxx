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
    
    BASE[(unsigned char)'A'] = 'T';
    BASE[(unsigned char)'T'] = 'A';
    BASE[(unsigned char)'U'] = 'A';
    BASE[(unsigned char)'C'] = 'G';
    BASE[(unsigned char)'G'] = 'C';

    BASE[(unsigned char)'R'] = 'Y';
    BASE[(unsigned char)'M'] = 'K';
    BASE[(unsigned char)'S'] = 'S';
    BASE[(unsigned char)'Y'] = 'R';
    BASE[(unsigned char)'K'] = 'M';
    BASE[(unsigned char)'W'] = 'W';
    BASE[(unsigned char)'V'] = 'B';
    BASE[(unsigned char)'B'] = 'V';
    BASE[(unsigned char)'D'] = 'H';
    BASE[(unsigned char)'H'] = 'D';
    BASE[(unsigned char)'N'] = 'N';
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

    INDEX[(unsigned char)'C'] = 0;
    INDEX[(unsigned char)'G'] = 1;
    INDEX[(unsigned char)'A'] = 2;
    INDEX[(unsigned char)'T'] = 3;
    INDEX[(unsigned char)'U'] = 3;

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

    FIELD[(unsigned char)'A'] =  1;
    FIELD[(unsigned char)'T'] =  2;
    FIELD[(unsigned char)'U'] =  2;
    FIELD[(unsigned char)'C'] =  4;
    FIELD[(unsigned char)'G'] =  8;

    FIELD[(unsigned char)'R'] =  9; // A  G
    FIELD[(unsigned char)'M'] =  5; // A C
    FIELD[(unsigned char)'S'] = 12; //   CG
    FIELD[(unsigned char)'Y'] =  6; //  TC
    FIELD[(unsigned char)'K'] = 10; //  T G
    FIELD[(unsigned char)'W'] =  3; // AT
    FIELD[(unsigned char)'V'] = 13; // A CG
    FIELD[(unsigned char)'B'] = 14; //  TCG
    FIELD[(unsigned char)'D'] = 11; // AT G
    FIELD[(unsigned char)'H'] =  7; // ATC
    FIELD[(unsigned char)'N'] = 15; // ATCG
  }
};

static BitField CHAR2BIT;

#else
#error PRD_Globals.hxx included twice
#endif // PRD_GLOBALS_HXX
