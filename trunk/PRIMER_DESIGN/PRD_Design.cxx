#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

#include "PRD_Design.hxx"
#include "PRD_SequenceIterator.hxx"
#include "PRD_SearchFIFO.hxx"

#include <cstdio>
#include <iostream>

using namespace std;

//
// Constructor
//
void PrimerDesign::init ( const char *sequence_, \
			  Range pos1_, Range pos2_, Range length_, Range distance_, \
			  Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_, \
			  int max_count_primerpairs_, double CG_factor_, double temp_factor_ )
{
  sequence = sequence_;
  setPositionalParameters ( pos1_, pos2_, length_, distance_ );
  setConditionalParameters( ratio_, temperature_, min_dist_to_next_, expand_UPAC_Codes_, max_count_primerpairs_, CG_factor_, temp_factor_ );
}

PrimerDesign::PrimerDesign( const char *sequence_, \
			    Range pos1_, Range pos2_, Range length_, Range distance_, \
			    Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_,\
			    int max_count_primerpairs_, double CG_factor_, double temp_factor_ )
{
  init ( sequence_, pos1_, pos2_, length_, distance_, ratio_, temperature_, min_dist_to_next_, expand_UPAC_Codes_, max_count_primerpairs_, CG_factor_, temp_factor_ ); 
}


PrimerDesign::PrimerDesign( const char *sequence_, \
			    Range pos1_, Range pos2_, Range length_, Range distance_, int max_count_primerpairs_, double CG_factor_, double temp_factor_ )
{
  init ( sequence_, pos1_, pos2_, length_, distance_, Range(0,0), Range(0,0), -1, false, max_count_primerpairs_, CG_factor_, temp_factor_ ); 
}


PrimerDesign::PrimerDesign( const char *sequence_ )
{
  init ( sequence_, Range(0,0), Range(0,0), Range(0,0), Range(0,0), Range(0,0), Range(0,0), -1, false, 10, 0.5, 0.5 );
}


//
// Destructor
//
PrimerDesign::~PrimerDesign ()
{
  if ( root1 ) delete root1;
  if ( root2 ) delete root2;
  if ( list1 ) delete list1;
  if ( list2 ) delete list2;
  if ( pairs ) delete [] pairs;
}


bool PrimerDesign::setPositionalParameters( Range pos1_, Range pos2_, Range length_, Range distance_ )
{
  // length > 0
  if ((length_.min() <= 0) || (length_.max() <= 0)) return false;

  // pos1_.max + length_.max < pos2_.min
  if (pos2_.min() <= pos1_.max() + length_.max()) return false;
  
  // use distance-parameter ? (-1 = no)
  if (distance_.min() > 0) {
    // pos2_.max - pos1_.min >= distance_.min
    if (distance_.min() > pos2_.max() - pos1_.min()) return false;
    // pos2_.min - pos1_.max < distance_.max
    if (distance_.max() <= pos2_.min() - pos1_.max()) return false;
  }

  // all parameters are valid at this point
  primer1         = pos1_;
  primer2         = pos2_;
  primer_length   = length_;
  primer_distance = distance_;
  return true;
}


void  PrimerDesign::setConditionalParameters( Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_, int max_count_primerpairs_, double CG_factor_, double temp_factor_ )
{
  if ((ratio_.min() < 0) || (ratio_.max() <= 0))
    CG_ratio = Range(-1,-1);
  else
    CG_ratio = ratio_;

  if ((temperature_.min() <= 0) || (temperature_.max() <= 0))
    temperature = Range (-1,-1);
  else
    temperature = temperature_;

  CG_factor                  = CG_factor_;
  temperature_factor         = temp_factor_;
  min_distance_to_next_match = min_dist_to_next_;
  expand_UPAC_Codes          = expand_UPAC_Codes_;
  max_count_primerpairs      = max_count_primerpairs_;

  if ( pairs ) delete [] pairs;
  pairs = new Pair[ max_count_primerpairs ];
}


//
//
//
void PrimerDesign::run ( int print_stages_ )
{

  buildPrimerTrees();
  if ( print_stages_ & PRINT_RAW_TREES ) printPrimerTrees();

  matchSequenceAgainstPrimerTrees();
  if ( print_stages_ & PRINT_MATCHED_TREES ) printPrimerTrees();

  convertTreesToLists();
  if ( print_stages_ & PRINT_PRIMER_LISTS ) printPrimerLists();

  evaluatePrimerPairs();
  if ( print_stages_ & PRINT_PRIMER_PAIRS ) printPrimerPairs();
}

//
// (re)construct raw primer trees
//

// add new child to parent or update existing child
// returns child_index
int PrimerDesign::insertNode ( Node *current_, char base_, PRD_Sequence_Pos pos_, int delivered_ )
{
  int  index     = CHAR2CHILD.INDEX[ base_ ];
  bool is_primer = primer_length.includes( delivered_ );
  
//   printf ( "(%li,%c,%i,",pos_,base_,delivered_ );

  // create new node if necessary
  // or update existing node if
  if ( current_->child[index] == NULL )
  {
    if ( is_primer )        // new child with positive last_base_index
      current_->child[index] = new Node ( current_,base_,pos_ );
    else                    
      current_->child[index] = new Node ( current_,base_,0 );
    current_->child_bits |= CHAR2BIT.FIELD[ base_ ];
  }
  else
    if ( is_primer )
    {
//       printf( "%li/",current_->child[index]->last_base_index );
      current_->child[index]->last_base_index = -pos_;
    }

//   printf ( "%li) ",current_->child[index]->last_base_index );

  return index;
}

void PrimerDesign::buildPrimerTrees ()
{
//   primer1.print("buildPrimerTrees : pos1\t\t","\n");
//   primer2.print("buildPrimerTrees : pos2\t\t","\n");
//   primer_length.print("buildPrimerTrees : length\t","\n");
//   printf("buildPrimerTrees : 0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789\n");
//   printf("buildPrimerTrees : %s\n",sequence);
//   printf("buildPrimerTrees : (pos,base,delivered,last_base_index)\n" );

  // destroy old trees if exist
  if (root1)
    delete root1;
  if (root2)
    delete root2;
  root1 = new Node();
  root2 = new Node();

  // init iterator with sequence
  SequenceIterator *sequence_iterator = new SequenceIterator ( sequence );
  char  base;
  int   child_index;
  Node *current_node;

  // build first tree
//   printf( "buildPrimerTrees : tree1\n" );
  for ( PRD_Sequence_Pos start_pos = primer1.min(); (start_pos < primer1.max()-primer_length.max()) && (sequence[start_pos] != '\x00'); start_pos++ )
  {
    // start iterator at new position
    sequence_iterator->restart( start_pos,primer_length.max(),SequenceIterator::FORWARD );
    sequence_iterator->nextBase();
    if ( sequence_iterator->pos != start_pos )
    {
      // sequence_iterator has skipped spaces => restart at first valid base
      start_pos = sequence_iterator->pos;
    }
    sequence_iterator->restart( start_pos,primer_length.max(),SequenceIterator::FORWARD );
//     printf( "buildPrimerTrees : %li\t",start_pos );

    // start at top of tree
    current_node = root1;

    // iterate through sequence till end-of-sequence or primer_length.max bases read
    base = sequence_iterator->nextBase();
    while ( base != SequenceIterator::EOS )
    {
      child_index = insertNode( current_node, base, sequence_iterator->pos, sequence_iterator->delivered );
      current_node = current_node->child[child_index];
      if (! ((base == 'A') || (base == 'T') || (base == 'U') || (base == 'C') || (base == 'G')) ) break; // stop at unsure bases

      // get next base
      base = sequence_iterator->nextBase();
    }
//     printf( "\n" );
  }

//   cout << endl << "buildPrimerTrees : root1 : ";
//   root1->print();
//   cout << endl << endl;
//   getchar();

  // build second tree
//   printf( "buildPrimerTrees : tree2\n" );
  for ( PRD_Sequence_Pos start_pos = primer2.min(); (start_pos < primer2.max()-primer_length.max()) && (sequence[start_pos] != '\x00'); start_pos++ )
  {
    // start iterator at new position
    sequence_iterator->restart( start_pos,primer_length.max(),SequenceIterator::FORWARD );
    sequence_iterator->nextBase();
    if ( sequence_iterator->pos != start_pos )
    {
      // sequence_iterator has skipped spaces => restart at first valid base
      start_pos = sequence_iterator->pos;
    }
    sequence_iterator->restart( start_pos,primer_length.max(),SequenceIterator::FORWARD );
//     printf( "buildPrimerTrees : %li\t",start_pos );

    // start at top of tree
    current_node = root2;

    // iterate through sequence till end-of-sequence or primer_length.max bases read
    base = sequence_iterator->nextBase();
    while ( base != SequenceIterator::EOS )
    {
      child_index = insertNode( current_node, base, sequence_iterator->pos, sequence_iterator->delivered );
      current_node = current_node->child[child_index];
      if (! ((base == 'A') || (base == 'T') || (base == 'U') || (base == 'C') || (base == 'G')) ) break; // stop at unsure bases

      // get next base
      base = sequence_iterator->nextBase();
    }
//     printf( "\n" );
  }

//   cout << endl << "buildPrimerTrees : root2 : ";
//   root2->print();
//   cout << endl << endl;

  delete sequence_iterator;
//   cout << endl;

//   printf("press enter to continue\n");
//   getchar();
}


//
// print all primers in trees
//
PRD_Sequence_Pos PrimerDesign::followUp( Node *node_, deque<char> *primer_, int direction_ )
{
  if ( direction_ == FORWARD ) primer_->push_front( node_->base );
                          else primer_->push_back( node_->base );

  if ( node_->parent == NULL ) return node_->last_base_index;
                          else followUp( node_->parent, primer_, direction_ );
  return 0;
}

void PrimerDesign::findNextPrimer( Node *start_at_, int depth_, int *counter_, int direction_ )
{
  // current node
  printf (  "findNextPrimer : start_at_ [ %c,%4li,%2i (%c %c %c %c %c) ]\n",
	    start_at_->base,
	    start_at_->last_base_index,
	    start_at_->child_bits,
	    (start_at_->child[1] != NULL) ? 'G' : ' ',
	    (start_at_->child[0] != NULL) ? 'C' : ' ',
	    (start_at_->child[3] != NULL) ? 'T' : ' ',
	    (start_at_->child[2] != NULL) ? 'A' : ' ',
	    (start_at_->child[4] != NULL) ? 'X' : ' ' );
  if ( primer_length.includes(depth_) && start_at_->isValidPrimer() )
//   {
//     (*counter_)++;
//     printf( "findNextPrimer : %3i",depth_ );

//     // print last_base_indizes
//     if ( direction_ == FORWARD ) printf( "%7li           ",start_at_->last_base_index );
//                             else printf( "      0           " );

//     // print bases
//     deque<char> primer;
//     if ( direction_ == FORWARD )
//     {
//       printf( "%7li               '",followUp( start_at_, &primer, direction_ ) );
//       primer.pop_front();                                                     // erase 'base' of root-node
//     }
//     else
//     {
//       printf( "%7li               '",start_at_->last_base_index );
//       followUp( start_at_, &primer, direction_ );
//       primer.pop_back();                                                      // erase 'base' of root-node
//     }
//     copy( primer.begin(), primer.end(), ostream_iterator<char>(cout) );
//     printf( "'\n" );
//   }

  // children
  depth_++;
  for ( int i=0; i < 4; i++ )
    if ( start_at_->child[i] != NULL ) findNextPrimer( start_at_->child[i],  depth_, counter_, direction_ );
}

void PrimerDesign::printPrimerTrees ()
{
  // start at root1 with zero depth
  int primer_counter;
  primer_counter = 0;
  cout << "findNextPrimer : depth  last_base:index  first_base:index  base" << endl;
  findNextPrimer( root1,0, &primer_counter, FORWARD );
  cout << "printPrimerTrees : " << primer_counter << " primer found" << endl << endl;

  // start at root2 with zero depth
  primer_counter = 0;
  cout << "findNextPrimer : depth  last_base:index  first_base:index  base" << endl;
  findNextPrimer( root2,0, &primer_counter, BACKWARD );
  cout << "printPrimerTrees : " << primer_counter << " primer found" << endl;
}


//
// match the sequence against the primer-trees
//
void PrimerDesign::matchSequenceAgainstPrimerTrees() 
{
  SearchFIFO        *fifo1             = new SearchFIFO( sequence,root1,primer_length,min_distance_to_next_match,expand_UPAC_Codes );
  SearchFIFO        *fifo2             = new SearchFIFO( sequence,root2,primer_length,min_distance_to_next_match,expand_UPAC_Codes );
  SequenceIterator  *sequence_iterator = new SequenceIterator( sequence,0,SequenceIterator::IGNORE_LENGTH,SequenceIterator::FORWARD );
  char               base;
  PRD_Sequence_Pos   pos;
//   int                max_fifo1_size = -1; // debug only
//   int                max_fifo2_size = -1; // debug only

//   printf( "root1 : [C %p, G %p, A %p, TU %p, %p]\n",root1->child[0],root1->child[1],root1->child[2],root1->child[3],root1->child[4] );
//   printf( "root2 : [C %p, G %p, A %p, TU %p, %p]\n",root2->child[0],root2->child[1],root2->child[2],root2->child[3],root2->child[4] );

//   printf( "matchSequenceAgainstPrimerTrees : tree 1/2 forward -- " );
  base = sequence_iterator->nextBase();
  while ( base != SequenceIterator::EOS )
  {
    pos = sequence_iterator->pos;

    // tree/fifo 1
    if ( primer1.includes( pos ) ) // flush fifo1 if in range of Primer 1
    {
      fifo1->flush();
    }
    else
    {
      //     printf( "matchSequenceAgainstPrimerTrees : (1) base %c pos %i\n",base,pos );

      // iterate through fifo1
      fifo1->iterateWith( pos,base );

      // append base to fifo1
      fifo1->push( pos );

      // debug
//       fifo1->print();
//       getchar();
//       if ( max_fifo1_size < fifo1->size ) max_fifo1_size = fifo1->size; // debug-info only
    }

    // tree/fifo2
    if ( primer2.includes( pos ) ) // flush fifo2 if in range of Primer 2
    {
      fifo2->flush();
    }
    else
    {
      //     printf( "matchSequenceAgainstPrimerTrees : (2) base %c pos %i\n",base,pos );

      // iterate through fifo2
      fifo2->iterateWith( pos,base );

      // append base to fifo2
      fifo2->push( pos );

      // debug
      //     fifo2->print();
      //     getchar();
//       if ( max_fifo2_size < fifo2->size ) max_fifo2_size = fifo2->size; // debug-info only
    }

    // get next base in sequence
    base = sequence_iterator->nextBase();

//     if ( max_fifo2_size < fifo2->size ) max_fifo2_size = fifo2->size; // debug-info only
  }

//   printf( "max_fifo1_size %i   max_fifo2_size %i   max_pos %li\n", max_fifo1_size, max_fifo2_size, pos );
//   printf( "1: %li pushes   %li iterates     2: %li pushes   %li iterates\n",fifo1->push_count,fifo1->iterate_count,fifo2->push_count,fifo2->iterate_count);

//   printf( "matchSequenceAgainstPrimerTrees : tree 1/2 backward -- " );

  sequence_iterator->restart( pos, SequenceIterator::IGNORE_LENGTH, SequenceIterator::BACKWARD );
  fifo1->flush();
//   fifo1->push_count    = 0; // debug-info only
//   fifo1->iterate_count = 0; // debug-info only
  fifo2->flush();
//   fifo2->push_count    = 0; // debug-info only
//   fifo2->iterate_count = 0; // debug-info only

  base = INVERT.BASE[ sequence_iterator->nextBase() ];
  while ( base != SequenceIterator::EOS )
  {
    pos = sequence_iterator->pos;

    // tree/fifo 1
    if ( primer1.includes( pos ) ) // flush fifo1 if in range of Primer 1
    {
      fifo1->flush();
    }
    else
    {
      //     printf( "matchSequenceAgainstPrimerTrees : (1) base %c pos %i\n",base,pos );

      // iterate through fifo1
      fifo1->iterateWith( pos,base );

      // append base to fifo1
      fifo1->push( pos );

      // debug
      //     fifo1->print();
      //     getchar();
///       if ( max_fifo1_size < fifo1->size ) max_fifo1_size = fifo1->size; // debug-info only
    }

    // tree/fifo2
    if ( primer2.includes( pos ) ) // flush fifo2 if in range of Primer 2
    {
      fifo2->flush();
    }
    else
    {
      //     printf( "matchSequenceAgainstPrimerTrees : (2) base %c pos %i\n",base,pos );

      // iterate through fifo2
      fifo2->iterateWith( pos,base );

      // append base to fifo2
      fifo2->push( base );

      // debug
      //     fifo2->print();
      //     getchar();
//       if ( max_fifo2_size < fifo2->size ) max_fifo2_size = fifo2->size; // debug-info only
    }

    // get next base in sequence
    base = sequence_iterator->nextBase();

//     if ( max_fifo2_size < fifo2->size ) max_fifo2_size = fifo2->size; // debug-info only
  }

//   printf( "max_fifo1_size %i   max_fifo2_size %i   min_pos %li\n", max_fifo1_size, max_fifo2_size, pos );
//   printf( "1: %li pushes   %li iterates     2: %li pushes   %li iterates\n",fifo1->push_count,fifo1->iterate_count,fifo2->push_count,fifo2->iterate_count);

  delete fifo1;
  delete fifo2;
  delete sequence_iterator;
}


//
// convertTreesToLists
//
void PrimerDesign::calcCGandAT ( int &CG_, int &AT_, Node *start_at_ )
{
  if ( start_at_ == NULL ) return;

  if ( (start_at_->base == 'C') || (start_at_->base == 'G') )
    CG_++;
  else
    AT_++;

  calcCGandAT( CG_, AT_, start_at_->parent );
}

void PrimerDesign::convertTreesToLists ()
{
//   CG_ratio.print( "convertTreesToLists : CG_ratio ","  " );
//   temperature.print( "temperature ","  " );
//   primer_distance.print( "primer_distance ","\n" ); 

  Node             *cur_node;
  Item             *cur_item;
  Item             *new_item;
  int               depth;
  int               AT;
  int               CG;
  PRD_Sequence_Pos  min_pos_1 = PRD_MAX_SEQUENCE_POS;
  PRD_Sequence_Pos  max_pos_1 = -1;
  PRD_Sequence_Pos  min_pos_2 = PRD_MAX_SEQUENCE_POS;
  PRD_Sequence_Pos  max_pos_2 = -1;
  pair< Node*,int > new_pair;      // < cur_node->child[i], depth >
  deque< pair<Node*,int> > *stack;
  

  // list 1
  cur_item = NULL;
  stack = new deque< pair<Node*,int> >;
  // push children of root on stack
  for ( int index = 0; index < 5; index++ )
    if ( root1->child[index] != NULL )
    {
      new_pair = pair<Node*,int>( root1->child[index],1 );
      stack->push_back( new_pair );
    }

  while ( !stack->empty() )
  {
    // next node
    cur_node = stack->back().first;
    depth    = stack->back().second;
    stack->pop_back();

    // handle node
    if ( primer_length.includes(depth) && cur_node->isValidPrimer() )
    {
      // calculate conditional parameters
      CG = AT = 0;
      calcCGandAT( CG, AT, cur_node );
      AT = 4*CG + 2*AT;
      CG = (CG * 100) / depth;
      
      // create new item if conditional parameters are in range
      if ( CG_ratio.includes(CG) && temperature.includes(AT) ) 
      {
	new_item = new Item( cur_node->last_base_index, depth, CG, AT, NULL );

	// begin list with new item or append to list
	if ( cur_item == NULL ) list1 = new_item;
                           else cur_item->next = new_item;

	cur_item = new_item;
// 	cur_item->print("convertTreesToLists : list1   "," added\n"); // debug-info only

	// store position
	if ( cur_node->last_base_index < min_pos_1 ) min_pos_1 = cur_node->last_base_index;
	if ( cur_node->last_base_index > max_pos_1 ) max_pos_1 = cur_node->last_base_index;
      }
//       else
//       {
// 	printf( "convertTreesToLists : list1   CG %i temp %i not valid\n", CG, AT ); // debug-info only
//       }
    }

    // push children on stack
    for ( int index = 0; index < 5; index++ )
      if ( cur_node->child[index] != NULL )
      {
	new_pair = pair<Node*,int>( cur_node->child[index],depth+1 );
	stack->push_back( new_pair );
      }
  }

//   printf( "convertTreesToLists : list1 : min_pos %li  max_pos %li\n", min_pos_1, max_pos_1 );

  // list 2
  cur_item = NULL;
  stack->clear();
  // push children of root on stack
  for ( int index = 0; index < 5; index++ )
    if ( root2->child[index] != NULL )
    {
      new_pair = pair<Node*,int>( root2->child[index],1 );
      stack->push_back( new_pair );
    }

  while ( !stack->empty() )
  {
    // next node
    cur_node = stack->back().first;
    depth    = stack->back().second;
    stack->pop_back();

    // push children on stack
    for ( int index = 0; index < 5; index++ )
      if ( cur_node->child[index] != NULL )
      {
	new_pair = pair<Node*,int>( cur_node->child[index],depth+1 );
	stack->push_back( new_pair );
      }

    // handle node
    if ( primer_length.includes(depth) && cur_node->isValidPrimer() )
    {
      // check position
      if ( primer_distance.min() > 0 )
	if ( !primer_distance.includes( cur_node->last_base_index-max_pos_1, cur_node->last_base_index-min_pos_1 ) )
	  continue;

      // calculate conditional parameters
      CG = AT = 0;
      calcCGandAT( CG, AT, cur_node );
      AT = 4*CG + 2*AT;
      CG = (CG * 100) / depth;
      
      // create new item if conditional parameters are in range
      if ( CG_ratio.includes(CG) && temperature.includes(AT) )
      {
	new_item = new Item( cur_node->last_base_index, depth, CG, AT, NULL );

	// begin list with new item or append to list
	if ( cur_item == NULL ) list2 = new_item;
                           else cur_item->next = new_item;

	cur_item = new_item;
// 	cur_item->print("convertTreesToLists : list2   "," added\n");

	// store position
	if ( cur_node->last_base_index < min_pos_2 ) min_pos_2 = cur_node->last_base_index;
	if ( cur_node->last_base_index > max_pos_2 ) max_pos_2 = cur_node->last_base_index;
      }
//       else
//       {
// 	printf( "convertTreesToLists : list2   CG %i temp %i not valid\n", CG, AT );
//       }
    }
  }
  delete stack;

//   printf( "convertTreesToLists : list2 : min_pos %li  max_pos %li\n", min_pos_2, max_pos_2 );

  // check positions of list1
  if ( primer_distance.min() > 0 )  // skip check if primer_distance < 0
  {
    Item *to_remove = NULL;
    cur_item = list1;
    while ( cur_item != NULL )
    {
      if ( !primer_distance.includes( max_pos_2-cur_item->start_pos, min_pos_2-cur_item->start_pos ) )
      {
	// primer in list 1 out of range of primers in list 2 => remove from list 1
//       printf ( "removing %i\n", cur_item->start_pos );
      
	if ( cur_item == list1 )
	{
	  // 'to delete'-item is first in list
	  list1 = list1->next;        // list 1 starts with next item
	  delete cur_item;            // delete former first item
	  cur_item = list1;           // new first item is next to be examined
	}
	else
	{
	  // run through list till 'next' is to delete
	  to_remove = cur_item;                     // remember 'to delete'-item
	  cur_item  = list1;                        // start at first
	  while ( cur_item ->next != to_remove )    // run till 'next' is 'to delete'
	    cur_item = cur_item->next;
	  cur_item->next = cur_item->next->next;    // unlink 'next' from list
	  cur_item = cur_item->next;                // 'next' of 'to delete' is next to be examined
	  delete to_remove;                         // ...
	}
      }
      else
	cur_item = cur_item->next;
    }
  }

  // debug / info only
//   int counter = 0;
//   for ( cur_item = list1; cur_item != NULL; cur_item = cur_item->next )
//     counter++;
//   printf ( "convertTreesToLists : %i primers in list1 ",counter );
//   counter = 0;
//   for ( cur_item = list2; cur_item != NULL; cur_item = cur_item->next )
//     counter++;
//   printf ( " %i primers in list2\n",counter );

//   printf("\n");
}


//
// printPrimerLists
//
void PrimerDesign::printPrimerLists ()
{
  int count = 0;

  printf( "printPrimerLists : list 1 : [(start_pos,length), CG_ratio, temperature]\n" );
  Item *current = list1;
  while ( current != NULL )
  {
    current->print( "","\n" );
    current = current->next;
    count++;
  }
  printf( " : %i valid primers\nprintPrimerLists : list 2 : [(start_pos,length), CG_ratio, temperature]\n", count );
  count = 0;
  current = list2;
  while ( current != NULL )
  {
    current->print( "","\n" );
    current = current->next;
    count++;
  }
  printf( " : %i valid primers\n", count );
}



//
// evaluatePrimerPairs
//
double PrimerDesign::evaluatePair ( Item *one_, Item *two_ )
{
  if ( (one_ == NULL) || (two_ == NULL) )
    return -1.0;

  return abs(one_->CG_ratio - two_->CG_ratio)*CG_factor + abs(one_->temperature - two_->temperature)*temperature_factor;
}

void PrimerDesign::insertPair( double rating_, Item *one_, Item *two_ )
{
  int index = -1;

  // search position
  for ( int i = max_count_primerpairs-1; (i >= 0) && (index == -1); i-- )
  {
    // maybe insert new pair ?
    if ( pairs[i].rating >= rating_)
      index = i+1;
  }

  if ( index == -1 ) index = 0;
//   if ( pairs[index-1].rating == rating_ ) return;

  // swap till position
  for ( int i = max_count_primerpairs-2; i >= index; i--)
  {
    pairs[i+1].rating = pairs[i].rating;
    pairs[i+1].one    = pairs[i].one;
    pairs[i+1].two    = pairs[i].two;
  }

  // insert new values
  pairs[index].rating = rating_;
  pairs[index].one    = one_;
  pairs[index].two    = two_;
//   printf( "insertPair : [%i] %6.2f\n", index, rating_ );
}

void PrimerDesign::evaluatePrimerPairs ()
{
  Item   *one = list1;
  Item   *two;
  double  rating;
//   long int counter = 0; // degub/info only

  // outer loop <=> run through list1
  while ( one != NULL )
  {
    // inner loop <=> run through list2
    two = list2;
    while ( two != NULL )
    {
      // evaluate pair
      rating = evaluatePair( one, two );
//       counter++;
      
      // test if rating is besster that the worst of the best
      if ( rating > pairs[max_count_primerpairs-1].rating )
      {
	// insert in the pairs
	insertPair( rating, one, two );

// 	printPrimerPairs();
// 	getchar();
      }

      // next in inner loop
      two = two->next;
    }

    // next in outer loop
    one = one->next;
  }

//   printf ( "evaluatePrimerPairs : %li pairs checked  ratings [ %.3f .. %.3f ]\n\n", counter, pairs[max_count_primerpairs-1].rating, pairs[0].rating );
}

void PrimerDesign::printPrimerPairs ()
{
  for (int i = 0; i < max_count_primerpairs; i++)
  {
    printf( "printPairs : [%3i]",i );
    pairs[i].print( "\t","\n");
  }
}
