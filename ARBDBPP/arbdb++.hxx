#ifndef arbdbpp_hxx_included
#define arbdbpp_hxx_included

/* KLASSENDEFINITIONEN fuer die Datenbankschnittstelle 

AD_ERR  Fehlerklasse, oft Rueckgabewert von Funktionen
        
AD_MAIN Hauptklasse, oeffnen und schliessen der Datenbank, sowie
beginn und Ende von Transaktionen

folgende Klassen entsprechen im wesentlichen den Containern der 
ARB Database. Nach der Deklaration eines Objekts muss vor einem
Funktionsaufruf init(), vor der Deallokierung ein exit()
mit den entsprechenden Parametern aufgerufen werden.

AD_SPECIES                 
AD_ALI          ->alignment
AD_CONT
AD_SEQ
AD_MARK

***********************************************************/

#ifndef _AW_KEY_CODES_INCLUDED
#include <aw_keysym.hxx>
#endif


#ifndef ARBDBPP_INCLUDED
#define ARBDBPP_INCLUDED

#ifndef GB_INCLUDED
typedef struct gb_data_base_type GBDATA;
#endif

#define ADPP_CORE *(int *)0=0;
#define AD_ERR_WARNING  1
// Punktzeichen fuer sequenz     
#define SEQ_POINT '.'   
#define ALIGN_STRING ".-~?"     // characters allowed to change in align_mod 
#define ADPP_IS_ALIGN_CHARACTER(chr) (strchr (ALIGN_STRING,chr))

const int MINCACH = 1;
const int MAXCACH = 0;  // schaltet cach ein bei open Funktionen
const int CORE = 1;

typedef enum ad_edit_modus {
    AD_allign       = 0,    // add & remove of . possible (default)
    AD_nowrite      = 1,    // no edits allowed
    AD_replace      = 2,    // all edits
    AD_insert       = 3     //
} AD_EDITMODI;

typedef enum ad_types {
    ad_none         = 0,
    ad_bit          = 1,
    ad_byte         = 2,
    ad_int          = 3,
    ad_float        = 4,
    ad_bits         = 6,
    ad_bytes        = 8,
    ad_ints         = 9,
    ad_floats       = 10,
    ad_string       = 12,
    ad_db           = 15,
    ad_type_max     = 16
} AD_TYPES;     // equal GBDATA types
/*******************************
class AD_ERR
*******************************/

class AD_ERR
{
    int modus;      // Mode  0 = Errors, 1 = Warnings
    int  anzahl;    // Number of errors
    char *text;     // error text
        
public:
    AD_ERR();               // create error
    AD_ERR(const char *errorstring); // create error
    AD_ERR(const char *,const int core); // setzt den Fehler und bricht mit core ab falls core == CORE
    ~AD_ERR();
    char *show();   // returns the error text
};


class AD_READWRITE {
    // Klasse, die es den anderen AD_Klassen ermoeglicht
    // Eintraege zu lesen und zu schreiben.
public:
    GBDATA *gbdataptr;      // jeweiliger Zeiger auf GBDATA eintrag
    // am Besten in initpntr bzw init auf jeweiligen
    // Eintrag setzen.
    char    *readstring(char *feld);
    int     readint(char *feld);
    float   readfloat(char *feld);
    AD_ERR  *writestring(char *feld,char *eintrag);
    AD_ERR  *writeint(char *feld,int eintrag);
    AD_ERR  *writefloat(char *feld,float eintrag);
    AD_ERR  *create_entry(char *key, AD_TYPES type);
    AD_TYPES read_type(char *key);
    GBDATA  *get_GBDATA(void) { return gbdataptr; };
};

/*************************
class: AD_MAIN 

***************************/
 

class AD_MAIN : public AD_READWRITE
{
    friend class AD_SPECIES; // fuer den zugriff auf gbd
    friend class AD_SAI;
    friend class AD_ALI;
    friend class AD_SEQ;
        
    GBDATA *gbd;            // Zeiger auf container der DB
    GBDATA *species_data;   // 
    GBDATA *extended_data;  // 
    GBDATA *presets;
    int     AD_fast;        // Flag fuer schnelen Zugriff 
    AD_EDITMODI      mode;  // for seq modes 
        
public:


    AD_MAIN();
    ~AD_MAIN();
        
    AD_ERR *open(const char *);
    AD_ERR *open(const char *,int );
    AD_ERR *close();
    AD_ERR *save(const char *modus);
    AD_ERR *save_as(const char *modus);
    AD_ERR *save_home(const char *modus);
    AD_ERR *push_transaction();
    AD_ERR *pop_transaction();
    AD_ERR *begin_transaction();
    AD_ERR *commit_transaction();
    AD_ERR *abort_transaction();
    AD_ERR *change_security_level(int level);
    int     time_stamp(void);

    int get_cach_flag();
};

/*******************************
class AD_ALI
container alignment
*******************************/

class AD_ALI : public AD_READWRITE
{
    friend class AD_CONT;
    
    AD_MAIN *ad_main;
    GBDATA  *gb_ali;
    GBDATA  *gb_aligned;
    GBDATA  *gb_name;
    GBDATA  *gb_len;
    GBDATA  *gb_type;
    char    *ad_name;
    long     ad_len;
    char    *ad_type;
    long     ad_aligned;


    int     count;
    int     last;
    AD_ERR *initpntr();
    AD_ERR *release();
public:
    AD_ALI();
    ~AD_ALI();

    AD_ERR *init(AD_MAIN *gbptr);
    AD_ERR *exit();
    AD_ERR *find(char*name);
    AD_ERR *first();
    AD_ERR *ddefault();
    AD_ERR *next();
    //      AD_ERR *default();

    AD_MAIN *get_ad_main() { return ad_main; };
    int     eof();          // 1 ,falls letztes alignment
    int     aligned();
    int     len();
    char *type();
    char *name();
    int time_stamp();

    void    operator = (AD_ALI& );

};


/**********************************************
AD_SPECIES
***********************************************/
class CONTLIST;

class AD_SPECIES : public AD_READWRITE
{
    GBDATA  *gb_spdata;     // um lange Zeigerzugriffe zu vermeiden
    // kopie von AD_MAIN.speciesdata
protected:
    AD_MAIN *ad_main;       // fuer DB zugriff
    GBDATA *gb_species;     // zeiger auf species
    GBDATA *gb_name;

    char *spname;           // enthaelt kopie des namens

    int last;               // 1 -> EOF
    int count;              // zaehler fuer die daranhaengenden
    // container

    AD_ERR *error();
    AD_ERR *ddefault();
    class CONTLIST *container;

    friend class AD_CONT;
    friend int AD_SPECIES_destroy(GBDATA *,AD_SPECIES *);
    friend int AD_SPECIES_name_change(GBDATA *,AD_SPECIES *);
    // Funktion die callback behandelt

    void initpntr();        // internes Aufbereiten der Klasse
    void release();         // behandelt speicherverwaltung


public:
    AD_SPECIES();
    ~AD_SPECIES();

    AD_ERR *init(AD_MAIN *);
    AD_ERR *exit();
    AD_ERR *first();
    AD_ERR *next();
    AD_ERR *firstmarked();
    AD_ERR *nextmarked();
    AD_ERR *find(const char *name);
    AD_ERR *create(const char *name);

    void    operator = (const AD_SPECIES&); // zum kopieren

    int time_stamp();

    const char      *name();
    int     flag();
    const char      *fullname();
    int     eof();
};


/**********************************************
AD_SAI
***********************************************/
class AD_SAI : public AD_SPECIES
{
    GBDATA  *gb_main;       // um lange Zeigerzugriffe zu vermeiden
    // kopie von AD_MAIN.speciesdata
public:
    AD_SAI();
    ~AD_SAI();

    int     flag();
    AD_ERR *init(AD_MAIN *);
    AD_ERR *exit();
    AD_ERR *first();
    AD_ERR *next();
    AD_ERR *firstmarked();
    AD_ERR *nextmarked();
    AD_ERR *find(char *);
    AD_ERR *create(char *name);

    char    *fullname();
    void    operator = (const AD_SAI&);     // zum kopieren
    char    *name();        // liefert name zurueck

};
/*************************************
 *AD_CONT
 ************************************/
class AD_CONT : public AD_READWRITE
                // Containerklasse in der die Sequenzinformationen fuer
                // ein Spezies gehalten und verwaltet wird
                // muss ein spezies und ein alignment objekt
                // beim initialisieren bekommen
{
    AD_SPECIES      *ad_species;
    AD_ALI          *ad_ali;

    GBDATA *gb_species; // wird 0, wenn species deleted
    GBDATA *gb_ali;         // pointer to ali_xxx container

    friend class AD_SEQ;
    friend class AD_STAT;
    // funktionen zum einfuegen in die AD_SPECIES Containerliste
    int con_insert(AD_SPECIES *,AD_ALI *);
    void con_remove(AD_SPECIES *,AD_ALI *);
    // gibt cach flag zurueck (implementiert um mehrfachzeiger
    int get_cach_flag();

public:
    AD_CONT();
    ~AD_CONT();

    AD_MAIN *get_ad_main() { return ad_ali->get_ad_main(); };
    AD_ERR *init(AD_SPECIES *,AD_ALI *);
    AD_ERR *create(AD_SPECIES *,AD_ALI *);
    int AD_CONT::eof(void);
    AD_ERR *exit();
};


// einfach verkettete Liste CONTLIST

struct CLISTENTRY {
    AD_SPECIES *entry;      // werden zum vergleich verwendet
    AD_ALI  *entry2;
    CLISTENTRY *next;
    CLISTENTRY();
};

class CONTLIST {
    // einfach verkettete liste mit AD_CONT eintraegen
    // mit der nachgeprueft werden soll welche container
    // enthalten sind.
    CLISTENTRY *first;
    int anzahl;


public:
    CONTLIST();
    ~CONTLIST();

    int insert(AD_SPECIES *,AD_ALI *);
    int element(AD_SPECIES *,AD_ALI *);
    void remove(AD_SPECIES *,AD_ALI *);
};


/*****************************
        AD_STAT
****************************/

class AD_STAT   // Sequenzklasse in der die Sequenz und ihre
// markierung gespeichert ist
{
    class AD_CONT *ad_cont;

    friend int AD_STAT_updatecall(GBDATA *,AD_STAT*);
    
    AD_TYPES  marktype;
    char     *markkey;
    char     *markdata;         // Markierungsdaten String
    float    *markdatafloat;
    GB_UINT4 *markdataint;
    long      nmark;            // anzahl der floats`

    char c_0, c_1;              // Zeichen fuer die markierung
    int  last;

    int     updated;            // flag fuer aufgetretenen update
    int     inited_object;      // fuer init/exit ueberpruefung
    GBDATA *gb_mark;            // Zeiger auf ali_xxx container
    GBDATA *gb_char_mark;
    GBDATA *GB_FLOAT_mark;
    GBDATA *GB_INT_mark;
    GBDATA *gb_markdata;        // aktuelle markierung (aller 3 typen)

    AD_ERR *initpntr();
    AD_ERR *release();
public:
    AD_STAT();
    ~AD_STAT();

    AD_ERR *init(AD_CONT *);
    AD_ERR *exit();

    AD_ERR *first();
    AD_ERR *first(AD_TYPES type);

    AD_ERR *next();
    AD_ERR *next(AD_TYPES type);

    int      *eof();
    AD_TYPES  type();
    GB_UINT4 *getint();

    char   *getbits();
    float  *getfloat();
    AD_ERR *put();
    AD_ERR *put(char *markings, int len);
    AD_ERR *put(float *markings, int len);
    AD_ERR *put(GB_UINT4 *markings, int len);
    int     time_stamp();
};


/**********************************
        AD SEQ
*********************************/
class AD_SEQ: public AD_READWRITE
              // Sequenzklasse in der die Sequenz und ihre
              // markierung gespeichert ist
{
    friend int AD_SEQ_updatecall(GBDATA *,AD_SEQ  *);
    GBDATA *gb_seq;         // zeiger auf ali_xxx container

protected:
    int     updated;        // flag fuer aufgetretenen update
    int     nseq_but_filter; // show filter, no sequence
    long    seq_len;        // hier steht die max laenge der sequenz

    char    *seq;               // Sequenzdata
    long     timestamp;         // Zeit des letzten zugriffs auf DB
    AD_CONT *ad_cont;

    AD_ERR *check_base(char chr,long position, int direction);
    AD_ERR *push(long position, int direction);
    AD_ERR *jump(long position, int direction);
    AD_ERR *fetch(long position, int direction);
    long    get_next_base(long position, int direction);
    long    get_next_gap(long position, int direction);

public:
    AD_SEQ();
    ~AD_SEQ();

    AD_ERR *init(AD_CONT *);
    AD_ERR *exit();

    AD_ERR *update();       // reread data froam database

    char        *get();         // read the sequence
    AD_ERR      *put();         // write the internal cash to the database
    AD_ERR      *create(void);
    AD_ERR      *changemode(AD_EDITMODI mod);
    AD_EDITMODI  mode();
    AD_MAIN *get_ad_main() { return ad_cont->get_ad_main(); };

    int     s_inited();
    int     time_stamp(void);
    int     len();
    // EDITIERFUNKTINEN
    // position ab 0 bis strlen(seq)
    //
    AD_ERR *insert(char *,long position, int direction);
    AD_ERR *remove(int len,long position, int direction);
    AD_ERR *replace(char *text,long position, int direction);
    AD_ERR *swap_gaps(long position, char ch);
    AD_ERR *command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, long &cursorpos, int & changed_flag);
};

#endif


#endif
