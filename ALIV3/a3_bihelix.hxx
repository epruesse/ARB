// -----------------------------------------------------------------------------

#ifndef _A3_BI_Helix_HXX
#define _A3_BI_Helix_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include "a3_arbdb.hxx"

// -----------------------------------------------------------------------------
//  Makros und Definitionen
// -----------------------------------------------------------------------------

#define HELIX_MAX_NON_ST        10

#define HELIX_AWAR_PAIR_TEMPLATE    "Helix/pairs/%s"
#define HELIX_AWAR_SYMBOL_TEMPLATE  "Helix/symbols/%s"

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

typedef enum
{
    HELIX_NONE,
    HELIX_STRONG_PAIR,
    HELIX_PAIR,
    HELIX_WEAK_PAIR,
    HELIX_NO_PAIR,
    HELIX_USER0,
    HELIX_USER1,
    HELIX_USER2,
    HELIX_USER3,
    HELIX_DEFAULT,
    HELIX_NON_STANDART0,
    HELIX_NON_STANDART1,
    HELIX_NON_STANDART2,
    HELIX_NON_STANDART3,
    HELIX_NON_STANDART4,
    HELIX_NON_STANDART5,
    HELIX_NON_STANDART6,
    HELIX_NON_STANDART7,
    HELIX_NON_STANDART8,
    HELIX_NON_STANDART9,
    HELIX_NO_MATCH,
    HELIX_MAX
} BI_PAIR_TYPE;

class A3_BI_Helix
{
    private:

    int     _check_pair(char left, char right, BI_PAIR_TYPE pair_type);

    public:

    struct A3_BI_Helix_entry
    {
        long            pair_pos;
        BI_PAIR_TYPE    pair_type;
        char           *helix_nr;
    }*entries;

    char       *pairs    [HELIX_MAX],
               *char_bind[HELIX_MAX];
    long        size;
    static char error[256];

            A3_BI_Helix(void);
           ~A3_BI_Helix(void);  

    char   *init    ( GBDATA *gb_main );
    char   *init    ( GBDATA *gb_main,
                      char   *alignment_name,
                      char   *helix_nr_name,
                      char   *helix_name);
    char   *init    ( GBDATA *gb_helix_nr,
                      GBDATA *gb_helix,
                      long    size);
    char   *init    ( char   *helix_nr,
                      char   *helix,
                      long    size);
};

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
