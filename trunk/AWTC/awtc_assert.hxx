#ifndef awtc_assert_included
#define awtc_assert_included 

#ifdef DEBUG 
# define awtc_assert(bed) 	do { if (!(bed)) *(int*)0=0; } while (0)
#else 
# define awtc_assert(bed) 
#endif // DEBUG

#endif // awtc_assert_included
