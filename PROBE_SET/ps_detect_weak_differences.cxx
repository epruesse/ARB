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

void PS_detect_weak_differences_stepdown( const PS_NodePtr ps_node, PS_BitMap *the_map, IDvector &upper_nodes, SpeciesID &max_ID ) {
    
    //
    // store maximum SpeciesID
    //
    SpeciesID nodeNum = ps_node->getNum();
    if (nodeNum > max_ID) max_ID = nodeNum;

    //
    // append node to upper-nodes-list
    //
    if (!ps_node->hasProbes()) nodeNum = -nodeNum;               // negative nodeNums indicate an empty probes-list in a node
    upper_nodes.push_back( nodeNum );

    //
    // step down the children
    //
    for (PS_NodeMapConstIterator i = ps_node->getChildrenBegin(); i != ps_node->getChildrenEnd(); ++i) {
        PS_detect_weak_differences_stepdown( i->second, the_map, upper_nodes, max_ID );
    }

    //
    // remove node from upper
    //
    upper_nodes.pop_back();

    //
    // set value in the map : true for all (upper-nodes-id,nodeNum) pairs
    //
    // rbegin() <-> walk from end to start
    IDvector::reverse_iterator upperIndex = upper_nodes.rbegin();
    // skip empty nodes at the end of the upper_nodes - list
    // because they have no probe to distinguish them from us
    for ( ; upperIndex != upper_nodes.rend(); ++upperIndex ) {
        if (*upperIndex >= 0) break;
    }
    // continue with the rest of the list
    if (nodeNum < 0) nodeNum = -nodeNum;
    for ( ; upperIndex != upper_nodes.rend(); ++upperIndex) {
        the_map->triangle_set( nodeNum, abs(*upperIndex), true );
        //printf( "(%i|%i) ", nodeNum, *upperIndex );
    }
    //if (nodeNum % 200 == 0) printf( "%i\n",nodeNum );
}

void PS_detect_weak_differences( const PS_NodePtr ps_root_node ) {
    PS_BitMap *theMap = new PS_BitMap( false );
    SpeciesID  maxID  = 0;
    IDvector   upperNodes;

    printf( "PS_detect_weak_differences_stepdown( 0, theMap, upperNodes, 0 )\n" );    
    for (PS_NodeMapConstIterator i = ps_root_node->getChildrenBegin(); i != ps_root_node->getChildrenEnd(); ++i ) {
        if (i->first % 10 == 0) printf( "PS_detect_weak_differences_stepdown( %i, theMap, upperNodes, %i )\n",i->first,maxID );
        PS_detect_weak_differences_stepdown( i->second, theMap, upperNodes, maxID );
        if (!upperNodes.empty()) {
            printf( "unclean ids :" );
            for (IDvector::iterator id = upperNodes.begin(); id != upperNodes.end(); ++id ) {
                printf( " %i",*id );
            }
            printf( "\n" );
            upperNodes.clear();
        }
    }
    printf( "max ID = %i\n(enter to continue)",maxID );
//     getchar();

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
    if (argc < 2) {
        printf("Missing arguments\n Usage %s <database name>\n",argv[0]);
        exit(1);
    }

    const char *input_DB_name = argv[1];

    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
    PS_FileBuffer *fb = new PS_FileBuffer( input_DB_name, PS_FileBuffer::READONLY );
    PS_NodePtr     root(new PS_Node(-1));
    root->load( fb );
    printf( "loaded database (enter to continue)\n" );
//     getchar();


    PS_detect_weak_differences( root );

    printf( "(enter to continue)\n" );
//     getchar();
    
    delete fb;
    root.SetNull();
    printf( "root should be destroyed now\n" );
    printf( "(enter to continue)\n" );
//     getchar();

    return 0;
}
