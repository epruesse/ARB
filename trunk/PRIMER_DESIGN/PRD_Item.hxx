#ifndef PRD_ITEM_HXX
#define PRD_ITEM_HXX

#include <cstdio>
#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class Item {

public:

  PRD_Sequence_Pos start_pos;		// index in sequence
  int              length;		// count of bases in primer

  int              GC_ratio;		// GC-ratio of primer
  int              temperature;		// temperature of primer

  Item *next;

  Item ( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ );
  Item ();
  ~Item () {};

  void  print             ( char *prefix_, char *suffix_ );	// print Items's values
  char* getPrimerSequence ( const char *sequence_ );		// return the string the Item describes
};

#else
#error PRD_Item.hxx included twice
#endif // PRD_ITEM_HXX

