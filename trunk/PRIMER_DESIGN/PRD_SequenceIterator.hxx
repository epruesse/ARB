#ifndef PRD_SEQUENCEITERATOR_HXX
#define PRD_SEQUENCEITERATOR_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class SequenceIterator {
private:

  const char *sequence;				// sequence of bases to be iterated
  int         max_length;			// maximum count of bases returned (afterwards only EOS is returned)
  int         direction;			// direction to walk through sequence (FORWARD / BACKWARD)
  
public:
  const static int  IGNORE_LENGTH = -1;		// may be used at max_length_ parameter
  const static char EOS           = '\x00';	// = End Of Sequence
  const static int  FORWARD       =  1;		// may be used at direction_ parameter
  const static int  BACKWARD      = -1;		// may be used at direction_ parameter

  PRD_Sequence_Pos pos;				// current index in sequence .. position of last returned base
  int              delivered;			// current count of returned bases

  SequenceIterator ( const char *sequence_, PRD_Sequence_Pos start_pos_, int max_length_, int direction_ );
  SequenceIterator ( const char *sequence );
  
  void restart ( PRD_Sequence_Pos start_pos_, int max_length_, int direction_ );
  char nextBase();
};

#else
#error PRD_SequenceIterator.hxx included twice
#endif // PRD_SEQUENCEITERATOR_HXX

