// =============================================================== //
//                                                                 //
//   File      : PRD_main.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in February 2001                    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PRD_Design.hxx"

using namespace std;

const char *test_seq1 = "...AUU---CUGG--U-UGAU-C-C-U-G........................";

const char *test_seq2 = "";
const char *test_seq3 = "";
const char *test_seq4 = "";

PrimerDesign *PD;

int main() {
    printf("before new PD enter to continue\n");
    getchar();

    PD = new PrimerDesign(test_seq4);
    if (!PD->setPositionalParameters(Range(1000, 1300),   Range(4800, 5000), Range(10, 20), Range(-1, -1))) {
        printf("invalid positional parameters\n");
        return 1;
    }
    PD->setConditionalParameters(Range(20, 40), Range(10, 80), -1, true, 20, 0.5, 0.5);
    PD->run (PrimerDesign::PRINT_PRIMER_PAIRS);

    delete PD;
}

