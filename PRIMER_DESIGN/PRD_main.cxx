#include <cstdio>
#include "PRD_Design.hxx"


using namespace std;

const char *test_seq1 = "...AUU---CUGG--U-UGAU-C-C-U-G........................";
 
const char *test_seq2 = "";
const char *test_seq3 = "";
const char *test_seq4 = "";

PrimerDesign *PD;
int main() 
{

printf("before new PD enter to continue\n");
getchar();

//    PD = new PrimerDesign(test_seq3);
//    if ( !PD->setPositionalParameters(Range(1330,1380),  Range(1650,1700),  Range(8,9),  Range(-1,-1) ) )

//   PD = new PrimerDesign(test_seq2);
//   if ( !PD->setPositionalParameters(Range(5,45),  Range(105,145),  Range(5,15),  Range(60,90) ) )

  PD = new PrimerDesign( test_seq4 );
  if ( !PD->setPositionalParameters( Range(1000,1300),  Range(4800,5000),  Range(10,20),  Range(-1,-1) ) )

  // setPositionalParameters( pos1, pos2, length, distance ) 
  {
    printf("invalid positional parameters\n");
    return 1;
  }
  // setConditionalParameters( ratio, temperature, min_dist_to_next, expand_UPAC_Codes, max_count_primerpairs, CG_factor, temp_factor ); 
  PD->setConditionalParameters(Range(20,40), Range(10,80), -1, true, 20, 0.5, 0.5);

  PD->run ( PrimerDesign::PRINT_PRIMER_PAIRS );

// printf("before delete PD enter to continue\n");
// getchar();

  delete PD;

// printf("after delete PD enter to continue\n");
// getchar();
}


