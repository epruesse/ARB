//#define COMMENT
//#define PRINT_TREE                    //
//#define PRINT_FORMATED_DBTREE         // ARB <*.tree> Format



//#define DEBUG_print_leftdistances
//#define DEBUG_print_rightdistances
//#define DEBUG_print_unsorted_array
//#define DEBUG_print_subnode_list
//#define DEBUG_print_nodeinfo


//#define GNUPLOT_both_relativedistances
//#define GNUPLOT_relationship
//#define GNUPLOT_print_sorted_array
//#define GNUPLOT_print_sorted_left_right_reldist
//#define GNUPLOT_print_expglaettg
//#define GNUPLOT_print_schnittpunkt_expglaettg



//**********************************************************************
  //      typedef's
  //**********************************************************************
  typedef enum {
        UNKNOWN,        // default
        ALIGNED_SEQUENCES,
        UNALIGNED_SEQUENCES
    }  AC_DBDATA_STATE;

typedef enum {
    ORDINARY,       // default
    LEFTEND,
    RIGHTEND
}  AC_SEQUENCE_INFO_STATE;

typedef enum {
    I_NODE = 1,     // default
    LEAF = 2,
    MASTER = 3
}  NODE_STATE;

typedef enum {
    FALSE,
    TRUE
}  BOOLEAN;





//**********************************************************************
  //      class   AC_SEQUENCE
  //**********************************************************************
  class AC_SEQUENCE {
  protected:
      AC_DBDATA_STATE ac_dbdata_state;
  public:
      GBDATA  *gb_data;
      char    *sequence;
      int     seq_len;
      long    quality;                // Wird beim new berechnet

      long            get_quality();
      virtual double  get_distance(AC_SEQUENCE *that)=0;


      AC_SEQUENCE(GBDATA *gb_data);
      virtual ~AC_SEQUENCE();
  };



//**********************************************************************
  //      class   AC_SEQUENCE_ALIGNED
  //**********************************************************************
  class AC_SEQUENCE_ALIGNED : public AC_SEQUENCE {
  public:
      double          get_distance(AC_SEQUENCE *that);


      AC_SEQUENCE_ALIGNED(GBDATA *gb_data);
      //r             AC_SEQUENCE_ALIGNED(char *seq);
      ~AC_SEQUENCE_ALIGNED();
  };


//**********************************************************************
  //      class   AC_SEQUENCE_UNALIGNED
  //**********************************************************************
  class AC_SEQUENCE_UNALIGNED : public AC_SEQUENCE {
  public:

      double          get_distance(AC_SEQUENCE *that);
      // diese Fkt will
      //noch programmiert werden !!! nur drin damit der Compiler
      // nicht meckert
      ////////////////////////////////////////////////////////////

      AC_SEQUENCE_UNALIGNED(GBDATA *gb_data);
      //r             AC_SEQUENCE_UNALIGNED(char *sequence);
      ~AC_SEQUENCE_UNALIGNED();
  };


//**********************************************************************
  //      struct
  //**********************************************************************
  struct AC_SEQUENCE_INFO {
      AC_SEQUENCE_INFO        *next;
      AC_SEQUENCE_INFO        *previous;
      AC_SEQUENCE             *seq;
      char                    *compressed_seq;
      GBDATA                  *gb_species;
      //r     GBDATA                  *gb_data;
      char                    *name;
      long                    number; // laufende Elementnummer (1. lesen)
      AC_SEQUENCE_INFO_STATE  state;
      long                    real_leftdistance;
      double                  relative_leftdistance;
      long                    real_rightdistance;
      double                  relative_rightdistance;
      double                  relationship;   // wird auch als "basepoint" verwendet!!!
      // schnittpunkt der Seq auf der Linie
      // zwischen leftend und rightend
  };








//**********************************************************************
  //      class  AC_SEQUENCE_LIST
  //**********************************************************************
  class AC_SEQUENCE_LIST {
  public:
      static char             *alignment_name;
      AC_SEQUENCE_INFO        *sequences;             // pointer auf Struct
      AC_SEQUENCE_INFO        *leftend;
      AC_SEQUENCE_INFO        *rightend;
      long                    nsequences;
      double                  max_relationship;


      void                    insert(AC_SEQUENCE_INFO *new_seq);
      void                    remove_sequence(AC_SEQUENCE_INFO *sequence);
      void                    remove_sequence_list(AC_SEQUENCE_INFO *sequence_list);
      char                    *load_all(GBDATA *gb_main,AC_DBDATA_STATE type);
      AC_SEQUENCE_INFO        *get_high_quality_sequence();
      void                    determine_distances_to(AC_SEQUENCE_INFO *reverence_seq,
                                                     AC_SEQUENCE_INFO_STATE reference_state);
      AC_SEQUENCE_INFO        *get_seq_with_max_dist_to();
      void                    determine_basepoints();
      void                    reset_AC_SEQUENCE_INFO_struct_values();


      AC_SEQUENCE_LIST();
      ~AC_SEQUENCE_LIST();
  };





//**********************************************************************
  //      class  AC_TREE
  //**********************************************************************
  class AC_TREE : public AC_SEQUENCE_LIST {
  public:
      NODE_STATE      node_state;             // I_NODE, LEAF, MASTER
      AC_TREE         *left;
      AC_TREE         *right;
      double          breakcondition_min_sequencenumber;
      double          breakcondition_relationship_distance;

      void            make_clustertree();
      void            remove_tree();          //Speicherfreigabe
      void            split();
      void            print_tree();
      void            print_formated_dbtree();

      void            divide_sequence_list(AC_TREE *that,
                                           AC_TREE *inode_left, AC_TREE *inode_right);
      void            separate_sequencelist(AC_SEQUENCE_INFO **sort_array,
                                            double intersection_value );


      AC_TREE();
      ~AC_TREE();
  };






//**********************************************************************
  //      TOYS_N_TOOLS Funktionen
  //**********************************************************************

  long    seq_info_cmp(void *i,void *j, char *);
double  get_intersection_of_list(AC_SEQUENCE_INFO **sort_array,
                                 double number_of);
double  get_intersection_expglaettg(AC_SEQUENCE_INFO **sort_array,
                                    double number_of);








//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//      DEBUG TOOLS
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void print_rightdistances(AC_TREE *root);               //DEBUG
void print_both_relativedistances(AC_TREE *root); //GNUPLOT
