#ifndef awt_seq_protein_hxx_included
#define awt_seq_protein_hxx_included

class AP_sequence_protein :  public  AP_sequence {

public:
	AWT_PDP		*sequence;
//	static char	*table;
	AP_sequence_protein(AP_tree_root *root);
	~AP_sequence_protein(void);
	AP_sequence 	*dup(void);		// used to get the real new element
	void set( char *sequence);
	AP_FLOAT combine(	const AP_sequence * lefts,
				const	AP_sequence *rights) ;
	AP_FLOAT real_len(void);
};



#endif
