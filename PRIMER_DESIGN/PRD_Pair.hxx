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

  void print ( char *prefix, char *suffix );
};

#else
#error PRD_Pair.hxx included twice
#endif // PRD_Pair_HXX
