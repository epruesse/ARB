#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include "ps_node.hxx"
#include "ps_bitset.hxx"
#include "ps_bitmap.hxx"
#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

//  ----------------------------------------------------
//      void PS_detect_weak_differences( const PS_NodePtr ps_root_node )
//  ----------------------------------------------------
//  recursively walk through database and make a triangle bool-matrix
//  of SpeciesID's where true means that the 2 species can be distinguished
//  by a probe
//
typedef vector<SpeciesID> IDvector;

void PS_detect_weak_differences_stepdown( const PS_NodePtr _ps_node, PS_BitMap *_map, IDvector &_upper_nodes ) {
    
    //
    // get SpeciesID
    //
    SpeciesID nodeNum = _ps_node->getNum();

    //
    // append node to upper-nodes-list
    //
    if (!_ps_node->hasProbes()) nodeNum = -nodeNum;               // negative nodeNums indicate an empty probes-list in a node
    _upper_nodes.push_back( nodeNum );

    //
    // step down the children
    //
    for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin(); i != _ps_node->getChildrenEnd(); ++i) {
        PS_detect_weak_differences_stepdown( i->second, _map, _upper_nodes );
    }

    //
    // remove node from upper
    //
    _upper_nodes.pop_back();

    //
    // set value in the map : true for all (upper-nodes-id,nodeNum) pairs
    //
    IDvector::reverse_iterator upperIndex = _upper_nodes.rbegin();    // rbegin() <-> walk from end to start
    // skip empty nodes at the end of the upper_nodes - list
    // because they have no probe to distinguish them from us
    for ( ; upperIndex != _upper_nodes.rend(); ++upperIndex ) {
        if (*upperIndex >= 0) break;
    }
    // continue with the rest of the list
    if (nodeNum < 0) nodeNum = -nodeNum;
    for ( ; upperIndex != _upper_nodes.rend(); ++upperIndex) {
        _map->triangle_set( nodeNum, abs(*upperIndex), true );
        //printf( "(%i|%i) ", nodeNum, *upperIndex );
    }
}

void PS_detect_weak_differences( const PS_NodePtr _ps_root_node, SpeciesID _max_ID ) {
    PS_BitMap *theMap = new PS_BitMap( false,_max_ID,PS_BitMap::TRIANGULAR );
    IDvector   upperNodes;
    
    printf( "PS_detect_weak_differences_stepdown :      1 of %i (%6.2f%%)\n",_max_ID,(1/(float)_max_ID)*100 );
    for (PS_NodeMapConstIterator i = _ps_root_node->getChildrenBegin(); i != _ps_root_node->getChildrenEnd(); ++i ) {
        //        if (i->first % 10 == 0)
        printf( "PS_detect_weak_differences_stepdown : %6i of %i (%6.2f%%)\n",i->first+1,_max_ID,((i->first+1)/(float)_max_ID)*100 );
        PS_detect_weak_differences_stepdown( i->second, theMap, upperNodes );
        if (!upperNodes.empty()) upperNodes.clear();
    }
    printf( "PS_detect_weak_differences_stepdown : %6i of %i (%6.2f%%)\n",_max_ID,_max_ID,100.0 );

    getchar();
    theMap->print();
    printf( "(enter to continue)\n" );
//     getchar();

    delete theMap;
//     printf( "(enter to continue)\n" );
//     getchar();
}


//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

    // open probe-set-database
    if (argc < 3) {
        printf("Missing arguments\n Usage %s <database name> <map name>\n",argv[0]);
        exit(1);
    }

    const char  *input_DB_name = argv[1];
    const char  *map_file_name = argv[2];
    unsigned int ID_count = 0;
    
    printf( "Opening probe-set-database '%s'..\n", map_file_name );
    PS_FileBuffer *fb = new PS_FileBuffer( map_file_name, PS_FileBuffer::READONLY );
    fb->get_uint( ID_count );
    printf( "there should be %i species in the database you specified ;)\n", ID_count );

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_NodePtr root(new PS_Node(-1));
    fb->reinit( input_DB_name, PS_FileBuffer::READONLY );
    root->load( fb );
    printf( "loaded database (enter to continue)\n" );
//     getchar();


    PS_detect_weak_differences( root, ID_count );

    printf( "(enter to continue)\n" );
//     getchar();
    
    delete fb;
    root.SetNull();
    printf( "root should be destroyed now\n" );
    printf( "(enter to continue)\n" );
//     getchar();

    return 0;
}
