#ifndef PRD_DESIGN_HXX
#define PRD_DESIGN_HXX

class PrimerDesign {
private:
  const char* sequence;

  // Laenge der Primer (min,max)

  // range fuer jeden der beiden Primer

  //range1, range2

  // Fester Abstand der beiden Primer (optional)

  //dist_min  // (-1 = ignorieren)
  //dist_max

  // GC Verhaeltniss (min, max)  ((G+C)/Basen)
  // Temperature (min, max)    (Temperature=4*(G+C) + 2(A+T/U))

  // Eindeutigkeit

  bool unique;
  int min_dist; // falls nicht eindeutig
  int max_mismatches;

public:
  PrimerDesign(const char *sequence_, ...) {}
  virtual ~PrimerDesign() {}


};


#else
#error PRD_design.hxx included twice
#endif // PRD_DESIGN_HXX
