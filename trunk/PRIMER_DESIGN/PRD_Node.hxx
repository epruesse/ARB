#ifndef PRD_NODE_HXX
#define PRD_NODE_HXX

#include <cstdio>
#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class Node {
private:

  void init(Node* parent_, char base_, PRD_Sequence_Pos last_index_);

public:

  Node            *parent;
  Node            *child[5]; // 0=C 1=G 2=A 3=T/U 4=X/Y/Z/N/...
  char             base;
  PRD_Sequence_Pos last_base_index; // abs(last_base_index) = pos    (last_base_index > 0) ? valid : invalid
  unsigned int     child_bits;

  Node ( Node* parent_, char base_, PRD_Sequence_Pos last_index_ );
  Node ( Node* parent_, char base_ );
  Node ();
  ~Node ();

  Node             *childByBase ( char base_ );
  bool              isValidPrimer ();       // last_base_index  > 0 ?
  bool              isPrimer ();            // last_base_index != 0 ?
  bool              isLeaf ();              // all children == NULL ?
  PRD_Sequence_Pos  lastBaseIndex ();       // abs(last_base_index)
  void              print ();
};

#else
#error PRD_Node.hxx included twice
#endif // PRD_NODE_HXX

