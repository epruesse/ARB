// =============================================================== //
//                                                                 //
//   File      : PRD_Pair.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in February 2001                    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PRD_Pair.hxx"
#include <arb_mem.h>
#include <cstdlib>

using namespace std;

//
// Constructors
//
Pair::Pair (Item *one_, Item *two_, double rating_)
{
  one    = one_;
  two    = two_;
  rating = rating_;
}

Pair::Pair ()
{
  one    = NULL;
  two    = NULL;
  rating = -1.0;
}

const char * Pair::get_primers (const char *sequence_)
{
  if (!one || !two) return 0;

  static char *result     = 0;
  static int   result_len = 0;

  int max_result = (one ? one->length : 6) + (two ? two->length : 6) + 2;
  if (max_result > result_len) {
    free(result);
    result = 0;
  }

  if (!result) {
      ARB_alloc(result, max_result+1);
      result_len = max_result;
  }

  char *pone = one->getPrimerSequence(sequence_);
  char *ptwo = two->getPrimerSequence(sequence_);

  sprintf(result, "%s\n%s", pone, ptwo);

  free(ptwo);
  free(pone);

  return result;
}


const char * Pair::get_result (const char *sequence_,   int max_primer_length, int max_position_length, int max_length_length)
{
  static char *result     = 0;
  static int   result_len = 0;
  char        *primer;

  int max_result = (one ? one->length : 6) + (two ? two->length : 6) + 200;
  if (max_result>result_len) {
    free(result);
    result = 0;
  }

  if (!result) {
      ARB_alloc(result, max_result+1);
      result_len = max_result;
  }

  char *r = result;

  r += sprintf(r,   "%7.3f ", rating);

  if (one) {
    primer  = one->getPrimerSequence(sequence_);
    r      += one->sprint(r,   primer, " ", max_primer_length, max_position_length, max_length_length);
    free(primer);
  }
  else {
    r += sprintf(r, "(error),");
  }

  if (two) {
    primer  = two->getPrimerSequence(sequence_);
    r      += two->sprint(r, primer, "", max_primer_length, max_position_length,   max_length_length);
    free(primer);
  }
  else {
    r += sprintf(r, "(error)");
  }

  return result;
}


//
// print pair with the values of the items
//
void Pair::print (const char *prefix_, const char *suffix_, const char *sequence_)
{
    const char *result = get_result(sequence_, 0, 0, 0);
    printf("%s[%s]%s", prefix_, result, suffix_);
}
