#include "PRD_Range.hxx"
#include <iostream>

using namespace std;

//
// constructors
//
Range::Range (const PRD_Sequence_Pos value1_, const PRD_Sequence_Pos value2_)
{
  minimum = value1_;
  maximum = value2_;
}

Range::Range ()
{
  minimum = 0;
  maximum = 0;
}


//
// check if given value is in range
//
bool Range::includes (PRD_Sequence_Pos value_)
{
   if ((value_ < minimum) || (value_ > maximum)) return false;

   return true;
}


//
// check if given range overlaps self
//
bool Range::includes(PRD_Sequence_Pos min_, PRD_Sequence_Pos max_)
{
  return includes(min_) || includes(max_);
}


//
// print range
//
void Range::print (const char *prefix_, const char *suffix_) {
  cout << prefix_ << "(" << minimum << "," << maximum << ")" << suffix_;
}
