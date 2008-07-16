#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "ac.hxx"



/*
  #ifdef COMMENT
  printf("%i      ",count);
  if (count % 100 == 0) {
  printf("%i ...\n",count);
  }
  #endif
*/


//***********************************************************************
#ifdef DEBUG_print_leftdistances

void print_leftdistances(AC_TREE *root) {

    char    *ordinary = "O";
    char    *leftend  = "L";
    char    *rightend = "R";

    printf("\n\n*** DEBUG *** :     leftdistances : \n\n");

    AC_SEQUENCE_INFO        *run_ptr = NULL;
    for( run_ptr = root->sequences; run_ptr ;
         run_ptr = run_ptr->next )
    {
        printf("Nr: %5i  ", run_ptr->number);
        printf("| ");
        if(run_ptr->state == ORDINARY) printf("%s | ", ordinary);
        if(run_ptr->state == LEFTEND) printf("%s | ", leftend);
        if(run_ptr->state == RIGHTEND) printf("%s | ", rightend);

        printf("Q: %5i  ", run_ptr->seq->quality);
        printf("Dist: ");
        printf(" %4i", run_ptr->real_leftdistance);
        printf("/%f\n", run_ptr->relative_leftdistance);
    } // endfor
    printf("\n\n");

} // end print_leftdistances()

#endif
//***********************************************************************




  //***********************************************************************
#ifdef DEBUG_print_rightdistances

  void print_rightdistances(AC_TREE *root) {

      char    *ordinary = "O";
      char    *leftend  = "L";
      char    *rightend = "R";

      printf("\n\n*** DEBUG *** :     rightdistances : \n\n");

      AC_SEQUENCE_INFO        *run_ptr = NULL;
      for( run_ptr = root->sequences; run_ptr ;
           run_ptr = run_ptr->next )
      {
          printf("Nr: %5i  ", run_ptr->number);
          printf("| ");
          if(run_ptr->state == ORDINARY) printf("%s | ", ordinary);
          if(run_ptr->state == LEFTEND) printf("%s | ", leftend);
          if(run_ptr->state == RIGHTEND) printf("%s | ", rightend);

          printf("Q: %5i  ", run_ptr->seq->quality);
          printf("Dist: ");
          printf(" %4i", run_ptr->real_rightdistance);
          printf("/%f\n", run_ptr->relative_rightdistance);
      } // endfor
      printf("\n\n");

  } // end print_rightdistances()

#endif
//***********************************************************************




  //***********************************************************************
#ifdef GNUPLOT_both_relativedistances

  void print_both_relativedistances(AC_TREE *root) {

      AC_SEQUENCE_INFO        *run_ptr = NULL;
      for( run_ptr = root->sequences; run_ptr ;
           run_ptr = run_ptr->next )
      {
          if(run_ptr->state == LEFTEND) printf("LEFTEND           ");
          else if(run_ptr->state == RIGHTEND) printf("RIGHTEND    ");
          else if(run_ptr->state == ORDINARY) printf("ord         ");
          printf("%f      ", run_ptr->relative_leftdistance);
          printf("%f\n", run_ptr->relative_rightdistance);
      }

  } // end print_both_relativedistances()

#endif
//***********************************************************************
