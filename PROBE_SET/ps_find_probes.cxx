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

template<class T>void PS_print_set_ranges( const char *_set_name, const set<T> &_set, const bool _cr_at_end = true ) {
    fflush( stdout );
    printf( "%s size (%3i) : ", _set_name, _set.size() );
    if (_set.size() == 0) {
        printf( "(empty)" );
    } else {
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
    }
    if (_cr_at_end) cout << "\n";
    fflush( stdout );
}


template<class T1,class T2>void PS_print_map_ranges( const char *_map_name, const map<T1,T2> &_map, const bool _compare_keys = true, const bool _cr_at_end = true ) {
    fflush( stdout );
    if (_map.size() == 0) {
        printf( "%s size (  0) : (empty)", _map_name );
    } else {
        if (_compare_keys) {
            printf( "%s size (%3i) : ", _map_name, _map.size() );
            map<T1,T2>::const_iterator range_begin = _map.begin();
            map<T1,T2>::const_iterator range_end   = range_begin;
            map<T1,T2>::const_iterator it = _map.begin();
            for ( ++it;
                  it != _map.end();
                  ++it ) {
                if (it->first == range_end->first+1) {
                    range_end = it;
                } else {
                    if (range_end == range_begin) {
                        cout << "(" << range_begin->first << "," << range_begin->second << ") ";
                    } else {
                        cout << "(" << range_begin->first << "-" << range_end->first << "," << range_begin->second << ") ";
                    }
                    range_begin = it;
                    range_end   = it;
                }
            }
            if (range_end == range_begin) {
                cout << "(" << range_begin->first << "," << range_begin->second << ") ";
            } else {
                cout << "(" << range_begin->first << "-" << range_end->first << "," << range_begin->second << ") ";
            }
        } else {
            map<T2,set<pair<T1,T1> > > value2indices;
            map<T2,unsigned long int>  value2count;
            pair<T1,T1> range;
            range.first  = _map.begin()->first;
            range.second = range.first;
            T2 cur_value = _map.begin()->second;
            map<T1,T2>::const_iterator it = _map.begin();
            for ( ++it;
                  it != _map.end();
                  ++it ) {
                if (it->second == cur_value) {
                    if (it->first == range.second+1) {
                        range.second = it->first;
                    } else {
                        value2indices[ cur_value ].insert( range );
                        value2count[ cur_value ] += range.second - range.first +1;
                        range.first  = it->first;
                        range.second = it->first;
                    }
                } else {
                    value2indices[ cur_value ].insert( range );
                    value2count[ cur_value ] += range.second - range.first +1;
                    range.first  = it->first;
                    range.second = it->first;
                    cur_value    = it->second;
                }
            }
            value2indices[ cur_value ].insert( range );
            value2count[ cur_value ] += range.second - range.first +1;
            for ( map<T2,set<pair<T1,T1> > >::const_iterator value = value2indices.begin();
                  value != value2indices.end();
                  ++value ) {
                if (value != value2indices.begin()) cout << "\n";
                printf( "%s size (%3i) : value (", _map_name, _map.size() );
                cout << value->first << ") count (" << value2count[ value->first ] << ") [";
                for ( set<pair<T1,T1> >::const_iterator indices = value->second.begin();
                      indices != value->second.end();
                      ++indices ) {
                    if (indices != value->second.begin()) cout << " ";
                    cout << indices->first;
                    if (indices->first != indices->second) cout << "-" << indices->second;
                }
                cout << "]," << value->first << ")";
            }
        }
    }
    if (_cr_at_end) cout << "\n";
    fflush( stdout );
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

//  count of trues for a source_id may be max 10%
//  (of the difference between __SPECIES_COUNT and the lowest
//  count of trues per species) higher than the lowest count
//  of trues for the species in __SOURCE_ID_SET
#define __TRESHOLD_PERCENTAGE_NEXT_SOURCE_ID_SET 10
//  target_id must be in min 90% of total target_id_sets
//  to be in __TARGET_ID_SET
#define __TRESHOLD_PERCENTAGE_NEXT_TARGET_ID_SET 90

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
                printf( " PS_find_probe_for_sets() : new candidate: count_matched_source (%3i/%.3f) count_matched_target (%i/%.3f) distance (%.3f) path (%i)\n",
                        __CANDIDATE_SOURCE_MATCH_COUNT,
                        __SOURCE_PERFECT_MATCH_COUNT,
                        __CANDIDATE_TARGET_MATCH_COUNT,
                        __TARGET_PERFECT_MATCH_COUNT,
                        __CANDIDATE_DISTANCE,
                        __CANDIDATE_PATH.size() );
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
    printf( "PS_find_probes() : source match %10.3f .. %10.3f\n", __SOURCE_MIN_MATCH_COUNT, __SOURCE_MAX_MATCH_COUNT );
    printf( "PS_find_probes() : target match %10.3f .. %10.3f\n", __TARGET_MIN_MATCH_COUNT, __TARGET_MAX_MATCH_COUNT );
    int c = 0;
    for ( PS_NodeMapConstIterator i = _root_node->getChildrenBegin();
          (i != _root_node->getChildrenEnd()) && (i->first < __MAX_SETS_ID);
          ++i,++c ) {
        //if (c % 100 == 0) printf( "PS_find_probe_for_sets( %i ) : %i/%i 1st level nodes -> %li/%li candidates of %li probes looked at\n", i->first, c+1, _root_node->countChildren(), __CANDIDATES_COUNTER, __CANDIDATES_TOTAL_COUNTER, __PROBES_TOTAL_COUNTER );
        __PATH.clear();
        PS_find_probe_for_sets( i->second );
    }
    printf( "PS_find_probes() : %li of %li candidates of %li probes visited\n", __CANDIDATES_COUNTER, __CANDIDATES_TOTAL_COUNTER, __PROBES_TOTAL_COUNTER );
    printf( "PS_find_probes() : CANDIDATE: count_matched_source (%3i) count_matched_target (%3i) distance (%.3f)\n", __CANDIDATE_SOURCE_MATCH_COUNT, __CANDIDATE_TARGET_MATCH_COUNT, __CANDIDATE_DISTANCE );
    PS_print_set_ranges( "PS_find_probes() : CANDIDATE: path", __CANDIDATE_PATH );
    fflush( stdout );
}


//  ----------------------------------------------------
//      void PS_calc_next_speciesid_sets()
//  ----------------------------------------------------
void PS_calc_next_speciesid_sets() {
    //
    // 1. __SOURCE_ID_SET
    //    scan bitmap for species that need more matches
    //
    SpeciesID highest_count;
    SpeciesID lowest_count;
    float     treshold;

    // first pass -- get lowest count of trues and calc treshold
    lowest_count  = __SPECIES_COUNT;
    highest_count = 0;
    for ( SpeciesID id = __MIN_ID;
          id <= __MAX_ID;
          ++id) {
        SpeciesID count = __MAP->getCountFor( id );
        if (count > highest_count) highest_count = count;
        if (count < lowest_count)  lowest_count  = count;
    }
    treshold = ( ((__SPECIES_COUNT-lowest_count) * __TRESHOLD_PERCENTAGE_NEXT_SOURCE_ID_SET) / 100.0 ) + lowest_count;
    printf( "PS_calc_next_speciesid_sets() : SOURCE count 1's [%i..%i]  treshold (%f)\n", lowest_count, highest_count, treshold ); 

    // second pass -- get IDs where count is below or equal treshold
    __SOURCE_ID_SET.clear();
    for ( SpeciesID id = __MIN_ID;
          id <= __MAX_ID;
          ++id) {
        if (__MAP->getCountFor( id ) <= treshold) __SOURCE_ID_SET.insert( id );
    }
    PS_print_set_ranges( "PS_calc_next_speciesid_sets() : __SOURCE_ID_SET", __SOURCE_ID_SET );

    //
    // 2. __TARGET_ID_SET
    //    scan bitmap for species IDs that need to be distinguished from MOST species in __SOURCE_ID_SET
    //
    ID2IDMap count_falses_per_id;

    // first -- get the IDs that need differentiation from __SOURCE_ID_SET
    unsigned long count_iterations = 0;
    for ( IDSetCIter source_id = __SOURCE_ID_SET.begin();
          source_id != __SOURCE_ID_SET.end();
          ++source_id,++count_iterations ) {
        //if (count_iterations % 100 == 0) printf( "PS_calc_next_speciesid_sets() : set #%li of %i\n", count_iterations+1, __SOURCE_ID_SET.size() );
        // get 'next_target_set'
        __MAP->getFalseIndicesFor( *source_id, __TARGET_ID_SET );
        // count falses per SpeciesID
        for ( PS_BitSet::IndexSet::iterator target_id = __TARGET_ID_SET.begin();
              target_id != __TARGET_ID_SET.end();
              ++target_id) {
            if (*target_id != *source_id) ++count_falses_per_id[ *target_id ];
        }
    }
    PS_print_map_ranges( "PS_calc_next_speciesid_sets() : count_falses_per_id", count_falses_per_id, false );

    // second -- get highest count of falses and calc treshold
    lowest_count  = __SPECIES_COUNT;
    highest_count = 0;
    for ( ID2IDMapCIter count_per_id = count_falses_per_id.begin();
          count_per_id != count_falses_per_id.end();
          ++count_per_id ) {
        if (count_per_id->second > highest_count) highest_count = count_per_id->second;
        if (count_per_id->second < lowest_count)  lowest_count  = count_per_id->second;
    }
    printf( "PS_calc_next_speciesid_sets() : TARGET count 0's [%i..%i]\n", lowest_count, highest_count ); 

    // third -- put all IDs in __TARGET_ID_SET that are needed most by species from __SOURCE_ID_SET
    __TARGET_ID_SET.clear();
    unsigned long int count_falses = 0;
    for ( ID2IDMapCIter count_per_id = count_falses_per_id.begin();
          count_per_id != count_falses_per_id.end();
          ++count_per_id ) {
        if (count_per_id->second == highest_count) {
            __TARGET_ID_SET.insert( count_per_id->first );
            count_falses += count_per_id->second;
        }
    }
    printf( "PS_calc_next_speciesid_sets() : %lu falses ", count_falses );
    PS_print_set_ranges( "__TARGET_ID_SET", __TARGET_ID_SET );
    
}


//  ----------------------------------------------------
//      void PS_apply_candidate_to_bitmap()
//  ----------------------------------------------------
void PS_apply_candidate_to_bitmap() {
    // iterate over all IDs except path
    IDSetCIter path_iter    = __CANDIDATE_PATH.begin();
    SpeciesID  next_path_ID = *path_iter;
    unsigned long int gain  = 0;
    unsigned long int sets  = 0;
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
            ++sets;
            if (not_in_path_ID > *path_ID) {
                gain += (__MAP->set( not_in_path_ID, *path_ID, true ) == false);
            } else {
                gain += (__MAP->set( *path_ID, not_in_path_ID, true ) == false);
            }
        }
    }
    printf( "PS_apply_candidate_to_bitmap() : gain %lu of %lu -- %.2f%%  -> wasted %lu\n", gain, sets, ((float)gain/sets)*100.0, sets-gain );
    //__MAP->recalcCounters();
}


//  ----------------------------------------------------
//      void PS_remove_candidate_from_tree( const PS_NodePtr _root )
//  ----------------------------------------------------
void PS_remove_candidate_from_tree( const PS_NodePtr _root ) {
    PS_NodePtr current_node = _root;
    for ( IDSetCIter id = __CANDIDATE_PATH.begin();
          id != __CANDIDATE_PATH.end();
          ++id ){
        current_node = current_node->getChild( *id ).second;
    }
    printf( "PS_remove_candidate_from_tree() : before " );
    current_node->printOnlyMe();
    current_node->removeProbes();
    printf( "\nPS_remove_candidate_from_tree() : after  " );
    current_node->printOnlyMe();
    printf( "\n" );
}


//  ====================================================
//  ====================================================
int main( int argc,  char *argv[] ) {

   // open probe-set-database
    if (argc < 3) {
        printf( "Missing argument\n Usage %s <database name> <result filename> [result filename prefix for output files]\n", argv[0] );
        exit( 1 );
    }

    const char *input_DB_name      = argv[1];
    const char *result_in_filename = argv[2];
    const char *result_out_prefix  = (argc > 3) ? argv[3] : 0;
    char       *buffer             = (char *)malloc( 4096 );

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
    __MIN_ID = db->getMinID();
    __SPECIES_COUNT = db->getSpeciesCount();
    printf( "min ID (%i)  max ID (%i)  count of species (%li)\n", __MIN_ID, __MAX_ID, __SPECIES_COUNT );
    printf( "..loaded database (enter to continue)\n" );
//    getchar();

    printf( "Opening result file %s..\n", result_in_filename );
    PS_FileBuffer *result_file = new PS_FileBuffer( result_in_filename, PS_FileBuffer::READONLY );
    long size;
    ID2IDSet noMatches;
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
        if (id1 < id2) {
            noMatches.insert( ID2IDPair(id1,id2) );
        } else {
            noMatches.insert( ID2IDPair(id2,id1) );
        }
    }
    printf( "\nreading one matches : " );
    result_file->get_long( size );
    printf( "(%li)\n", size );
    long          path_length;
    SpeciesID     path_id;
    IDSet         path;
    IDID2IDSetMap oneMatchesMap;
    for ( long i=0;
          i < size;
          ++i) {
        path.clear();
        result_file->get_int( id1 );
        result_file->get_int( id2 );
        result_file->get_long( path_length );
        printf( "%i,%i path(%li)", id1, id2, path_length );
        while (path_length-- > 0) {
            result_file->get_int( path_id );
            printf( " %i", path_id );
            path.insert( path_id );
        }
        printf( "\n" );
        oneMatchesMap[ ID2IDPair(id1,id2) ] = path;
    }
    printf( "loading preset bitmap...\n" );
    __MAP = new PS_BitMap_Counted( result_file );
    printf( "..loaded result file (enter to continue)\n" );
//     getchar();

    long iteration = 0;
    while (true) {
        ++iteration;

        PS_calc_next_speciesid_sets();
        printf( "\n----------\ncalculated next speciesid-sets (enter to continue)\n----------\n\n" );
    
        PS_find_probes( db->getRootNode() );
        printf( "\n----------\nfound probe for speciesid-sets (enter to continue)\n----------\n\n" );

        if (__CANDIDATE_PATH.size() == 0) break;

        PS_apply_candidate_to_bitmap();
        printf( "\n----------\napplied candidate to bitmap (enter to continue)\n----------\n\n" );

        PS_remove_candidate_from_tree( db->getRootNode() );
        printf( "\n----------\nremoved candidate from tree (enter to continue)\n----------\n\n" );

        if (result_out_prefix) {
            // open data file
            sprintf( buffer, "%s.%06li", result_out_prefix, iteration );
            char *data_name = strdup( buffer );
            result_file->reinit( data_name, PS_FileBuffer::WRITEONLY );
            // output data
            size = sprintf( buffer, "# CANDIDATE #%li path (%i)\n", iteration, __CANDIDATE_PATH.size() );
            result_file->put( buffer, size );
            for ( IDSetCIter i = __CANDIDATE_PATH.begin();
                  i != __CANDIDATE_PATH.end();
                  ++i ) {
                size = sprintf( buffer, "# %i\n", *i );
                result_file->put( buffer, size );
            }
            size = sprintf( buffer, "#noMatches (%i)\n", noMatches.size() );
            result_file->put( buffer, size );
            for ( ID2IDSetCIter p = noMatches.begin();
                  p != noMatches.end();
                  ++p ) {
                size = sprintf( buffer, "%i %i\n", p->first, p->second );
                result_file->put( buffer, size );
            }
            size = sprintf( buffer, "\n\n" );
            result_file->put( buffer, size );
            sprintf( buffer, "Bitmap after %li. candidate", iteration );
            char *title = strdup( buffer );
            __MAP->printGNUplot( title, buffer, result_file );
            // open gnuplot command file
            sprintf( buffer, "%s.%06li.gnuplot", result_out_prefix, iteration );
            result_file->reinit( buffer, PS_FileBuffer::WRITEONLY );
            // output gnuplot commands
            size = sprintf( buffer, "set terminal png color\nset output '%06li.bitmap.png\n", iteration );
            result_file->put( buffer, size );
            size = sprintf( buffer, "set yrange [%i:%i] reverse\nset xrange [%i:%i]\n", __MIN_ID,__MAX_ID,__MIN_ID,__MAX_ID );
            result_file->put( buffer, size );
            size = sprintf( buffer, "set size square\nset title '%li. iteration'\nset pointsize 0.5\n", iteration );
            result_file->put( buffer, size );
            size = sprintf( buffer, "plot '%s' index 1 title 'match ()' with dots 2,'%s' index 0 title 'no match (30)' with points 1\n", data_name, data_name );
            result_file->put( buffer, size );
            size = sprintf( buffer, "set terminal png color\nset output '%06li.counters.png\n", iteration );
            result_file->put( buffer, size );
            size = sprintf( buffer, "set yrange [] noreverse\nset size nosquare\n" );
            result_file->put( buffer, size );
            size = sprintf( buffer, "plot '%s' index 2 title 'count/species' with impulses 2,'%s' index 3 title 'species/count' with points 1", data_name, data_name );
            result_file->put( buffer, size );

            result_file->flush();
            free( title );
            free( data_name );
            printf( "\n----------\nfinished output of bitmap in GNUplot format (enter to continue)\n----------\n\n" );
        }
        if (iteration == 10) break;
    }

    printf( "cleaning up... result file\n" );
    delete result_file;
    printf( "cleaning up... database\n" );
    delete db;
    printf( "cleaning up... bitmap\n" );
    fflush( stdout );
    delete __MAP;
    printf( "(enter to continue)\n" );
//    getchar();

    return 0;
}
