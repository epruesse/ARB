// =========================================================== //
//                                                             //
//   File      : adtools.hxx                                   //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ADTOOLS_HXX
#define ADTOOLS_HXX

const long max_value = 0x7fffffff;

#include <arbdb++.hxx>

typedef enum {
    ADT_SEARCH_FORWARD  = 1,
    ADT_SEARCH_BACKWARD = -1,
    ADT_SEARCH_ALL      = 0
}   ADT_SEARCH_DIRECTION;


typedef enum {
    ADT_NO_REPLACE              = -1,
    ADT_REPLACE_ONLY            = 0,
    ADT_REPLACE_AND_SEARCH_NEXT = 4,
    ADT_SEARCH_AND_REPLACE_NEXT = 1,
    ADT_REPLACE_REST_SEQUENCE   = 2,
    ADT_REPLACE_REST_EDITOR     = 3
}   ADT_REPLACE_OPTION;


typedef enum {
    NO_BUTTON,
    //----------search
    BUTTON_SEARCH_FORWARD,
    BUTTON_SEARCH_BACKWARD,
    BUTTON_SEARCH_ALL,
    //----------replace
    BUTTON_NO_REPLACE,
    BUTTON_REPLACE_ONLY,
    BUTTON_REPLACE_AND_SEARCH_NEXT,
    BUTTON_SEARCH_AND_REPLACE_NEXT,
    BUTTON_REPLACE_REST_SEQUENCE,
    BUTTON_REPLACE_REST_EDITOR,
    //----------complement
    BUTTON_COMPL_BLOCK,
    BUTTON_COMPL_REST_SEQUENCE,
    BUTTON_COMPL_SEQUENCE,
    BUTTON_COMPL_REST_TEXT,
    BUTTON_COMPL_ALL
}   ADT_BUTTON;

typedef enum {
    ADT_DONT_STOPP_REPLACE = 1,
    ADT_STOPP_REPLACE      = 0
}   ADT_REPLACE_LOOP;

typedef enum {
    ADT_DB_CLOSED = 0,
    ADT_DB_OPEN   = 1
}   ADT_DB_STATUS;

typedef enum {
    ADT_ALITYPE_UNDEFINED,
    ADT_ALITYPE_RNA,
    ADT_ALITYPE_DNA,
    ADT_ALITYPE_AMI
}   ADT_ACID;

typedef enum {
    NO  = 0,
    YES = 1
}   ADT_UTIL;

#define GAP_SYMBOL   80         // zeichen, ausser NULL
#define NOGAP_SYMBOL 70

/***************************************************************************
class ADT_COMPLEMENT

***************************************************************************/

class ADT_COMPLEMENT {
    friend      class ADT_SEQUENCE;

    //**********
private:
    char *char_array;
    char *make_char_array();    // vollst. Zeichensatz

    //**********
public:

    const char *species_name;
    const char *alignment_type;
    const char *alignment_name;
    char       *sequence;
    char       *sequence_buffer;

    long *index_buffer;

    ADT_ACID   adt_acid;
    ADT_UTIL   complement_seq;
    ADT_UTIL   invert_seq;
    ADT_UTIL   seq_is_complemented;
    ADT_UTIL   seq_is_inverted;
    ADT_UTIL   take_cursor;
    ADT_UTIL   take_borders;
    ADT_UTIL   remove_gaps_points;
    ADT_BUTTON which_button;

    long alignment_length;
    long sequence_length;
    long left_border;
    long right_border;

    ADT_COMPLEMENT(void);
    ~ADT_COMPLEMENT(void);

    AD_ERR *complement_compile(void); //veraendert Zeichensatz
    AD_ERR *complement_buffers(void); //Puffer fuer Seq + Index

};


/***************************************************************************
class ADT_EDIT

***************************************************************************/

class ADT_EDIT {
    friend      class ADT_SEQUENCE;

public:
    long selection;             //Sequenz markiert
    long found_matchp;          //Matchpattern gefunden?
    long actual_cursorpos_editor;
    long replace_overflow;
    long mistakes_found;        //Anzahl gefundener Fehler
    long seq_equal_match;

    ADT_DB_STATUS db_status;
    ADT_UTIL      border_overflow;

    ADT_EDIT(void);
    ~ADT_EDIT(void);

};


/***************************************************************************
class ADT_SEARCH

***************************************************************************/
class ADT_SEARCH {
    friend      class ADT_SEQUENCE;
private:

    char *matchpattern_buffer;
    char *show_search_array();
    char *search_array;
    char *save_sequence;        // Sicherungskopie der Sequenz
    // fuer Fehlerfall bei Replace

public:
    char *seq_anfang;           // Sequenz oder Sequenz-Puffer
    long *seq_index_start;      // Index-Puffer
    char *matchpattern;         //? wildcard;
    // i'm the father of this string
    char *replace_string;       // Replace String
    long  mistakes_allowed;     // Anzahl der beim Vergleich
    // Sequenz<->Matchpattern erlaubten
    // Fehler
    long  gaps;                 //what about gaps
    long  upper_eq_lower;       //gross/KLEIN-Schreibung
    long  t_equal_u;            //t und u gleichbehandeln
    long  search_start_cursor_pos;
    long  replace_start_cursor_pos;

    ADT_SEARCH_DIRECTION search_direction;
    ADT_REPLACE_OPTION   replace_option;
    ADT_REPLACE_LOOP     replace_loop_sequence;

    ADT_SEARCH(void);
    ~ADT_SEARCH(void);

    AD_ERR *compile(void);      //veraendert Zeichensatz

    long found_cursor_pos;      //Stelle, ab der die Sequenz mit dem
    //Matchpattern unter Beruecksichtigung
    //der "erlaubten" Fehleranzahl ueber  = 
    //einstimmt.
    long string_replace;        // 0    == kein String replaced, default;
    // 1                                 == String replaced;

};

/***************************************************************************
class ADT_ALI

****************************************************************************/

class ADT_ALI : public AD_ALI {
    char *gapsequence;          // realsequenz mit - an ausgeblendeten stellen
    int  *gapshowoffset;        // fuer umsetzung show -> realcoord
    int  *gaprealoffset;
    int   gapshowoffset_len;
    int   inited;

    // es gilt:
    // showoffset[showoffset[showpos]] + showpos = realpos
    // realpos - gap[realoffset[realpos]] = showpos
    // fuer die gap verwaltung


public:
    ~ADT_ALI();
    ADT_ALI();

    void init(AD_MAIN *ad_main);

    void    gap_init();
    AD_ERR *gap_make(int position,int len);
    AD_ERR *gap_delete(int showposition);
    AD_ERR *gap_update(char *oldseq,char *newseq);

    int gap_inside(int showposition1,int showposition2);
    int gap_inside(int realposition);
    int gap_behind(int showposition);
    int gap_realpos(int showposition);
    int gap_showpos(int realposition);


    char *gap_make_real_sequence(char *realseq,const char *showseq);
    char *gap_make_show_sequence(const char *realseq,char *showseq);
    void  operator = (ADT_ALI& );
};


class ADT_SEARCH_REPLACE {
    friend      class ADT_SEQUENCE;
private:

    char *matchpattern_buffer;
    char *show_search_array();
    char *search_array;
    char *save_sequence;        // Sicherungskopie der Sequenz
    // fuer Fehlerfall bei Replace

public:
    char *seq_anfang;           // Sequenz oder Sequenz-Puffer
    long *seq_index_start;      // Index-Puffer
    char *matchpattern;         //? wildcard;
    // i'm the father of this string

    char *replace_string;       // Replace String
    long  mistakes_allowed;     // Anzahl der beim Vergleich
    // Sequenz<->Matchpattern erlaubten
    // Fehler

    long gaps;                  //what about gaps
    long upper_eq_lower;        //gross/KLEIN-Schreibung
    long t_equal_u;             //t und u gleichbehandeln
    long search_start_cursor_pos; //Position, ab der die Suche in der
    //Sequenz gestartet wird.
    //search: actual_cursorpos_editor +/- 1;

    long        replace_start_cursor_pos; //Position, ab der der Replace-String
    //in die Sequenz eingefuegt wird.
    //replace: actual_cursorpos_editor;



    ADT_SEARCH_DIRECTION search_direction;
    ADT_REPLACE_OPTION   replace_option;
    ADT_REPLACE_LOOP     replace_loop_sequence;

    ADT_SEARCH_REPLACE(void);
    ~ADT_SEARCH_REPLACE(void);

    AD_ERR *compile(void);      //veraendert Zeichensatz


    long found_cursor_pos;      //Stelle, ab der die Sequenz mit dem
    //Matchpattern unter Beruecksichtigung
    //der "erlaubten" Fehleranzahl ueber =
    //einstimmt.

    long string_replace;        // 0 == kein String replaced, default;
    // 1                              == String replaced;

};

#define GAP_SYMBOL   80         // zeichen, ausser NULL
#define NOGAP_SYMBOL 70

class ADT_SEQUENCE : public AD_SEQ  {

    class ADT_ALI *adt_ali;
    int            Cursorposition;
    long           show_timestamp;

    AD_ERR      *make_sequence_buffer(ADT_SEARCH *ptr_adt_search, ADT_EDIT *prt_adt_edit);
    AD_ERR      *rewrite_from_sequence_buffer(ADT_SEARCH *ptr_adt_search, ADT_EDIT *prt_adt_edit);

public:
    ADT_SEQUENCE();
    ~ADT_SEQUENCE();

    void    init(ADT_ALI *adtali,AD_CONT * adcont);
    // SHOWSEQUENCEN
    void    show_update();      // must be called after this->update
    int     show_len();
    char   *show_get();
    AD_ERR *show_put();
    AD_ERR *show_insert(char *text,int position);
    AD_ERR *show_remove(int len,int position);
    AD_ERR *show_replace(char *text, int position);
    AD_ERR *show_command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, long &cursorpos, int & changed_flag);

    AD_ERR *show_edit_seq_search(ADT_SEARCH *ptr_adt_search, ADT_EDIT *ptr_adt_edit);
    AD_ERR *show_edit_search(ADT_SEARCH *ptr_adt_search, ADT_EDIT *ptr_adt_edit);
    AD_ERR *show_edit_replace(ADT_SEARCH *ptr_adt_search, ADT_EDIT *ptr_adt_edit);
    AD_ERR *show_edit_seq_compl(ADT_COMPLEMENT *ptr_adt_complement, ADT_EDIT *ptr_adt_edit);
    AD_ERR *show_edit_complement(ADT_COMPLEMENT *ptr_adt_complement, ADT_EDIT *ptr_adt_edit);
    AD_ERR *show_edit_invert(ADT_COMPLEMENT *ptr_adt_complement, ADT_EDIT *ptr_adt_edit);

};

#else
#error adtools.hxx included twice
#endif // ADTOOLS_HXX
