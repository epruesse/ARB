#include "PRD_Item.hxx"

using namespace std;

//
// Constructors
//
Item::Item( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ )
{
  start_pos      = pos_;
  length         = length_;
  CG_ratio       = ratio_;
  temperature    = temperature_;
  next           = next_;
}
Item::Item ()
{
  start_pos      = -1;
  length         = -1;
  CG_ratio       = -1;
  temperature    = -1;
  next           = NULL;
}


//
// Destructor
//
Item::~Item ()
{
  if ( next )
    delete next;
}


//
// print
//
void Item::print ( char *prefix_, char *suffix_ )
{
  printf( "%s[(%li,%i),%i,%i]%s", prefix_, start_pos, length, CG_ratio, temperature, suffix_ );
}
