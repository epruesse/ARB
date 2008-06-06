#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <math.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "ac.hxx"






// *********************************************************************
//
//      zur Festlegung der Sortierrichtung des ARRAYS bei dem
//      Funktionsaufruf von:
//
//                              GB_mergesort()
//
//      innerhalb der Proc:  AC_SEQUENCE_LIST::divide_sequence_list()
//
//      ]Sortierreihenfolge "<=" von 0 an aufsteigend.
//
// *********************************************************************
long seq_info_cmp(void *i,void *j, char *)
{       AC_SEQUENCE_INFO *a = (AC_SEQUENCE_INFO *)i;
    AC_SEQUENCE_INFO *b = (AC_SEQUENCE_INFO *)j;

    if (a->relationship  < b->relationship  ) return -1;
    if (a->relationship  > b->relationship  ) return 1;
    return(0);
} // end seq_info_cmp()
// *********************************************************************





// *********************************************************************
//
//              get_intersection_expglaettg
//
//      diese Funktion bestimmt den Schnittpunkt folgendermassen:
//
//
// *********************************************************************

double  get_intersection_expglaettg(AC_SEQUENCE_INFO **sort_array, double number_of)
{

    AC_SEQUENCE_INFO        **array_ptr = sort_array;

    double  x, prev_x  = 0;                 // Relationship
    double  y, prev_y  = 0;                 // Differenz x[i]-x[i-1]
    double  z, prev_z  = 1;                 // exponentielle Glaettung
    double  zpar       = 1;                 // Gewichtung mit Parabel
    double  anzahl_lire_product = 0;        // Produkt aus AnzahlLinkerSeqs x AnzahlRechterSeqs
    double  base    = 0;                    // Basis fuer Exponentialfunktion
    double  nenner  = 0;
    double  zaehler = 0;
    double  potenz_nenner  = 0;
    double  potenz_zaehler = 0;
    double  intersection_value = 0;         // Schnittpunkt
    long    anzahl_aller_seq  = 0;
    long    anzahl_seq_links  = 0;
    long    anzahl_seq_rechts = 0;
    long    dekrement   = 0;

    double  min_x  = 0;             // Min Relationship -> Schnittpunkt
    double  min_zpar   = 0;                 // Minimum
    double  min_max_anzahl_lire_product = 0;  // maximales Produkt aus
    //    AnzahlLinkerSeqs x AnzahlRechterSeqs
    //    eines Minimums !!!
    long    min_sequenznumber  = 0;         // Nummer der Sequenz

    dekrement    = number_of;
    anzahl_aller_seq  = number_of;
    anzahl_seq_links  = 0;
    anzahl_seq_rechts = anzahl_aller_seq;   // schnittpkt wird die erste seq des re knotens

    base            = 32;
    potenz_nenner   = 1.5;
    potenz_zaehler  = 2 * potenz_nenner;


    //-------------------------------- f(x[i]) = f(x[i-1]) * ( 1/32^[f(x[i])-f(x[i-1])] +1 )
    for( array_ptr,dekrement; 0 < dekrement; )  {

        //------------------------------------------ Zuweisung Relationship
        x = (*array_ptr)->relationship;
        //------------------------------------- Ende Zuweisung Relationship

        //---------------------------------------- Bestimmung der Abstaende
        y = x - prev_x;
        //----------------------------------- Ende Bestimmung der Abstaende

        //------------------------------------------------ Glaettungsfunktion
        if( dekrement == number_of )  {         // NUR BEI erster Sequenz "LEFTEND" !!
            z = 1;
            anzahl_seq_links = 1;           // sonst division durch "0" !!
            anzahl_lire_product = anzahl_aller_seq;
        }
        else {
            z = (( prev_z * ( 1/pow(base,y) ) ) + 1 );
            //                      z = (( prev_z * ( 1/pow(2,y) ) ) + 1 );
            //                      z = 1/pow(2,y);


            //                      zpar = z;


        } // endelse
        //------------------------------------------- Ende Glaettungsfunktion

        //----------------------------------- Gewichtungsfunktion Parabel z/nenner^?

        zaehler  = z*pow(anzahl_aller_seq,potenz_zaehler);
        anzahl_lire_product = anzahl_seq_links * anzahl_seq_rechts;
        nenner   = pow(anzahl_lire_product, potenz_nenner);
        zpar = zaehler/nenner;
        ///////////             zpar = z/nenner;

            if( dekrement == number_of )  {
                anzahl_seq_links = 0;           // Korrektur !!
                min_zpar  = zpar;               // Vorbelegung Schnittpunktbestimmung
            }
            anzahl_seq_links++;
            anzahl_seq_rechts--;
            //------------------------------------------ Ende Gewichtungsfunktion

            //------------------------------------- Bestimmung des Schnittpunktes
            if(   (zpar < min_zpar) ||
                  ((zpar == min_zpar) && (anzahl_lire_product > min_max_anzahl_lire_product))   )  {
                min_x = x;
                min_zpar  = zpar;
                min_max_anzahl_lire_product = anzahl_lire_product;
                min_sequenznumber  = (*array_ptr)->number;

            }
            //-------------------------------- Ende Bestimmung des Schnittpunktes


#ifdef GNUPLOT_print_expglaettg

            printf("%f      ",x);

            //              printf("%f      \n",y);

            //              printf("%f      \n",z);

            printf("%f      \n",zpar);

            //              printf("%s\n", (*array_ptr)->name);

#endif

            prev_y = y;
            prev_x = x;
            prev_z  = z;


            array_ptr++;
            dekrement--;


    } // endfor

    intersection_value  = min_x;



#ifdef GNUPLOT_print_schnittpunkt_expglaettg
    printf("\n\n");
    printf("#GNU  Return Intersection_value:        %f\n",intersection_value);
    printf("#GNU  Minimum bei Seq-Nr: %d", min_sequenznumber);
    printf("\n\n");
#endif

    return intersection_value;

} // end get_intersection_expglaettg()
// *******************************************************************
