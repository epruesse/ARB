#include "PRD_SequenceIterator.hxx"
#include <cstdio>

using namespace std;

//
// Constructor
//
SequenceIterator::SequenceIterator ( const char *sequence_, PRD_Sequence_Pos start_pos_, int max_length_, int direction_ )
{
  sequence   = sequence_;
  max_length = max_length_;
  pos        = start_pos_ -1; // -1 because nextBase() starts with pos++
  delivered  = 0;
  direction  = ( direction_ < 0 ) ? BACKWARD : FORWARD;
}

SequenceIterator::SequenceIterator ( const char *sequence_ )
{
  sequence   = sequence_;
  max_length = 0;
  pos        = 0;
  delivered  = 0;
  direction  = FORWARD;
}


//
// restart at new position
//
void SequenceIterator::restart ( PRD_Sequence_Pos start_pos_, int max_length_, int direction_ )
{
  max_length = max_length_;
  pos        = start_pos_ -1; // -1 because nextBase() starts with pos++
  delivered  = 0;
  direction  = ( direction_ < 0 ) ? BACKWARD : FORWARD;
}


//
// get next base if not at the end of sequence or already delivered max_length bases
//
char SequenceIterator::nextBase ()
{
  if ((max_length != IGNORE_LENGTH) && (delivered >= max_length)) return EOS;

  char cur_char;

  pos += direction;                                                 // <=> pos++ if direction=FORWARD else pos--
  if ( pos < 0 ) return EOS;                                        // reached front end ?
  cur_char = sequence[pos];
  if (cur_char > 'Z') cur_char = cur_char - ('a' - 'A');            // convert to upper case

  while ((cur_char != 'A') &&
	 (cur_char != 'T') &&
	 (cur_char != 'U') &&
	 (cur_char != 'C') &&
	 (cur_char != 'G') &&
	 (cur_char != 'N') &&
	 (cur_char != 'R') &&
	 (cur_char != 'Y') &&
	 (cur_char != 'M') &&
	 (cur_char != 'K') &&
	 (cur_char != 'W') &&
	 (cur_char != 'S') &&
	 (cur_char != 'B') &&
	 (cur_char != 'D') &&
	 (cur_char != 'H') &&
	 (cur_char != 'V') &&
	 (cur_char != EOS))
  {
    pos += direction;                                               // <=> pos++ if direction=FORWARD else pos--
    if ( pos < 0 ) return EOS;                                      // reached front end ?
    cur_char = sequence[pos];
    if (cur_char > 'Z') cur_char = cur_char - ('a' - 'A');          // convert to upper case
  };

  if (cur_char != EOS) delivered++;

  return cur_char;
}
