#include "PRD_Node.hxx"

using namespace std;


//
// Constructors
//
void Node::init ( Node* parent_, char base_, PRD_Sequence_Pos last_index_ )
{
  parent          = parent_;
  child[0]        = NULL;
  child[1]        = NULL;
  child[2]        = NULL;
  child[3]        = NULL;
  child[4]        = NULL;
  base            = base_;
  last_base_index = last_index_;
  child_bits      = 0;
}

Node::Node ( Node* parent_, char base_, PRD_Sequence_Pos last_index_ )
{
  init ( parent_,base_,last_index_ );
}

Node::Node ( Node* parent_, char base_ )
{
  init (parent_,base_,0);
}

Node::Node ()
{
  init (NULL,' ',0);
}


//
// Destructor
//
Node::~Node ()
{
  if ( child[0] ) delete child[0];
  if ( child[1] ) delete child[1];
  if ( child[2] ) delete child[2];
  if ( child[3] ) delete child[3];
  if ( child[4] ) delete child[4];
}


//
// childByBase
//
// returns a pointer to the child for given base or NULL
//
Node* Node::childByBase ( char base_ )
{
  return child[ CHAR2CHILD.INDEX[ base_ ] ];
}


//
// isPrimer
//
// true if last_base_index not zero
//
bool Node::isPrimer () {
  return ( last_base_index != 0 );
}


//
// isValidPrimer
//
// true if last_base_index greater than zero
//
bool Node::isValidPrimer () {
  return ( last_base_index > 0 );
}


//
// isLeaf
//
// true if all children are NULL
//
bool Node::isLeaf () {
  return ( child_bits == 0 );
}


//
// print
//
// recursively print Node and its children
//
void Node::print () {
  printf ( "[%c,%li,%i (",base,last_base_index,child_bits );
  if ( child[0] ) { child[0]->print(); printf(","); }
  if ( child[1] ) { child[1]->print(); printf(","); }
  if ( child[2] ) { child[2]->print(); printf(","); }
  if ( child[3] ) { child[3]->print(); printf(","); }
  if ( child[4] ) { child[4]->print(); }
  printf(")]");
}
