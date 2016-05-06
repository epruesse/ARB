// =============================================================== //
//                                                                 //
//   File      : fb_test.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_bitmap.hxx"
#include "ps_node.hxx"

#include <iostream>

#include <sys/times.h>

static void runtests(FILE *out, const char *outputfile) {
    {
        PS_BitSet_Fast *x = new PS_BitSet_Fast(false, 20);
        x->setTrue(0);
        x->setTrue(3);
        x->setTrue(4);
        x->setTrue(7);
        x->setTrue(10);
        x->setTrue(11);
        x->setTrue(14);
        x->print(true, 20);
        PS_BitSet::IndexSet indices;
        x->getTrueIndices(indices);
        fprintf(out, " true  indices (%2zu) : ", indices.size());
        for (PS_BitSet::IndexSet::iterator i=indices.begin(); i != indices.end(); ++i) {
            fprintf(out, " %4li", *i);
        }
        x->getTrueIndices(indices, 15);
        fprintf(out, "\n true  indices (%2zu) : ", indices.size());
        for (PS_BitSet::IndexSet::iterator i=indices.begin(); i != indices.end(); ++i) {
            fprintf(out, " %4li", *i);
        }
        x->getFalseIndices(indices);
        fprintf(out, "\n false indices (%2zu) : ", indices.size());
        for (PS_BitSet::IndexSet::iterator i=indices.begin(); i != indices.end(); ++i) {
            fprintf(out, " %4li", *i);
        }
        x->getFalseIndices(indices, 15);
        fprintf(out, "\n false indices (%2zu) : ", indices.size());
        for (PS_BitSet::IndexSet::iterator i=indices.begin(); i != indices.end(); ++i) {
            fprintf(out, " %4li", *i);
        }
        fprintf(out, "\n");
        delete x;
    }

    {
        PS_BitMap *map = new PS_BitMap_Fast(false, 10);
        for (long i = 0; i < 10; ++i) {
            map->set(i, i, true);
            map->set(0, i, true);
            map->set(i, 0, true);
            map->set(9, i, true);
        }
        map->print();

        PS_FileBuffer *fb1 = new PS_FileBuffer(outputfile, PS_FileBuffer::WRITEONLY);
        map->save(fb1);
        fb1->reinit(outputfile, PS_FileBuffer::READONLY);
        PS_BitMap_Counted *map2 = new PS_BitMap_Counted(fb1);
        map2->print();

        // map2->setTrue(5, 8); // exceeds capacity
        map2->setTrue(5, 7);
        map2->print();
        map2->recalcCounters();
        map2->print();

        delete map;
        delete map2;
        delete fb1;
    }

    {
        char str[] = "ABCDEFG";
        int a      = 1;
        int b      = 1;
        fprintf(out, "%i %c %i\n", a, str[a],    a+1); a++;
        fprintf(out, "%i %c %i\n", b, str[b+1], b+1); b++;
    }
    {
        ID2IDSet *s = new ID2IDSet;
        s->insert(ID2IDPair(10, 40));
        s->insert(ID2IDPair(8, 20));
        s->insert(ID2IDPair(1, 4));
        s->insert(ID2IDPair(8, 40));
        s->insert(ID2IDPair(40, 70));
        s->insert(ID2IDPair(20, 80));
        for (ID2IDSetCIter i = s->begin(); i != s->end(); ++i) {
            fprintf(out, "%6i %6i\n", i->first, i->second);
        }
        delete s;
    }

    {
        PS_FileBuffer *fb2 = new PS_FileBuffer(outputfile, true);
        char *data = (char *)malloc(1024);
        // fb2->get(data, 4096);
        fb2->get(data, 100);

        free(data);
        delete fb2;
    }
}

int ARB_main(int , char *[]) {
    runtests(stdout, "testdata");
    return 0;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <arb_msg.h>
#include <arb_file.h>

void TEST_probeset_basics() {
    const char *textout = "tools/probeset.out";
    const char *dataout = "tools/probeset.data";

    GB_unlink(dataout);

    FILE *out = fopen(textout, "wt");
    if (!out) {
        TEST_EXPECT_NO_ERROR(GB_IO_error("writing", textout));
    }
    runtests(out, dataout);
    fclose(out);


    // TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(textout));
    // TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(dataout));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
