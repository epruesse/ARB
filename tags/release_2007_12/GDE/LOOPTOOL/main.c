#include <stdio.h>
#include <string.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cursor.h>
#include <xview/panel.h>
#include "loop.h"
#include "globals.h"

main(ac,av)
int ac;
char **av;
{
	extern /* XView CONVERSION - Due to a name space clash with Xlib, the SunView data-type Window is now Xv_Window in XView. */WindowSetup();
	int j;

	seqnum= -1;
	infile=NULL;
	seqlen=0;
	constraint_state = DISTANCE;
	modified=TRUE;
	sho_con = FALSE;

	switch (ac)
	{
		case 1:
			fprintf(stderr,"Usage: %s [-t template] datafile\n",
			av[0]);
			exit(1);
			break;

		case 2:
			ErrorOut("Cannot open sequence file",
			(infile=fopen(av[ac-1],"r")));
			ReadSeqSet(&dataset);
			break;

		case 3:
			if(av[1][0] == '-' && av[1][1] == 't')
			{
				ErrorOut("Cannot open template file",
				(tempfile=fopen(av[2],"r")));
			}
			else
                        {
                                fprintf(stderr,
                                "Usage: %s [-t template] datafile\n",av[0]);
                                exit(1);
                        }
			break;

		case 4:
			if(av[1][0] == '-' && av[1][1] == 't')
                        {
                                ErrorOut("Cannot open template file",
                                (tempfile=fopen(av[2],"r")));
                        }
			else
			{
				fprintf(stderr,
				"Usage: %s [-t template] datafile",av[0]);
				exit(1);
			}
			ErrorOut("Cannot open sequence file",
                        (infile=fopen(av[ac - 1],"r")));
			ReadSeqSet(&dataset);
			break;
	};
	WindowSetup();
	exit(0);
}


ErrorOut3(msg,code)
char *msg;
int code;
{
        if(code == 0)
        {
                fprintf(stderr,"%s \n",msg);
                exit(1);
        }
        return;
}
