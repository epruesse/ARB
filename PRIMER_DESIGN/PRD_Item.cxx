#include "PRD_Item.hxx"
#include "PRD_SequenceIterator.hxx"

#include <arb_mem.h>

#include <cstdlib>
#include <cstring>

using namespace std;

//
// Constructors
//
Item::Item(PRD_Sequence_Pos pos_, PRD_Sequence_Pos offset_, int length_, int ratio_, int temperature_, Item *next_)
{
    end_pos        = pos_;
    start_pos      = -1;
    offset         = offset_;
    length         = length_;
    GC_ratio       = ratio_;
    temperature    = temperature_;
    next           = next_;
}
Item::Item ()
{
    end_pos        = -1;
    start_pos      = -1;
    offset         = -1;
    length         = -1;
    GC_ratio       = -1;
    temperature    = -1;
    next           = NULL;
}


//
// print
//
void Item::print (const char *prefix_, const char *suffix_)
{
    printf("%s[(%li,%li,%i),(%i,%i)]%s", prefix_, start_pos, offset, length, GC_ratio, temperature, suffix_);
}


//
// sprint
//
int Item::sprint (char *buf, const char *primer, const char *suffix_,   int max_primer_length, int max_position_length, int max_length_length)
{
    return sprintf(buf,  "| %-*s %*li %*i %3i %3i%s",
                   max_primer_length, primer,
                   max_position_length, start_pos+1, // +1 for biologists ;-)
                   max_length_length, length,
                   GC_ratio, temperature, suffix_);
}

inline char *PRD_strdup(const char *s) {
    size_t  len    = strlen(s);
    char   *result = ARB_alloc<char>(len+1);
    strcpy(result, s);
    return result;
}

//
// getPrimerSequence
//
// note : the sequence isn't stored in the Item, therefore must be given
//
char* Item::getPrimerSequence(const char *sequence_)
{
    char *primer;
    if (length <= 0) return PRD_strdup("Item::getPrimerSequence : length <= 0 :(");

    ARB_alloc(primer, length+1);
    SequenceIterator *iterator = new SequenceIterator(sequence_, end_pos, SequenceIterator::IGNORE,   length, SequenceIterator::BACKWARD);  // init iterator

    for (int i = length-1; i >= 0; --i)   // grab as many bases as length is
        primer[i] = iterator->nextBase();
    primer[length] = '\x00';              // finish string

    start_pos = iterator->pos;

    delete iterator;                      // give up iterator

    return primer;
}
