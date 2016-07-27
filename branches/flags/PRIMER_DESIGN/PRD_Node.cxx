#include "PRD_Node.hxx"

using namespace std;

//
// Constructors
//
void Node::init (Node* parent_, char base_, PRD_Sequence_Pos last_index_, PRD_Sequence_Pos offset_)
{
  parent          = parent_;
  child[0]        = NULL;
  child[1]        = NULL;
  child[2]        = NULL;
  child[3]        = NULL;
  base            = base_;
  last_base_index = last_index_;
  child_bits      = 0;
  offset          = offset_;
}

Node::Node (Node* parent_, char base_, PRD_Sequence_Pos last_index_, PRD_Sequence_Pos offset_)
{
  init (parent_, base_, last_index_, offset_);
}

Node::Node (Node* parent_, char base_, PRD_Sequence_Pos last_index_)
{
  init (parent_, base_, last_index_, 0);
}

Node::Node (Node* parent_, char base_)
{
  init (parent_, base_, 0, 0);
}

Node::Node ()
{
  init (NULL, ' ', 0, 0);
}


//
// Destructor
//
Node::~Node ()
{
  if (child[0]) delete child[0];
  if (child[1]) delete child[1];
  if (child[2]) delete child[2];
  if (child[3]) delete child[3];
}


//
// print
//
// recursively print Node and its children
//
void Node::print () {
  printf ("[%c,%li,%li,%u (", base, last_base_index, offset, child_bits);
  if (child[0]) { child[0]->print(); printf(","); }
  if (child[1]) { child[1]->print(); printf(","); }
  if (child[2]) { child[2]->print(); printf(","); }
  if (child[3]) { child[3]->print(); }
  printf(")]");
}
