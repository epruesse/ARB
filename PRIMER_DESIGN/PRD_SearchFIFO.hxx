#ifndef PRD_SEARCHFIFO_HXX
#define PRD_SEARCHFIFO_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif
#ifndef   PRD_NODE_HXX
#include "PRD_Node.hxx"
#endif
#ifndef   PRD_RANGE_HXX
#include "PRD_Range.hxx"
#endif

typedef struct SearchParameter {
  Node             *node;         // where am i currently in the tree
  SearchParameter  *next;
  SearchParameter  *previous;
};

class SearchFIFO {
private:
 
  SearchParameter  *begin;
  SearchParameter  *end;
  SearchParameter  *current;

  const char       *sequence;
  Node             *root;
  Range             primer_length;
  bool              expand_UPAC_Codes;
  PRD_Sequence_Pos  min_distance_to_next_match;
  
  void init( const char *sequence_, Node *root_, Range primer_length_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_UPAC_Codes_ );
  void erase      ( SearchParameter *param_ );
  void push_front ( Node *child_of_current_ );
public:

  int               size;

  unsigned long int push_count;
  unsigned long int iterate_count;
 
  SearchFIFO ( const char *sequence_, Node *root_, Range primer_length_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_UPAC_Codes_ );
  SearchFIFO ();
  ~SearchFIFO ();
 
  void push        ( char base_ );
  void iterateWith ( PRD_Sequence_Pos pos_, char base_ );
  void flush       ();
  void print       ();
};
 
#else
#error PRD_SearchFIFO.hxx included twice
#endif // PRD_SEARCHFIFO_HXX
