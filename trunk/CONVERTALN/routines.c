
#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ----------------------------------------------------------
*	Function count_base().
*		Count bases A, T, G, C and others.
*/
void
count_base(base_a, base_t, base_g, base_c, base_other)
int	*base_a, *base_t, *base_g, *base_c, *base_other;
{
	int	indi;

	(*base_a)=(*base_c)=(*base_t)=(*base_g)=(*base_other)=0;
	for(indi=0; indi<data.seq_length; indi++)
		switch(data.sequence[indi])	{
		case 'a':
		case 'A':
			(*base_a)++;
			break;
		case 't':
		case 'T':
		case 'u':
		case 'U':
			(*base_t)++;
			break;
		case 'g':
		case 'G':
			(*base_g)++;
			break;
		case 'c':
		case 'C':
			(*base_c)++;
			break;
		default:
			(*base_other)++;
		}
}
/* ------------------------------------------------------------------
*	Function replace_entry().
    *		Free space of string1 and replace string1 by string2.
*/
void replace_entry(string1, string2)
     char	    **string1;
     const char	 *string2;
{
/* 	void	Freespace(); */
/* 	char	*Dupstr(); */

	Freespace(string1);
	(*string1)=(char*)Dupstr(string2);

}
