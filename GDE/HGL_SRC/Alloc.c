#include <stdio.h>
#include "global_defs.h"
/* #include <malloc.h> */

/*
*	Alloc.c
*	Memory functions for Harvard Genome Laboratory.
*	Last revised 6/3/91
*
*       Print error message, and die
*/
void ErrorOut4(code,string)
int code;
char *string;
{
        if (code == 0)
        {
                fprintf(stderr,"Error:%s\n",string);
                exit(1);
        }
        return;
}


/*
*       Calloc count*size bytes with memory aligned to size.
*	Return pointer to new block.
*/
char *Calloc(count,size)
int count,size;
/*unsigned count,size;*/
{
        char *temp;
        temp = calloc(count,(unsigned)size);

	if(count*size == 0)
		fprintf(stderr,"Allocate ZERO blocks?\n");
        ErrorOut(temp,"Cannot allocate memory");
        return(temp);
}

/*
*	Reallocate memory at block, expand to size.
*	Return pointer to (possibly) new block.
*/
char *Realloc(block,size)
char *block;
unsigned size;
{
        char *temp;
        temp=realloc(block,size);
        ErrorOut(temp,"Cannot change memory size");
        return(temp);
}

/*
*	Free block Allocated by Calloc.
*	Return error code from free().
*/

void Cfree(block)
char* block;
{
	extern void Warning();
        if(block != NULL)
        {
#ifdef SUN4
	    if(free(block) == 0)
	      Warning("Error in Cfree...");
#endif
        }
/*        else
                Warning("Error in Cfree, NULL block");
*/
        return;
}



/*
*	Print Warning message to stderr.
*/
void Warning(s)
char *s;
{
	fprintf(stderr,"Warning:%s\n",s);
	return;
}


/*
*	Get array element from a sequence structure.  The index
*	is relative to the alignment.
*/
char GetElem(seq,indx)
Sequence *seq;	/*Sequence to search*/
int indx;	/*Index relative to the global offset*/
{
        if((indx<seq->offset) || (indx >= seq->offset + seq->seqlen))
		return('-');
        else
		return((char)(seq->c_elem[indx-seq->offset]));
}

/*
*	Replace the array element at seq[indx] with elem.  The index
*       is relative to the alignment.
*/

void ReplaceElem(seq,indx,elem)
Sequence *seq;	/*Sequence */
int indx;		/*Position to overwrite (replace) */
unsigned char elem;		/*Character to replace with */
{
        int j;
	extern char *Calloc();
	int width;

/*
*	If no c_elem has been allocated yet...
*/
/*	if(index("abcdefghijklmnopqrstuvwxyz-0123456789",elem)==0)
		fprintf(stderr,"Warning (ReplaceElem) elem = %c\n",elem);
*/
	width = seq->offset-indx;
	if(seq->seqlen == 0 && elem != '-')
	{
		if(seq->seqmaxlen == 0 || seq->c_elem == NULL)
		{
			seq->c_elem = Calloc(4,sizeof(char));
			seq->offset = indx;
			seq->seqmaxlen = 4;
		}
		seq->seqlen = 1;
		seq->c_elem[0] = elem;
		seq->offset = indx;
	}
/*
*	If inserting before the c_elem (< offset)
*/
        else if((indx<seq->offset) && (elem!='-'))
	{
		seq->seqmaxlen += width;
		seq->c_elem = Realloc(seq->c_elem,seq->seqmaxlen*sizeof(char));
		for(j=seq->seqmaxlen-1;j>=width;j--)
			seq->c_elem[j] = seq->c_elem[j-width];
		for(j=0;j<width;j++)
			seq->c_elem[j] = '-';
		seq->c_elem[0] = elem;
		seq->seqlen += width;
		seq->offset = indx;
	}
/*
* 	if inserting after c_elem (indx > offset + seqlen)
*/
	else if((indx>=seq->offset+seq->seqlen) && (elem!='-'))
        {
		if(indx-seq->offset >= seq->seqmaxlen)
		{
			seq->seqmaxlen = indx-seq->offset+256;
			seq->c_elem = Realloc(seq->c_elem,seq->seqmaxlen*
				sizeof(char));
		}
		for(j=seq->seqlen;j<seq->seqmaxlen;j++)
			seq->c_elem[j] = '-';
		seq->c_elem[indx-seq->offset] = elem;
		seq->seqlen = indx-seq->offset+1;
        }
	else
	{
		if(indx-(seq->offset)>=0 && indx-(seq->offset)<seq->seqlen)
                	seq->c_elem[indx-(seq->offset)] = elem;
		else if(elem!='-')
			fprintf(stderr,"%c better be a -\n",elem);
	}
        return;
}


/*
*	InsertElem is a modification of InsertElems, and should be
*	optimized.		s.s.5/6/91
*/
int InsertElem(a,b,ch)
Sequence *a;    /* Sequence */
int b;          /*Position to insert BEFORE*/
char ch;       /*element to insert */
{
    char c[2];
    c[0]=ch;
    c[1] = '\0';

    return (InsertElems(a,b,c));
}


/*
*	Make a copy of Sequence one, place in Sequence two
*/
void SeqCopy(one,two)
Sequence *one,*two;
{
	int j;
	*two = *one;
	if(two->seqmaxlen)
		two->c_elem = Calloc(one->seqmaxlen,sizeof(char));
	if(two->commentsmaxlen)
		two->comments = Calloc(one->commentsmaxlen,sizeof(char));
	for(j=0;j<one->seqlen;j++)
		two->c_elem[j] = one->c_elem[j];
	for(j=0;j<one->commentslen;j++)
		two->comments[j] = one->comments[j];
	return;
}


/*
*	Normalize seq (remove leading indels in the c_elem;
*/
void SeqNormal(seq)
Sequence *seq;
{
	int len,j,shift_width,trailer;
	char *c_elem;
	len = seq->seqlen;

	c_elem = seq->c_elem;

	if(len == 0) return;

	for(shift_width=0; (shift_width<len) && (c_elem[shift_width] == '-');
		shift_width++);

	for(j=0;j<len-shift_width;j++)
		c_elem[j] = c_elem[j+shift_width];

	seq->seqlen -= shift_width;
	seq->offset += shift_width;
	for(trailer=seq->seqlen-1;(c_elem[trailer] =='-' ||
		c_elem[trailer] == '\0') && trailer>=0;
		trailer--)
			c_elem[trailer] = '\0';
	seq->seqlen = trailer+1;
	return;
}

void SeqRev(seq,min,max)
Sequence *seq;
int min,max;
/*
	SeqRev will reverse a given sequence within a window from
	min to max (inclusive).  The idea is to allow several sequences
	to be reversed in such a manner as to allow them to remain aligned.

	    BEFORE				    AFTER
    min |           | max		    min |	    |max
	aaaacccgggttt				tttgggcccaaaa
	aaa-cccg-g				---g-gccc-aaa
	----cccgggt				--tgggccc
*/
{
	int j;
	char temp1,temp2;
	extern char GetElem();
	extern void ReplaceElem();

	for(j=0;j<= (max-min)/2;j++)
	{
		temp1 = GetElem(seq,min+j);
		temp2 = GetElem(seq,max-j);
		ReplaceElem(seq,min+j,(unsigned char)temp2);
		ReplaceElem(seq,max-j,(unsigned char)temp1);
	}

	seq->direction *= -1;

	SeqNormal(seq);
	return;
}


/* sequence complementing. */
void SeqComp(seq)
Sequence *seq;
{
        int j;
	unsigned char in,out,case_bit;
	char *c;
        static int tmatr[16] = {'-','a','c','m','g','r','s','v',
                        't','w','y','h','k','d','b','n'};

        static int matr[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x01,0x0e,0x02,0x0d,0,0,0x04,0x0b,0,0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,
        0x08,0x08,0x07,0x09,0x00,0x0a,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,
        0x0b,0,0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,0x08,0x08,0x07,0x09,0x00,0x0a,
        0,0,0,0,0x00,0
        };

	c = seq->c_elem;
	for(j=0;j<seq->seqlen;j++)
	{
/*
*	Save Case bit...
*/
	        case_bit = c[j] & 32;
		out = 0;
		in = matr[c[j]];
		if(in&1)
			out|=8;
		if(in&2)
			out|=4;
		if(in&4)
			out|=2;
		if(in&8)
			out|=1;

		if(case_bit == 0)
		  c[j] = toupper(tmatr[out]);
		else
		  c[j] = tmatr[out];
	}

	seq->direction *= -1;
	seq->strandedness = ( seq->strandedness == 2)?1:
	                    ( seq->strandedness == 1)?2:
                            0;
	return;

}
