#ifndef PRD_ITEM_HXX
#define PRD_ITEM_HXX

#include <cstdio>
#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class Item {

public:

    PRD_Sequence_Pos end_pos;		// index in sequence
    PRD_Sequence_Pos start_position;		// index in sequence (or -1 if not calculated)
    int              length;		// count of bases in primer

    int              GC_ratio;		// GC-ratio of primer
    int              temperature;		// temperature of primer

    Item *next;

    Item ( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ );
    Item ();
    ~Item () {};

    void  print             ( char *prefix_, char *suffix_ ); // print Items's values
    int   sprint ( char *buf, char *prefix_, char *suffix_ , int max_primer_length, int max_position_length, int max_length_length);
    char* getPrimerSequence ( const char *sequence_ ); // return the string the Item describes

};

#else
#error PRD_Item.hxx included twice
#endif // PRD_ITEM_HXX

