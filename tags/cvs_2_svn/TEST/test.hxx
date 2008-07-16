#define MEM_ALLOC_FACTOR 3/2
#define MEM_ALLOC_INIT_MASTER   3000
#define MEM_ALLOC_INIT_CLIENT   5000
#define MEM_ALLOC_INIT_MASTERS  30
#define MEM_ALLOC_INIT_CLIENTS  100
#define REL_DIFF_CLIENT_MASTER(x)       x/150
#define MAX_REL_DIFF_CLIENT_MASTER(x)   x/30
#define MAX_MASTERS             20
#define MAX_MASTERS_REMOVE_N    3



#define PT_MAX_NN               3
#define PT_NAME_HASH_SIZE 10000

typedef enum PT_bases_enum      {
    PT_QU = 0,
    PT_N = 1,
    PT_A, PT_C, PT_G, PT_T, PT_B_MAX } PT_BASES;

#define PT_BASE_2_ASCII "?NACGT????"

#define  MAX(a, b) (((a) > (b)) ? (a) : (b))

// x is the relative index to the masters rel_2_abs list
// y is the offset of rel_2_abs[rel]
class master_gap_compress {
public:
    int     index;  // used to save the master !!!
    int     ref_cnt;
    class client_gap_compress *main_child;
    int     alt_master;             // is there any alternative master
    int     memsize;
    int     len;                    // save index
    long *rel_2_abs;                // save mem
    short *rel_2_abss;              // save mem
    master_gap_compress(void);
    ~master_gap_compress(void);
    void optimize(void);
    long read(long rel, long def_abs);
    void write(long rel, long abs);

    int save(FILE *out, int index_write, int &pos);
    int load(FILE *in, char *baseaddr);
};

class client_gap_compress {
    long old_m;
    long old_rel;
public:
    master_gap_compress *master;    // save index as index !!!!!
    int     memsize;
    int     len;                    // save index
    struct client_gap_compress_data {       // save mem
        long rel;
        short dx;
        short x;
        long y;
    }       *l;
    long    abs_seq_len;            // save index
    long    rel_seq_len;            // save index


    client_gap_compress(master_gap_compress *mgc);
    void clean(master_gap_compress *mgc);
    ~client_gap_compress(void);
    void optimize(int realloc_flag);
    void basic_write(long rel, long x, long y);
    void write(long rel, long abs);
    void basic_write_sequence(char *sequence, long seq_len, int gap1,int gap2);
    void basic_write_master_sequence(char *sequence, long seq_len, int gap1,int gap2);
    void write_sequence(char *sequence, long seq_len, int gap1,int gap2);
    long rel_2_abs(long rel);
    int save(FILE *out, int index_write, int &pos);
    int load(FILE *in, char *baseaddr,master_gap_compress **masters);
};

typedef int     gap_index;

class gap_compress {
    master_gap_compress *search_best_master(client_gap_compress *client,
                                            master_gap_compress *exclude,long max_diff,
                                            char *sequence, long seq_len, int gap1,int gap2, int &best_alt);
    int                     memmasters;
    int                     memclients;
public:
    int                     nmasters;       // save index
    master_gap_compress     **masters;

    int                     nclients;       // save index
    client_gap_compress     **clients;

    gap_compress(void);
    ~gap_compress(void);

    gap_index       write_sequence(char *sequence, long seq_len,
                                   int gap1, int gap2);

    char    *read_sequence(gap_index index);        // returns a '+-' seq.
    // returns an index for rel_2_abs
    void    optimize(void);
    long    rel_2_abs(gap_index index, long rel);
    void    statistic(int full=1);

    int save(FILE *out, int index_write, int &pos);
    int load(FILE *in, char *baseaddr);
};

class pt_species_class {
public:
    //*************** not a public section !!!!
    int     save(FILE *out, int index_write, int &pos);
    int     load(FILE *in, char *baseaddr);
    client_gap_compress             *cgc;
    void    calc_gap_index(gap_compress *gc);
    char    *abs_sequence;          // including all gaps

    void    set(char *sequence, long seq_len, char *name, char *fullname);

    //*************** here starts the read only section

    char    *sequence;              // save mem
    long    abs_seq_len;            // save index
    long    seq_len;                // save index
    char    *name;                  // save index
    char    *fullname;              // save index
    gap_index index;                // save index

    pt_species_class(void);
    ~pt_species_class(void);

    // ************* the real public section
    int     get(int pos) { return sequence[pos]; };
    long    rel_2_abs(long rel);    // after load

    void    test_print(void);       // after load
};

class pt_main_class {
    void    calc_name_hash(void);
    void    calc_max_size_char_count(void);
    void    calc_bi_ecoli(void);
    void    init(GBDATA *gb_main);

public:
    gap_compress    *gc;
    pt_main_class(void);
    ~pt_main_class(void);

    int     data_count;             // save index
    char *ecoli;                    // save index

    pt_species_class        *species;

    int     save(GBDATA *gb_main, FILE *out, int index_write, int &pos);
    int     load(FILE *in, char *baseaddr);


    long    namehash;               /* name to int */
    int     max_size;               /* maximum sequence len */
    long    char_count;             /* number of all 'acgtuACGTU' */
    void    *bi_ecoli;

    char    *alignment_name;
    long    abs_2_ecoli(long pos);
    int     main_convert(GBDATA *gb_main, char *basename);
    int     import_all(char *basename);
};


char *pt_getstring(FILE *in);   // reads short strings !!!!
int pt_putstring(char *s,FILE *out);
