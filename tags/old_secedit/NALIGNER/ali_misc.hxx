
#ifndef _ALI_MISC_INC_
#define _ALI_MISC_INC_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#define ALI_A_CODE 	0
#define ALI_C_CODE 	1
#define ALI_G_CODE 	2
#define ALI_U_CODE 	3
#define ALI_GAP_CODE 4
#define ALI_N_CODE	5
#define ALI_DOT_CODE 6
#define ALI_UNDEF_CODE	200

/*****************************************************************************
 *
 * Some Error Funktions
 *
 *****************************************************************************/

inline void ali_message(const char *message, const char *func = "")
{
    fprintf(stdout,"%s %s\n",func,message);
}

inline void ali_warning(const char *message, const char *func = "")
{
    fprintf(stderr,"WARNING %s: %s\n",func,message);
}

void ali_error(const char *message, const char *func = "") __attribute__((noreturn));
void ali_fatal_error(const char *message, const char *func = "") __attribute__((noreturn));

inline void *CALLOC(long i,long j)	{
    char *v = (char *)malloc(i*j);
    if (!v) {
	ali_fatal_error("Out of Memory");
    }
    memset(v,0,i*j);
    return v;
}


/*****************************************************************************
 *
 * Some Converters
 *
 *****************************************************************************/

inline int ali_is_base(char c)
{
    return (c == 'a' || c == 'A' || c == 'c' || c == 'C' || 
	    c == 'g' || c == 'G' || c == 'u' || c == 'U' ||
	    c == 't' || c == 'T' || c == 'n' || c == 'N');
}

inline int ali_is_base(unsigned char c)
{
    return (  (c <= 3) || (c == 5));
}

inline int ali_is_real_base(char c)
{
    return (c == 'a' || c == 'A' || c == 'c' || c == 'C' || 
	    c == 'g' || c == 'G' || c == 'u' || c == 'U' ||
	    c == 't' || c == 'T');
}

inline int ali_is_real_base(unsigned char c)
{
    return ( c <= 3);
}

inline int ali_is_real_base_or_gap(char c)
{
    return (c == 'a' || c == 'A' || c == 'c' || c == 'C' || 
	    c == 'g' || c == 'G' || c == 'u' || c == 'U' ||
	    c == 't' || c == 'T' || c == '-');
}

inline int ali_is_real_base_or_gap(unsigned char c)
{
    return ( c <= 4);
}

inline int ali_is_dot(char c)
{
    return (c == '.');
}

inline int ali_is_dot(unsigned char c)
{
    return (c == 6);
}

inline int ali_is_nbase(char c)
{
    return (c == 'n');
}

inline int ali_is_nbase(unsigned char c)
{
    return (c == 5);
}

inline int ali_is_gap(char c)
{
    return (c == '-');
}

inline int ali_is_gap(unsigned char c)
{
    return (c == 4);
}

inline unsigned char ali_base_to_number(char c, int no_gap_flag = 0)
{
    switch (c) {
	case 'a': case 'A': return(0);
	case 'c': case 'C': return(1);
	case 'g': case 'G': return(2);
	case 'u': case 'U': case 't': case 'T': return(3);
	case '-': if (no_gap_flag == 0)
	    return(4);
	else
	    return(6);
	case 'n': case 'N': return(5);
	case '.': return(6);
	default:
	    ali_warning("Replace unknowen Base by 'n'");
	    return(5);
    }
}

inline char ali_number_to_base(unsigned char n)
{
    switch(n) {
	case 0: return 'a';
	case 1: return 'c';
	case 2: return 'g';
	case 3: return 'u';
	case 4: return '-';
	case 5: return 'n';
	case 6: return '.';
	default:
	    ali_warning("Replace unknowen Number by '.'");
	    printf("received %d\n",n);
	    ali_fatal_error("STOP");
	    return '.';
    }
}

inline void ali_string_to_sequence(char *sequence)
{
    for (; *sequence != '\0' && !ali_is_base(*sequence); sequence++)
	*sequence = (char) ali_base_to_number(*sequence,1);

    for (; *sequence != '\0'; sequence++)
	*sequence = (char) ali_base_to_number(*sequence);
}

inline void ali_sequence_to_string(unsigned char *sequence, 	  unsigned long length)
{
    for (; length-- > 0; sequence++)
	*sequence = (unsigned char) ali_number_to_base(*sequence);
}

inline void ali_sequence_to_postree_sequence(unsigned char *sequence,		unsigned long length)
{
    for (; length-- > 0; sequence++)
	if (ali_is_base(*sequence)) {
	    if (ali_is_nbase(*sequence))
		*sequence = 4;
	}
	else {
	    ali_warning("Unknowen symbol replaced by 'n'");
	    *sequence = 4;
	}
}

inline void ali_print_sequence(unsigned char *sequence,		 unsigned long length)
{
    for (; length-- > 0; sequence++)
	printf("%d ",*sequence);
}

#endif
