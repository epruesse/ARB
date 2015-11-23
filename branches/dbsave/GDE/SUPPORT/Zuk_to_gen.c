#include <stdio.h>

typedef struct Sequence
{
        int len;
        char name[80];
        char type[8];
        char *nuc;
} Sequence;


main()
{
	char a[5000],b[5000],Inline[132];
	int pos1,pos2,pos3,i,j,k,FLAG;
	Sequence pair[2];


	for(j=0;j<5000;j++)
		b[j]='-';
	FLAG = (int)gets(Inline);
	for(j=0;FLAG;j++)
	{
		FLAG = (int)gets(Inline);
		sscanf(Inline,"%d",&pos1);
		if((sscanf(Inline,"%*6c %c  %d %d %d",&(a[j]),&k,&pos2,&pos3)
		== 4) && (FLAG))
		{
			if(pos3!=0)
			{
				if(pos1<pos3)
				{
					b[pos1-1] = '[';
					b[pos3-1] = ']';
				}
				else
                		{
                		        b[pos3-1] = '[';
                		        b[pos1-1] = ']';
                		}
			}
		}
		else
		{
			pair[0].len = j;
			strcpy(pair[0].name,"HELIX");
			strcpy(pair[0].type,"TEXT");

			pair[1].len = j;
/*
			sscanf(Inline,"%*24c %s",pair[1].name);
*/
			strcpy(pair[1].name,"Sequence");
			strcpy(pair[1].type,"RNA");

			pair[0].nuc = b;
			pair[1].nuc = a;

			WriteGen(pair,stdout,2);
			for(j=0;j<5000;j++) b[j]='-';
			j = -1;
		}
	}
	exit(0);
}



WriteGen(seq,file,numseq)
Sequence *seq;
FILE *file;
int numseq;
{
        register i,j;
        char temp[14];

        for(j=0;j<numseq;j++)
                fprintf(file,"%-.12s\n",seq[j].name);

        fprintf(file,"ZZZZZZZZZZ\n");

        for(j=0;j<numseq;j++)
        {
                strcpy(temp,seq[j].name);
                for(i=strlen(temp);i<13;i++)
                        temp[i] = ' ';
                temp[i] = '\0';
                fprintf(file,"LOCUS      %-.12s %s	%d BP\n",temp,
		seq[j].type,seq[j].len);
 
                fprintf(file,"ORIGIN");
                for(i=0;i<seq[j].len;i++)
                {
                        if(i%60 == 0)
                                fprintf(file,"\n%9d",i+1);
                        if(i%10 == 0)
                                fprintf(file," ");
                        fprintf(file,"%c",seq[j].nuc[i]);
                }
                fprintf(file,"\n//\n");
        }
        return 0;
}

