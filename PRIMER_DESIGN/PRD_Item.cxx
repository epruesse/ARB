#include "PRD_Item.hxx"
#include "PRD_SequenceIterator.hxx"
#include <stdlib.h>

using namespace std;

//
// Constructors
//
Item::Item( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ )
{
    end_pos        = pos_;
    start_position = -1;
  length           = length_;
  GC_ratio         = ratio_;
  temperature      = temperature_;
  next             = next_;
}
Item::Item ()
{
  end_pos      = -1;
  start_position = -1;
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
  printf( "%s[(%li,%i),(%i,%i)]%s", prefix_, start_position, length, GC_ratio, temperature, suffix_ );
}
int Item::sprint ( char *buf, char *primer, char *suffix_ , int max_primer_length, int max_position_length, int max_length_length)
{
  return sprintf(buf,  "| %-*s %*li %*i %3i %3i%s",
                 max_primer_length, primer,
                 max_position_length, start_position+1, // +1 for biologists ;-)
                 max_length_length, length,
                 GC_ratio, temperature, suffix_ );
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
  SequenceIterator *iterator = new SequenceIterator( sequence_, end_pos, length, SequenceIterator::BACKWARD ); // init iterator

  for ( int i = length-1; i >= 0; --i ) // grab as many bases as length is
    primer[i]    = iterator->nextBase();
  primer[length] = '\x00';		// finish string

  start_position = iterator->pos;
  delete iterator;			    // give up iterator

//   gb_assert(strlen(primer) == length);

  return primer;
}
