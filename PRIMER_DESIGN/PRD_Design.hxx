#ifndef PRD_DESIGN_HXX
#define PRD_DESIGN_HXX

#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif
#include "PRD_Range.hxx"
#include "PRD_Node.hxx"
#include "PRD_Item.hxx"
#include "PRD_Pair.hxx"

#include <deque.h>
#include <arbdb.h>

class PrimerDesign {
private:

  // zu untersuchendes Genom

  const char* sequence;

  // Laenge der Primer (Anzahl der enthaltenen Basen)

  Range primer_length;

  // Position der Primer ( [min,max) )

  Range primer1;
  Range primer2;

  // Abstand der Primer (bzgl. der Positionen)
  // <= 0 <=> ingorieren

  Range primer_distance;

  // maximale Anzahl zurueckzuliefernder Primerpaare

  int   max_count_primerpairs;

  // CG Verhaeltniss der Primer in Prozent
  // <= 0 <=> ingorieren
  // [ Verhaeltiss = (G+C)/Basen ]

  Range CG_ratio;

  // Temperatur der Primer
  // <= 0 <=> ingorieren
  // [ Temperatur = 4*(G+C) + 2*(A+T) ]

  Range temperature;

  // Faktoren zur Gewichtung von CG-Verhaeltniss und
  // Temperatur bei der Bewertung von Primerpaaren
  // [0.0 .. 1.0]
  // !!! CG_factor + temperature_factor == 1  !!!

  double CG_factor;
  double temperature_factor;

  // Eindeutigkeit der Primer bzgl. des Genoms

  int  min_distance_to_next_match; // <= 0 <=> eindeutig
  bool expand_UPAC_Codes;          // expand UPAC-Codes while matching Sequence vs. Trees



  //
  // INTERNAL STUFF
  //
  const static int FORWARD  =  1;
  const static int BACKWARD = -1;

  // primerbaume
  Node* root1;
  Node* root2;

  // primerlisten
  Item* list1;
  Item* list2;

  // primerpaarfeld
    Pair* pairs;

    GB_ERROR error;

public:
  PrimerDesign( const char *sequence_,          \
                Range       pos1_, Range pos2_, Range length_, Range distance_, \
		Range               ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_, \
		int                 max_count_primerpairs_, double CG_factor_, double temp_factor_ );
  PrimerDesign( const char *sequence_,          \
		Range               pos1_, Range pos2_, Range length_, Range distance_, \
		int                 max_count_primerpairs_, double CG_factor_, double temp_factor_ );
  PrimerDesign( const char *sequence_ );
  ~PrimerDesign();

  bool setPositionalParameters ( Range pos1_, Range pos2_, Range length_, Range distance_ ); // true = valid parameters
  void setConditionalParameters( Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_, int max_count_primerpairs_, double CG_factor_, double temp_factor_ );

  void buildPrimerTrees ();
  void printPrimerTrees ();
  void matchSequenceAgainstPrimerTrees ();

  void convertTreesToLists ();
  void printPrimerLists    ();

  void evaluatePrimerPairs ();
  void printPrimerPairs    ();

    void run ( int print_stages_ );

    GB_ERROR get_error() const { return error; }
public:
  const static int PRINT_RAW_TREES     = 1;
  const static int PRINT_MATCHED_TREES = 2;
  const static int PRINT_PRIMER_LISTS  = 4;
  const static int PRINT_PRIMER_PAIRS  = 8;

private:
  void              init           ( const char *sequence_, Range pos1_, Range pos2_, Range length_, Range distance_, Range ratio_, Range temperature_, int min_dist_to_next_, bool expand_UPAC_Codes_, int max_count_primerpairs_, double CG_factor_, double temp_factor_ );
  PRD_Sequence_Pos  followUp       ( Node *node_, deque<char> *primer_, int direction_ );
  void              findNextPrimer ( Node *start_at_, int depth_, int *counter_, int delivered_ );
  int               insertNode     ( Node *current_, char base_, PRD_Sequence_Pos pos_, int delivered_ );
  void              calcCGandAT    ( int &CG_, int &AT_, Node *start_at_ );
  double            evaluatePair   ( Item *one_, Item *two_ );
  void              insertPair     ( double rating_, Item *one_, Item *two_ );
};


#else
#error PRD_Design.hxx included twice
#endif // PRD_DESIGN_HXX
