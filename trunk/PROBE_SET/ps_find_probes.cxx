#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#ifndef __ALGORITHM__
#include <algorithm>
#endif
#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif
#ifndef PS_CANDIDATE_HXX
#include "ps_candidate.hxx"
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


void PS_print_time_diff( const struct tms *_since ) {
    struct tms now;
    times( &now );
    printf( "time used : user (" );
    unsigned int minutes = (now.tms_utime-_since->tms_utime)/CLK_TCK / 60;
    unsigned int hours   = minutes / 60; 
    minutes -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs) system (", (float)(now.tms_utime-_since->tms_utime)/CLK_TCK-(hours*3600)-(minutes*60) );
    minutes  = (now.tms_stime-_since->tms_stime)/CLK_TCK / 60;
    hours    = minutes / 60; 
    minutes -= hours * 60;
    if (hours > 0) printf( "%uh ", hours );
    if (minutes > 0) printf( "%um ", minutes );
    printf( "%.3fs)\n",  (float)(now.tms_stime-_since->tms_stime)/CLK_TCK-(hours*3600)-(minutes*60) );
}


//
// common globals
//

SpeciesID                __MAX_ID;
SpeciesID                __MIN_ID;
unsigned long            __SPECIES_COUNT;
PS_BitMap_Counted       *__MAP;
PS_BitMap_Counted       *__PRESET_MAP;
IDSet                    __SOURCE_ID_SET;
PS_BitSet::IndexSet      __TARGET_ID_SET;

//
// globals for PS_calc_next_sets
//

//  count of trues for a source_id may be max 10%
//  (of the difference between __SPECIES_COUNT and the lowest
//  count of trues per species) higher than the lowest count
//  of trues for the species in __SOURCE_ID_SET
#define __TRESHOLD_PERCENTAGE_NEXT_SOURCE_ID_SET 10
//  target_id must be at least in 90% of total target_id_sets
//  to be in __TARGET_ID_SET
#define __TRESHOLD_PERCENTAGE_NEXT_TARGET_ID_SET 90

SpeciesID               __MIN_SETS_ID;
SpeciesID               __MAX_SETS_ID;

//
// globals for PS_find_probes
//

//  initially a probe must match 40-60% of species
//  in the ID-sets to be a candidate
//  if PS_find_probes fails to find candidates within this range
//  it is repeated with a broadened range (by 5% each border)
#define __MIN_PERCENTAGE_SET_MATCH 40
#define __MAX_PERCENTAGE_SET_MATCH 60

float                   __SOURCE_MIN_MATCH_COUNT;
float                   __SOURCE_MAX_MATCH_COUNT;
float                   __TARGET_MIN_MATCH_COUNT;
float                   __TARGET_MAX_MATCH_COUNT;
float                   __SOURCE_PERFECT_MATCH_COUNT;
float                   __TARGET_PERFECT_MATCH_COUNT;
unsigned long           __PROBES_COUNTER;
PS_CandidatePtr         __CANDIDATES_ROOT;
unsigned long           __CANDIDATES_COUNTER;
unsigned long           __CANDIDATES_TODO;
unsigned long           __CANDIDATES_FINISHED;
unsigned long           __MAX_DEPTH;

//
// globals for PS_find_probe_for_sets
//             PS_test_candidate_on_bitmap
//

IDSet                   __PATH;

//
// globals for PS_descend
//
char                   *__PATH_IN_CANDIDATES;


//  ----------------------------------------------------
//      void PS_test_candidate_on_bitmap()
//  ----------------------------------------------------
//  returns number of locations in __MAP that would be switched
//  from false to true by __PATH
//
unsigned long int PS_test_candidate_on_bitmap() {
    // iterate over all IDs except path
    IDSetCIter path_iter    = __PATH.begin();
    SpeciesID  next_path_ID = *path_iter;
    unsigned long int gain  = 0;
    for ( SpeciesID not_in_path_ID = __MIN_ID;
          not_in_path_ID <= __MAX_ID;
          ++not_in_path_ID) {
        if (not_in_path_ID == next_path_ID) {   // if i run into a ID in path
            ++path_iter;                        // advance to next path ID
            next_path_ID = (path_iter == __PATH.end()) ? -1 : *path_iter;
            continue;                           // skip this ID
        }
        // iterate over path IDs
        for ( IDSetCIter path_ID = __PATH.begin();
              path_ID != __PATH.end();
              ++path_ID) {
            if (not_in_path_ID == *path_ID)
                continue;   // obviously a probe cant differ a species from itself
            if (not_in_path_ID > *path_ID) {
                gain += (__MAP->get( not_in_path_ID, *path_ID ) == false);
            } else {
                gain += (__MAP->get( *path_ID, not_in_path_ID ) == false);
            }
        }
    }
    return gain;
}


//  ----------------------------------------------------
//      void PS_find_probe_for_sets( const PS_NodePtr      _ps_node,
//                                         PS_CandidatePtr _candidate_parent )
//  ----------------------------------------------------
//  scan subtree starting with _ps_node for candidates
//
void PS_find_probe_for_sets( const PS_NodePtr      _ps_node,
                                   PS_CandidatePtr _candidate_parent ) {
    SpeciesID id         = _ps_node->getNum();
    bool      has_probes = _ps_node->hasProbes();

    //
    // append ID to path
    //
    __PATH.insert( id );
    
    //
    // dont look at path until ID is greater than lowest ID in the sets of IDs
    // also dont use a node if its already used as candidate
    //
    if ((id >= __MIN_SETS_ID) && has_probes && !_candidate_parent->alreadyUsedNode(_ps_node)) {
        ++__PROBES_COUNTER;
        // make intersections of __PATH with __SOURCE_ID_SET and __TARGET_ID_SET
        IDSet matched_source_IDs;
        IDSet matched_target_IDs;
        set_intersection( __SOURCE_ID_SET.begin(), __SOURCE_ID_SET.end(),
                          __PATH.begin(), __PATH.end(),
                          inserter( matched_source_IDs, matched_source_IDs.begin() ) );
        set_intersection( __TARGET_ID_SET.begin(), __TARGET_ID_SET.end(),
                          __PATH.begin(), __PATH.end(),
                          inserter( matched_target_IDs, matched_target_IDs.begin() ) );
        long count_matched_source = matched_source_IDs.size();
        long count_matched_target = matched_target_IDs.size();
        if ((count_matched_source > __SOURCE_MIN_MATCH_COUNT) &&
            (count_matched_source < __SOURCE_MAX_MATCH_COUNT) &&
            (count_matched_target > __TARGET_MIN_MATCH_COUNT) &&
            (count_matched_target < __TARGET_MAX_MATCH_COUNT)) {
            if (!_candidate_parent->hasChild( _ps_node )) { // did i look at this probe yet ?
                float distance_to_perfect_match = fabs( __SOURCE_PERFECT_MATCH_COUNT - count_matched_source ) + fabs( __TARGET_PERFECT_MATCH_COUNT - count_matched_target );
                unsigned long gain = PS_test_candidate_on_bitmap();   // no -> calc gain and make new candidate node
                int status = (gain < __SPECIES_COUNT / 100) ? 0 : _candidate_parent->addChild( distance_to_perfect_match, gain, _ps_node, __PATH );
                if (status > 0) {
                    if (status == 2) {
//                         printf( " PS_find_probe_for_sets() : new candidate: count_matched_source (%li/%.0f) count_matched_target (%li/%.0f) distance (%4.0f) path (%i) gain (%li) node (%p)\n",
//                                 count_matched_source, __SOURCE_PERFECT_MATCH_COUNT,
//                                 count_matched_target, __TARGET_PERFECT_MATCH_COUNT,
//                                 distance_to_perfect_match,
//                                 __PATH.size(), gain, &(*_ps_node) );
                        ++__CANDIDATES_COUNTER;
                    } else {
//                         printf( " PS_find_probe_for_sets() : upd candidate: count_matched_source (%li/%.0f) count_matched_target (%li/%.0f) distance (%4.0f) path (%i) gain (%li) node (%p)\n",
//                                 count_matched_source, __SOURCE_PERFECT_MATCH_COUNT,
//                                 count_matched_target, __TARGET_PERFECT_MATCH_COUNT,
//                                 distance_to_perfect_match,
//                                 __PATH.size(), gain, &(*_ps_node) );
                    }
                    fflush( stdout );
                } // if status (of addChild)
            } // if !hasChild
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
            PS_find_probe_for_sets( i->second, _candidate_parent );
        }
    }

    //
    // remove ID from path
    //
    __PATH.erase( id );
}


//  ----------------------------------------------------
//      void PS_find_probes( const PS_NodePtr      _root_node,
//                           const int             _round,
//                                 PS_CandidatePtr _candidate_parent,
//                           const float           _filling_level )
//  ----------------------------------------------------
//  scan PS_Node-tree for candidates to raise filling level of __MAP
//
void PS_find_probes( const PS_NodePtr      _root_node,
                     const int             _round,
                           PS_CandidatePtr _candidate_parent,
                     const float           _filling_level ) {
    __PROBES_COUNTER             = 0;
    __CANDIDATES_COUNTER         = 0;
    __SOURCE_MIN_MATCH_COUNT     = (__SOURCE_ID_SET.size() * (__MIN_PERCENTAGE_SET_MATCH - (_round * 5))) / 100.0;
    __SOURCE_MAX_MATCH_COUNT     = (__SOURCE_ID_SET.size() * (__MAX_PERCENTAGE_SET_MATCH + (_round * 5))) / 100.0;
    __TARGET_MIN_MATCH_COUNT     = (__TARGET_ID_SET.size() * (__MIN_PERCENTAGE_SET_MATCH - (_round * 5))) / 100.0;
    __TARGET_MAX_MATCH_COUNT     = (__TARGET_ID_SET.size() * (__MAX_PERCENTAGE_SET_MATCH + (_round * 5))) / 100.0;
    __SOURCE_PERFECT_MATCH_COUNT = __SOURCE_ID_SET.size() / 2.0;
    __TARGET_PERFECT_MATCH_COUNT = __TARGET_ID_SET.size() / 2.0;
    __MIN_SETS_ID = (*__SOURCE_ID_SET.begin()  < *__TARGET_ID_SET.begin())  ? *__SOURCE_ID_SET.begin()  : *__TARGET_ID_SET.begin();
    __MAX_SETS_ID = (*__SOURCE_ID_SET.rbegin() < *__TARGET_ID_SET.rbegin()) ? *__SOURCE_ID_SET.rbegin() : *__TARGET_ID_SET.rbegin();
    printf( "PS_find_probes(%i) : source match %10.3f .. %10.3f   target match %10.3f .. %10.3f\n", _round, __SOURCE_MIN_MATCH_COUNT, __SOURCE_MAX_MATCH_COUNT, __TARGET_MIN_MATCH_COUNT, __TARGET_MAX_MATCH_COUNT ); fflush( stdout );
    int c = 0;
    struct tms before;
    times( &before );
    for ( PS_NodeMapConstIterator i = _root_node->getChildrenBegin();
          (i != _root_node->getChildrenEnd()) && (i->first < __MAX_SETS_ID);
          ++i,++c ) {
        //if (c % 100 == 0) printf( "PS_find_probe_for_sets( %i ) : %i/%i 1st level nodes -> %li/%li candidates of %li probes looked at\n", i->first, c+1, _root_node->countChildren(), __CANDIDATES_COUNTER, __CANDIDATES_TOTAL_COUNTER, __PROBES_TOTAL_COUNTER );
        __PATH.clear();
        PS_find_probe_for_sets( i->second, _candidate_parent );
    }
    printf( "PS_find_probes(%i) : %li candidates of %li probes visited  ", _round, __CANDIDATES_COUNTER, __PROBES_COUNTER );
    PS_print_time_diff( &before );
    // reduce candidates to best,worst and middle gain
    _candidate_parent->reduceChildren( _filling_level );
    for ( PS_CandidateMapCIter c = _candidate_parent->children.begin();
          c != _candidate_parent->children.end();
          ++c ) {
        printf( "PS_find_probes(%i) : candidate (%p) gain (%li) distance (%.3f)\n", _round, &(*c->second->node), c->first, c->second->distance );
        ++__CANDIDATES_TODO;
    }
    fflush( stdout );
}


//  ----------------------------------------------------
//      void PS_calc_next_speciesid_sets()
//  ----------------------------------------------------
//  scan __MAP to find sets of IDs that need differentiation most
//
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
    printf( "PS_calc_next_speciesid_sets() : SOURCE count 1's [%i..%i]  treshold (%.3f)", lowest_count, highest_count, treshold ); 

    // second pass -- get IDs where count is below or equal treshold
    __SOURCE_ID_SET.clear();
    for ( SpeciesID id = __MIN_ID;
          id <= __MAX_ID;
          ++id) {
        if (__MAP->getCountFor( id ) <= treshold) __SOURCE_ID_SET.insert( id );
    }
    PS_print_set_ranges( " __SOURCE_ID_SET", __SOURCE_ID_SET );

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
            if ((*target_id < __MIN_ID) || (*target_id > __MAX_ID)) continue; // skip ID's that are outside DB-IDs-range (like zero if __MIN_ID is one)
            if (*target_id != *source_id) ++count_falses_per_id[ *target_id ];
        }
    }
    //PS_print_map_ranges( "PS_calc_next_speciesid_sets() : count_falses_per_id", count_falses_per_id, false );

    // second -- get highest count of falses and calc treshold
    lowest_count  = __SPECIES_COUNT;
    highest_count = 0;
    for ( ID2IDMapCIter count_per_id = count_falses_per_id.begin();
          count_per_id != count_falses_per_id.end();
          ++count_per_id ) {
        if (count_per_id->second > highest_count) highest_count = count_per_id->second;
        if (count_per_id->second < lowest_count)  lowest_count  = count_per_id->second;
    }
    printf( "PS_calc_next_speciesid_sets() : TARGET count 0's [%i..%i]", lowest_count, highest_count ); 

    // third -- put all IDs in __TARGET_ID_SET that are needed most by species from __SOURCE_ID_SET
    __TARGET_ID_SET.clear();
    for ( ID2IDMapCIter count_per_id = count_falses_per_id.begin();
          count_per_id != count_falses_per_id.end();
          ++count_per_id ) {
        if (count_per_id->second == highest_count) __TARGET_ID_SET.insert( count_per_id->first );
    }
    PS_print_set_ranges( " __TARGET_ID_SET", __TARGET_ID_SET );
}


//  ----------------------------------------------------
//      void PS_apply_path_to_bitmap( IDSet &_path, bool silent )
//  ----------------------------------------------------
//  set true in __MAP for all combinations of IDs (in _path , not in _path)
//
void PS_apply_path_to_bitmap( IDSet &_path, bool silent = false ) {
    // iterate over all IDs except path
    IDSetCIter path_iter    = _path.begin();
    SpeciesID  next_path_ID = *path_iter;
    unsigned long int gain  = 0;
    for ( SpeciesID not_in_path_ID = __MIN_ID;
          not_in_path_ID <= __MAX_ID;
          ++not_in_path_ID) {
        if (not_in_path_ID == next_path_ID) {   // if i run into a ID in path
            ++path_iter;                        // advance to next path ID
            next_path_ID = (path_iter == _path.end()) ? -1 : *path_iter;
            continue;                           // skip this ID
        }
        // iterate over path IDs
        for ( IDSetCIter path_ID = _path.begin();
              path_ID != _path.end();
              ++path_ID) {
            if (not_in_path_ID == *path_ID) continue;   // obviously a probe cant differ a species from itself
            if (not_in_path_ID > *path_ID) {
                gain += (__MAP->set( not_in_path_ID, *path_ID, true ) == false);
            } else {
                gain += (__MAP->set( *path_ID, not_in_path_ID, true ) == false);
            }
        }
    }
    unsigned long int sets =  (__SPECIES_COUNT-_path.size())*_path.size();
    if (!silent) printf( "PS_apply_path_to_bitmap() : gain %lu of %lu -- %.2f%%  -> wasted %lu\n", gain, sets, ((float)gain/sets)*100.0, sets-gain );
    //__MAP->recalcCounters();
}


//  ----------------------------------------------------
//      float PS_filling_level()
//  ----------------------------------------------------
//  returns filling level of __MAP
//
float PS_filling_level() {
    unsigned long int all_bits   = ((__SPECIES_COUNT-1)*__SPECIES_COUNT)/2;
    unsigned long int trues      = __MAP->getCountOfTrues();
    float             percentage = ((float)trues / all_bits)*100.0;
    printf( "PS_filling_level() : bitmap now has %lu trues and %lu falses -- %.2f%% filled\n", trues, all_bits-trues, percentage );
    return percentage;
}


//  ----------------------------------------------------
//      void PS_GNUPlot( const char     *_out_prefix,
//                       const long      _iteration,
//                       const IDSet    &_path,
//                       const ID2IDSet &_noMatches )
//  ----------------------------------------------------
//  generate data- and commandfiles for GNUPlot to display __MAP and its count_trues_per_id
//
void PS_GNUPlot( const char     *_out_prefix,
                 const long      _iteration,
                 const IDSet    &_path,
                 const ID2IDSet &_noMatches ) {
    char *buffer = (char *)malloc( 4096 );
    // open data file
    sprintf( buffer, "%s.%06li", _out_prefix, _iteration );
    char *data_name = strdup( buffer );
    PS_FileBuffer *file = new PS_FileBuffer( data_name, PS_FileBuffer::WRITEONLY );
    // output data
    long size = sprintf( buffer, "# CANDIDATE #%li path (%i)\n", _iteration, _path.size() );
    file->put( buffer, size );
    for ( IDSetCIter i = _path.begin();
          i != _path.end();
          ++i ) {
        size = sprintf( buffer, "# %i\n", *i );
        file->put( buffer, size );
    }
    size = sprintf( buffer, "#noMatches (%i)\n", _noMatches.size() );
    file->put( buffer, size );
    for ( ID2IDSetCIter p = _noMatches.begin();
          p != _noMatches.end();
          ++p ) {
        size = sprintf( buffer, "%i %i\n", p->first, p->second );
        file->put( buffer, size );
    }
    size = sprintf( buffer, "\n\n" );
    file->put( buffer, size );
    sprintf( buffer, "Bitmap after %li. candidate", _iteration );
    char *title = strdup( buffer );
    __MAP->printGNUplot( title, buffer, file );
    // open gnuplot command file
    sprintf( buffer, "%s.%06li.gnuplot", _out_prefix, _iteration );
    file->reinit( buffer, PS_FileBuffer::WRITEONLY );
    // output gnuplot commands
    size = sprintf( buffer, "set terminal png color\nset output '%06li.bitmap.png\n", _iteration );
    file->put( buffer, size );
    size = sprintf( buffer, "set yrange [%i:%i] reverse\nset xrange [%i:%i]\n", __MIN_ID,__MAX_ID,__MIN_ID,__MAX_ID );
    file->put( buffer, size );
    size = sprintf( buffer, "set size square\nset title '%li. iteration'\nset pointsize 0.5\n", _iteration );
    file->put( buffer, size );
    size = sprintf( buffer, "plot '%s' index 1 title 'match ()' with dots 2,'%s' index 0 title 'no match (30)' with points 1\n", data_name, data_name );
    file->put( buffer, size );
    size = sprintf( buffer, "set terminal png color\nset output '%06li.counters.png\n", _iteration );
    file->put( buffer, size );
    size = sprintf( buffer, "set yrange [] noreverse\nset size nosquare\n" );
    file->put( buffer, size );
    size = sprintf( buffer, "plot '%s' index 2 title 'count/species' with impulses 2,'%s' index 3 title 'species/count' with points 1", data_name, data_name );
    file->put( buffer, size );
    
    delete file;
    free( title );
    free( data_name );
    free( buffer );
}


//  ----------------------------------------------------
//      PS_CandidatePtr PS_ascend( PS_CandidatePtr _last_candidate )
//  ----------------------------------------------------
//  'remove' _last_candidate from __MAP by applying its parent's
//  paths to a fresh __MAP
//
PS_CandidatePtr PS_ascend( PS_CandidatePtr _last_candidate ) {
    //
    // make fresh __MAP
    //
    __MAP->copy( __PRESET_MAP );
    //
    // apply paths of parent-candidates to __MAP
    //
    PS_CandidatePtr parent = _last_candidate->parent;
    while ((parent != 0) && (parent->path.size() > 0)) {
        PS_apply_path_to_bitmap( parent->path, true );
        parent = parent->parent;
    }
    printf( "ASCEND " );
    PS_filling_level();
    //
    // return the parent of the given candidate
    //
    return _last_candidate->parent;
}


//  ----------------------------------------------------
//      void PS_descend(       PS_CandidatePtr _candidate_parent,
//                       const PS_NodePtr      _root_node,
//                             unsigned long   _depth,
//                       const float           _filling_level )
//  ----------------------------------------------------
//
void PS_descend(       PS_CandidatePtr _candidate_parent,
                 const PS_NodePtr      _root_node,
                       unsigned long   _depth,
                 const float           _filling_level ) {
    struct tms before;
    times( &before );

    printf( "\nDESCEND ==================== depth (%lu / %lu) candidates (%lu / %lu -> %lu left) ====================\n",
            _depth,
            __MAX_DEPTH,
            __CANDIDATES_FINISHED,
            __CANDIDATES_TODO,
            __CANDIDATES_TODO-__CANDIDATES_FINISHED  ); fflush( stdout );
    printf( "DESCEND PATH : (%s)\n\n", __PATH_IN_CANDIDATES );

    //
    // calc source- and target-set of IDs
    //
    PS_calc_next_speciesid_sets();
    printf( "\nDESCEND ---------- calculated next speciesid-sets ----------\n\n" ); fflush( stdout );

    //
    // search for candidates to improve filling level of __MAP using the source- and target-set of IDs
    // max. 3 rounds
    //
    int round = 0;
    for (; (round < 3) && (_candidate_parent->children.size() == 0); ++round) {
        PS_find_probes( _root_node, round, _candidate_parent, _filling_level );
    }
    printf( "\nDESCEND ---------- searched probe for speciesid-sets [rounds : %i] candidates (%lu / %lu -> %lu left) ----------\n\n",
            round,
            __CANDIDATES_FINISHED,
            __CANDIDATES_TODO,
            __CANDIDATES_TODO-__CANDIDATES_FINISHED ); fflush( stdout );
    
    //
    // descend down each (of the max 3) candidate(s)
    //
    char count = '0'+_candidate_parent->children.size();
    for ( PS_CandidateMapRIter next_candidate_it = _candidate_parent->children.rbegin();
          next_candidate_it != _candidate_parent->children.rend();
          ++next_candidate_it, --count ) {
        PS_CandidatePtr next_candidate = &(*next_candidate_it->second);
        ++__CANDIDATES_FINISHED;
        //
        // apply candidate to __MAP
        //
        PS_apply_path_to_bitmap( next_candidate->path );
        next_candidate->filling_level = PS_filling_level();
        next_candidate->depth         = _depth+1;
        //
        // step down
        //
        if ((next_candidate->filling_level < 98.0) && (_depth+1 < __MAX_DEPTH)) {
            __PATH_IN_CANDIDATES[ _depth ] = count;
            __PATH_IN_CANDIDATES[ _depth+1 ] = '\x00';
            PS_descend( next_candidate, _root_node, _depth+1, next_candidate->filling_level );
        }
        //
        // remove candidate from __MAP
        //
        PS_ascend( next_candidate );
    }

    printf( "\nDESCEND ~~~~~~~~~~~~~~~~~~~~ depth (%li) max depth (%li) candidates (%lu / %lu -> %lu left) ~~~~~~~~~~~~~~~~~~~~ ",
            _depth,
            __MAX_DEPTH,
            __CANDIDATES_FINISHED,
            __CANDIDATES_TODO,
            __CANDIDATES_TODO-__CANDIDATES_FINISHED );
    PS_print_time_diff( &before );
    printf( "\n" ); fflush( stdout );
}


//  ----------------------------------------------------
//      void PS_search_best_candidate( PS_CandidatePtr  _candidate_parent,
//                                     PS_CandidatePtr &_best_candidate )
//  ----------------------------------------------------
//
void PS_search_best_candidate( PS_CandidatePtr  _candidate_parent,
                               PS_CandidatePtr &_best_candidate ) {
    for ( PS_CandidateMapRIter next_candidate_it = _candidate_parent->children.rbegin();
          next_candidate_it != _candidate_parent->children.rend();
          ++next_candidate_it ) {
        PS_CandidatePtr next_candidate = &(*next_candidate_it->second);
        if (next_candidate->filling_level > _best_candidate->filling_level) {
            _best_candidate = next_candidate;
        }
        PS_search_best_candidate( next_candidate, _best_candidate );
    }
}


//  ====================================================
//  ====================================================
int main( int   argc,
          char *argv[] ) {
    //
    // check arguments
    //
    if (argc < 3) {
        printf( "Missing argument\n Usage %s <database name> <result filename> [result filename prefix for output files]\n", argv[0] );
        exit( 1 );
    }

    const char *input_DB_name      = argv[1];
    const char *result_in_filename = argv[2];
    const char *result_out_prefix  = (argc > 3) ? argv[3] : 0;

    //
    // open probe-set-database
    //
    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
    __MIN_ID = db->getMinID();
    __SPECIES_COUNT = db->getSpeciesCount();
    printf( "min ID (%i)  max ID (%i)  count of species (%li)\n", __MIN_ID, __MAX_ID, __SPECIES_COUNT );
    printf( "..loaded database (enter to continue)\n" ); fflush( stdout );

    //
    // open result file for preset bitmap
    //
    printf( "Opening result file %s..\n", result_in_filename );
    PS_FileBuffer *result_file = new PS_FileBuffer( result_in_filename, PS_FileBuffer::READONLY );
    long size;
    ID2IDSet noMatches;
    SpeciesID id1,id2;
    printf( "reading no matches : " );
    result_file->get_long( size );
    printf( "(%li)\n", size ); fflush( stdout );
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
    printf( "(%li)\n", size ); fflush( stdout );
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
    printf( "loading preset bitmap..\n" ); fflush( stdout );
    __MAP        = new PS_BitMap_Counted( result_file );
    __PRESET_MAP = new PS_BitMap_Counted( false, __MAX_ID+1 );
    __PRESET_MAP->copy( __MAP );
    printf( "..loaded result file (enter to continue)\n" );
    printf( "cleaning up... result file\n" ); fflush( stdout );
    delete result_file;

    //
    // recursively build a tree of candidates
    //
    __MAX_DEPTH = 0;
    for ( long species_count = __SPECIES_COUNT; species_count > 0; species_count >>= 1 ) {
        ++__MAX_DEPTH;
    }
    __MAX_DEPTH = (__MAX_DEPTH * 3) >> 1;
    __PATH_IN_CANDIDATES = (char *)calloc( __MAX_DEPTH+1, sizeof(char) );
    __CANDIDATES_TODO = 0;
    __CANDIDATES_FINISHED = 0;
    __CANDIDATES_ROOT = new PS_Candidate( 0.0 );
    PS_descend( __CANDIDATES_ROOT, db->getConstRootNode(), 0, 0.0 );

    printf( "CANDIDATES :\n" );
    __CANDIDATES_ROOT->print();
    printf( "\n" );

    //
    // scan candidates-tree for best candidate
    //
    PS_CandidatePtr best_candidate = __CANDIDATES_ROOT;
    printf( "searching best candidate..\n" );
    PS_search_best_candidate( __CANDIDATES_ROOT, best_candidate );
    printf( "best candidate : depth (%lu) filling level (%.2f%%)\n", best_candidate->depth, best_candidate->filling_level ); fflush( stdout );

    //
    // starting with the best candidate print the __MAP for each
    // depth walking up to root of candidates-tree
    //
    if (result_out_prefix) {
        // put __MAP in the state 'after applying best_candidate and its parents'
        PS_ascend( best_candidate );
        PS_apply_path_to_bitmap( best_candidate->path );
        while (best_candidate) {
            PS_GNUPlot( result_out_prefix, __MAX_DEPTH--, best_candidate->path, noMatches );
            best_candidate = PS_ascend( best_candidate );
        }
    }

    //
    // clean up
    //
    free( __PATH_IN_CANDIDATES );
    printf( "cleaning up... database\n" ); fflush( stdout );
    delete db;
    printf( "cleaning up... bitmaps\n" ); fflush( stdout );    
    delete __PRESET_MAP;
    delete __MAP;

    return 0;
}
