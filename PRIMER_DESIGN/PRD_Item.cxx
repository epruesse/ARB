#include "PRD_Item.hxx"
#include "PRD_SequenceIterator.hxx"
#include <stdlib.h>

using namespace std;

//
// Constructors
//
Item::Item( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ )
{
  start_pos      = pos_;
  length         = length_;
  GC_ratio       = ratio_;
  temperature    = temperature_;
  next           = next_;
}
Item::Item ()
{
  start_pos      = -1;
  length         = -1;
  GC_ratio       = -1;
  temperature    = -1;
  next           = NULL;
}


//
// print
//
void Item::print ( char *prefix_, char *suffix_ )
{
  printf( "%s[(%li,%i),(%i,%i)]%s", prefix_, start_pos, length, GC_ratio, temperature, suffix_ );
}


//
// getPrimerSequence
//
// note : the sequence isnt stored in the Item, therefore must be given
//
char* Item::getPrimerSequence( const char *sequence_ )
{
  char *primer;
  if ( length <= 0 ) return "Item::getPrimerSequence : length <= 0 :(";

  primer = (char *)malloc((size_t)length+1);	// allocate memory ( needs to be given back via free() )
  SequenceIterator *iterator = new SequenceIterator( sequence_, start_pos, length, SequenceIterator::FORWARD ); // init iterator
  
  for ( int i = 0; i < length; i++ )	// grab as many bases as length is
    primer[i] = iterator->nextBase();
  primer[length] = '\x00';		// finish string

  delete iterator;			// give up iterator
  
  return primer;
}
