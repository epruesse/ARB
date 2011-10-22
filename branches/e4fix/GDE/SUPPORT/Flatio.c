/* #include <malloc.h> */
#include <stdio.h>
#define TRUE 1
#define FALSE 0
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

struct data_format
{
	int length;
	char *nuc;
	int offset;
	char name[64];
	char type;
};


ErrorOut(code,string)
int code;
char *string;
{
        if (code == 0)
        {
                fprintf(stderr,"Error:%s\n",string);
                exit(1);
        }
        return 0;
}

char *Calloc2(count,size)
     int count,size;
{
        char *temp;

        temp = (char*)calloc(count,size);
        if(temp == NULL)
        {
                fprintf(stdout,"Error in Calloc\n");
                exit(-1);
        }
        else
                return(temp);
}

char *Realloc(block,size)
char *block;
int size;
{
    char *temp;
    temp =(char*)realloc(block,size);
    if(temp == NULL)
    {
                fprintf(stdout,"Error in Calloc\n");
                exit(-1);
    }
    else
        return(temp);
}



int ReadFlat(file,align,maxseqs)
FILE *file;
struct data_format align[];
int maxseqs;
{
	int j,len=0, count=-1,offset;
	unsigned maxlen = 1024;
	char Inline[1025];

	if(file == NULL)
            ErrorOut(0, "Cannot open data file");

	for(;fgets(Inline,1024,file) != NULL;)
	{
		Inline[strlen(Inline)-1] = '\0';
		switch(Inline[0])
		{
			case '>':
			case '#':
			case '%':
			case '"':
			case '@':
	                        offset = 0;
	                        for(j=0;j<strlen(Inline);j++)
	                        {
	                                if(Inline[j] == '(')
	                                {
	                                        sscanf((char*)
	                                                (Inline+j+1),"%d",
							&offset);
	                                        Inline[j] = '\0';
	                                }
	                        }

				if(count != -1)
				{
					align[count].length = len;
					align[count].nuc[len] = '\0';
					maxlen = len;
				}

				count++;
				if(count > maxseqs)
                                    ErrorOut(0, "Sorry, alignment is too large");

				align[count].nuc = Calloc2(maxlen,sizeof(char));
				align[count].type = Inline[0];
				align[count].offset = offset;
				if( align[count].nuc == NULL)
                                    ErrorOut(0, "Calloc problem");

				sscanf((char*)(Inline+1),"%s",
					align[count].name);
				len = 0;
				break;
			default:
				if(len+strlen(Inline) > maxlen)
				{
					maxlen = (maxlen+strlen(Inline))*2;
					align[count].nuc =
					Realloc(align[count].nuc, maxlen);
				}
				for(j=0;j<strlen(Inline);j++)
					align[count].nuc[j+len] = Inline[j];
				len += strlen(Inline);
				break;
		}
	}
	if(count == -1) exit(1);

	align[count].length = len;
        align[count].nuc[len] = '\0';
	return(++count);
}


Errorout2(string)
char *string;
{
	fprintf(stderr,"%s\n",string);
	exit(1);
}

WriteData(file,data,count)
FILE *file;
struct data_format data[];
int count;
{
        int i,j;
        for(j = 0 ; j<count;j++)
        {
		if(data[j].offset)
	                fprintf(file,"\n%c%s(%d)",data[j].type,data[j].name,
			data[j].offset);
		else
	                fprintf(file,"\n%c%s",data[j].type,data[j].name);

                for(i=0;i<data[j].length;i++)
                {
                        if(i%60 == 0)
                                fputc('\n',file);
                        fputc(data[j].nuc[i],file);
                }
        }
        return 0;
}


