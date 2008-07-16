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
//      class AC_SEQUENCE_LIST  Konstruktor/Destruktor
//
// *********************************************************************
AC_SEQUENCE_LIST::AC_SEQUENCE_LIST()  {
    sequences               = NULL;         // Struct an dem die AC_SEQUENCE haengen
    leftend                 = NULL;
    rightend                = NULL;
    nsequences              = 0;
    max_relationship        = 0.0;
}



AC_SEQUENCE_LIST::~AC_SEQUENCE_LIST()  {
}
// *********************************************************************


// *********************************************************************
//
//      class AC_SEQUENCE_LIST:: remove_sequence()
//
//              gibt den Speicher der Liste wieder frei
//
// *********************************************************************
void    AC_SEQUENCE_LIST::remove_sequence(AC_SEQUENCE_INFO *sequence)  {

    delete(sequence->seq);
    sequence->seq = NULL;

    free(sequence->name);
    sequence->name = NULL;

} // end AC_SEQUENCE_LIST::remove_sequence()
// *********************************************************************


// *********************************************************************
//
//      class AC_SEQUENCE_LIST:: remove_sequence_list()
//
//              gibt den Speicher der Liste wieder frei
//
// *********************************************************************
void    AC_SEQUENCE_LIST::remove_sequence_list(AC_SEQUENCE_INFO *sequence_list)  {

    AC_SEQUENCE_INFO        *ptr, *actual_sequence;

    if(sequence_list)  {
        for(ptr = sequence_list; ptr; ) {       //die naechsten Seqs
            actual_sequence = ptr;
            ptr = ptr->next;
            remove_sequence(actual_sequence);       //loesche Sequenz
            free (actual_sequence); //loesche Sequenzinfo = Strukt
        } //endfor
        sequence_list = NULL;
    } //endif

} // end AC_SEQUENCE_LIST::remove_sequence_list()
// *********************************************************************




// *********************************************************************
//      class AC_SEQUENCE_LIST : member_function        insert()
//              -       haengt neues listenelement am Anfang der liste ein
//              oder
//              -       haengt das erste listenelement an die ROOT/I_NODE
// *********************************************************************
void AC_SEQUENCE_LIST::insert( AC_SEQUENCE_INFO *new_seq)  {

    new_seq->next = this->sequences;
    if (this->sequences) this->sequences->previous = new_seq;
    this->sequences = new_seq;
    new_seq->previous = NULL;

    this->nsequences = ++this->nsequences;

} // end AC_SEQUENCE_LIST::insert()
// *********************************************************************




// *********************************************************************
//
//      class AC_SEQUENCE_LIST::get_high_quality_sequence()
//
//              liefert die Sequence mit der hoechsten Qualitaet.
//              Sie nimmt im Zweifelsfall die erste mit der Hoechsten....
//
// *********************************************************************
AC_SEQUENCE_INFO  *AC_SEQUENCE_LIST::get_high_quality_sequence()  {

    long                    maximum = 0;
    AC_SEQUENCE_INFO        *highest_quality_sequence;
    AC_SEQUENCE_INFO        *ac_sequence_info;
    AC_SEQUENCE_INFO        *ptr;

    highest_quality_sequence = ac_sequence_info = sequences;

#ifdef COMMENT
    long    count = 0;
    printf("Wait a minute -> I'm searching for the highest quality sequence...\n\n");
#endif

    for( ptr = ac_sequence_info ; ptr ; ptr = ptr->next )
    {
        if( maximum < ptr->seq->quality ) {
            maximum = ptr->seq->quality;
            highest_quality_sequence = ptr;
        } // endif

#ifdef COMMENT
        if (count % 1000 == 0) {
            printf(".");
        }
        count++;
#endif

    } // endfor

    //printf("\n\nMaxQuality :  %d\n\n", maximum);


    return highest_quality_sequence;

} // end AC_SEQUENCE_LIST::get_high_quality_sequence()
// *********************************************************************




// *********************************************************************
//
//      class AC_SEQUENCE_LIST::determine_distances_to()
//
//              bestimmt die reale_Distanz und
//              bestimmt die relative Distanz
//              zwischen zwei Sequenzen.
//
// *********************************************************************
void AC_SEQUENCE_LIST::determine_distances_to(AC_SEQUENCE_INFO *reverence_seq,
                                              AC_SEQUENCE_INFO_STATE reference_state) {

    AC_SEQUENCE_INFO        *ac_sequence_info;
    AC_SEQUENCE_INFO        *ptr;
    AC_SEQUENCE             *that;
    long                    real_distance;
    double          relative_distance;

    //-------------------------------------------- hole referenzsequenz
    that = reverence_seq->seq;
    that->sequence = GB_read_string(that->gb_data); // kopiert Reverenzsequenz
    //----------------------------------------------- laufe liste durch
    ac_sequence_info = this->sequences;
    for(ptr = ac_sequence_info; ptr ; ptr = ptr->next) {
        if(ptr->state!=reference_state) {

            //---------------------------------------- bestimme Distanz
            relative_distance = ptr->seq->get_distance(that);
            //----------------------------------------------- merks dir

            if(reference_state == LEFTEND) {
                ptr->relative_leftdistance = relative_distance;
            } else {        // RIGHTEND
                ptr->relative_rightdistance = relative_distance;
            } // endelse

        } // endif
    } // endfor
    delete that->sequence;
    that->sequence = NULL;          // loescht Reverenzsequenz


} // end AC_SEQUENCE_LIST::determine_distances_to()
// *********************************************************************



// *********************************************************************
//
//      class AC_SEQUENCE_LIST::get_seq_with_max_dist_to()
//
//              liefert die erste Sequence mit
//                      dem groessten Abstand
//              bei
//                      der groessten Qualitaet
//
//              also    MAX{ Abstand * Qualitaet }
//
// *********************************************************************
AC_SEQUENCE_INFO  *AC_SEQUENCE_LIST::get_seq_with_max_dist_to()  {

    long    maxvalue = 0;
    long    product = 0;
    AC_SEQUENCE_INFO        *seq_with_max_dist;
    AC_SEQUENCE_INFO        *ac_sequence_info;
    AC_SEQUENCE_INFO        *ptr;

    ac_sequence_info = this->sequences;

    for( ptr = ac_sequence_info ; ptr ; ptr = ptr->next )
    {
        product = ptr->relative_leftdistance * ptr->seq->quality;
        if( maxvalue <= product ) {
            maxvalue = product;
            seq_with_max_dist = ptr;
        } // endif
    } // endfor

    if( 0 == maxvalue )  seq_with_max_dist = NULL;

    return seq_with_max_dist;

} // end AC_SEQUENCE_LIST::get_seq_with_max_dist_to()
// *********************************************************************


// *********************************************************************
//
//      class AC_SEQUENCE_LIST::determine_basepoints()
//
//
//              betrachte die Kanten vom Leftend zum Rightend,
//                      der Abstand relative_leftdistance vom Rightend
//                      ist die Summe der Kanten.
//
//              berechne ueber ein Gleichungssystem den Schnittpunkt
//                      der aktuellen Sequenz mit obiger Gerade.
//
//
//
// *********************************************************************
void    AC_SEQUENCE_LIST::determine_basepoints()  {


    AC_SEQUENCE_INFO        *ac_sequence_info;
    AC_SEQUENCE_INFO        *ptr;
    double                  baseline, basepoint;
    double                  li, re;

    ac_sequence_info = this->sequences;

    baseline = this->rightend->relative_leftdistance; // baseline = Abstand zwischen
    // Leftend und Rightend
#ifdef GNUPLOT_relationship

    //      printf("\n\nBaseline : %f", baseline*100);
    printf("\n#GNUPLOT Basepoints : \n\n");
#endif

    for( ptr = ac_sequence_info ; ptr ; ptr = ptr->next )
    {
        li = ptr->relative_leftdistance;
        re = ptr->relative_rightdistance;
        basepoint = (baseline - (baseline - li + re)/2) * 100;
        if( basepoint < 0 ) basepoint = 0;      // kommt vor!!
        if(100 < basepoint ) basepoint = 100;   // kommt vor!!
        ptr->relationship = basepoint;

#ifdef GNUPLOT_relationship
        printf("%f\n",ptr->relationship);
        //      printf("%f      ",ptr->relationship);
#endif

    } //endfor

} // end AC_SEQUENCE_LIST::determine_basepoints()
// *********************************************************************




// *********************************************************************
//
//      class AC_SEQUENCE_LIST::reset_AC_SEQUENCE_INFO_struct_values()
//
//              Setzt folgende Werte im Struct AC_SEQUENCE_INFO auf die
//              Default-Werte zurueck:
//                      AC_SEQUENCE_INFO_STATE  state,
//                      long            real_leftdistance;
//                      double          relative_leftdistance;
//                      long            real_rightdistance;
//                      double          relative_rightdistance;
//                      double          relationship;
//
// *********************************************************************
void AC_SEQUENCE_LIST::reset_AC_SEQUENCE_INFO_struct_values() {

    AC_SEQUENCE_INFO        *ac_sequence_info;
    AC_SEQUENCE_INFO        *ptr;

    ac_sequence_info = sequences;

    for(ptr = ac_sequence_info; ptr; ptr = ptr->next ) {
        ptr->state                      = ORDINARY;
        ptr->real_leftdistance          = 0;
        ptr->relative_leftdistance      = 0.0;
        ptr->real_rightdistance         = 0;
        ptr->relative_rightdistance     = 0.0;
        ptr->relationship               = 0;
    } // endfor

} // end AC_SEQUENCE_LIST::reset_AC_SEQUENCE_INFO_struct_values()
// *********************************************************************









//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||


// *********************************************************************
//
//      class AC_TREE   Konstruktor/Destruktor
//
// *********************************************************************
AC_TREE::AC_TREE()  {
    node_state      = I_NODE;                       // LEAF oder MASTER
    left            = NULL;
    right           = NULL;

    breakcondition_min_sequencenumber = 1;
    breakcondition_relationship_distance = 1;
}


AC_TREE::~AC_TREE()  {
}
// *********************************************************************





// *********************************************************************
//
//      class AC_TREE:: remove_tree()
//
//              hier wird rekursiv der Clusterbaum geloescht
//
// *********************************************************************
void    AC_TREE::remove_tree()  {

    AC_SEQUENCE_INFO        *sequence_list;

    if(this)  {

        sequence_list = this->sequences;
        if(sequence_list)  {
            remove_sequence_list(sequence_list);
        }


        if(this->left) this->left->remove_tree();
        if(this->right) this->right->remove_tree();
        free (this);
    }

} // end AC_TREE::remove_tree()
// *********************************************************************





// *********************************************************************
//
//      class AC_TREE::print_tree()
//
//              hier wird rekursiv der Clusterbaum rausgeschrieben
//
// *********************************************************************
void    AC_TREE::print_tree()  {

    AC_SEQUENCE_INFO        *ptr;


    if(this)  {
        printf("nodestate:      ");
        if(node_state == I_NODE) printf("*** I_NODE\n");
        else if(node_state == LEAF) printf(">>> LEAF\n");
        if(this->sequences)  {
            for(ptr = this->sequences; ptr; ptr = ptr->next) {      //die naechsten Seqs
                printf("                    Seqnumber: %d       '%s'\n", ptr->number, ptr->name);
            } //endfor
            printf("\n\n");

        }
    }
    if(this->left) this->left->print_tree();
    if(this->right) this->right->print_tree();



} // end AC_TREE::print_tree()
// *********************************************************************



// *********************************************************************
//
//      class AC_TREE::print_formated_dbtree()
//
//              hier wird rekursiv der Clusterbaum rausgeschrieben
//              und das in dem wunderbaren ARB-Format
//              (Knoten,Knoten) oder so ??
//
// *********************************************************************
void    AC_TREE::print_formated_dbtree()  {

    AC_SEQUENCE_INFO        *ptr;

    if(this->node_state == I_NODE)  {
        printf("\n(");
        this->left->print_formated_dbtree();

        if( (this->right->node_state == I_NODE) ||
            ( (this->right->node_state == LEAF) &&
              (this->right->sequences != NULL) )  )  {
            printf(",");
            this->right->print_formated_dbtree();
        }
        printf("\n)");
    }

    if(this->node_state == LEAF)  {
        if(this->sequences) {
            ptr = this->sequences;

            printf("\n      '%s'", ptr->name);      //schreibe erste Seq
            printf(" :0.00001");
            ptr = ptr->next;

            for(ptr; ptr; ptr = ptr->next) {        //die naechsten Seqs
                printf(",\n     '%s'", ptr->name);
                printf(" :0.00001");
            } //endfor
        } //endif
    } //endif


} // end AC_TREE::print_formated_dbtree()
// *********************************************************************






// *********************************************************************
//
//      class AC_TREE::make_clustertree()
//
//              hier wird rekursiv der Clusterbaum aufgebaut.
//              Die Sequenzliste am root Knoten ist nicht leer.
//
// *********************************************************************
void    AC_TREE::make_clustertree()  {

    if ( (this->node_state == I_NODE) ) {
        split();        // Verteile Liste auf die Soehne
        // Setzt node_state auf LEAF als Abbruchsbedingung
    }

    if ( (this->node_state == I_NODE) ) {
        this->left->make_clustertree();
        this->right->make_clustertree();
    }

} // end AC_TREE::make_clustertree()
// *********************************************************************





// *********************************************************************
//      class AC_TREE::split()
//
//              bestimme LEFTEND
//              bestimme Distanzen zum LEFTEND (real/relativ)
//              bestimme RIGHTEND
//              bestimme        Distanzen zum RIGHTEND (real/relativ)
//
//              bestimme        Schnittpunkt auf der Leftend <-> Rightendachse!!!
//
//              legen den Schnittpunkt fest
//              generiere zwei neue Knoten
//              teile die Sequenzliste auf die Knoten auf
//
// *********************************************************************
void    AC_TREE::split()  {

    //---------------------------------------------------- leftend
    this->leftend  = get_high_quality_sequence();
    this->leftend->state = LEFTEND;
    this->determine_distances_to(leftend, LEFTEND); // real, relative

    //--------------------------------------------------- rightend
    this->rightend = get_seq_with_max_dist_to();

    if( this->rightend != NULL )  {
        this->rightend->state = RIGHTEND;
        this->determine_distances_to(rightend, RIGHTEND); // real,relative

#ifdef DEBUG_print_rightdistances
        print_rightdistances(this);
#endif
#ifdef GNUPLOT_both_relativedistances
        print_both_relativedistances(this);
        printf("\n\n");
#endif
        //------------------------------------------------------------ relationship
        this->determine_basepoints();   // basepoints

        //---------------------------------------------- zwei neue Soehne = I_Nodes
        AC_TREE *inode_left = new AC_TREE;
        this->left = inode_left;
        AC_TREE *inode_right    = new AC_TREE;
        this->right     = inode_right;
        //-------------------------------------- Aufteilen der Liste auf die Soehne
        this->divide_sequence_list(this, inode_left, inode_right);


        //========================== Abbruchsbedingung fuer Seqenz-Listenaufteilung
        if( (this->left->max_relationship <= breakcondition_relationship_distance) ||
            (this->left->nsequences <= breakcondition_min_sequencenumber) ) {
            this->left->node_state = LEAF;
        }
        if( (this->right->max_relationship <= breakcondition_relationship_distance) ||
            (this->right->nsequences <= breakcondition_min_sequencenumber) ) {
            this->right->node_state = LEAF;
        }
        //=========================================================================

    } //endif

    else {  this->node_state = LEAF;        // kein rightend gefunden
    }

} // end AC_TREE::split()
// *********************************************************************




// *********************************************************************
//
//      class AC_TREE::divide_sequence_list()
//
//              allokiert Speicher fuer das Sortier-Array
//              liest die Werte in das Array ein
//
//
// *********************************************************************
void AC_TREE::divide_sequence_list(AC_TREE *that, AC_TREE *inode_left,
                                   AC_TREE *inode_right)  {

    //------------------------------------------------------- allokieren memory sortarray[]
    AC_SEQUENCE_INFO  **sort_array;
    long              number_of;
    number_of = this->nsequences;
    sort_array = (AC_SEQUENCE_INFO **)calloc(sizeof(void *),number_of);

#ifdef DEBUG_print_unsorted_array
    printf("\n\nunsorted array: \n\n");
#endif
    //------------------------------------------------------ einlesen der seqenptr in array
    AC_SEQUENCE_INFO  *ac_sequence_info;
    AC_SEQUENCE_INFO  *seq_ptr;
    AC_SEQUENCE_INFO  **array_ptr = sort_array;

    ac_sequence_info = this->sequences;

    for(seq_ptr = ac_sequence_info; seq_ptr; seq_ptr = seq_ptr->next) {
        *array_ptr= seq_ptr;

#ifdef DEBUG_print_unsorted_array
        //      printf("%f\n", (*array_ptr)->relationship);
        printf("%f      ", (*array_ptr)->relationship);
#endif
        array_ptr++;
    } // end for
    //-------------------------------------------------- ende einlesen der seqenptr in array

#ifdef DEBUG_print_unsorted_array
    printf("\n\n");
#endif

    //------------------------------------------------------------------ sortieren des array
    //==================================================================
    GB_mergesort((void **) (sort_array), 0, number_of, seq_info_cmp,0);
    //==================================================================
    //------------------------------------------------------------------------ end sortieren



    // zum GNUPLOT Ausdruck
    long dekrement;
#ifdef GNUPLOT_print_sorted_array
    //      long    dekrement;
    dekrement = number_of;
    printf("\n\n#GNU sorted array: \n\n");
    for(array_ptr = sort_array; 0 < dekrement; array_ptr++, dekrement--)  {
        printf("%f      \n", (*array_ptr)->relationship);
        //              printf("%f      10\n", (*array_ptr)->relationship);
        //              printf("%f      ", (*array_ptr)->relationship);
    }
    printf("\n\n");
#endif

#ifdef GNUPLOT_print_sorted_left_right_reldist
    //      long dekrement;
    dekrement = number_of;
    printf("\n\nsorted array relative left/right distances : \n\n");
    for(array_ptr = sort_array; 0 < dekrement; array_ptr++, dekrement--)  {
        printf("%f ",
               (*array_ptr)->relative_leftdistance);
        printf("%f\n",
               (*array_ptr)->relative_rightdistance);
    }
    printf("\n\n");
#endif


    //----------------------------------------------------------------- Schnittpunktbestimmung
    double intersection_value = get_intersection_expglaettg(sort_array, number_of);

    //------------------------------------------------------ Umhaengen der Seqs auf die Soehne
    separate_sequencelist(sort_array, intersection_value);

    //--------------------------------------------------------- Zuruecksetzen der Struct-Werte
    this->left->reset_AC_SEQUENCE_INFO_struct_values();
    this->right->reset_AC_SEQUENCE_INFO_struct_values();

    free (sort_array);

} // end AC_TREE::divide_sequence_list()
// *********************************************************************



// *********************************************************************
//
//      class AC_TREE::separate_sequencelist()
//
//              geht das sortierte array durch und haengt den ersten Teil an
//              den linken Knoten,
//              den zweiten Knoten, d.h. das Element das den intersection_value
//              liefert sammt Rest an den zweiten Knoten.
//              Zu merken ist der Verwandtschaftsgrad der beiden Seqs zu den
//              aktuellen Referenz-Sequenzen.
//
//
//
// *********************************************************************
void AC_TREE::separate_sequencelist(AC_SEQUENCE_INFO **sort_array,
                                    double intersection_value )
{
    AC_TREE          *inode;
    AC_SEQUENCE_INFO **array_ptr = sort_array;
    long    dekrement = this->nsequences;
    double  basepoint;

    for( array_ptr,dekrement; 0 < dekrement; array_ptr++, dekrement--) {
        basepoint = (*array_ptr)->relationship;

        if( (*array_ptr)->state != ORDINARY ) {
            if( (*array_ptr)->state == LEFTEND ) {          // LEFTEND nach links
                inode = this->left;
            }
            else if( (*array_ptr)->state == RIGHTEND ) {    // RIGHTEND nach rechts
                inode = this->right;
            }
        }

        else {                                  // state == ORDINARY

            if( (basepoint < intersection_value) || (0 == basepoint) ) {
                inode = this->left;
            }
            else if( intersection_value == basepoint ) {
                inode = this->right;
            }
            else if( basepoint >= intersection_value) {
                inode = this->right;
            }
        }

        inode->insert(*array_ptr);      // einfuegen des listenelements nach left oder right
        //              inode->nsequences++;            // wird bereits von insert() erledigt !

        if( basepoint < intersection_value ) {  // Abbruchsbedingung groesster Abstand zu
            this->left->max_relationship = basepoint;       // LEFTEND/RIGHTEND
        }
        else {
            this->right->max_relationship = abs(100 - intersection_value);
        }
    }
    this->sequences = NULL;                 // sicher ist sicher !!!
    // d.h. am aktuellen Knoten haengt
    // keine !! Sequenzenliste mehr

#ifdef DEBUG_print_nodeinfo
    printf("\n\n");
    printf("\n***************************************************************************\n");
    printf("Status des Knotens:                     %d\n", this->node_state);
    printf("Status des Knotens:             %d              ", this->left->node_state);
    printf("%d", this->right->node_state);
    printf("\n\n");


    printf("Anzahl Seqs im Knoten:                  %d\n", this->nsequences);
    printf("Anzahl Seqs LEFT/RIGHT:         %d              ", this->left->nsequences);
    printf("%d", this->right->nsequences);

    printf("\n\n");
    printf("Relationship zu LEFT/RIGHT:     %f      ", this->left->max_relationship);
    printf("%f\n", this->right->max_relationship);



    printf("\n\n\n");
#endif



#ifdef DEBUG_print_subnode_list
    AC_SEQUENCE_INFO        *subnode_list;
    printf("\n\n");
    subnode_list = this->left->sequences;
    for( subnode_list; subnode_list; ) {
        printf("Seq_Nr : %d             ", subnode_list->number);
        printf("left_side : %f\n", subnode_list->relationship);
        subnode_list = subnode_list->next;
    }
    printf("\n\n");
    subnode_list = this->right->sequences;
    for( subnode_list; subnode_list; ) {
        printf("Seq_Nr : %d             ", subnode_list->number);
        printf("right_side : %f\n", subnode_list->relationship);
        subnode_list = subnode_list->next;
    }
    printf("\n\n");
#endif

} // end separate_sequencelist()
// *********************************************************************
