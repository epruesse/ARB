#include "PRD_Range.hxx"
#include <iostream>

using namespace std;

Range::Range ( const PRD_Sequence_Pos value1_, const PRD_Sequence_Pos value2_ )
{
  if (value1_ <= value2_)
  {
    minimum = value1_;
    maximum = value2_;
  }
  else
  {
    minimum = value2_;
    maximum = value1_;
  }
}

Range::Range ()
{
  minimum = 0;
  maximum = 0;
}

bool Range::includes ( PRD_Sequence_Pos value_ )
{
   if ((value_ < minimum) || (value_ > maximum))
     return false;

   return true;
}

bool Range::includes( PRD_Sequence_Pos min_, PRD_Sequence_Pos max_ )
{
  return includes( min_ ) || includes( max_ );
}

void Range::print ( char *prefix_, char *suffix_ ) {
  cout << prefix_ << "(" << minimum << "," << maximum << ")" << suffix_;
}

void Range::println () {
  print("","");
  cout << endl;
}
