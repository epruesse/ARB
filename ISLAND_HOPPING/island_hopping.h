
#ifndef awtc_island_hopping_h_included
#define awtc_island_hopping_h_included

typedef const char *GB_ERROR;

class IslandHopping {

 private:

  int alignment_length;

  int firstColumn;
  int lastColumn;

  const char *ref_sequence;   // with gaps

  const char *toAlign_sequence; // with gaps

  const char *helix; // with gaps

  char *output_sequence;

  int freqs;
  double fT;
  double fC;
  double fA;
  double fG;

  int rates;
  double rTC;
  double rTA;
  double rTG;
  double rCA;
  double rCG;
  double rAG;

  double dist;
  double supp;
  double gap;
  double thres;

 public:

  IslandHopping() {

   alignment_length = 0;

   firstColumn = 0;
   lastColumn = -1;

   ref_sequence = 0;

   toAlign_sequence = 0;

   helix = 0;

   output_sequence = 0;

   freqs=0;
   fT=0.25;
   fC=0.25;
   fA=0.25;
   fG=0.25;

   rates=0;
   rTC=4.0;
   rTA=1.0;
   rTG=1.0;
   rCA=1.0;
   rCG=1.0;
   rAG=4.0;

   dist=0.3;
   supp=0.5;
   gap=10.;
   thres=0.005;

  }

  virtual ~IslandHopping() {
   delete output_sequence;
  }

  void set_alignment_length(int len) { alignment_length = len; }

  void set_ref_sequence(const char *ref_seq) { ref_sequence = ref_seq; }

  void set_toAlign_sequence(const char *toAlign_seq) { toAlign_sequence = toAlign_seq; }

  void set_helix(const char *hel) { helix = hel; }

  void set_range(int first_col,int last_col) {
   firstColumn=first_col;
   lastColumn=last_col;
  }

  const char *get_result() const { return output_sequence; }

  GB_ERROR do_align();

};

#endif
