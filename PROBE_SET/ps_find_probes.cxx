#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __ALGORITHM__
#include <algorithm>
#endif
#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif

template<class T>void PS_print_set_ranges( const char *_set_name, const set<T> &_set, bool _cr_at_end = true ) {
    printf( "%s size (%3i) : ", _set_name, _set.size() );
    set<T>::const_iterator range_begin = _set.begin();
    set<T>::const_iterator range_end   = range_begin;
    set<T>::const_iterator it = _set.begin();
    for ( ++it;
          it != _set.end();
          ++it ) {
        if (*it == (*range_end)+1) {
            range_end = it;
        } else {
            if (range_end == range_begin) {
                cout << *range_begin << " ";
            } else {
                cout << *range_begin << "-" << *range_end << " ";
            }
            range_begin = it;
            range_end   = it;
        }
    }
    cout << *range_begin;
    if (range_end != range_begin) cout << "-" << *range_end;
    if (_cr_at_end) cout << "\n";
}


//
// common globals
//

SpeciesID                __MAX_ID;
SpeciesID                __MIN_ID;
long                     __SPECIES_COUNT;
PS_BitMap_Counted       *__MAP;
IDSet                   __SOURCE_ID_SET;
PS_BitSet::IndexSet     __TARGET_ID_SET;

//
// globals for PS_calc_next_sets
//

//  max 10% of total speciescount difference in the
//  count of trues for the species in __SOURCE_ID_SET
#define __TRESHOLD_PERCENTAGE_NEXT_SPECIES_SET 10

SpeciesID               __MIN_SETS_ID;
SpeciesID               __MAX_SETS_ID;

//
// globals for PS_find_probes
//

//  a probe must match 45-55% of species in
//  the ID-sets to be a candidate
#define __MIN_PERCENTAGE_SET_MATCH 45
#define __MAX_PERCENTAGE_SET_MATCH 55

float                   __SOURCE_MIN_MATCH_COUNT;
float                   __SOURCE_MAX_MATCH_COUNT;
float                   __TARGET_MIN_MATCH_COUNT;
float                   __TARGET_MAX_MATCH_COUNT;
float                   __SOURCE_PERFECT_MATCH_COUNT;
float                   __TARGET_PERFECT_MATCH_COUNT;
float                   __CANDIDATE_DISTANCE;
int                     __CANDIDATE_SOURCE_MATCH_COUNT;
int                     __CANDIDATE_TARGET_MATCH_COUNT;
IDSet                   __CANDIDATE_PATH;
PS_NodePtr              __CANDIDATE_NODE;
long                    __CANDIDATES_COUNTER;
long                    __CANDIDATES_TOTAL_COUNTER;
long                    __PROBES_TOTAL_COUNTER;

//
// globals for PS_find_probe_for_sets
//

IDSet                   __PATH;


//  ----------------------------------------------------
//      void PS_find_probe_for_sets( const PS_NodePtr _ps_node )
//  ----------------------------------------------------
void PS_find_probe_for_sets( const PS_NodePtr _ps_node ) {
    SpeciesID id         = _ps_node->getNum();
    bool      has_probes = _ps_node->hasProbes();

    //
    // append ID to path
    //
    __PATH.insert( id );
    
    //
    // dont look at path until ID is greater than lowest ID in the sets of IDs
    //
    if ((id >= __MIN_SETS_ID) && has_probes) {
        ++__PROBES_TOTAL_COUNTER;
        // make intersections of __PATH with __SOURCE_ID_SET and __TARGET_ID_SET
        IDSet matched_source_IDs;
        IDSet matched_target_IDs;
        set_intersection( __SOURCE_ID_SET.begin(), __SOURCE_ID_SET.end(),
                          __PATH.begin(), __PATH.end(),
                          inserter( matched_source_IDs, matched_source_IDs.begin() ) );
        set_intersection( __TARGET_ID_SET.begin(), __TARGET_ID_SET.end(),
                          __PATH.begin(), __PATH.end(),
                          inserter( matched_target_IDs, matched_target_IDs.begin() ) );
        int count_matched_source = matched_source_IDs.size();
        int count_matched_target = matched_target_IDs.size();
        if ((count_matched_source > __SOURCE_MIN_MATCH_COUNT) &&
            (count_matched_source < __SOURCE_MAX_MATCH_COUNT) &&
            (count_matched_target > __TARGET_MIN_MATCH_COUNT) &&
            (count_matched_target < __TARGET_MAX_MATCH_COUNT)) {
            ++__CANDIDATES_TOTAL_COUNTER;
            float distance_to_perfect_match = fabs( __SOURCE_PERFECT_MATCH_COUNT - count_matched_source ) + fabs( __TARGET_PERFECT_MATCH_COUNT - count_matched_target );
            if (distance_to_perfect_match < __CANDIDATE_DISTANCE) {
                ++__CANDIDATES_COUNTER;
                __CANDIDATE_PATH               = __PATH;
                __CANDIDATE_NODE               = _ps_node;
                __CANDIDATE_SOURCE_MATCH_COUNT = count_matched_source;
                __CANDIDATE_TARGET_MATCH_COUNT = count_matched_target;
                __CANDIDATE_DISTANCE           = distance_to_perfect_match;
            }
        }
    }

    //
    // step down the children if either ID is lower than highest
    // ID in the set of ID-pairs or the node has no probes
    //
    if ((id < __MAX_SETS_ID) || (! has_probes)) {
        for ( PS_NodeMapConstIterator i = _ps_node->getChildrenBegin();
              i != _ps_node->getChildrenEnd();
              ++i) {
            PS_find_probe_for_sets( i->second );
        }
    }

    //
    // remove ID from path
    //
    __PATH.erase( id );
}


//  ----------------------------------------------------
//      void PS_find_probes( const PS_NodePtr _root_node )
//  ----------------------------------------------------
void PS_find_probes( const PS_NodePtr _root_node ) {
    __PROBES_TOTAL_COUNTER         = 0;
    __CANDIDATES_COUNTER           = 0;
    __CANDIDATES_TOTAL_COUNTER     = 0;
    __CANDIDATE_DISTANCE           = __SPECIES_COUNT;
    __CANDIDATE_SOURCE_MATCH_COUNT = 0;
    __CANDIDATE_TARGET_MATCH_COUNT = 0;
    __CANDIDATE_PATH.clear();
    __SOURCE_MIN_MATCH_COUNT = (__SOURCE_ID_SET.size() * __MIN_PERCENTAGE_SET_MATCH) / 100.0;
    __SOURCE_MAX_MATCH_COUNT = (__SOURCE_ID_SET.size() * __MAX_PERCENTAGE_SET_MATCH) / 100.0;
    __TARGET_MIN_MATCH_COUNT = (__TARGET_ID_SET.size() * __MIN_PERCENTAGE_SET_MATCH) / 100.0;
    __TARGET_MAX_MATCH_COUNT = (__TARGET_ID_SET.size() * __MAX_PERCENTAGE_SET_MATCH) / 100.0;
    __SOURCE_PERFECT_MATCH_COUNT = __SOURCE_ID_SET.size() / 2.0;
    __TARGET_PERFECT_MATCH_COUNT = __TARGET_ID_SET.size() / 2.0;
    __MIN_SETS_ID = (*__SOURCE_ID_SET.begin()  < *__TARGET_ID_SET.begin())  ? *__SOURCE_ID_SET.begin()  : *__TARGET_ID_SET.begin();
    __MAX_SETS_ID = (*__SOURCE_ID_SET.rbegin() < *__TARGET_ID_SET.rbegin()) ? *__SOURCE_ID_SET.rbegin() : *__TARGET_ID_SET.rbegin();
    printf( "source match %10.3f .. %10.3f\n", __SOURCE_MIN_MATCH_COUNT, __SOURCE_MAX_MATCH_COUNT );
    printf( "target match %10.3f .. %10.3f\n", __TARGET_MIN_MATCH_COUNT, __TARGET_MAX_MATCH_COUNT );
    int c = 0;
    for ( PS_NodeMapConstIterator i = _root_node->getChildrenBegin();
          (i != _root_node->getChildrenEnd()) && (i->first < __MAX_SETS_ID);
          ++i,++c ) {
        if (c % 100 == 0) printf( "PS_find_probe_for_sets( %i ) : %i/%i 1st level nodes -> %li/%li candidates of %li probes looked at\n", i->first, c+1, _root_node->countChildren(), __CANDIDATES_COUNTER, __CANDIDATES_TOTAL_COUNTER, __PROBES_TOTAL_COUNTER );
        __PATH.clear();
        PS_find_probe_for_sets( i->second );
    }
    printf( "%li of %li candidates\n", __CANDIDATES_COUNTER, __CANDIDATES_TOTAL_COUNTER );
    printf( "CANDIDATE:\ncount_matched_source (%3i)\ncount_matched_target (%3i)\ndistance (%.3f)\n", __CANDIDATE_SOURCE_MATCH_COUNT, __CANDIDATE_TARGET_MATCH_COUNT, __CANDIDATE_DISTANCE );
    PS_print_set_ranges( "path", __CANDIDATE_PATH );
}


//  ----------------------------------------------------
//      void PS_calc_next_speciesid_sets()
//  ----------------------------------------------------
void PS_calc_next_speciesid_sets() {
    //
    // 1. scan bitmap for species that need more matches
    //
    SpeciesID lowest_count;
    float     treshold;

    // first pass -- get lowest count
    lowest_count = __SPECIES_COUNT;
    for ( SpeciesID id = __MIN_ID;
          id <= __MAX_ID;
          ++id) {
        SpeciesID count = __MAP->getCountFor( id );
        if (count < lowest_count) lowest_count = count;
    }

    // second pass -- get IDs where count is below or equal treshold
    treshold = ( (__SPECIES_COUNT * __TRESHOLD_PERCENTAGE_NEXT_SPECIES_SET) / 100.0 ) + lowest_count;
    printf( "lowest count (%i)  treshold (%f)\n", lowest_count, treshold ); 
    __SOURCE_ID_SET.clear();
    for ( SpeciesID id = __MIN_ID;
          id <= __MAX_ID;
          ++id) {
        SpeciesID count = __MAP->getCountFor( id );
        if (count <= treshold) __SOURCE_ID_SET.insert( id );
    }
    PS_print_set_ranges( "__SOURCE_ID_SET", __SOURCE_ID_SET );

    //
    // 2. scan bitmap for species IDs that need to be distinguished from ALL species in __SOURCE_ID_SET
    //
    PS_BitSet::IndexSet next_target_set;
    IDSetCIter          source_id;

    // get the IDs that need differentiation from first ID in __SOURCE_ID_SET
    source_id = __SOURCE_ID_SET.begin();
    __MAP->getFalseIndicesFor( *source_id, __TARGET_ID_SET );
    PS_print_set_ranges( "__TARGET_ID_SET before intersecting\n", __TARGET_ID_SET );

    // now iterate over remaining IDs in __SOURCE_ID_SET
    unsigned long count_iterations = 0;
    for ( ++source_id;
          source_id != __SOURCE_ID_SET.end();
          ++source_id,++count_iterations ) {
        if (count_iterations % 100 == 0) printf( "intersecting set #%li of %i\n", count_iterations+1, __SOURCE_ID_SET.size() );
        // get 'next_target_set'
        __MAP->getFalseIndicesFor( *source_id, next_target_set );
        // make __TARGET_ID_SET the intersection of __TARGET_ID_SET and next_target_set
        // by removing all IDs from __TARGET_ID_SET that are not in next_target_set
        PS_BitSet::IndexSet::iterator id_to_remove = __TARGET_ID_SET.end();
        for ( PS_BitSet::IndexSet::iterator target_id = __TARGET_ID_SET.begin();
              target_id != __TARGET_ID_SET.end();
              ++target_id) {
            if (id_to_remove != __TARGET_ID_SET.end()) {     // remove ID that was not found in previous iteration
                __TARGET_ID_SET.erase( id_to_remove );
                id_to_remove = __TARGET_ID_SET.end();
            }
            if (next_target_set.find( *target_id  ) == next_target_set.end()) {
                id_to_remove = target_id;
            }
        }
        if (id_to_remove != __TARGET_ID_SET.end()) {         // remove ID that was not found in last iteration (if any)
            __TARGET_ID_SET.erase( id_to_remove );
        }
    }
    PS_print_set_ranges( "__TARGET_ID_SET after intersecting\n", __TARGET_ID_SET );
    
}


//  ----------------------------------------------------
//      void PS_apply_candidate_to_bitmap()
//  ----------------------------------------------------
void PS_apply_candidate_to_bitmap() {
    // iterate over all IDs except path
    IDSetCIter path_iter    = __CANDIDATE_PATH.begin();
    SpeciesID  next_path_ID = *path_iter;
    for ( SpeciesID not_in_path_ID = __MIN_ID;
          not_in_path_ID <= __MAX_ID;
          ++not_in_path_ID) {
        if (not_in_path_ID == next_path_ID) {   // if i run into a ID in path
            ++path_iter;                        // advance to next path ID
            next_path_ID = (path_iter == __CANDIDATE_PATH.end()) ? -1 : *path_iter;
            continue;                           // skip this ID
        }
        // iterate over path IDs
        for ( IDSetCIter path_ID = __CANDIDATE_PATH.begin();
              path_ID != __CANDIDATE_PATH.end();
              ++path_ID) {
            if (not_in_path_ID == *path_ID) continue;   // obviously a probe cant differ a species from itself
            if (not_in_path_ID > *path_ID) {
                __MAP->setTrue( not_in_path_ID,*path_ID );
            } else {
                __MAP->setTrue( *path_ID,not_in_path_ID );
            }
        }
    }
    __MAP->recalcCounters();
}


//  ====================================================
//  ====================================================
int main( int argc,  char *argv[] ) {

   // open probe-set-database
    if (argc < 3) {
        printf( "Missing argument\n Usage %s <database name> <result filename> [bitmap]\n", argv[0] );
        exit( 1 );
    }

    const char *input_DB_name   = argv[1];
    const char *result_filename = argv[2];
    const char *bitmap_filename = (argc > 3) ? argv[2] : 0;
    char       *buffer          = (char *)malloc( 1024 );

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
    __MIN_ID = db->getMinID();
    __SPECIES_COUNT = db->getSpeciesCount();
    printf( "min ID (%i)  max ID (%i)  count of species (%li)\n", __MIN_ID, __MAX_ID, __SPECIES_COUNT );
    printf( "..loaded database (enter to continue)\n" );
//    getchar();

    printf( "Opening result file %s..\n", result_filename );
    PS_FileBuffer *result_file = new PS_FileBuffer( result_filename, PS_FileBuffer::READONLY );
    long size;
    SpeciesID id1,id2;
    printf( "reading no matches : " );
    result_file->get_long( size );
    printf( "(%li)\n", size );
    for ( long i=0;
          i < size;
          ++i ) {
        result_file->get_int( id1 );
        result_file->get_int( id2 );
        printf( "  %i,%i", id1, id2 );
    }
    printf( "\nreading one matches : " );
    result_file->get_long( size );
    printf( "(%li)\n", size );
    long path_length;
    SpeciesID path_id;
    for ( long i=0;
          i < size;
          ++i) {
        result_file->get_int( id1 );
        result_file->get_int( id2 );
        result_file->get_long( path_length );
        printf( "%i,%i path(%li)", id1, id2, path_length );
        while (path_length-- > 0) {
            result_file->get_int( path_id );
            printf( " %i", path_id );
        }
        printf( "\n" );
    }
    printf( "loading preset bitmap...\n" );
    __MAP = new PS_BitMap_Counted( result_file );
    printf( "..loaded result file (enter to continue)\n" );
//    getchar();

    PS_calc_next_speciesid_sets();
    printf( "calculated next speciesid-sets (enter to continue)\n" );
//    getchar();
    
    PS_find_probes( db->getRootNode() );
    printf( "found probe for speciesid-sets (enter to continue)\n" );
//    getchar();

//    PS_apply_candidate_to_bitmap();
    printf( "applied candidate to bitmap (enter to continue)\n" );
//    getchar();

    if (bitmap_filename) {
        string title("Bitmap after first candidate");
        result_file->reinit( bitmap_filename, PS_FileBuffer::WRITEONLY );
        __MAP->printGNUplot( title.c_str(), buffer, result_file );
        printf( "finished output of bitmap in GNUplot format (enter to continue)\n" );
        //    getchar();
    }

    printf( "cleaning up... result file\n" );
    delete result_file;
    printf( "cleaning up... database\n" );
    delete db;
    printf( "cleaning up... bitmap\n" );
    delete __MAP;
    printf( "(enter to continue)\n" );
//    getchar();

    return 0;
}
