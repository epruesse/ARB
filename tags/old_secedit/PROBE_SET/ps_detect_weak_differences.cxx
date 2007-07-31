#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif
#include "ps_tools.hxx"

// common globals
SpeciesID          __MAX_ID;
SpeciesID          __MIN_ID;
PS_BitMap_Fast    *__MAP;
// globals for PS_detect_weak_differences
IDVector          *__PATH;
IDVector          *__INVERSE_PATH;
unsigned long int  __COUNT_SET_OPS  = 0;
unsigned long int  __COUNT_SET_OPS2 = 0;
char              *__NODES_LEFT;
// globals for PS_print_and_evaluate_map
IDSet             *__PATHSET;
IDID2IDSetMap     *__PAIR2PATH;
SpeciesID          __ONEMATCH_MIN_ID;
SpeciesID          __ONEMATCH_MAX_ID;

//  ----------------------------------------------------
//      void PS_print_path()
//  ----------------------------------------------------
void PS_print_path() {
    printf( "__PATH %3i :",__PATH->size() );
    int c = 1;
    for (IDVectorCIter i = __PATH->begin(); i != __PATH->end(); ++i,++c) {
        if (c % 20 == 0) printf( "\n" );
        printf( " %3i", *i );
    }
    printf( "\n" );
}


//  ----------------------------------------------------
//      void PS_inverse_print_path()
//  ----------------------------------------------------
void PS_print_inverse_path() {
    printf( "__INVERSE_PATH %3i :",__INVERSE_PATH->size() );
    int c = 1;
    for (IDVectorCIter i = __INVERSE_PATH->begin(); i != __INVERSE_PATH->end(); ++i,++c) {
        if (c % 20 == 0) printf( "\n" );
        printf( " %3i", *i );
    }
    printf( "\n" );
}


//  ----------------------------------------------------
//      void PS_detect_weak_differences( const PS_NodePtr _root_node )
//  ----------------------------------------------------
//  Recursively walk through tree and make a bool-matrix of SpeciesID's
//  where true means that the 2 species can be distinguished by a probe.
//
//  The first occurence of a pair of distinguishable IDs is stored as (smaller_ID,bigger_ID).
//  The following ocurences of this pair are stored as (bigger_ID,smaller_ID).
//  (this allows us to find pairs of SpeciesIDs that can be distinguished by exactly one probe)
//
void PS_detect_weak_differences_stepdown( const PS_NodePtr _ps_node,
                                          const SpeciesID  _parent_ID,
                                          const long       _depth ) {

    SpeciesID id = _ps_node->getNum();
    if (_depth < 60) {
        printf( "%s", __NODES_LEFT );
        for (int i = 0; i < 60; ++i) printf( "\b" );
        fflush( stdout );
    }
    //
    // append IDs to paths
    //
    __PATH->push_back( id );                                                                    // append id to path
    for (SpeciesID i = (_parent_ID < __MIN_ID) ? __MIN_ID : _parent_ID+1; i < id; ++i) {        // append parent_id+1 .. id-1 to inverse path
        //printf( "%i ",i );
        __INVERSE_PATH->push_back( i );
    }

    //
    // set values in the maps if node has probes
    //
    if (_ps_node->hasProbes()) {
        if (_ps_node->hasPositiveProbes() && _ps_node->hasInverseProbes()) {
//            PS_print_path();
//            PS_print_inverse_path();
            unsigned long int set_ops = 2*__PATH->size()*(__MAX_ID-id-1+__INVERSE_PATH->size());
            if (ULONG_MAX - __COUNT_SET_OPS < set_ops) {
                set_ops = set_ops - (ULONG_MAX - __COUNT_SET_OPS);
                __COUNT_SET_OPS = 0;
                ++__COUNT_SET_OPS2;
            }
            __COUNT_SET_OPS = __COUNT_SET_OPS + set_ops;

            SpeciesID inverse_path_ID;
            // path loop
            for (IDVectorCIter it_path = __PATH->begin();
                 it_path != __PATH->end();
                 ++it_path) {
                SpeciesID path_ID = *it_path;
                // inverse path loop (explicit)
                for (IDVectorCIter it_inverse_path = __INVERSE_PATH->begin();
                     it_inverse_path != __INVERSE_PATH->end();
                     ++it_inverse_path ) {
                    inverse_path_ID = *it_inverse_path;

                    __MAP->setTrue( path_ID, inverse_path_ID );
                    __MAP->setTrue( inverse_path_ID, path_ID );
                }

                // inverse path loop (implicit)
                for (inverse_path_ID = id+1; inverse_path_ID < __MAX_ID; ++inverse_path_ID) { // skip to id ABOVE current node id
                    __MAP->setTrue( path_ID, inverse_path_ID );
                    __MAP->setTrue( inverse_path_ID, path_ID );
                }
            }
        } else {
//            PS_print_path();
//            PS_print_inverse_path();
            unsigned long int set_ops = __PATH->size()*(__MAX_ID-id-1+__INVERSE_PATH->size());
            if (ULONG_MAX - __COUNT_SET_OPS < set_ops) {
                set_ops = set_ops - (ULONG_MAX - __COUNT_SET_OPS);
                __COUNT_SET_OPS = 0;
                ++__COUNT_SET_OPS2;
            }
            __COUNT_SET_OPS = __COUNT_SET_OPS + set_ops;

            SpeciesID inverse_path_ID;
            SpeciesID smaller_ID;
            SpeciesID bigger_ID;
            // path loop
            for (IDVectorCIter it_path = __PATH->begin();
                 it_path != __PATH->end();
                 ++it_path) {
                SpeciesID path_ID = *it_path;
                // inverse path loop (explicit)
                for (IDVectorCIter it_inverse_path = __INVERSE_PATH->begin();
                     it_inverse_path != __INVERSE_PATH->end();
                     ++it_inverse_path ) {
                    inverse_path_ID = *it_inverse_path;
                    smaller_ID = (path_ID < inverse_path_ID) ? path_ID : inverse_path_ID;
                    bigger_ID  = (path_ID > inverse_path_ID) ? path_ID : inverse_path_ID;

                    if (__MAP->get( smaller_ID, bigger_ID )) {
                        __MAP->setTrue( bigger_ID, smaller_ID );
                    } else {
                        __MAP->setTrue( smaller_ID, bigger_ID );
                    }
                }

                // inverse path loop (implicit)
                for (inverse_path_ID = id+1; inverse_path_ID < __MAX_ID; ++inverse_path_ID) { // skip to id ABOVE current node id
                    smaller_ID = (path_ID < inverse_path_ID) ? path_ID : inverse_path_ID;
                    bigger_ID  = (path_ID > inverse_path_ID) ? path_ID : inverse_path_ID;

                    if (__MAP->get( smaller_ID, bigger_ID )) {
                        __MAP->setTrue( bigger_ID, smaller_ID );
                    } else {
                        __MAP->setTrue( smaller_ID, bigger_ID );
                    }
                }
            }
        }
    }
    //
    // step down the children
    //
    int c = _ps_node->countChildren()-1;
    for ( PS_NodeMapConstIterator i = _ps_node->getChildrenBegin();
          i != _ps_node->getChildrenEnd();
          ++i,--c ) {
        if (_depth < 60) {
            if (c < 10) {
                __NODES_LEFT[ _depth ] = '0'+c;
            } else {
                __NODES_LEFT[ _depth ] = '+';
            }
        }
        PS_detect_weak_differences_stepdown( i->second, id, _depth+1 );
    }
    if (_depth < 60) __NODES_LEFT[ _depth ] = ' ';

    //
    // remove IDs from paths
    //
    __PATH->pop_back();
    while ((__INVERSE_PATH->back() > _parent_ID) && (!__INVERSE_PATH->empty())) {
        __INVERSE_PATH->pop_back();
    }
}

void PS_detect_weak_differences( const PS_NodePtr _root_node ) {
    //
    // make bitmap
    //
    __PATH         = new IDVector;
    __INVERSE_PATH = new IDVector;

    int c = 0;
    struct tms before;
    times( &before );
    struct tms before_first_level_node;
    for (PS_NodeMapConstIterator i = _root_node->getChildrenBegin(); i != _root_node->getChildrenEnd(); ++i,++c ) {
        if (_root_node->countChildren()-c-1 < 10) {
            __NODES_LEFT[0] = '0'+_root_node->countChildren()-c-1;
        } else {
            __NODES_LEFT[0] = '+';
        }
        if ((c < 50) || (c % 100 == 0)) {
            times( &before_first_level_node );
            printf( "PS_detect_weak_differences_stepdown( %i ) : %i. of %i  ", i->first, c+1, _root_node->countChildren() ); fflush( stdout );
        }
        PS_detect_weak_differences_stepdown( i->second, -1, 1 );
        if ((c < 50) || (c % 100 == 0)) {
            PS_print_time_diff( &before_first_level_node, "this node ", "  " );
            PS_print_time_diff( &before, "total ", "\n" );
        }
    }
    printf( "%lu * %lu + %lu set operations performed\n", __COUNT_SET_OPS2, ULONG_MAX, __COUNT_SET_OPS ); fflush( stdout );

    delete __PATH;
    delete __INVERSE_PATH;
}

//  ----------------------------------------------------
//      void PS_print_and_evaluate_map( const PS_NodePtr _root_node )
//  ----------------------------------------------------
typedef map<ID2IDPair,PS_NodePtr>    IDID2NodeMap;
typedef IDID2NodeMap::iterator       IDID2NodeMapIter;
typedef IDID2NodeMap::const_iterator IDID2NodeMapCIter;
void PS_find_probes_for_pairs( const PS_NodePtr  _ps_node, 
                                       ID2IDSet &_pairs ) {
    SpeciesID id         = _ps_node->getNum();
    bool      has_probes = _ps_node->hasProbes();

    //
    // append ID to path
    //
    __PATHSET->insert( id );

    //
    // dont look at path until ID is greater than lowest ID in the set of ID-pairs
    //
    if ((id >= __ONEMATCH_MIN_ID) && has_probes) {
        for (ID2IDSetCIter pair=_pairs.begin(); pair != _pairs.end(); ++pair) {
            // look for pair-IDs in the path
            bool found_first  = __PATHSET->find( pair->first  ) != __PATHSET->end();
            bool found_second = __PATHSET->find( pair->second ) != __PATHSET->end();
            if (found_first ^ found_second) { // ^ is XOR
                printf( "found path for (%i,%i) at %p ", pair->first, pair->second,&(*_ps_node) );
                _ps_node->printOnlyMe();
                (*__PAIR2PATH)[ *pair ] = *__PATHSET;   // store path
                _pairs.erase( pair );                   // remove found pair
                // scan pairs for new min,max IDs
                if ((pair->first == __ONEMATCH_MIN_ID) || (pair->second == __ONEMATCH_MAX_ID)) {
                    __ONEMATCH_MIN_ID = __MAX_ID;
                    __ONEMATCH_MAX_ID = -1;
                    for (ID2IDSetCIter p=_pairs.begin(); p != _pairs.end(); ++p) {
                        if (p->first  < __ONEMATCH_MIN_ID) __ONEMATCH_MIN_ID = p->first;
                        if (p->second > __ONEMATCH_MAX_ID) __ONEMATCH_MAX_ID = p->second;
                    }
                    printf( " new MIN,MAX (%d,%d)", __ONEMATCH_MIN_ID, __ONEMATCH_MAX_ID );
                }
                printf( "\n" );
            }
        }
    }
        
    //
    // step down the children unless all paths are found
    // if either ID is lower than highest ID in the set of ID-pairs
    // or the node has no probes
    //
    if ((id < __ONEMATCH_MAX_ID) || (! has_probes)) {
        for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin();
             (i != _ps_node->getChildrenEnd()) && (!_pairs.empty());
             ++i) {
            PS_find_probes_for_pairs( i->second, _pairs );
        }
    }

    //
    // remove ID from path
    //
    __PATHSET->erase( id );
}

void PS_print_and_evaluate_map( const PS_NodePtr _root_node, const char *_result_filename ) {
    //
    // print and evaluate bitmap
    //
    printf( "\n\n----------------- bitmap ---------------\n\n" );
    SpeciesID smaller_id;
    SpeciesID bigger_id;
    ID2IDSet  noMatch;
    ID2IDSet  oneMatch;
    bool      bit1;
    bool      bit2;
    __ONEMATCH_MIN_ID = __MAX_ID;
    __ONEMATCH_MAX_ID = __MIN_ID;
    for (SpeciesID id1 = __MIN_ID; id1 <= __MAX_ID; ++id1) {
//         printf( "[%6i] ",id1 );
        for (SpeciesID id2 = __MIN_ID; id2 <= id1; ++id2) {
            smaller_id = (id1 < id2) ? id1 : id2;
            bigger_id  = (id1 < id2) ? id2 : id1;
            bit1       = __MAP->get( smaller_id, bigger_id );
            bit2       = __MAP->get( bigger_id, smaller_id );
            if (bit1 && bit2) {
//                 printf( "2" );
            } else if (bit1) {
//                 printf( "1" );
                oneMatch.insert( ID2IDPair(smaller_id,bigger_id) );
                if (smaller_id < __ONEMATCH_MIN_ID) __ONEMATCH_MIN_ID = smaller_id;
                if (bigger_id  > __ONEMATCH_MAX_ID) __ONEMATCH_MAX_ID = bigger_id;
            } else {
//                 printf( "0" );
                if (id1 != id2) noMatch.insert( ID2IDPair(smaller_id,bigger_id) ); // there are no probes to distinguish a species from itself .. obviously
            }
        }
//         printf( "\n" );
    }
    printf( "(enter to continue)\n" );
//    getchar();

    printf( "\n\n----------------- no matches ---------------\n\n" );
    if (!_result_filename) {
        for (ID2IDSetCIter i = noMatch.begin(); i != noMatch.end(); ++i) {
            printf( "%6i %6i\n", i->first, i->second );
        }
    }
    printf( "%u no matches\n(enter to continue)\n", noMatch.size() );
//    getchar();

    printf( "\n\n----------------- one match ---------------\n\n" );
    if (!_result_filename) {
        for (ID2IDSetCIter i = oneMatch.begin(); i != oneMatch.end(); ++i) {
            printf( "%6i %6i\n", i->first, i->second );
        }
    }
    printf( "%u one matches\n(enter to continue)\n", oneMatch.size() );
//    getchar();
    //
    // find paths for pairs
    //
    __PATHSET   = new IDSet;
    __PAIR2PATH = new IDID2IDSetMap;
    int c = 0;
    for (PS_NodeMapConstIterator i = _root_node->getChildrenBegin();
         (i != _root_node->getChildrenEnd()) && (!oneMatch.empty());
         ++i,++c ) {
        if ((c < 50) || (c % 100 == 0)) printf( "PS_find_probes_for_pairs( %i ) : %i of %i\n", i->first, c+1, _root_node->countChildren() );
        PS_find_probes_for_pairs( i->second, oneMatch );
    }
    //
    // print paths
    //
    for (IDID2IDSetMapCIter i = __PAIR2PATH->begin();
         i != __PAIR2PATH->end();
         ++i) {
        printf( "\nPair (%i,%i) Setsize (%d)", i->first.first, i->first.second, i->second.size() );
        PS_NodePtr current_node = _root_node;
        long c = 0;
        for (IDSetCIter path_id=i->second.begin();
             path_id !=i->second.end();
             ++path_id,++c) {
            current_node = current_node->getChild( *path_id ).second;
            if (c % 10 == 0) printf( "\n" );
            printf( "%6i%s ", *path_id, (current_node->hasProbes()) ? ((current_node->hasInverseProbes()) ? "*" : "+") : "-" );
        }
        printf( "\nFinal Node : %p ", &(*current_node) );
        current_node->printOnlyMe();
        printf( "\n" );
    }
    printf( "\n%u paths\n", __PAIR2PATH->size() );
    //
    // oups
    //
    if (!oneMatch.empty()) {
        printf( "did not find a path for these :\n" );
        for (ID2IDSetCIter i = oneMatch.begin(); i != oneMatch.end(); ++i) {
            printf( "%6i %6i\n", i->first, i->second );
        }
    }
    //
    // make preset map
    //
    PS_BitMap_Counted *preset = new PS_BitMap_Counted( false, __MAX_ID+1 );
    // set bits for no matches
    for (ID2IDSetCIter pair=noMatch.begin(); pair != noMatch.end(); ++pair) {
        preset->setTrue( pair->second, pair->first );
    }
    // iterate over paths
    for (IDID2IDSetMapCIter i = __PAIR2PATH->begin();
         i != __PAIR2PATH->end();
         ++i) {
        // iterate over all IDs except path
        IDSetCIter path_iter    = i->second.begin();
        SpeciesID  next_path_id = *path_iter;
        for (SpeciesID id = __MIN_ID; id <= __MAX_ID; ++id) {
            if (id == next_path_id) {   // if i run into a ID in path
                ++path_iter;            // advance to next path ID
                next_path_id = (path_iter == i->second.end()) ? -1 : *path_iter;
                continue;               // skip this ID
            }
            // iterate over path IDs
            for (IDSetCIter path_id = i->second.begin(); path_id != i->second.end(); ++path_id) {
                if (id == *path_id) continue;   // obviously a probe cant differ a species from itself
                if (id > *path_id) {
                    preset->setTrue( id,*path_id );
                } else {
                    preset->setTrue( *path_id,id );
                }
            }
        }
    }
    preset->recalcCounters();
    if (!_result_filename) preset->print();
    //
    // save results
    //
    if (_result_filename) {
        PS_FileBuffer *result_file = new PS_FileBuffer( _result_filename, PS_FileBuffer::WRITEONLY );
        // no matches
        printf( "saving no matches to %s...\n", _result_filename );
        result_file->put_long( noMatch.size() );
        for (ID2IDSetCIter i = noMatch.begin(); i != noMatch.end(); ++i) {
            result_file->put_int( i->first );
            result_file->put_int( i->second );
        }
        // one matches
        printf( "saving one matches to %s...\n", _result_filename );
        result_file->put_long( __PAIR2PATH->size() );
        for (IDID2IDSetMapCIter i = __PAIR2PATH->begin(); i != __PAIR2PATH->end(); ++i) {
            result_file->put_int( i->first.first );
            result_file->put_int( i->first.second );
            result_file->put_long( i->second.size() );
            for (IDSetCIter path_id=i->second.begin(); path_id !=i->second.end(); ++path_id) {
                result_file->put_int( *path_id );
            }
        }
        // preset bitmap
        printf( "saving preset bitmap to %s...\n", _result_filename );
        preset->save( result_file );
        delete result_file;
    }
    delete preset;
    delete __PATHSET;
    delete __PAIR2PATH;
}

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

   // open probe-set-database
    if (argc < 2) {
        printf( "Missing argument\n Usage %s <database name> [[-]bitmap filename] [+result filename]\n ", argv[0] );
        printf( "if bitmap filename begins with '-' it is loaded, else its created\n " );
        printf( "result filename MUST be preceded by '+'\n" );
        exit( 1 );
    }

    const char *input_DB_name   = argv[1];
    const char *bitmap_filename = 0;
    const char *result_filename = 0;

    if (argc > 2) {
        if (argv[2][0] == '+') {
            result_filename = argv[2]+1;
        } else {
            bitmap_filename = argv[2];
        }
    }
    if (argc > 3) {
        if (argv[3][0] == '+') {
            result_filename = argv[3]+1;
        } else {
            bitmap_filename = argv[3];
        }
    }
    
    struct tms before;
    times( &before );
    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
    __MIN_ID = db->getMinID();
    PS_print_time_diff( &before, "(enter to continue)  " );
//    getchar();

    __MAP = new PS_BitMap_Fast( false, __MAX_ID+1 );
    if (!bitmap_filename || (bitmap_filename[0] != '-')) {
        printf( "detecting..\n" ); fflush( stdout );
        __NODES_LEFT = (char*)malloc( 61 );
        memset( __NODES_LEFT, ' ', 60 );
        __NODES_LEFT[ 60 ] = '\x00';
        PS_detect_weak_differences( db->getRootNode() );
        free( __NODES_LEFT );
        if (bitmap_filename) {
            printf( "saving bitmap to file %s\n", bitmap_filename );
            PS_FileBuffer *mapfile = new PS_FileBuffer( bitmap_filename, PS_FileBuffer::WRITEONLY );
            __MAP->save( mapfile );
            delete mapfile;
        }
    } else if (bitmap_filename) {
        printf( "loading bitmap from file %s\n",bitmap_filename+1 );
        PS_FileBuffer *mapfile = new PS_FileBuffer( bitmap_filename+1, PS_FileBuffer::READONLY );
        __MAP->load( mapfile );
        delete mapfile;
    }
    printf( "(enter to continue)\n" );
//    getchar();

    times( &before );
    PS_print_and_evaluate_map( db->getRootNode(), result_filename );
    PS_print_time_diff( &before, "(enter to continue)  " );
//    getchar();
    delete __MAP;
    
    printf( "removing database from memory\n" );
    delete db;
    printf( "(enter to continue)\n" );
//    getchar();

    return 0;
}
