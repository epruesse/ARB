#include <stdlib.h>

#include "ps_bitmap.hxx"
#include "ps_defs.hxx"

int main( void ) {
    PS_BitMap *map = new PS_BitMap_Fast( false, 10 );
    for (long i = 0; i < 10; ++i) {
        map->set( i,i,true );
        map->set( 0,i,true );
        map->set( i,0,true );
        map->set( 9,i,true );
//         map->set( i,9,true );
    }
    map->print();

//     PS_BitSet *set = new PS_BitSet_Counted( false, 5 );
//     for (long i = 0; i < 5; i+=2) {
//         set->set( i,true );
//     }
//     set->print();
//     delete set;
//     return 0;

    PS_FileBuffer *fb1 = new PS_FileBuffer( "testdata",PS_FileBuffer::WRITEONLY );
    map->save( fb1 );
//     set->save( fb1 );
    fb1->reinit( "testdata",PS_FileBuffer::READONLY );
    PS_BitMap_Counted *map2 = new PS_BitMap_Counted( fb1 );
    map2->print();

    map2->setTrue( 5,8 );
    map2->print();
    map2->recalcCounters();
    map2->print();
//     PS_BitSet *set2 = new PS_BitSet( true );
//     set2->load( fb1 );
//     set2->print();

    delete map;
    delete map2;
//     delete set;
//     delete set2;
    delete fb1;
    return 0;

    char str[] = "ABCDEFG";
    int a = 1;
    int b = 1;
    printf( "%i %c %i\n", a,str[a++],a );
    printf( "%i %c %i\n", b,str[++b],b );
    return 0;

    ID2IDSet *s = new ID2IDSet;
    s->insert( ID2IDPair(10,40) );
    s->insert( ID2IDPair(8,20) );
    s->insert( ID2IDPair(1,4) );
    s->insert( ID2IDPair(8,40) );
    s->insert( ID2IDPair(40,70) );
    s->insert( ID2IDPair(20,80) );
    for (ID2IDSetCIter i = s->begin(); i!= s->end(); ++i) {
        printf( "%6i %6i\n", i->first, i->second );
    }
    delete s;
    return 0;

    PS_FileBuffer *fb2 = new PS_FileBuffer( "testdata",true );
    char *data = (char *)malloc(1024);
    fb2->get(data,4096);
    fb2->get(data,100);

    free(data);
    delete fb2;
    return 0;
}
