#ifndef PRD_RANGE_HXX
#define PRD_RANGE_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class Range {
private:
  PRD_Sequence_Pos minimum;
  PRD_Sequence_Pos maximum;

public:
  Range ( const PRD_Sequence_Pos value1_, const PRD_Sequence_Pos value2_ );
  Range ();
  ~Range () {};

  PRD_Sequence_Pos  min()   { return minimum; }
  PRD_Sequence_Pos  max()   { return maximum; }
  PRD_Sequence_Pos  range() { return maximum - minimum; }
  bool includes ( PRD_Sequence_Pos value_ );				// check if given value is in range
  bool includes ( PRD_Sequence_Pos min_, PRD_Sequence_Pos max_ );	// check if given range overlaps self
  void print    ( char *prefix_,char *suffix_ );			// print range
};

#else
#error PRD_Range.hxx included twice
#endif // PRD_RANGE_HXX

