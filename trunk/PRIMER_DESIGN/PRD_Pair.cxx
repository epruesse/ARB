#include "PRD_Pair.hxx"
#include <stdlib.h>

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
// print pair with the values of the items
//
void Pair::print ( char *prefix_, char *suffix_, const char *sequence_ )
{
  char *primer;

  printf( "%s[%.3f (", prefix_, rating );

  if ( one ) {
    primer = one->getPrimerSequence(sequence_);
    one->print( primer,"," );
    free( primer );
  }
  else
    printf("(nil),");

  if ( two ) {
    primer = two->getPrimerSequence(sequence_);
    two->print( primer, "" );
    free( primer );
  }
  else
    printf("(nil)");

  printf("]%s", suffix_);
}
