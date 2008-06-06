#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "ac.hxx"







char *AC_SEQUENCE_LIST::alignment_name = 0;


// **********************************************************************
//  function
//
//      liest die Sequenzen der DB ein
//      allokiert Speicher fuer Sequence
//      allokiert Speicher fuer Struct
//      bestimmt die Qualitaet der Sequence
//      nummeriert die eingelesenen Sequencen durch
//      haengt die Sequence am Ende der Liste an
//      bestimmt die Anzahl aller Listenelemente am ROOT-Knoten
// **********************************************************************
char *AC_SEQUENCE_LIST::load_all(GBDATA *gb_main, AC_DBDATA_STATE ac_dbdata_state){

    GB_transaction dummy(gb_main);
    GBDATA  *gb_species_data = GB_search(gb_main, "species_data",
                                         GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    GBDATA  *gb_species;
    GBDATA  *gb_name;
    GBDATA  *gb_data;
    AC_SEQUENCE     *ac_sequence, *sequence_work;
    AC_SEQUENCE_INFO    *new_ac_seq_info;
    int count = 0;

#ifdef COMMENT
    printf("\nStart reading DB : \n\n");
#endif

    for ( gb_species = GBT_first_species(gb_species_data);
          gb_species;
          gb_species = GBT_next_species(gb_species) ){

        gb_data = GBT_read_sequence(gb_species,alignment_name);
        if (!gb_data) continue;
        gb_name = GB_search(gb_species,"name",GB_FIND);
        if (!gb_name) continue;

        char *name = GB_read_string(gb_name);

        if (ac_dbdata_state == ALIGNED_SEQUENCES) {
            ac_sequence = new AC_SEQUENCE_ALIGNED(gb_data); // gbdata !!!!!!!
        }else{
            ac_sequence = new AC_SEQUENCE_UNALIGNED(gb_data);// gbdata !!!!!!!
        }
        new_ac_seq_info = (AC_SEQUENCE_INFO *)
            calloc(sizeof(AC_SEQUENCE_INFO),1);
        new_ac_seq_info->gb_species = gb_species;
        new_ac_seq_info->seq = ac_sequence;
        new_ac_seq_info->name = name;
        new_ac_seq_info->number = ++count;
        //--------------------------------------------------------
        sequence_work = new_ac_seq_info->seq;       // Pointer auf Class AC_SEQUENCE
        sequence_work->quality = sequence_work->get_quality();
        //--------------------------------------------------------
        this->insert(new_ac_seq_info);
        //--------------------------------------------------------

#ifdef COMMENT
        if (count % 100 == 0) {
            printf("%i ...\n",count);
        }
#endif

    } // end for

#ifdef COMMENT
    printf("\nread sequences :  %i \n\n",count);
#endif

    return 0;
} // end AC_SEQUENCE_LIST::load_all()
// **********************************************************************





// **********************************************************************
//  function    main()
// **********************************************************************
int main(int argc, char **argv){

    char        *path;
    GBDATA  *gb_main;
    //-------------------------------------- progr aufruf + prametercheck
    if(argc < 2) {
        fprintf(stderr,  "*** ERROR  :  Load Error\n");
    } else {
        path = argv[1];
    }
#ifdef COMMENT
    printf("\f");
#endif

    AC_DBDATA_STATE ac_dbdata_state = ALIGNED_SEQUENCES;

#ifdef COMMENT
    printf("\n\nSorry for waiting, I'm searching for your database\n\n");
#endif

    if(!(gb_main = GB_open(path,"rw"))) {
        GB_print_error();
        return -1;
    }
    //-------------------------------------------------------- db oeffnen
    GB_begin_transaction(gb_main);
    //------------------------------------------------- db daten einlesen
    AC_TREE *root = new AC_TREE;
    root->load_all(gb_main, ac_dbdata_state);   // baut liste

    //-------------------------------------------------------------------
    if( 0 < root->nsequences ) {
        root->make_clustertree();
    }
    else {
        printf("\n\n*****  EXIT - Nothing to compress!\n\n");
        return 0;
    }


#ifdef PRINT_TREE
    printf("\n");
    root->print_tree();
    printf("\n");
#endif
#ifdef PRINT_FORMATED_DBTREE
    printf("\n");
    root->print_formated_dbtree();
    printf("\n");
#endif




    GB_commit_transaction(gb_main);

    GB_close(gb_main);

    root->remove_tree();        // gibt speicher wieder frei

    return 0;
} // end main()
// **********************************************************************
