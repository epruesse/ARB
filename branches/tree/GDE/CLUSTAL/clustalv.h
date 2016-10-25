/*********************CLUSTALV.H*********************************************/
/****************************************************************************/

   /*
   Main header file for ClustalV.  Uncomment ONE of the following 4 lines
   depending on which compiler you wish to use.
   */

/*#define VMS 1  	       	 VAX VMS */

/*#define MAC 1		 	Think_C for MacIntosh */

/*#define MSDOS 1	 	Turbo C for PC's */

#define UNIX 1	 	/* Ultrix/Decstation or Gnu C for Sun */

/***************************************************************************/
/***************************************************************************/

#include "general.h"

#define MAXNAMES		12	/* Max chars read for seq. names */
#define MAXTITLES		60      /* Title length */
#define FILENAMELEN 	256             /* Max. file name length */
	
#define UNKNOWN   0
#define EMBLSWISS 1
#define PIR 	  2
#define PEARSON   3

#define PAGE_LEN       22   	/* Number of lines of help sent to screen */


#define DIRDELIM '/'
#define MAXLEN		15000
#define MAXN		300
#define FSIZE	 	25000
#define LINELENGTH     	60
#define GCG_LINELENGTH 	50
