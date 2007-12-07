/* ----------------------------------------------------------------------------
   Include-Dateien
---------------------------------------------------------------------------- */

#include "a3_basen.h"

/* ----------------------------------------------------------------------------
   Globale Variable
---------------------------------------------------------------------------- */

const int BCharacter[BASEN] =
{
    'A', 'C', 'G', 'U', 'n', '.'
};

const double BComplement[BASEN][BASEN] =
{
/*      A    C    G    U    n    .  */
/*  A                               */
      { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
    
/*  C                               */
      { 0.0, 0.0, 1.5, 0.0, 0.0, 0.0 },
/*  G                               */
      { 0.0, 1.5, 0.0, 0.8, 0.0, 0.0 },
/*  U                               */
      { 1.0, 0.0, 0.8, 0.0, 0.0, 0.0 },
/*  n                               */
      { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 },
/*  .                               */
      { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
};

const Base BIndex[CHARS] =
{
    /* 00       01       02       03       04       05       06       07    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 08       09       0A       0B       0C       0D       0E       0F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 10       11       12       13       14       15       16       17    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 18       19       1A       1B       1C       1D       1E       1F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 20       21       22       23       24       25       26       27    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 28       29       2A       2B       2C       -        .        2F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INSERT,  ANY,     INVALID,
    /* 30       31       32       33       34       35       36       37    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 38       39       3A       3B       3C       3D       3E       3F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 40       A        42       C        44       45       46       G     */
       INVALID, ADENIN,  INVALID, CYTOSIN, INVALID, INVALID, INVALID, GUANIN,
    /* 48       49       4A       4B       4C       4D       4E       4F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 50       51       52       53       T        U        56       57    */
       INVALID, INVALID, INVALID, INVALID, URACIL,  URACIL,  INVALID, INVALID,
    /* 58       59       5A       5B       5C       5D       5E       5F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 60       61       62       63       64       65       66       67    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 68       69       6A       6B       6C       6D       n        6F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, ONE,     INVALID,
    /* 70       71       72       73       74       75       76       77    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 78       79       7A       7B       7C       7D       7E       7F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 80       81       82       83       84       85       86       87    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 88       89       8A       8B       8C       8D       8E       8F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 90       91       92       93       94       95       96       97    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* 98       99       9A       9B       9C       9D       9E       9F    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* A0       A1       A2       A3       A4       A5       A6       A7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* A8       A9       AA       AB       AC       AD       AE       AF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* B0       B1       B2       B3       B4       B5       B6       B7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* B8       B9       BA       BB       BC       BD       BE       BF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* C0       C1       C2       C3       C4       C5       C6       C7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* C8       C9       CA       CB       CC       CD       CE       CF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* D0       D1       D2       D3       D4       D5       D6       D7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* D8       D9       DA       DB       DC       DD       DE       DF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* E0       E1       E2       E3       E4       E5       E6       E7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* E8       E9       EA       EB       EC       ED       EE       EF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* F0       F1       F2       F3       F4       F5       F6       F7    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
    /* F8       F9       FA       FB       FC       FD       FE       FF    */
       INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
};
