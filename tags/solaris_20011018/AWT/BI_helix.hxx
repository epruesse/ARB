#ifndef BI_helix_hxx_included
#define BI_helix_hxx_included

#define HELIX_MAX_NON_ST 10

#ifndef NDEBUG
# define bi_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define bi_assert(bed)
#endif

#define HELIX_AWAR_PAIR_TEMPLATE "Helix/pairs/%s"
#define HELIX_AWAR_SYMBOL_TEMPLATE "Helix/symbols/%s"

typedef enum {
	HELIX_NONE,			// used in entries
	HELIX_STRONG_PAIR,
	HELIX_PAIR,			// used in entries
	HELIX_WEAK_PAIR,
	HELIX_NO_PAIR,			// used in entries
	HELIX_USER0,
	HELIX_USER1,
	HELIX_USER2,
	HELIX_USER3,
	HELIX_DEFAULT,
	HELIX_NON_STANDART0,			// used in entries
	HELIX_NON_STANDART1,			// used in entries
	HELIX_NON_STANDART2,			// used in entries
	HELIX_NON_STANDART3,			// used in entries
	HELIX_NON_STANDART4,			// used in entries
	HELIX_NON_STANDART5,			// used in entries
	HELIX_NON_STANDART6,			// used in entries
	HELIX_NON_STANDART7,			// used in entries
	HELIX_NON_STANDART8,			// used in entries
	HELIX_NON_STANDART9,			// used in entries
	HELIX_NO_MATCH,
	HELIX_MAX
} BI_PAIR_TYPE;


class BI_helix {
	private:
	int _check_pair(char left, char right, BI_PAIR_TYPE pair_type);
	void _init(void);
	int	deleteable;
	public:
		// read only section
	struct BI_helix_entry {
	    long pair_pos;
	    BI_PAIR_TYPE pair_type;
	    char	*helix_nr;
	} *entries;
	// **read only:
	char	*pairs[HELIX_MAX];
	char	*char_bind[HELIX_MAX];
	size_t size;
	static char error[256];

	// ***** read and write
	BI_helix(void);
#ifdef _USE_AW_WINDOW
	BI_helix(AW_root *aw_root); 
#endif
	~BI_helix(void);	

	const char *init(GBDATA *gb_main);
	const char *init(GBDATA *gb_main, const char *alignment_name, const char *helix_nr_name = "HELIX_NR", const char *helix_name = "HELIX");
	const char *init(GBDATA *gb_helix_nr,GBDATA *gb_helix,size_t size);
	const char *init(char *helix_nr, char *helix, size_t size);

	int check_pair(char left, char right, BI_PAIR_TYPE pair_type);
		// returns 1 of bases form a pair
	char *seq_2_helix(char *sequence,char undefsymbol = ' ');

#ifdef _USE_AW_WINDOW
	char get_symbol(char left, char right, BI_PAIR_TYPE pair_type);

	int show_helix( void *device, int gc1 , char *sequence,
		AW_pos x, AW_pos y,
		AW_bitset filter,
		AW_CL cd1, AW_CL cd2);
#endif
};

#ifdef _USE_AW_WINDOW
AW_window *create_helix_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs);
#endif


class BI_ecoli_ref {
	private:
		long _abs_len;
		long _rel_len;

		long *_abs_2_rel1;
		long *_abs_2_rel2;
		long *_rel_2_abs;
		void	exit(void);
	public:
	BI_ecoli_ref(void);
	~BI_ecoli_ref(void);
	const char *init(GBDATA *gb_main);
	const char *init(GBDATA *gb_main,char *alignment_name, char *ref_name);
	const char *init(char *seq,long size);

	const char *abs_2_rel(long in,long &out,long &add);
	const char *rel_2_abs(long in,long add,long &out);
};


#endif
