#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

#include "PRD_Design.hxx"
#include "PRD_SequenceIterator.hxx"
#include "PRD_SearchFIFO.hxx"

#include <cstdio>
#include <iostream>
#include <cmath>

// using namespace std; (does not work well with Solaris CC)
using std::printf;
using std::deque;
using std::cout;
using std::endl;
using std::pair;

//
// Constructor
//
void PrimerDesign::init ( const char *sequence_, long int seqLength_,
                          Range pos1_, Range pos2_, Range length_, Range distance_,
                          Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_,
                          int   max_count_primerpairs_, double GC_factor_, double temp_factor_ )
{
    error     = 0;
    sequence  = sequence_;
    seqLength = seqLength_;
    root1     = 0;
    root2     = 0;
    list1     = 0;
    list2     = 0;
    pairs     = 0;

    show_status_txt    = 0;
    show_status_double = 0;

    setPositionalParameters ( pos1_, pos2_, length_, distance_ );
    setConditionalParameters( ratio_, temperature_, min_dist_to_next_, expand_IUPAC_Codes_, max_count_primerpairs_, GC_factor_, temp_factor_ );
}

PrimerDesign::PrimerDesign( const char *sequence_, long int seqLength_,
                            Range pos1_, Range pos2_, Range length_, Range distance_,
                            Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_,
                            int   max_count_primerpairs_, double GC_factor_, double temp_factor_ )
{
    init ( sequence_, seqLength_, pos1_, pos2_, length_, distance_, ratio_, temperature_, min_dist_to_next_, expand_IUPAC_Codes_, max_count_primerpairs_, GC_factor_, temp_factor_ );
}


PrimerDesign::PrimerDesign( const char *sequence_, long int seqLength_,
                            Range pos1_, Range pos2_, Range length_, Range distance_, int max_count_primerpairs_, double GC_factor_, double temp_factor_ )
{
    init ( sequence_, seqLength_, pos1_, pos2_, length_, distance_, Range(0,0), Range(0,0), -1, false, max_count_primerpairs_, GC_factor_, temp_factor_ );
}


PrimerDesign::PrimerDesign( const char *sequence_ , long int seqLength_)
{
    init ( sequence_, seqLength_, Range(0,0), Range(0,0), Range(0,0), Range(0,0), Range(0,0), Range(0,0), -1, false, 10, 0.5, 0.5 );
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


void PrimerDesign::setPositionalParameters( Range pos1_, Range pos2_, Range length_, Range distance_ )
{
    // first take all parameters, then test em
    primer1         = (pos1_.min() < pos2_.min()) ? pos1_ : pos2_;
    primer2         = (pos1_.min() < pos2_.min()) ? pos2_ : pos1_;
    primer_length   = length_;
    primer_distance = distance_;

    // length > 0
    if ( (length_.min() <= 0) || (length_.max() <= 0)  ) {
        error = "invalid primer length (length <= 0)";
        return;
    }

    // determine end of ranges
    SequenceIterator *i = new SequenceIterator( sequence, primer1.min(), SequenceIterator::IGNORE, primer1.max(), SequenceIterator::FORWARD );
    while ( i->nextBase() != SequenceIterator::EOS ) ;
    primer1.max( i->pos );
    i->restart( primer2.min(), SequenceIterator::IGNORE, primer2.max(), SequenceIterator::FORWARD );
    while ( i->nextBase() != SequenceIterator::EOS ) ;
    primer2.max( i->pos );

    // primer1.max > primer2_.min =>  distance must be given
    if ( (primer2.min() <= primer1.max()) && (distance_.min() <= 0) ) {
        error = "using overlapping primer positions you MUST give a primer distance";
        return;
    }

    // use distance-parameter ? (-1 = no)
    if ( distance_.min() > 0 ) {
        // pos2_.max - pos1_.min must be greater than distance_.min
        if ( distance_.min() > primer2.max() - primer1.min() ) {
            error = "invalid minimal primer distance (more than right.max - left.min)";
            return;
        }
        // pos2_.min - pos1_.max must be less than distance_.max
        if ( distance_.max() < primer2.min() - primer1.max() ) {
            error = "invalid maximal primer distance (less than right.min - left.max)";
            return;
        }
    }
}


void  PrimerDesign::setConditionalParameters( Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_, int max_count_primerpairs_, double GC_factor_, double temp_factor_ )
{
    if ( (ratio_.min() < 0) || (ratio_.max() <= 0) )
        GC_ratio = Range(-1,-1);
    else
        GC_ratio = ratio_;

    if ( (temperature_.min() <= 0) || (temperature_.max() <= 0) )
        temperature = Range (-1,-1);
    else
        temperature = temperature_;

    GC_factor                  = GC_factor_;
    temperature_factor         = temp_factor_;
    min_distance_to_next_match = min_dist_to_next_;
    expand_IUPAC_Codes         = expand_IUPAC_Codes_;
    max_count_primerpairs      = max_count_primerpairs_;

    if ( pairs ) delete [] pairs;
    pairs = new Pair[ max_count_primerpairs ];
}


//
// run the necessary steps .. abort on error
//
void PrimerDesign::run ( int print_stages_ )
{
#if defined(DEBUG)
    printf("PrimerDesign : parameters are\n");
    primer1.print("left                    : ","\n");
    primer2.print("right                   : ","\n");
    primer_length.print("length                  : ","\n");
    primer_distance.print("distance                : ","\n");
    GC_ratio.print("GC ratio                : ","\n");
    temperature.print("temperature             : ","\n");
    printf("GC factor               : %f\n",GC_factor);
    printf("temp. factor            : %f\n",temperature_factor);
    printf("min. dist to next match : %i\n",min_distance_to_next_match);
    printf("expand IUPAC            : %s\n", expand_IUPAC_Codes ? "true" : "false");
    printf("error                   : %s\n",error );
#endif

    if ( error ) {
        printf( "not run cause : %s\n",error );
        return;
    }

#if defined(DEBUG)
    printf("sizeof(Node) = %i\n", sizeof(Node));
#endif // DEBUG

    show_status("searching possible primers");
    buildPrimerTrees();
    if ( error ) return;
    if ( print_stages_ & PRINT_RAW_TREES ) printPrimerTrees();

    show_status("match possible primers vs. sequence");
    matchSequenceAgainstPrimerTrees();
    if ( error ) return;
    if ( print_stages_ & PRINT_MATCHED_TREES ) printPrimerTrees();

    show_status("evaluating primer pairs"); show_status(0.0);
    convertTreesToLists();
    if ( error ) return;
    if ( print_stages_ & PRINT_PRIMER_LISTS ) printPrimerLists();

    evaluatePrimerPairs();
    if ( error ) return;
    if ( print_stages_ & PRINT_PRIMER_PAIRS ) printPrimerPairs();
}

//
// (re)construct raw primer trees
//

// add new child to parent or update existing child
// returns child_index
int PrimerDesign::insertNode ( Node *current_, unsigned char base_, PRD_Sequence_Pos pos_, int delivered_, int offset_, int left_, int right_ )
{
    int  index     = CHAR2CHILD.INDEX[ base_ ];
    bool is_primer = primer_length.includes( delivered_ ); // new child is primer if true

  //
  // create new node if necessary or update existing node if its a primer
  //
  if ( current_->child[index] == NULL ) {
    total_node_counter_left  += left_;
    total_node_counter_right += right_;

    if ( is_primer ) {
        current_->child[index]     = new Node ( current_,base_,pos_,offset_ ); // primer => new child with positive last_base_index
        primer_node_counter_left  += left_;
        primer_node_counter_right += right_;
    } else {
        current_->child[index] = new Node ( current_,base_,0 ); // no primer => new child with zero position

    }
    current_->child_bits |= CHAR2BIT.FIELD[ base_ ];				// update child_bits of current node
  }
  else {
    if ( is_primer ) {
      current_->child[index]->last_base_index = -pos_;				// primer, but already exists => set pos to negative
      primer_node_counter_left  -= left_;
      primer_node_counter_right -= right_;
    }
  }

  return index;
}

// check if (sub)tree contains a valid primer
bool PrimerDesign::treeContainsPrimer ( Node *start )
{
    if ( start->isValidPrimer() ) return true;

    for ( int i = 0; i < 4; i++ ) {
        if ( start->child[i] != NULL ) {
            if ( treeContainsPrimer( start->child[i] ) ) return true;
        }
    }

    return false;
}

// remove non-primer-leaves and branches from the tree
void PrimerDesign::clearTree ( Node *start, int left_, int right_ )
{
  // check children
  for ( int i = 0; i < 4; i++ )
    if ( start->child[i] != NULL ) clearTree( start->child[i], left_, right_ );

  // check self
  if ( start->isLeaf() && (!start->isValidPrimer()) && (start->parent != NULL) ) {
    total_node_counter_left   -= left_;
    total_node_counter_right  -= right_;

    // remove self from parent
    start->parent->child[ CHAR2CHILD.INDEX[ start->base ] ] = NULL;
    start->parent->child_bits &= ~CHAR2BIT.FIELD[ start->base ];

    // erase node
    delete start;
  }
}

void PrimerDesign::buildPrimerTrees ()
{
#if defined(DEBUG)
    primer1.print("buildPrimerTrees : pos1\t\t","\n");
    primer2.print("buildPrimerTrees : pos2\t\t","\n");
    primer_length.print("buildPrimerTrees : length\t","\n");
    //    printf("buildPrimerTrees : 0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789\n");
    //    printf("buildPrimerTrees : %s\n",sequence);
    printf("buildPrimerTrees : (pos,base,delivered,last_base_index)\n" );
#endif

    //
    // destroy old trees if exist
    //
    if (root1) delete root1;
    if (root2) delete root2;
    root1 = new Node();
    root2 = new Node();
    total_node_counter_left   = 0;
    total_node_counter_right  = 0;
    primer_node_counter_left  = 0;
    primer_node_counter_right = 0;

    //
    // init iterator with sequence
    //
    SequenceIterator *sequence_iterator = new SequenceIterator( sequence );
    char  base;
    int   child_index;
    Node *current_node;
    int   offset = 0;

    //
    // init. offset-counter
    //
    sequence_iterator->restart( 0,primer1.min(),SequenceIterator::IGNORE,SequenceIterator::FORWARD );	// start at 1st absolute pos in aligned seq.
    while ( sequence_iterator->nextBase() != SequenceIterator::EOS )					// count bases till left range
        offset++;

    //
    // build first tree
    //
    long int primer1_length = primer1.max()-primer1.min()+1;
    show_status("searching possible primers -- left");

    for ( PRD_Sequence_Pos start_pos = primer1.min();
          (start_pos < primer1.max()-primer_length.min()) && (sequence[start_pos] != '\x00');
          start_pos++ ) {

        show_status((double)(start_pos-primer1.min())/primer1_length/2);

        // start iterator at new position
        sequence_iterator->restart( start_pos,primer1.max(),primer_length.max(),SequenceIterator::FORWARD );
        sequence_iterator->nextBase();
        if ( sequence_iterator->pos != start_pos ) { // sequence_iterator has skipped spaces = > restart at first valid base
            start_pos                                                                        = sequence_iterator->pos;
        }
        sequence_iterator->restart( start_pos,primer1.max(),primer_length.max(),SequenceIterator::FORWARD );

        // start at top of tree
        current_node = root1;

        // iterate through sequence till end-of-sequence or primer_length.max bases read
        base = sequence_iterator->nextBase();
        while ( base != SequenceIterator::EOS ) {
            if (! ((base == 'A') || (base == 'T') || (base == 'U') || (base == 'C') || (base == 'G')) ) break; // stop at IUPAC-Codes
            child_index  = insertNode( current_node, base, sequence_iterator->pos, sequence_iterator->delivered, offset, 1,0 );
            current_node = current_node->child[child_index];

            // get next base
            base = sequence_iterator->nextBase();
        }

        offset++;
    }

    //
    // clear left tree
    //
#if defined(DEBUG)
    printf ("		%li nodes left (%li primers)   %li nodes right (%li primers)\n", total_node_counter_left, primer_node_counter_left, total_node_counter_right, primer_node_counter_right );
    printf ("		clearing left tree\n" );
#endif
    show_status("clearing left primertree");
    clearTree( root1, 1, 0 );
#if defined(DEBUG)
    printf ("		%li nodes left (%li primers)   %li nodes right (%li primers)\n", total_node_counter_left, primer_node_counter_left, total_node_counter_right, primer_node_counter_right );
#endif

    //
    // count bases till right range
    //
    sequence_iterator->restart( sequence_iterator->pos, primer2.min(),SequenceIterator::IGNORE,SequenceIterator::FORWARD ); // run from current pos
    while ( sequence_iterator->nextBase() != SequenceIterator::EOS )							  // till begin of right range
        offset++;

    if ( !treeContainsPrimer( root1 ) ) {
        error = "no primer in left range found .. maybe only spaces in that range ?";
        delete sequence_iterator;
        return;
    }

    //
    // build second tree
    //
    long int primer2_length = primer2.max()-primer2.min()+1;
    show_status("searching possible primers -- right");

    for ( PRD_Sequence_Pos start_pos = primer2.min();
          (start_pos < primer2.max()-primer_length.min()) && (sequence[start_pos] != '\x00');
          start_pos++ ) {

        show_status((double)(start_pos-primer2.min())/primer2_length/2+0.5);

        // start iterator at new position
        sequence_iterator->restart( start_pos,primer2.max(),primer_length.max(),SequenceIterator::FORWARD );
        sequence_iterator->nextBase();
        if ( sequence_iterator->pos != start_pos ) { // sequence_iterator has skipped spaces = > restart at first valid base
            start_pos                                                                        = sequence_iterator->pos;
        }
        sequence_iterator->restart( start_pos,primer2.max(),primer_length.max(),SequenceIterator::FORWARD );

        // start at top of tree
        current_node = root2;

        // iterate through sequence till end-of-sequence or primer_length.max bases read
        base = sequence_iterator->nextBase();
        while ( base != SequenceIterator::EOS ) {
            if (! ((base == 'A') || (base == 'T') || (base == 'U') || (base == 'C') || (base == 'G')) ) break; // stop at unsure bases
            child_index = insertNode( current_node, base, sequence_iterator->pos, sequence_iterator->delivered, offset+sequence_iterator->delivered, 0,1 );
            current_node = current_node->child[child_index];

            // get next base
            base = sequence_iterator->nextBase();
        }

        offset++;
    }

    if ( !treeContainsPrimer( root2 ) ) {
        error = "no primer in right range found .. maybe only spaces in that range ?";
    }

    delete sequence_iterator;

    //
    // clear left tree
    //
#if defined(DEBUG)
    printf ("		%li nodes left (%li primers)   %li nodes right (%li primers)\n", total_node_counter_left, primer_node_counter_left, total_node_counter_right, primer_node_counter_right );
    printf ("		clearing right tree\n" );
#endif
    show_status("clearing right primertree");
    clearTree( root2, 0, 1 );
#if defined(DEBUG)
    printf ("		%li nodes left (%li primers)   %li nodes right (%li primers)\n", total_node_counter_left, primer_node_counter_left, total_node_counter_right, primer_node_counter_right );
#endif
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
    printf (  "findNextPrimer : start_at_ [ %c,%4li,%2i (%c %c %c %c) ]\n",
              start_at_->base,
              start_at_->last_base_index,
              start_at_->child_bits,
              (start_at_->child[1] != NULL) ? 'G' : ' ',
              (start_at_->child[0] != NULL) ? 'C' : ' ',
              (start_at_->child[3] != NULL) ? 'T' : ' ',
              (start_at_->child[2] != NULL) ? 'X' : ' ' );
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
    SearchFIFO       *fifo1             = new SearchFIFO( root1,min_distance_to_next_match,expand_IUPAC_Codes );
    SearchFIFO       *fifo2             = new SearchFIFO( root2,min_distance_to_next_match,expand_IUPAC_Codes );
    SequenceIterator *sequence_iterator = new SequenceIterator( sequence,0,SequenceIterator::IGNORE,SequenceIterator::IGNORE,SequenceIterator::FORWARD );
    char              base;
    PRD_Sequence_Pos  pos               = sequence_iterator->pos;

#if defined(DEBUG)
    printf( "root1 : [C %p, G %p, A %p, TU %p]\n",root1->child[0],root1->child[1],root1->child[2],root1->child[3] );
    printf( "root2 : [C %p, G %p, A %p, TU %p]\n",root2->child[0],root2->child[1],root2->child[2],root2->child[3] );
#endif

    show_status("match possible primers vs. seq. (forward)");
    base    = sequence_iterator->nextBase();
    while ( base != SequenceIterator::EOS ) {
        pos = sequence_iterator->pos;

        if ((pos&0x7f) == 0) {
            show_status((double)pos/seqLength/2);
        }

        // tree/fifo 1
        if ( primer1.includes( pos ) ) { // flush fifo1 if in range of Primer 1
            fifo1->flush();
        }
        else {
            // iterate through fifo1
            fifo1->iterateWith( pos,base );

            // append base to fifo1
            fifo1->push( pos );
        }

        // tree/fifo2
        if ( primer2.includes( pos ) ) { // flush fifo2 if in range of Primer 2
            fifo2->flush();
        }
        else {
            // iterate through fifo2
            fifo2->iterateWith( pos,base );

            // append base to fifo2
            fifo2->push( pos );
        }

        // get next base in sequence
        base = sequence_iterator->nextBase();
    }

    sequence_iterator->restart( pos, 0, SequenceIterator::IGNORE, SequenceIterator::BACKWARD );
    fifo1->flush();
    fifo2->flush();

    show_status("match possible primers vs. seq. (backward)");
    base = INVERT.BASE[ sequence_iterator->nextBase() ];
    while ( base != SequenceIterator::EOS ) {
        pos = sequence_iterator->pos;

        if ((pos&0x7f) == 0) {
            show_status((double)(seqLength-pos)/seqLength/2+0.5);
        }

        // tree/fifo 1
        if ( primer1.includes( pos ) ) { // flush fifo1 if in range of Primer 1
            fifo1->flush();
        }
        else {
            // iterate through fifo1
            fifo1->iterateWith( pos,base );

            // append base to fifo1
            fifo1->push( pos );
        }

        // tree/fifo2
        if ( primer2.includes( pos ) ) { // flush fifo2 if in range of Primer 2
            fifo2->flush();
        }
        else {
            // iterate through fifo2
            fifo2->iterateWith( pos,base );

            // append base to fifo2
            fifo2->push( base );
        }

        // get next base in sequence
        base = INVERT.BASE[ sequence_iterator->nextBase() ];
    }

    delete fifo1;
    delete fifo2;
    delete sequence_iterator;

    if ( !treeContainsPrimer( root1 ) ) {
        error = "no primer in left range after match vs. sequence .. maybe some clustered IUPAC-Codes in sequence ?";
        return;
    }

    if ( !treeContainsPrimer( root2 ) ) {
        error = "no primer in right range after match vs. sequence .. maybe some clustered IUPAC-Codes in sequence ?";
    }
}


//
// convertTreesToLists
//
// this also checks GC-ratio, temperature and distance
//
void PrimerDesign::calcGCandAT ( int &GC_, int &AT_, Node *start_at_ )
{
    if ( start_at_ == NULL ) return;

    if ( (start_at_->base == 'C') || (start_at_->base == 'G') )
        GC_++;
    else
        AT_++;

    calcGCandAT( GC_, AT_, start_at_->parent );
}

void PrimerDesign::convertTreesToLists ()
{
#ifdef DEBUG
    GC_ratio.print( "convertTreesToLists : GC_ratio ","  " );
    temperature.print( "temperature ","  " );
    primer_distance.print( "primer_distance ","\n" );
#endif

    Node             *cur_node;
    Item             *cur_item;
    Item             *new_item;
    int               depth;
    int               AT;
    int               GC;
    bool              GC_and_temperature_matched;
    PRD_Sequence_Pos  min_offset_1 = PRD_MAX_SEQUENCE_POS;
    PRD_Sequence_Pos  max_offset_1 = -1;
    PRD_Sequence_Pos  min_offset_2 = PRD_MAX_SEQUENCE_POS;
    PRD_Sequence_Pos  max_offset_2 = -1;
    pair< Node*,int > new_pair;      // < cur_node->child[i], depth >
    deque< pair<Node*,int> > *stack;

    //
    // list 1
    //
    cur_item = NULL;
    stack = new deque< pair<Node*,int> >;
    // push children of root on stack
    for ( int index = 0; index < 4; index++ )
        if ( root1->child[index] != NULL ) {
            new_pair = pair<Node*,int>( root1->child[index],1 );
            stack->push_back( new_pair );
        }

    if ( !stack->empty() ) {
        GC_and_temperature_matched = false;

        while ( !stack->empty() ) {
            // next node
            cur_node = stack->back().first;
            depth    = stack->back().second;
            stack->pop_back();

            // handle node
            if ( primer_length.includes(depth) && cur_node->isValidPrimer() ) {
                // calculate conditional parameters
                GC = AT = 0;
                calcGCandAT( GC, AT, cur_node );
                AT = 4*GC + 2*AT;
                GC = (GC * 100) / depth;

                // create new item if conditional parameters are in range
                if ( GC_ratio.includes(GC) && temperature.includes(AT) ) {
                    GC_and_temperature_matched = true;
                    new_item = new Item( cur_node->last_base_index, cur_node->offset, depth, GC, AT, NULL );

                    // begin list with new item or append to list
                    if ( cur_item == NULL ) list1          = new_item;
                    else cur_item->next = new_item;

                    cur_item = new_item;

                    // store position
                    if ( cur_node->offset < min_offset_1 ) min_offset_1 = cur_node->offset;
                    if ( cur_node->offset > max_offset_1 ) max_offset_1 = cur_node->offset;
                }
            }

            // push children on stack
            for ( int index = 0; index < 4; index++ )
                if ( cur_node->child[index] != NULL ) {
                    new_pair = pair<Node*,int>( cur_node->child[index],depth+1 );
                    stack->push_back( new_pair );
                }
        }
        if ( !GC_and_temperature_matched ) {
            error = "no primer over left range matched the given GC-Ratio/Temperature";
            delete stack;
            return;
        }
    }
    else {
        error = "convertTreesToLists : primertree over left range was empty :( ?";
        delete stack;
        return;
    }

#if defined(DEBUG)
    printf( "convertTreesToLists : list1 : min_offset %7li  max_offset %7li\n", min_offset_1, max_offset_1 );
#endif

    //
    // list 2
    //
    cur_item = NULL;
    stack->clear();
    bool distance_matched = false;
    // push children of root on stack
    for ( int index = 0; index < 4; index++ )
        if ( root2->child[index] != NULL )
        {
            new_pair = pair<Node*,int>( root2->child[index],1 );
            stack->push_back( new_pair );
        }

    if ( !stack->empty() ) {
        GC_and_temperature_matched = false;

        while ( !stack->empty() ) {
            // next node
            cur_node = stack->back().first;
            depth    = stack->back().second;
            stack->pop_back();

            // push children on stack
            for ( int index = 0; index < 4; index++ )
                if ( cur_node->child[index] != NULL ) {
                    new_pair = pair<Node*,int>( cur_node->child[index],depth+1 );
                    stack->push_back( new_pair );
                }

            // handle node
            if ( primer_length.includes(depth) && cur_node->isValidPrimer() ) {
                // check position
                if ( primer_distance.min() > 0 ) {
                    distance_matched = primer_distance.includes( cur_node->offset-max_offset_1, cur_node->offset-min_offset_1 );
                    if ( !distance_matched ) continue;
                }

                // calculate conditional parameters
                GC = AT = 0;
                calcGCandAT( GC, AT, cur_node );
                AT = 4*GC + 2*AT;
                GC = (GC * 100) / depth;

                // create new item if conditional parameters are in range
                if ( GC_ratio.includes(GC) && temperature.includes(AT) ) {
                    GC_and_temperature_matched = true;
                    new_item = new Item( cur_node->last_base_index, cur_node->offset, depth, GC, AT, NULL );

                    // begin list with new item or append to list
                    if ( cur_item == NULL ) list2          = new_item;
                    else cur_item->next = new_item;

                    cur_item = new_item;

                    // store position
                    if ( cur_node->offset < min_offset_2 ) min_offset_2 = cur_node->offset;
                    if ( cur_node->offset > max_offset_2 ) max_offset_2 = cur_node->offset;
                }
            }
        }

        if ( primer_distance.min() > 0 ) {
            if ( !distance_matched ) {
                error = "no primer over right range was in reach of primers in left range .. check distance parameter";
                delete stack;
                return;
            }
        }

        if ( !GC_and_temperature_matched ) {
            error = "no primer over right range matched the given GC-Ratio and Temperature";
            delete stack;
            return;
        }
    }
    else {
        error = "convertTreesToLists : primertree over right range was empty :( ?";
        delete stack;
        return;
    }

    delete stack;

#if defined(DEBUG)
    printf( "convertTreesToLists : list2 : min_offset %7li  max_offset %7li\n", min_offset_2, max_offset_2 );
#endif

    //
    // check positions of list1
    //
    if ( primer_distance.min() > 0 ) {  // skip check if primer_distance < 0
        Item *to_remove = NULL;
        cur_item = list1;

        while ( cur_item != NULL ) {
            if ( !primer_distance.includes( max_offset_2-cur_item->offset, min_offset_2-cur_item->offset ) ) {
                // primer in list 1 out of range of primers in list 2 => remove from list 1

                if ( cur_item == list1 ) {
                    // 'to delete'-item is first in list
                    list1 = list1->next;				// list 1 starts with next item
                    delete cur_item;				// delete former first item
                    cur_item = list1;				// new first item is next to be examined
                }
                else {
                    // run through list till 'next' is to delete
                    to_remove = cur_item;				// remember 'to delete'-item
                    cur_item  = list1;                       	// start at first
                    while ( cur_item ->next != to_remove )    	// run till 'next' is 'to delete'
                        cur_item = cur_item->next;
                    cur_item->next = cur_item->next->next;    	// unlink 'next' from list
                    cur_item = cur_item->next;                	// 'next' of 'to delete' is next to be examined
                    delete to_remove;                         	// ...
                }
            }
            else
                cur_item = cur_item->next;
        }

        int counter = 0;
        for ( cur_item = list1; cur_item != NULL; cur_item = cur_item->next )
            counter++;
        if ( counter == 0 ) {
            error = "no primers over left range are in reach of primers over right range .. check distance parameter";
            return;
        }
    }

    int counter = 0;
    for ( cur_item = list1; cur_item != NULL; cur_item = cur_item->next )
        counter++;
    if ( counter == 0 ) {
        error = "no primers left in left range after GC-ratio/Temperature/Distance - Check";
        return;
    }

    counter = 0;
    for ( cur_item = list2; cur_item != NULL; cur_item = cur_item->next )
        counter++;
    if ( counter == 0 ) {
        error = "no primers left in right range after GC-ratio/Temperature/Distance - Check";
        return;
    }
}


//
// printPrimerLists
//
void PrimerDesign::printPrimerLists ()
{
    int count = 0;

    printf( "printPrimerLists : list 1 : [(start_pos,offset,length), GC_ratio, temperature]\n" );
    Item *current = list1;
    while ( current != NULL ) {
        current->print( "","\n" );
        current = current->next;
        count++;
    }
    printf( " : %i valid primers\nprintPrimerLists : list 2 : [(start_pos,offset,length), GC_ratio, temperature]\n", count );
    count = 0;
    current = list2;
    while ( current != NULL ) {
        current->print( "","\n" );
        current = current->next;
        count++;
    }
    printf( " : %i valid primers\n", count );
}



//
// evaluatePrimerPairs
//
inline double PrimerDesign::evaluatePair ( Item *one_, Item *two_ )
{
    if ( (one_ == NULL) || (two_ == NULL) )
        return -1.0;

    return abs(one_->GC_ratio - two_->GC_ratio)*GC_factor + abs(one_->temperature - two_->temperature)*temperature_factor;
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

    // swap till position
    for ( int i = max_count_primerpairs-2; i >= index; i--) {
        pairs[i+1].rating = pairs[i].rating;
        pairs[i+1].one    = pairs[i].one;
        pairs[i+1].two    = pairs[i].two;
    }

    // insert new values
    pairs[index].rating = rating_;
    pairs[index].one    = one_;
    pairs[index].two    = two_;

#if defined(DEBUG)
    printf( "insertPair : [%3i] %6.2f\n", index, rating_ );
#endif
}

void PrimerDesign::evaluatePrimerPairs ()
{
    Item    *one = list1;
    Item    *two;
    double   rating;
    long int counter = 0;

#ifdef DEBUG
    printf ( "evaluatePrimerPairs : ...\ninsertPair : [index], rating\n" );
#endif

    show_status("evaluating primer pairs");
    int list1_elems = 0;
    int elems       = 0;
    while (one) {
        list1_elems++;
        one = one->next;
    }
    one = list1;

    // outer loop <= > run through list1
    while ( one != NULL )
    {
        show_status(GBS_global_string("evaluating primer pairs (%6.3f)", pairs[0].rating));
        show_status((double)elems/list1_elems);
        elems++;

        // inner loop <= > run through list2
        two            = list2;
        while ( two != NULL )
        {
            // evaluate pair
            rating = evaluatePair( one, two );

            // test if rating is besster that the worst of the best
            if ( rating > pairs[max_count_primerpairs-1].rating )
            {
                // insert in the pairs
                insertPair( rating, one, two );
                counter++;
            }

            // next in inner loop
            two = two->next;
        }

        // next in outer loop
        one = one->next;
    }

    if ( counter == 0 ) error = "no primer pair could be found, sorry :(";

#ifdef DEBUG
    printf ( "evaluatePrimerPairs : ratings [ %.3f .. %.3f ]\n\n", pairs[max_count_primerpairs-1].rating, pairs[0].rating );
#endif
}

void PrimerDesign::printPrimerPairs ()
{
#ifdef DEBUG
    printf ( "printPairs [index] [rating ( primer1[(start_pos,offset,length),(GC,temp)] , primer2[(pos,offs,len),(GC,temp)])] \n" );
#endif
    for (int i = 0; i < max_count_primerpairs; i++) {
        printf( "printPairs : [%3i]",i );
        pairs[i].print( "\t","\n",sequence );
    }
}

//  ------------------------------------------------------------
//      const char *PrimerDesign::get_result(int num) const
//  ------------------------------------------------------------
const char *PrimerDesign::get_result( int num, const char *&primers, int max_primer_length, int max_position_length, int max_length_length )  const
{
    if ( (num < 0) || (num >= max_count_primerpairs) ) return 0;		// check for valid index
    if ( !pairs[num].one || !pairs[num].two )          return 0;		// check for valid items at given index

    primers = pairs[num].get_primers( sequence );
    return pairs[num].get_result( sequence,  max_primer_length,  max_position_length,  max_length_length );
}
