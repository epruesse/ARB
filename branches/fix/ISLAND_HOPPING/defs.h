// ============================================================= //
//                                                               //
//   File      : defs.h                                          //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef DEFS_H
#define DEFS_H

#define BUFFERLENGTH 200000

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define INTEGER_MAX        2147483648
#define REAL_MAX           1.7976931348623157E+308
#define REAL_MIN           2.2250738585072014E-308
#define LOG_REAL_MIN       -708.396

#define INTEGER_MAX2EXP    31
#define REAL_MAX2EXP       1024
#define REAL_MIN2EXP       -1022

#define LOG2               0.6931471805599453094172321214581765680755
#define PI                 3.141592653589793238462643383279502884197

#define Make(x,ref)                ((x)->t=ELEMENT,(x)->r=ref)
#define MakeList(x)                ((x)->t=ELEMENT,(x)->r=List_R)
#define MakeInteger(x,val)         ((x)->t=INTEGER,(x)->u.v.i=val)

#define Is(x,ref)                  ((x)->t==ELEMENT&&(x)->r==ref)
#define IsList(x)                  ((x)->t==ELEMENT&&(x)->r==List_R)
#define IsInteger(x)             ((x)->t==INTEGER)
#define IsReal(x)                ((x)->t==REAL)
#define IsNumber(x)              ((x)->t==INTEGER||(x)->t==REAL)

#define Is0(x)                   ((x)->t==INTEGER&&(x)->u.v.i==0||(x)->t==REAL&&(x)->u.v.r==0.)
#define Is1(x)                   ((x)->t==INTEGER&&(x)->u.v.i==1||(x)->t==REAL&&(x)->u.v.r==1.)
#define IsMinus1(x)              ((x)->t==INTEGER&&(x)->u.v.i==-1||(x)->t==REAL&&(x)->u.v.r==-1.)
#define IsPositiveNumber(x)      ((x)->t==INTEGER&&(x)->u.v.i>0||(x)->t==REAL&&(x)->u.v.r>0.)
#define IsNegativeNumber(x)      ((x)->t==INTEGER&&(x)->u.v.i<0||(x)->t==REAL&&(x)->u.v.r<0.)
#define IsPositiveInteger(x)     ((x)->t==INTEGER&&(x)->u.v.i>0)
#define IsNegativeInteger(x)     ((x)->t==INTEGER&&(x)->u.v.i<0)

#define Has1Arg(x)               ((x)->l&&!(x)->l->n)
#define Has2Args(x)              ((x)->l&&(x)->l->n&&!(x)->l->n->n)

#else
#error defs.h included twice
#endif // DEFS_H
