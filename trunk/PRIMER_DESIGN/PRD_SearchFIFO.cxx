#include "PRD_SearchFIFO.hxx"
#include <cstdio>
 
using namespace std;
 
//
// Constructor
//
void SearchFIFO::init( const char *sequence_, Node *root_, Range primer_length_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_UPAC_Codes_ )
{
  begin   = NULL;
  end     = NULL;
  current = NULL;
  size    = 0;

  sequence                   = sequence_;
  root                       = root_;
  primer_length              = primer_length_;
  expand_UPAC_Codes          = expand_UPAC_Codes_;
  min_distance_to_next_match = min_distance_to_next_match_;

//   printf( "SearchFIFO::init : sequence %p root %p length[%i,%i]\n", sequence_, root_, primer_length_.min() , primer_length_.max() );
}

SearchFIFO::SearchFIFO ( const char *sequence_, Node *root_, Range primer_length_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_UPAC_Codes_ )
{
  init( sequence_,root_,primer_length_,min_distance_to_next_match_,expand_UPAC_Codes_ );
}

SearchFIFO::SearchFIFO ()
{
  init( NULL,NULL,Range(0,0),-1,false );
}


//
// Destructor
//
SearchFIFO::~SearchFIFO ()
{
  flush();
}


//
// flush : clear FIFO
//
void SearchFIFO::flush ()
{
  SearchParameter *to_delete;

  current = begin;
  while (current != NULL)
  {
    to_delete = current;
    current   = current->next;
    delete to_delete;
  }

  begin   = NULL;
  end     = NULL;
  current = NULL;
  size    = 0;
}


//
// append new SearchParameter to the FIFO
//
// the current_node is determined from root with the given base
// multiple parameters are generated for unsure bases like 'N'
//
void SearchFIFO::push ( char base_ )
{
  push_count++;

  unsigned int     bits = CHAR2BIT.FIELD[ base_ ];
  Node            *child_of_root;
  SearchParameter *new_parameter;

//   printf( "push : base %c   bits %u %u %u %u %u\n", base_, bits, bits & 1, bits & 2, bits & 4 , bits & 8  );
//   printf( "root : [C %p, G %p, A %p, TU %p, %p]\n",root->child[0],root->child[1],root->child[2],root->child[3],root->child[4] );

  if ( bits & 1 )
  {
    child_of_root = root->child[ 2 ]; // 2 = A
    if ( child_of_root != NULL )
    {
      new_parameter = new SearchParameter;

      //   printf( "push : child_of_root %p new_parameter %p\n",  child_of_root, new_parameter );
      new_parameter->node      = child_of_root;
      new_parameter->next      = NULL;
      new_parameter->previous  = end;

//       printf( "push : %p (%c,%p,%p)\n", new_parameter, new_parameter->node->base, new_parameter->previous, new_parameter->next );

      if (end   != NULL) end->next = new_parameter;
      if (begin == NULL) begin = new_parameter;
      end = new_parameter;
      size++;
    }
  }

  if ( bits & 2 )
  {
    child_of_root = root->child[ 3 ]; // 3 = T/U
    if ( child_of_root != NULL )
    {
      new_parameter = new SearchParameter;

      //   printf( "push : child_of_root %p new_parameter %p\n",  child_of_root, new_parameter );
      new_parameter->node      = child_of_root;
      new_parameter->next      = NULL;
      new_parameter->previous  = end;

//       printf( "push : %p (%c,,%p,%p)\n", new_parameter, new_parameter->node->base, new_parameter->previous, new_parameter->next );

      if (end   != NULL) end->next = new_parameter;
      if (begin == NULL) begin = new_parameter;
      end = new_parameter;
      size++;
    }
  }

  if ( bits & 4 )
  {
    child_of_root = root->child[ 0 ]; // 0 = C
    if ( child_of_root != NULL )
    {
      new_parameter = new SearchParameter;

      //   printf( "push : child_of_root %p new_parameter %p\n",  child_of_root, new_parameter );
      new_parameter->node      = child_of_root;
      new_parameter->next      = NULL;
      new_parameter->previous  = end;

//       printf( "push : %p (%c,%p,%p)\n", new_parameter, new_parameter->node->base, new_parameter->previous, new_parameter->next );

      if (end   != NULL) end->next = new_parameter;
      if (begin == NULL) begin = new_parameter;
      end = new_parameter;
      size++;
    }
  }

  if ( bits & 8 )
  {
    child_of_root = root->child[ 1 ]; // 1 = G
    if ( child_of_root != NULL )
    {
      new_parameter = new SearchParameter;

      //   printf( "push : child_of_root %p new_parameter %p\n",  child_of_root, new_parameter );
      new_parameter->node      = child_of_root;
      new_parameter->next      = NULL;
      new_parameter->previous  = end;

//       printf( "push : %p (%c,%p,%p)\n", new_parameter, new_parameter->node->base, new_parameter->previous, new_parameter->next );

      if (end   != NULL) end->next = new_parameter;
      if (begin == NULL) begin = new_parameter;
      end = new_parameter;
      size++;
    }
  }
//   getchar();
}


//
// step down in tree at all the SearchParameters with the given base(s)
//
void SearchFIFO::push_front( Node *child_of_current_ )
{
//   printf( "push_front : base %c\n", child_of_current_->base );

  push_count++;

  SearchParameter *new_parameter = new SearchParameter;

  new_parameter->node      = child_of_current_;
  new_parameter->next      = begin;
  new_parameter->previous  = NULL;

//   printf( "push_front : %p (%c,%p,%p)\n", new_parameter, new_parameter->node->base, new_parameter->previous, new_parameter->next );

  if (end   == NULL) end             = new_parameter;
  if (begin != NULL) begin->previous = new_parameter;
  begin = new_parameter;
  size++;
}

void SearchFIFO::iterateWith ( PRD_Sequence_Pos pos_, char base_ )
{
  iterate_count++;
  //   printf( "iterateWith : base %c pos %i begin %p end %p abort %s\n", base_, pos_, begin, end, (begin == NULL) ? "true" : "false" );
  if ( begin == NULL ) return;

  Node *child;
  int   bits;
  
  current = begin;
  //   printf( "iterateWith : " );

  if ( !expand_UPAC_Codes )
  {
    while ( current != NULL )
    {

      // get childnode of parameters current node
      child = current->node->childByBase( base_ );

      //     printf( "child %p ",child );
      //     if ( child != NULL ) printf( "[%c] ", child->base );

      // erase parameter if child doesnt exist
      if ( child == NULL )
      {
	//       printf( "no child:erase " );
	erase( current );
      }
      else
      {
	// step down as child exists
	current->node = child;
	//       printf ( "stepped down ");
	
	// invalidate if node is primer and is in range
	if ( child->isValidPrimer() )
	{
	  if ( min_distance_to_next_match <= 0 )
	    child->last_base_index = -pos_;
	  else
	    if ( min_distance_to_next_match >= abs(pos_ - child->last_base_index) )
	      child->last_base_index = -pos_;
	  // 	printf ( "invalidated " );
	}

	// erase parameter if child is leaf
	if ( child->isLeaf() )
        {
	  // 	printf ( "leaf:erasing ");
	  erase( current );
	  // erase node in tree if leaf and invalid primer
	  if ( !child->isValidPrimer() )
	  {
	    // 	  printf("invalidPrimer:removing ");
	    // remove child from parent
	    child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
	    child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
	    // erase node
	    delete child;
	  }
	} 
      }

      // next parameter (or begin if erase killed current while pointing to begin)
      current = (current == NULL) ? begin : current->next;
    }
  }
  else // expand UPAC-Codes
  {
    while ( current != NULL )
    {
      bits = current->node->child_bits & CHAR2BIT.FIELD[ base_ ];

      if ( bits & 1 ) // A
      {
	child = current->node->child[ 2 ]; // 2 = A
	
	// invalidate if node is primer and is in range
	if ( child->isValidPrimer() )
	{
	  if ( min_distance_to_next_match <= 0 )
	    child->last_base_index = -pos_;
	  else
	    if ( min_distance_to_next_match >= abs(pos_ - child->last_base_index) )
	      child->last_base_index = -pos_;
	  // 	printf ( "invalidated " );
	}

	// erase child if child is leaf
	if ( child->isLeaf() )
	{
	  // erase node in tree if leaf and invalid primer
	  if ( !child->isValidPrimer() )
	  {
	    // 	      printf("invalidPrimer:removing ");
	    // remove child from parent
	    child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
	    child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
	    // erase node
	    delete child;
	  }
	}
	else // add new parameter at the begin of fifo
	{
	  push_front( child );
	}
      }

      if ( bits & 2 ) // T/U
      {
	child = current->node->child[ 3 ]; // 3 = T/U

	// invalidate if node is primer and is in range
	if ( child->isValidPrimer() )
	{
	  if ( min_distance_to_next_match <= 0 )
	    child->last_base_index = -pos_;
	  else
	    if ( min_distance_to_next_match >= abs(pos_ - child->last_base_index) )
	      child->last_base_index = -pos_;
	  // 	printf ( "invalidated " );
	}

	// erase child if child is leaf
	if ( child->isLeaf() )
	{
	  // erase node in tree if leaf and invalid primer
	  if ( !child->isValidPrimer() )
	  {
	    // 	      printf("invalidPrimer:removing ");
	    // remove child from parent
	    child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
	    child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
	    // erase node
	    delete child;
	  }
	}
	else // add new parameter at the begin of fifo
	{
	  push_front( child );
	}
      }

      if ( bits & 4 ) // C
      {
	child = current->node->child[ 0 ]; // 0 = C

	// invalidate if node is primer and is in range
	if ( child->isValidPrimer() )
	{
	  if ( min_distance_to_next_match <= 0 )
	    child->last_base_index = -pos_;
	  else
	    if ( min_distance_to_next_match >= abs(pos_ - child->last_base_index) )
	      child->last_base_index = -pos_;
	  // 	printf ( "invalidated " );
	}

	// erase child if child is leaf
	if ( child->isLeaf() )
	{
	  // erase node in tree if leaf and invalid primer
	  if ( !child->isValidPrimer() )
	  {
	    // 	      printf("invalidPrimer:removing ");
	    // remove child from parent
	    child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
	    child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
	    // erase node
	    delete child;
	  }
	}
	else // add new parameter at the begin of fifo
	{
	  push_front( child );
	}
      }

      if ( bits & 8 ) // G
      {
	child = current->node->child[ 1 ]; // 1 = G

	// invalidate if node is primer and is in range
	if ( child->isValidPrimer() )
	{
	  if ( min_distance_to_next_match <= 0 )
	    child->last_base_index = -pos_;
	  else
	    if ( min_distance_to_next_match >= abs(pos_ - child->last_base_index) )
	      child->last_base_index = -pos_;
	  // 	printf ( "invalidated " );
	}

	// erase child if child is leaf
	if ( child->isLeaf() )
	{
	  // erase node in tree if leaf and invalid primer
	  if ( !child->isValidPrimer() )
	  {
	    // 	      printf("invalidPrimer:removing ");
	    // remove child from parent
	    child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
	    child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
	    // erase node
	    delete child;
	  }
	}
	else // add new parameter at the begin of fifo
	{
	  push_front( child );
	}
      }

      // erase old parameter
      erase( current );

      // next parameter (or begin if erase killed current while pointing to begin)
      current = (current == NULL) ? begin : current->next;
    }
  }

//   printf ( "\n" );
}


//
// erase given parameter while not invalidating current
//
void SearchFIFO::erase ( SearchParameter *param_ )
{
//   printf( "erase : %p (%c,%p,%p) ", param_, param_->node->base, param_->previous , param_->next );

  // unlink from doublelinked list
  if ( param_->next     != NULL ) param_->next->previous = param_->previous; else end   = param_->previous;
  if ( param_->previous != NULL ) param_->previous->next = param_->next;     else begin = param_->next;
//   printf("unlinked ");

  // set current to current->previous if current is to delete
  if ( param_ == current ) current = param_->previous;
//   printf("current adjusted to %p ",current);

  delete param_;
  size--;
//   printf("deleted\n");
}


void SearchFIFO::print ()
{
  current = begin;
  if ( begin != NULL)
    printf( "print : begin %p (%p[%c],%p,%p)\t", begin, begin->node, begin->node->base, begin->previous, begin->next );
  else
    printf( "print : begin (nil)\n" );

  if ( end != NULL )
    printf( "end %p (%p[%c],%p,%p)\n", end, end->node, end->node->base, end->previous, end->next );
  else
    printf( "end (nil)\n" );

  while (current != NULL)
  {
    printf( "print : %p (%p[%c],%p,%p)\n", current, current->node, current->node->base, current->previous, current->next );
    current = current->next;
  }
}
