#include "PRD_SearchFIFO.hxx"
#include <cstdio>
#include <cmath>

using std::printf;
using std::fabs;

//
// Constructor
//
void SearchFIFO::init( Node *root_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_IUPAC_Codes_ )
{
    begin   = NULL;
    end     = NULL;
    current = NULL;

    root                       = root_;
    expand_IUPAC_Codes         = expand_IUPAC_Codes_;
    min_distance_to_next_match = min_distance_to_next_match_;
}

SearchFIFO::SearchFIFO ( Node *root_, PRD_Sequence_Pos min_distance_to_next_match_, bool expand_IUPAC_Codes_ )
{
    init( root_,min_distance_to_next_match_,expand_IUPAC_Codes_ );
}

SearchFIFO::SearchFIFO ()
{
    init( NULL,-1,false );
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
}


//
// append new SearchParameter to the FIFO
//
// the current_node is determined from root with the given base
// multiple parameters are generated for unsure bases like 'N'
//
void SearchFIFO::push ( unsigned char base_ )
{
    unsigned int     bits = CHAR2BIT.FIELD[ base_ ];
    Node            *child_of_root;
    SearchParameter *new_parameter;

    if ( bits & 1 ) {
        child_of_root = root->child[ 2 ]; // 2 = A
        if ( child_of_root != NULL ) {
            new_parameter = new SearchParameter;

            new_parameter->node      = child_of_root;
            new_parameter->next      = NULL;
            new_parameter->previous  = end;

            if (end   != NULL) end->next = new_parameter;
            if (begin == NULL) begin = new_parameter;
            end = new_parameter;
        }
    }

    if ( bits & 2 ) {
        child_of_root = root->child[ 3 ]; // 3 = T/U
        if ( child_of_root != NULL ) {
            new_parameter = new SearchParameter;

            new_parameter->node      = child_of_root;
            new_parameter->next      = NULL;
            new_parameter->previous  = end;

            if (end   != NULL) end->next = new_parameter;
            if (begin == NULL) begin = new_parameter;
            end = new_parameter;
        }
    }

    if ( bits & 4 ) {
        child_of_root = root->child[ 0 ]; // 0 = C
        if ( child_of_root != NULL ) {
            new_parameter = new SearchParameter;

            new_parameter->node      = child_of_root;
            new_parameter->next      = NULL;
            new_parameter->previous  = end;

            if (end   != NULL) end->next = new_parameter;
            if (begin == NULL) begin = new_parameter;
            end = new_parameter;
        }
    }

    if ( bits & 8 ) {
        child_of_root = root->child[ 1 ]; // 1 = G
        if ( child_of_root != NULL ) {
            new_parameter = new SearchParameter;

            new_parameter->node      = child_of_root;
            new_parameter->next      = NULL;
            new_parameter->previous  = end;

            if (end   != NULL) end->next = new_parameter;
            if (begin == NULL) begin = new_parameter;
            end = new_parameter;
        }
    }

}


//
// step down in tree at all the SearchParameters with the given base(s)
//
void SearchFIFO::push_front( Node *child_of_current_ )
{
    SearchParameter *new_parameter = new SearchParameter;

    new_parameter->node      = child_of_current_;
    new_parameter->next      = begin;
    new_parameter->previous  = NULL;

    if (end   == NULL) end             = new_parameter;
    if (begin != NULL) begin->previous = new_parameter;
    begin = new_parameter;
}

void SearchFIFO::iterateWith ( PRD_Sequence_Pos pos_, unsigned char base_ )
{
    if ( begin == NULL ) return;

    Node *child;
    int   bits;

    current = begin;

    if ( !expand_IUPAC_Codes ) {
        while ( current != NULL ) {

            // get childnode of parameters current node
            child = current->node->childByBase( base_ );

            // erase parameter if child doesnt exist
            if ( child == NULL ) {
                erase( current );
            }
            else {
                // step down as child exists
                current->node = child;

                // invalidate if node is primer and is in range
                if ( child->isValidPrimer() ) {
                    if ( min_distance_to_next_match <= 0 ) {
                        child->last_base_index = -pos_;
                    }
                    else {
                        if ( min_distance_to_next_match >= fabs(double(pos_ - child->last_base_index)) ) child->last_base_index = -pos_;
                    }
                }

                // erase parameter if child is leaf
                if ( child->isLeaf() ) {
                    erase( current );

                    // erase node in tree if leaf and invalid primer
                    if ( !child->isValidPrimer() ) {
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
    else { // expand IUPAC-Codes
        while ( current != NULL ) {
            bits = current->node->child_bits & CHAR2BIT.FIELD[ base_ ];

            if ( bits & 1 ) { // A
                child = current->node->child[ 2 ]; // 2 = A

                // invalidate if node is primer and is in range
                if ( child->isValidPrimer() ) {
                    if ( min_distance_to_next_match <= 0 ) {
                        child->last_base_index = -pos_;
                    }
                    else {
                        if ( min_distance_to_next_match >= fabs(double(pos_ - child->last_base_index)) ) child->last_base_index = -pos_;
                    }
                }

                // erase child if child is leaf
                if ( child->isLeaf() ) {
                    // erase node in tree if leaf and invalid primer
                    if ( !child->isValidPrimer() ) {
                        // remove child from parent
                        child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
                        child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
                        // erase node
                        delete child;
                    }
                }
                else { // add new parameter at the begin of fifo
                    push_front( child );
                }
            }

            if ( bits & 2 ) { // T/U
                child = current->node->child[ 3 ]; // 3 = T/U

                // invalidate if node is primer and is in range
                if ( child->isValidPrimer() ) {
                    if ( min_distance_to_next_match <= 0 ) {
                        child->last_base_index = -pos_;
                    }
                    else {
                        if ( min_distance_to_next_match >= fabs(double(pos_ - child->last_base_index)) ) child->last_base_index = -pos_;
                    }
                }

                // erase child if child is leaf
                if ( child->isLeaf() ) {
                    // erase node in tree if leaf and invalid primer
                    if ( !child->isValidPrimer() ) {
                        // remove child from parent
                        child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
                        child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
                        // erase node
                        delete child;
                    }
                }
                else { // add new parameter at the begin of fifo
                    push_front( child );
                }
            }

            if ( bits & 4 ) { // C
                child = current->node->child[ 0 ]; // 0 = C

                // invalidate if node is primer and is in range
                if ( child->isValidPrimer() ) {
                    if ( min_distance_to_next_match <= 0 ) {
                        child->last_base_index = -pos_;
                    }
                    else {
                        if ( min_distance_to_next_match >= fabs(double(pos_ - child->last_base_index)) ) child->last_base_index = -pos_;
                    }
                }

                // erase child if child is leaf
                if ( child->isLeaf() ) {
                    // erase node in tree if leaf and invalid primer
                    if ( !child->isValidPrimer() ) {
                        // remove child from parent
                        child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
                        child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
                        // erase node
                        delete child;
                    }
                }
                else { // add new parameter at the begin of fifo
                    push_front( child );
                }
            }

            if ( bits & 8 ) { // G
                child = current->node->child[ 1 ]; // 1 = G

                // invalidate if node is primer and is in range
                if ( child->isValidPrimer() ) {
                    if ( min_distance_to_next_match <= 0 ) {
                        child->last_base_index = -pos_;
                    }
                    else {
                        if ( min_distance_to_next_match >= fabs(double(pos_ - child->last_base_index)) ) child->last_base_index = -pos_;
                    }
                }

                // erase child if child is leaf
                if ( child->isLeaf() ) {
                    // erase node in tree if leaf and invalid primer
                    if ( !child->isValidPrimer() ) {
                        // remove child from parent
                        child->parent->child[ CHAR2CHILD.INDEX[ base_ ] ] = NULL;
                        child->parent->child_bits &= ~CHAR2BIT.FIELD[ base_ ];
                        // erase node
                        delete child;
                    }
                }
                else { // add new parameter at the begin of fifo
                    push_front( child );
                }
            }

            // erase old parameter
            erase( current );

            // next parameter (or begin if erase killed current while pointing to begin)
            current = (current == NULL) ? begin : current->next;
        }
    }
}


//
// erase given parameter while not invalidating current
//
void SearchFIFO::erase ( SearchParameter *param_ )
{
    // unlink from doublelinked list
    if ( param_->next     != NULL ) param_->next->previous = param_->previous; else end   = param_->previous;
    if ( param_->previous != NULL ) param_->previous->next = param_->next;     else begin = param_->next;

    // set current to current->previous if current is to delete
    if ( param_ == current ) current = param_->previous;

    delete param_;
}


//
// print positions-list
//
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

    while ( current != NULL ) {
        printf( "print : %p (%p[%c],%p,%p)\n", current, current->node, current->node->base, current->previous, current->next );
        current = current->next;
    }
}
