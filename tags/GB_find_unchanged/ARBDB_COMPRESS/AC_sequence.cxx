#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "ac.hxx"




// ********************************************************************
//
//              class AC_SEQUENCE : Konstruktor/Destruktor
//
// ********************************************************************
AC_SEQUENCE::AC_SEQUENCE(GBDATA *gb_data_i) {   // gb_data is nur Link
    //r     sequence                = seq;
    gb_data                 = gb_data_i;
    quality                 = 0;
    seq_len                 = GB_read_string_count(gb_data);
}

AC_SEQUENCE::~AC_SEQUENCE(){

}
// *********************************************************************


// *********************************************************************
//      class AC_SEQUENCE::get_quality(GBDATA *gb_data)
//
//              bestimmt die Anzahl der Buchstaben einer Sequenz.
// *********************************************************************
long  AC_SEQUENCE::get_quality() {

    char *runner;
    long counter = 0;
    char *temp_sequence  = GB_read_char_pntr(gb_data);
    for(runner = temp_sequence; *runner; runner++) {
        if(isalpha(*runner)) counter++;
    }

    return counter;
} // end AC_SEQUENCE::get_quality(GBDATA *gb_data)
// *********************************************************************


//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


// *********************************************************************
//
//              class AC_SEQUENCE_ALIGNED : Konstruktor/Destruktor
//
// *********************************************************************
AC_SEQUENCE_ALIGNED::AC_SEQUENCE_ALIGNED(GBDATA *gb_data_i) : AC_SEQUENCE (gb_data_i) {
    ac_dbdata_state = ALIGNED_SEQUENCES;
}



AC_SEQUENCE_ALIGNED::~AC_SEQUENCE_ALIGNED()  {

}
// *********************************************************************



// *********************************************************************
//      class AC_SEQUENCE_ALIGNED::get_distance()
//
//              liefert die Distanz zwischen der aktuellen Sequenz und
//              der Referenzsequenz.
//              Es muessen zwei vergleichbare Zeichen (= Buchstaben)
//              aufeinandertreffen. Sind die Zeichen unterschiedlich,
//              erhoeht sich die Distanz um + 1 .
//
// *********************************************************************
double AC_SEQUENCE_ALIGNED::get_distance(AC_SEQUENCE *that)  {

    long        distance, equals;
    char        *revptr, *seqptr;

    revptr = that->sequence;                // Reverenzsequenz

    seqptr  = GB_read_char_pntr(gb_data);

    //      seqptr = this->sequence;                // aktuelle Sequenz
    distance = 0;
    equals = 0;
    char    reg1, reg2;
    for( ; (reg1 = *revptr) && (reg2 = *seqptr); revptr++, seqptr++ ) {
        if( (!isalpha(reg1)) || !isalpha(reg2))  continue;
        if (reg1==reg2) {
            equals++;
        }else{
            distance++;
        } // endif
    } // endfor

    return distance/(distance + equals + 1.0);

} // end AC_SEQUENCE_ALIGNED::get_distance()
// *********************************************************************


//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


// *********************************************************************
//
//              class AC_SEQUENCE_UNALIGNED : Konstruktor/Destruktor
//
// *********************************************************************
AC_SEQUENCE_UNALIGNED::AC_SEQUENCE_UNALIGNED(GBDATA *gb_data_i) : AC_SEQUENCE (gb_data_i) {
    ac_dbdata_state = UNALIGNED_SEQUENCES;
}



AC_SEQUENCE_UNALIGNED::~AC_SEQUENCE_UNALIGNED()  {

}
// *********************************************************************



// *********************************************************************
//      class AC_SEQUENCE_ALIGNED::get_distance()
//
//              Dummyfunktion zum Compilerberuhigen
//
//
// *********************************************************************
double AC_SEQUENCE_UNALIGNED::get_distance(AC_SEQUENCE *that)  {

    return 0;
} // end AC_SEQUENCE_UNALIGNED::get_distance()
// *********************************************************************




