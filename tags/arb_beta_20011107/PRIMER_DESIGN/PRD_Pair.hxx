#ifndef PRD_PAIR_HXX
#define PRD_PAIR_HXX

#include <cstdio>
#ifndef   PRD_ITEM_HXX
#include "PRD_Item.hxx"
#endif

class Pair {

public:

  Item   *one;
  Item   *two;
  double  rating;

  Pair ( Item *one_, Item *two_, double rating_ );
  Pair ();
  ~Pair () {};

  void        print        ( char *prefix, char *suffix, const char *sequence_ ); // print pair with the values of the items
  const char *get_result   ( const char *sequence_ , int max_primer_length, int max_position_length, int max_length_length);
  const char * get_primers ( const char *sequence_ );
};

#else
#error PRD_Pair.hxx included twice
#endif // PRD_Pair_HXX