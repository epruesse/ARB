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
 
    SearchParameter *begin;     // start of list of positions in tree
    SearchParameter *end;       // end of list of positions in tree
    SearchParameter *current;   // points to the currently examined position in the list

    Node             *root;     // rootnode of primertree to be searched in
    bool              expand_IUPAC_Codes; // enable/disable expasnion of IUPAC codes
    PRD_Sequence_Pos  min_distance_to_next_match; // if a match is found out of that distance its ignored (not deleted)

    void init( Node *root_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_IUPAC_Codes_ );
    void erase      ( SearchParameter *param_ ); // erase the position from the list (tries not to invalidate current)
    void push_front ( Node *child_of_current_ ); // append new position in front of the list

public:

    SearchFIFO ( Node *root_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_IUPAC_Codes_ );
    SearchFIFO ();
    ~SearchFIFO ();

    void push        ( unsigned char base_ ); // append new position (node=root) to the end of the list
    void iterateWith ( PRD_Sequence_Pos pos_, unsigned char base_ ); // tries to iterate all positions in the list with the given base
    void flush       ();        // erase all positions of the list
    void print       ();        // print all positions in the list
};

#else
#error PRD_SearchFIFO.hxx included twice
#endif // PRD_SEARCHFIFO_HXX
