#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/times.h>

#include "ps_tools.hxx"
#include "ps_database.hxx"
#include "ps_candidate.hxx"


//  ----------------------------------------------------
//      void PS_get_leaf_candidates( PS_CandidatePtr  _candidate_parent,
//                                   PS_CandidateSet &_leaf_candidates )
//  ----------------------------------------------------
//
void PS_get_leaf_candidates( PS_CandidatePtr  _candidate_parent,
                             PS_CandidateSet &_leaf_candidates ) {

    for ( PS_CandidateByGainMapRIter next_candidate = _candidate_parent->children.rbegin();
          next_candidate != _candidate_parent->children.rend();
          ++next_candidate ) {

        if (next_candidate->second->children.size() == 0){
            _leaf_candidates.insert( next_candidate->second );
        }
        PS_get_leaf_candidates( &(*next_candidate->second), _leaf_candidates );
    }
}


//  ----------------------------------------------------
//      void PS_get_node_paths( PS_CandidateSet                     &_leaf_candidates,
//                              PS_Candidate2NodeSetPairByLengthMap &_paths )
//  ----------------------------------------------------
typedef set<PS_Node *>                                   PS_NodeSet;
typedef pair<PS_CandidatePtr,PS_NodeSet>                 PS_Candidate2NodeSetPair;
typedef multimap<unsigned long,PS_Candidate2NodeSetPair> PS_Candidate2NodeSetPairByLengthMap;
void PS_get_node_paths( PS_CandidateSet                     &_leaf_candidates,
                        PS_Candidate2NodeSetPairByLengthMap &_paths ) {
    PS_NodeSet nodepath;
    for ( PS_CandidateSetCIter candidate_iter = _leaf_candidates.begin();
          candidate_iter != _leaf_candidates.end();
          ++candidate_iter ) {
        // get nodepath of candidate
        PS_CandidateSPtr candidate_sptr = *candidate_iter;
        PS_CandidatePtr  candidate      = &(*candidate_sptr);
        nodepath.clear();
        while ((candidate) &&
               (!candidate->node.Null())) {
            nodepath.insert( &(*(candidate->node)) );
            candidate = candidate->parent;
        }
        // compare nodepath of candidate with stored paths
        PS_Candidate2NodeSetPairByLengthMap::iterator stored_path;
        bool found = false;
        for ( stored_path = _paths.find( candidate->depth );    // find first path with path length == depth
              ((stored_path != _paths.end()) &&                 // while not at end of paths
               (stored_path->first == candidate->depth) &&      //   and path length == depth
               (!found));                                       //   and not found yet
              ++stored_path ) {
            found = stored_path->second.second == nodepath;
        }
        if (!found) {
            // not found -> store nodepath
            candidate_sptr = *candidate_iter;
            candidate      = &(*candidate_sptr);
            _paths.insert( pair<unsigned long,PS_Candidate2NodeSetPair>( candidate->depth,PS_Candidate2NodeSetPair( candidate,nodepath ) ) );
        }
    }
}


//  ----------------------------------------------------
//      void PS_eval_node_paths( PS_Candidate2NodeSetPairByLengthMap &_paths )
//  ----------------------------------------------------
typedef set<unsigned long>  ULSet;
typedef set<unsigned short> USSet;
typedef map<float,float>    FFMap;
typedef set<float>          FSet;
inline unsigned long PS_calc_temp( const PS_ProbePtr &_probe ) {
    return ( 4*(_probe->length - _probe->GC_content) + 2*_probe->GC_content );
}
void PS_calc_sums_for_nodepath( PS_NodeSet &_nodepath,
                                ULSet      &_sums ) {
    // iterate over nodes in path
    ULSet sums_for_node;
    ULSet temperatures;

    for ( PS_NodeSet::const_iterator node = _nodepath.begin();
          node != _nodepath.end();
          ++node ) {

        // iterate over probes of node
        sums_for_node.clear();
        temperatures.clear();

        for ( PS_ProbeSetCIter probe_iter = (*node)->getProbesBegin();
              probe_iter != (*node)->getProbesEnd();
              ++probe_iter ) {
            PS_ProbePtr probe = *probe_iter;

            // test if already calculated sums for temperature of probe
            unsigned long temperature = PS_calc_temp( probe );
            ULSet::iterator found = temperatures.find( temperature );
            if (found != temperatures.end()) continue;

            // calc sums
            temperatures.insert( temperature );
//             printf( "node [%p] temp (%lu) sums ( ", *node, temperature );

            for ( ULSet::iterator sum = _sums.begin();
                  sum != _sums.end();
                  ++sum ) {
                sums_for_node.insert( *sum + temperature );
            }

//             for ( ULSet::iterator sum = sums_for_node.begin();
//                   sum != sums_for_node.end();
//                   ++sum ) {
//                 printf( "%lu ", *sum );
//             }
//             printf( ")\n");
        }

        // store sums_for_node in _sums
        _sums = sums_for_node;
    }
}
bool PS_calc_min_sum_of_square_distances_to_average( PS_NodeSet &_nodepath,
                                                     FFMap      &_averages,
                                                     float      &_best_average,
                                                     float      &_min_sum ) {
    // iterate over nodes in path
    ULSet temperatures;
    FSet  sums_for_average;
    float min_sum_for_node = -2.0;
    float best_average     = -2.0;

    for ( PS_NodeSet::const_iterator node = _nodepath.begin();
          (node != _nodepath.end()) &&
          ((_min_sum < 0) ||
           (_min_sum > min_sum_for_node));
          ++node ) {

        // iterate over averages
        for ( FFMap::iterator average = _averages.begin();
              average != _averages.end();
              ++average ) {
            if ((_min_sum > 0) && (average->second > _min_sum)) continue;

            // iterate over probes of node
//             printf( "\nnode [%p] avg (%.3f,%.3f)", *node, average->first, average->second );
            sums_for_average.clear();
            temperatures.clear();

            for ( PS_ProbeSetCIter probe_iter = (*node)->getProbesBegin();
                  probe_iter != (*node)->getProbesEnd();
                  ++probe_iter ) {
                PS_ProbePtr probe = *probe_iter;

                // test if already calculated sum for GC-content of probe
                unsigned long temperature = PS_calc_temp( probe );
                ULSet::iterator found = temperatures.find( temperature );
                if (found != temperatures.end()) continue;

                // calc sum
                temperatures.insert( temperature );
                float distance = fabsf( average->first - temperature );
                float sum      = average->second + ( distance * distance );
                sums_for_average.insert( sum );
//                 printf( "  temp (%lu) sum (%.3f)",  temperature, sum );
            }

            // store min sum
            average->second  = *sums_for_average.begin();
//             printf( "  -> %.3f", average->second );
        }

        // update min sum for node
        min_sum_for_node = _averages.begin()->second;
        best_average     = _averages.begin()->first;
        for ( FFMap::iterator average = _averages.begin();
              average != _averages.end();
              ++average ) {
            if (average->second < min_sum_for_node) {
                min_sum_for_node = average->second;
                best_average     = average->first;
            }
        }
//         printf( " => min_sum_for_node (%.3f) @ average (%.3f)", min_sum_for_node, best_average );
    }
//     printf( "\n" );

    if ((_min_sum < 0) || (min_sum_for_node < _min_sum)) {
        printf( "UPDATED _best_average (%7.3f) _min_sum (%10.3f)  <-  best_average (%7.3f) min_sum_for_node (%10.3f)\n", _best_average, _min_sum, best_average, min_sum_for_node );
        _min_sum      = min_sum_for_node;
        _best_average = best_average;
        return true;
    } else {
        printf( "aborted _best_average (%7.3f) _min_sum (%10.3f)      best_average (%7.3f) min_sum_for_node (%10.3f)\n", _best_average, _min_sum, best_average, min_sum_for_node );
        return false;
    }
}
void PS_eval_node_paths( PS_Candidate2NodeSetPairByLengthMap &_paths,
                         float                               &_min_sum_of_square_distances_to_average,
                         float                               &_best_average ) {
    //
    // initially calc min square distance for one candidate
    //
    PS_Candidate2NodeSetPairByLengthMap::iterator stored_path = _paths.begin();
    // calc sums of GC-contents of probes for nodes in candidate's path
    ULSet sums;
    sums.insert( 0 );
    PS_calc_sums_for_nodepath( stored_path->second.second, sums );
    // calc average GC-contents
//     printf( "averages : ( " );
    FFMap averages;
    for ( ULSet::iterator sum = sums.begin();
          sum != sums.end();
          ++sum ) {
        float avg = (float)(*sum) / stored_path->first;
        averages[ avg ] = 0.0;
//         printf( "%7.3f,  0.000 ", avg );
    }
//         printf( ")\n" );
    // search minimum of sum of square distance to average
    _min_sum_of_square_distances_to_average = -1;
    printf( "[%p] distance: ", stored_path->second.first );
    PS_calc_min_sum_of_square_distances_to_average( stored_path->second.second,
                                                    averages,
                                                    _best_average,
                                                    _min_sum_of_square_distances_to_average );
//     printf( "averages : ( " );
//     for (FFMap::iterator avg = averages.begin();
//          avg != averages.end();
//          ++avg ) {
//         printf( "%7.3f,%7.3f ", avg->first, avg->second );
//     }
//     printf( ")\n\n" );

    // iterate over remaining candidates
    PS_Candidate2NodeSetPairByLengthMap::iterator best_candidate = stored_path;
    for ( ++stored_path;
          stored_path != _paths.end();
          ++stored_path ) {

        // calc sums
        sums.clear();
        sums.insert( 0 );
        PS_calc_sums_for_nodepath( stored_path->second.second, sums );

        // calc averages
        averages.clear();
//         printf( "averages : ( " );
        for ( ULSet::iterator sum = sums.begin();
              sum != sums.end();
              ++sum ) {
            float avg = (float)(*sum) / stored_path->first;
            averages[ avg ] = 0.0;
//             printf( "%7.3f,  0.000 ", avg );
        }
//         printf( ")\n" );

        // search min distance
        printf( "[%p] distance: ", stored_path->second.first );
        if (PS_calc_min_sum_of_square_distances_to_average( stored_path->second.second,
                                                            averages,
                                                            _best_average,
                                                            _min_sum_of_square_distances_to_average )) {
            best_candidate = stored_path;
        }
//         printf( "averages : ( " );
//         for (FFMap::iterator avg = averages.begin();
//              avg != averages.end();
//              ++avg ) {
//             printf( "%7.3f,%7.3f ", avg->first, avg->second );
//         }
//         printf( ")\n\n" );
    }

    // remove all but best candidate
    PS_Candidate2NodeSetPairByLengthMap::iterator to_delete = _paths.end();
    for ( stored_path  = _paths.begin();
          stored_path != _paths.end();
          ++stored_path ) {
        if (to_delete != _paths.end()) {
            _paths.erase( to_delete );
            to_delete = _paths.end();
        }
        if (stored_path != best_candidate) to_delete = stored_path;
    }
    if (to_delete != _paths.end()) {
        _paths.erase( to_delete );
        to_delete = _paths.end();
    }
}


//  ----------------------------------------------------
//      void PS_remove_bad_probes( PS_NodeSet        &_nodes,
//                                 float              _average,
//                                 set<unsigned int> &_probe_lengths )
//  ----------------------------------------------------
void PS_remove_bad_probes( PS_NodeSet        &_nodes,
                           float              _average,
                           set<unsigned int> &_probe_lengths ) {
    for ( PS_NodeSet::const_iterator node_iter = _nodes.begin();
          node_iter != _nodes.end();
          ++node_iter ) {
        PS_Node *node = *node_iter;

        // calc min distance
        float min_distance = _average * _average;
        for ( PS_ProbeSetCIter probe = node->getProbesBegin();
              probe != node->getProbesEnd();
              ++probe ) {
            float distance = fabsf( _average -  PS_calc_temp( *probe ) );
            if (distance < min_distance) min_distance = distance;
        }

        // remove probes with distance > min_distance
//         printf( "average (%.3f) min_distance (%.3f)\n", _average, min_distance );
        PS_ProbeSetCIter to_delete = node->getProbesEnd();
        for ( PS_ProbeSetCIter probe = node->getProbesBegin();
              probe != node->getProbesEnd();
              ++probe ) {
            if (to_delete != node->getProbesEnd()) {
                node->removeProbe( to_delete );
                to_delete = node->getProbesEnd();
            }
            float distance = fabsf( _average -  PS_calc_temp( *probe ) );
            if (distance > min_distance) {
//                 printf( "node [%p] probes (%u) delete probe l(%u) gc(%u) temp(%lu) distance(%.3f)\n",
//                         node,
//                         node->countProbes(),
//                         (*probe)->length,
//                         (*probe)->GC_content,
//                         PS_calc_temp( *probe ),
//                         distance );
//                 fflush( stdout );
                to_delete = probe;
            }
        }
        if (to_delete != node->getProbesEnd()) {
            node->removeProbe( to_delete );
            to_delete = node->getProbesEnd();
        }

        // select probe with greatest length
        if (node->countProbes() > 1) {

            // get greatest length
            unsigned max_length = 0;
            for ( PS_ProbeSetCIter probe = node->getProbesBegin();
              probe != node->getProbesEnd();
              ++probe ) {
                if (max_length < (*probe)->length) max_length = (*probe)->length;
            }

            // remove probes with length < max_length
//             printf( "max_length (%u)\n", max_length );
            for ( PS_ProbeSetCIter probe = node->getProbesBegin();
                  probe != node->getProbesEnd();
                  ++probe ) {
                if (to_delete != node->getProbesEnd()) {
                    node->removeProbe( to_delete );
                    to_delete = node->getProbesEnd();
                }
                if ((*probe)->length < max_length) {
//                     printf( "node [%p] probes (%u) delete probe l(%u)\n",
//                         node,
//                         node->countProbes(),
//                         (*probe)->length );
//                     fflush( stdout );
                    to_delete = probe;
                }
            }
            if (to_delete != node->getProbesEnd()) {
                node->removeProbe( to_delete );
                to_delete = node->getProbesEnd();
            }
        }
        _probe_lengths.insert( (*node->getProbesBegin())->length );
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
        printf( "Missing argument\n Usage %s <database name> <final candidates filename>\n", argv[0] );
        exit( 1 );
    }

    const char *input_DB_name             = argv[1];
    const char *final_candidates_filename = argv[2];

    //
    // open probe-set-database
    //
    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    SpeciesID     max_id      = db->getMaxID();
    unsigned long bits_in_map = ((max_id+1)*max_id)/2 + max_id+1;
    printf( "..loaded database (enter to continue)\n" ); fflush( stdout );

    //
    // candidates
    //
    printf( "opening candidates-file '%s'..\n", final_candidates_filename );
    PS_Candidate  *candidates_root = new PS_Candidate( 0.0 );
    PS_FileBuffer *candidates_file = new PS_FileBuffer( final_candidates_filename, PS_FileBuffer::READONLY );
    candidates_root->load( candidates_file, bits_in_map, db->getConstRootNode() );
    delete candidates_file;
    printf( "..loaded candidates file (enter to continue)\n" ); fflush( stdout );

//     printf( "probes per candidate..\n" );
//     candidates_root->printProbes( max_id - db->getMinID() + 1 );

    //
    // scan candidates-tree for leaf candidates
    //
    printf( "\nsearching leaf candidates.. " );
    PS_CandidateSet leaf_candidates;
    PS_get_leaf_candidates( candidates_root, leaf_candidates );
    printf( "%i\n", leaf_candidates.size() );

    //
    // scan leaf-candidates for non-permutated node-paths
    //
    printf( "\ngetting node paths for leaf candidates.." );
    PS_Candidate2NodeSetPairByLengthMap node_paths;
    PS_get_node_paths( leaf_candidates, node_paths );
    printf( "%i\n", node_paths.size() );
    for ( PS_Candidate2NodeSetPairByLengthMap::const_iterator stored_path = node_paths.begin();
          stored_path != node_paths.end();
          ++stored_path ) {
        printf( "length (%3lu)  candidate [%p]  nodes (%3u) ",
                stored_path->first,
                stored_path->second.first,
                stored_path->second.second.size() );
        unsigned long sum = 0;
        for ( PS_NodeSet::const_iterator node = stored_path->second.second.begin();
              node != stored_path->second.second.end();
              ++node ) {
            sum += (*node)->countProbes();
        }
        printf( "probes (%6lu)\n", sum ); fflush( stdout );
    }

    //
    // eval node paths
    //
    printf( "\nevaluating node paths of leaf candidates..\n" );
    float min_sum_of_square_distances_to_average;
    float average;
    PS_eval_node_paths( node_paths, min_sum_of_square_distances_to_average, average  );
    printf( "   best candidate : average (%f)  sum of square distances to average (%f)\n   ", average, min_sum_of_square_distances_to_average );
    node_paths.begin()->second.first->print();

    //
    // remove probes with unwanted distance or length
    //
    printf( "\nremoving unwanted probes from best candidate..\n" );
    set<unsigned int> probe_lengths;
    PS_remove_bad_probes( node_paths.begin()->second.second, average, probe_lengths );

    //
    // write out paths to probes
    //
    char *final_candidates_paths_filename = (char *)malloc( strlen(final_candidates_filename)+1+6 );
    strcpy( final_candidates_paths_filename, final_candidates_filename );
    strcat( final_candidates_paths_filename, ".paths" );
    printf( "Writing final candidate's paths to '%s'..\n", final_candidates_paths_filename );
    PS_FileBuffer *final_candidates_paths_file = new PS_FileBuffer( final_candidates_paths_filename, PS_FileBuffer::WRITEONLY );
    PS_CandidatePtr c = node_paths.begin()->second.first;
    unsigned long min_temp = PS_calc_temp( *c->node->getProbesBegin() );
    unsigned long max_temp = min_temp;
    // write count of paths
    final_candidates_paths_file->put_ulong( c->depth );
    // write used probe lengths (informal)
    final_candidates_paths_file->put_uint( probe_lengths.size() );
    printf( "   probe lengths :" );
    for ( set<unsigned int>::iterator length = probe_lengths.begin();
          length != probe_lengths.end();
          ++length ) {
        final_candidates_paths_file->put_uint( *length );
        printf( " %u", *length );
    }
    printf( "\n" );
    // write paths
    while (c && !c->node.Null()) {
        PS_ProbeSetCIter probe = c->node->getProbesBegin();
        unsigned long temp = PS_calc_temp( *probe );
        if (temp < min_temp) min_temp = temp;
        if (temp > max_temp) max_temp = temp;
        printf( "depth (%lu) candidate [%p] node [%p] probe (q_%+i__l_%2u__gc_%2u__temp_%3lu)\n",
                c->depth,
                c,
                &(*(c->node)),
                (*probe)->quality,
                (*probe)->length,
                (*probe)->GC_content,
                temp );
        // write probe length
        final_candidates_paths_file->put_uint( (*probe)->length );
        // write probe GC-content
        final_candidates_paths_file->put_uint( (*probe)->GC_content );

        if ((*probe)->quality >= 0) {
            // write path length
            final_candidates_paths_file->put_uint( c->path.size() );
            // write path
            for ( IDSetCIter id = c->path.begin();
                  id != c->path.end();
                  ++id ) {
                final_candidates_paths_file->put_int( *id );
            }
        } else {
            // write inverse path length
            final_candidates_paths_file->put_uint( db->getSpeciesCount() - c->path.size() );
            // write inverse path
            IDSetCIter path_it = c->path.begin();
            SpeciesID  path_id = *path_it;
            for ( SpeciesID id = db->getMinID();
                  id <= db->getMaxID();
                  ++id ) {
                if (id == path_id) {
                    ++path_it;
                    path_id = (path_it == c->path.end()) ? -1 : *path_it;
                    continue;
                }
                final_candidates_paths_file->put_int( id );
            }
        }
        // write dummy probe data
        for ( unsigned int i = 0; i < (*probe)->length; ++i ) {
            final_candidates_paths_file->put_char( '\x00' );
        }
        c = c->parent;
    }
    // write probe lengths again
    // this set is used store remaining 'todo probe-lengths'
    final_candidates_paths_file->put_uint( probe_lengths.size() );
    for ( set<unsigned int>::iterator length = probe_lengths.begin();
          length != probe_lengths.end();
          ++length ) {
        final_candidates_paths_file->put_uint( *length );
    }

    printf( "   temperature range %lu..%lu\n", min_temp, max_temp );
    free( final_candidates_paths_filename );
    delete final_candidates_paths_file;

    //
    // clean up
    //
    printf( "cleaning up... candidates\n" ); fflush( stdout );
    delete candidates_root;
    printf( "cleaning up... database\n" ); fflush( stdout );
    delete db;

    return 0;
}
