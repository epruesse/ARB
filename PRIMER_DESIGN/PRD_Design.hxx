// =============================================================== //
//                                                                 //
//   File      : PRD_Design.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in February 2001                    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PRD_DESIGN_HXX
#define PRD_DESIGN_HXX

#include "PRD_Range.hxx"
#include "PRD_Node.hxx"
#include "PRD_Pair.hxx"

#include <deque>

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#if defined(DEBUG)
// # define DUMP_PRIMER
#endif

class PrimerDesign : virtual Noncopyable {
    // zu untersuchendes Genom
    // sequence of bases/spaces to be examined

    const char* sequence;
    long int    seqLength;

    // Laenge der Primer (Anzahl der enthaltenen Basen)
    // count of bases of primers (min/max)
    Range primer_length;

    // Position der Primer ( [min,max) )
    // left/right position of primers

    Range primer1;
    Range primer2;

    // Abstand der Primer (bzgl. der Basen dazwischen, <=0 = ignorieren)
    // min/max distance of primers (regarding number of bases between, <=0 = ignore)

    Range primer_distance;

    // maximale Anzahl zurueckzuliefernder Primerpaare
    // max. count of returned primer pairs

    int   max_count_primerpairs;

    // GC-Verhaeltniss der Primer in Prozent <=0 = ignorieren
    // GC-ratio of primers in percent <=0 = ignore
    // [ ratio = (G+C)/Basen ]

    Range GC_ratio;

    // Temperatur der Primer <=0 = ignorieren
    // temperature of primers <=0 = ignore
    // [ temperature = 4*(G+C) + 2*(A+T) ]

    Range temperature;

    // Faktoren zur Gewichtung von CG-Verhaeltniss und Temperatur bei der Bewertung von Primerpaaren
    // factors to assess the GC-ratio and temperature at the evaluation of primerpairs
    // [ 0.0 .. 1.0 ]

    double GC_factor;
    double temperature_factor;

    // wird ein Primer ausserhalb dieser Distanz nocheinmal gefunden wird das Vorkommen ignoriert ( <= 0 = eindeutig )
    // is a primer found again out of this range its occurrence is ignored ( <=0 = explicit match )

    int  min_distance_to_next_match;

    // erweitern der IUPAC-Codes beim matching der sequence gegen die primerbaeume ?
    // expand IUPAC-Codes while matching sequence vs. primertrees ?

    bool expand_IUPAC_Codes;


    //
    // INTERNAL STUFF
    //
    static const int FORWARD  =  1;
    static const int BACKWARD = -1;

    // primertrees
    Node* root1;
    Node* root2;

    // primerlists
    Item* list1;
    Item* list2;

    // primerpairs
    Pair* pairs;

    GB_ERROR error;

    unsigned long int total_node_counter_left;
    unsigned long int total_node_counter_right;
    unsigned long int primer_node_counter_left;
    unsigned long int primer_node_counter_right;

    void reset_node_counters() {
        total_node_counter_left   = 0;
        total_node_counter_right  = 0;
        primer_node_counter_left  = 0;
        primer_node_counter_right = 0;
    }

public:
    PrimerDesign(const char *sequence_, long int seqLength_,
                 Range       pos1_, Range pos2_, Range length_, Range distance_,
                  Range       ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_,
                  int         max_count_primerpairs_, double GC_factor_, double temp_factor_);
    PrimerDesign(const char *sequence_, long int seqLength_,
                  Range       pos1_, Range pos2_, Range length_, Range distance_,
                  int         max_count_primerpairs_, double GC_factor_, double temp_factor_);

    PrimerDesign(const char *sequence_, long int seqLength_);
    ~PrimerDesign();

    void setPositionalParameters (Range pos1_, Range pos2_, Range length_, Range distance_);
    void setConditionalParameters(Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_, int max_count_primerpairs_, double GC_factor_, double temp_factor_);

    void buildPrimerTrees ();
    void matchSequenceAgainstPrimerTrees ();
    void convertTreesToLists ();
    void evaluatePrimerPairs ();
    
#if defined(DUMP_PRIMER)
    void printPrimerTrees ();
    void printPrimerLists    ();
    void printPrimerPairs    ();
#endif

    void run();

    GB_ERROR get_error() const { return error; }

    PRD_Sequence_Pos  get_max_primer_length() const { return primer_length.max(); }
    PRD_Sequence_Pos  get_max_primer_pos()    const { return primer2.max(); }
    const char       *get_result(int num, const char *&primers, int max_primer_length, int max_position_length, int max_length_length) const;   // return 0 if no more results (primers is set to "leftPrimer,rightPrimer")

public:
    static const int PRINT_RAW_TREES     = 1;
    static const int PRINT_MATCHED_TREES = 2;
    static const int PRINT_PRIMER_LISTS  = 4;
    static const int PRINT_PRIMER_PAIRS  = 8;

private:
    void             init               (const char *sequence_, long int seqLength_, Range pos1_, Range pos2_, Range length_, Range distance_, Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_IUPAC_Codes_, int max_count_primerpairs_, double GC_factor_, double temp_factor_);
    PRD_Sequence_Pos followUp           (Node *node_, std::deque<char> *primer_, int direction_);
    void             findNextPrimer     (Node *start_at_, int depth_, int *counter_, int delivered_);
    int              insertNode         (Node *current_, unsigned char base_, PRD_Sequence_Pos pos_, int delivered_, int offset_, int left_, int right_);
    void             clearTree          (Node *start, int left_, int right_);
    bool             treeContainsPrimer (Node *start);
    void             calcGCandAT        (int &GC_, int &AT_, Node *start_at_);
    double           evaluatePair       (Item *one_, Item *two_);
    void             insertPair         (double rating_, Item *one_, Item *two_);
};

#else
#error PRD_Design.hxx included twice
#endif // PRD_DESIGN_HXX
