#include "PRD_Pair.hxx"

using namespace std;

//
// Constructors
//
Pair::Pair ( Item *one_, Item *two_, double rating_ )
{
  one    = one_;
  two    = two_;
  rating = rating_;
}

Pair::Pair ()
{
  one    = NULL;
  two    = NULL;
  rating = -1.0;
}

//
// print
//
void Pair::print ( char *prefix_, char *suffix_ )
{
  printf( "%s[%.3f (%p,%p)]%s", prefix_, rating, one, two, suffix_ );
}
