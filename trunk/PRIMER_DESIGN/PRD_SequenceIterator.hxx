#ifndef PRD_SEQUENCEITERATOR_HXX
#define PRD_SEQUENCEITERATOR_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class SequenceIterator {
private:

    const char *sequence;				// sequence of bases to be iterated
    int         max_length;			// maximum count of bases returned (afterwards only EOS is returned)
    int         stop_position;			// stop at this position even if not yet max_length bases delivered
    int         direction;			// direction to walk through sequence (FORWARD / BACKWARD)

public:
    static const int  IGNORE   = -1; // may be used at max_length_ or stop_pos_ parameter
    static const char EOS      = '\x00'; // = End Of Sequence
    static const int  FORWARD  = 1; // may be used at direction_ parameter
    static const int  BACKWARD = -1; // may be used at direction_ parameter

    PRD_Sequence_Pos pos;				// current index in sequence .. position of last returned base
    int              delivered;			// current count of returned bases

    SequenceIterator ( const char *sequence_, PRD_Sequence_Pos start_pos_, PRD_Sequence_Pos stop_pos_, int max_length_, int direction_ );
    SequenceIterator ( const char *sequence );

    void restart ( PRD_Sequence_Pos start_pos_, PRD_Sequence_Pos stop_pos_, int max_length_, int direction_ );
    unsigned char nextBase();
};

#else
#error PRD_SequenceIterator.hxx included twice
#endif // PRD_SEQUENCEITERATOR_HXX

