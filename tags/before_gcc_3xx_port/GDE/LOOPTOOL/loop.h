#define PRINT_WID 550
#define PRINT_HT 750
#define BASE_TO_BASE_DIST 1.0
#define CW 1
#define CCW -1
#define CWID 5
#define CHT 6
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define NULL 0
#define PI_val 3.1415926535
#define PI_o2 PI_val / 2
#define TWO_PI 2*PI_val

/*	Text attributed	*/

#define HILITE 1
#define BOLD 2
#define ITALIC 4
#define LOLITE 8

/*	Constraint types	*/

#define ANGULAR 1
#define POSITIONAL 2
#define DISTANCE 3

/*	colors	*/
#define BLACK 0
#define MAGENTA 1
#define YELLOW 2
#define BLUE 3
#define CYAN 4
#define GREEN 5
#define RED 6
#define WHITE 7

#define Max(a,b) (a)>(b)?(a):(b)
#define Min(a,b) (a)<(b)?(a):(b)
#define distance(a,b,c,d) sqrt( ((a)-(c))*((a)-(c))+((b)-(d))*((b)-(d)) )
#define sqr(a) ((a)*(a))

typedef struct distanceconstraint
{
	double dist;		/*distance*/
	int pair;		/*pairing partner*/
} Dcon;

typedef struct positionalconstraint
{
	double dx,dy;		/*delta x and y*/
	int pair;		/*pairing partner*/
} Pcon;

typedef struct labelstruct
{
	double dist,dx,dy,x,y;
	char text[80];
	int distflag;
} Label;


typedef struct basestruct
{
	double x,y;		/*position*/
	int rel_loc;		/*location in alignment*/
	int known;		/*is position known?*/
	int pair;		/*pase pair*/
	char nuc;		/*this base (a,g,c,t,u etc)*/
	int dir;		/*direction of traversal*/
	int attr;		/*text attributes*/
	int size;		/*font size*/
	Dcon dforw,dback;	/*distance constraint*/
	Pcon *pos;		/*positional constraints*/
	int posnum;		/*number of positional constraints*/
	int depth;		/*depth of helix nesting*/
	Label *label;		/*label for nucc*/
} Base;

typedef struct sequencestruct
{
	int len;		/*sequence len*/
	char *name;		/*sequence name*/
	char *sequence;		/*base list*/
}Sequence;

typedef struct datasetstruct
{
	int siz;		/*number of taxa (plus helix)*/
	int len;		/*length of ALIGNMENT in bases*/
	Sequence helix;		/*helix information*/
	Sequence taxa[4096];	/*individual taxa*/
} DataSet;

typedef struct loopstackstruct
{
	double dist;		/*distance to LAST base*/
	int nucnum;		/*nucleotide index*/
} LoopStak;

