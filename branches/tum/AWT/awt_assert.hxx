#ifndef awt_assert_included
#define awt_assert_included 

#ifdef DEBUG 
# define awt_assert(bed) 	do { if (!(bed)) *(int*)0=0; } while (0)
#else 
# define awt_assert(bed) 
#endif // DEBUG

#endif // awt_assert_included
