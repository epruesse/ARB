#ifndef PRD_SEQUENCEITERATOR_HXX
#define PRD_SEQUENCEITERATOR_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class SequenceIterator {
private:

  const char *sequence;
  int         max_length;
  int         direction;
  
public:
  const static int  IGNORE_LENGTH = -1;
  const static char EOS           = '\x00';
  const static int  FORWARD       =  1;
  const static int  BACKWARD      = -1;

  PRD_Sequence_Pos pos;
  int              delivered;

  SequenceIterator ( const char *sequence_, PRD_Sequence_Pos start_pos_, int max_length_, int direction_ );
  SequenceIterator ( const char *sequence );
  
  void restart ( PRD_Sequence_Pos start_pos_, int max_length_, int direction_ );
  char nextBase();
};

#else
#error PRD_SequenceIterator.hxx included twice
#endif // PRD_SEQUENCEITERATOR_HXX

