#define CLUSTER_ANZ_MAX 10
#define GA_SAFETY 40
// max loop repeat count
#define GB_read_APfloat (AP_FLOAT)GB_read_float
#define GB_write_APfloat GB_write_float

enum AP_init_status {
    INIT_NONE = 0,
    INIT_MAIN = 1,
    INIT_START = 2,
    INIT_OPT  = 4,
    INIT_JOB  = 8,
    INIT_ALL = 15
};

enum GA_JOB_MODE {
    GA_CROSSOVER,
    GA_KERNIGHAN,
    GA_CREATEJOBS,
    GA_NNI,
    GA_NONE
};

struct GA_tree {
    int ref_count;
    AP_FLOAT criteria;
    AP_tree_nlen *tree;
    long id;

};

struct GA_job {
    AP_FLOAT criteria;
    int cluster0;
    int cluster1;
    GA_tree * tree0;        // fuer return
    GA_tree * tree1;
    long id0;
    long id1;
    GA_JOB_MODE mode;
    void  printl() {
        cout << "JOB Clu0 " << cluster0 <<
            " tr " << id0 <<
            " *  CLu1 " << cluster1 << " tr " << id1 <<
            " * mode " << mode << " crit " << criteria
             << "\n"; cout.flush(); };
    void calcCrit(AP_FLOAT crit0, AP_FLOAT crit1) {
        criteria = crit0 + crit1; };
};


class GA_genetic {
    AP_init_status init_status;
    AP_tree *tree_prototype;
    // presets
    int min_job; // min jobs per cluster before removing jobs
    int max_cluster; // max cluster
    int max_jobs; // max jobs
    int maxTree; // max trees per clustergbp
    int jobCount;
    AP_FLOAT bestTree;

    int jobOpt;
    int jobCrossover;
    int jobOther;
    int jobKL;

    // array of trees in cluster
    long  **treelist; // treeids
    int *treePerCluster; // clusterid

    int clusterCount;


    // stored DB pointers (which won't change)
    GBDATA *gb_tree_start;
    GBDATA *gb_treeName;
    GBDATA *gb_tree_opt;
    GBDATA *gb_joblist;
    GBDATA *gb_genetic;
    GBDATA *gb_main;
    GBDATA *gb_presets;
    GBDATA *gb_jobCount;
    GBDATA *gb_bestTree;
    GBDATA *gb_maxTree;
    // writes and reads compressd tree from database
    double AP_atof(char *str);
    char * write_tree_rek(AP_tree *node, char *dest, long mode);
    AP_tree  *read_tree_rek(char **data);
    AP_ERR  *write_tree(GBDATA *gb_tree, GA_tree *tree);
    GA_tree *read_tree(GBDATA *gb_tree, long tree_id);
    AP_ERR *create_job_container(GA_job *job);
    GBDATA *get_cluster(GBDATA * container, int cluster);
    GBDATA *get_tree(GBDATA *container, long tree_id);
    long get_treeid(GBDATA *gbtree);

    AP_ERR * read_presets();
public:
    GA_genetic();
    ~GA_genetic();
    void init(GBDATA *gbmain);
    void init_first(GBDATA *gbmain); // dummy zum testen der db eintraege generiert
    void exit();
    // for errorfile
    FILE *fout;

    AP_ERR  *put_start_tree(AP_tree *tree, const long name, int  cluster);
    AP_ERR *remove_job(GBDATA *gb_cluster);
    AP_ERR *delete_job(GBDATA *gb_job);
    AP_ERR *delete_tree(GBDATA *gb_cluster, GBDATA *gb_tree);
    AP_ERR *create_jobs(GA_tree *tree, int cluster);
    GA_tree *get_start_tree(int cluster);
    AP_ERR  *put_opt_tree(char *tree, int cluster);
    GA_job  *get_job(int cluster);
    AP_ERR *put_job(int cluster, GA_job *job);
    AP_ERR * put_optimized(GA_tree *tree, int cluster);

    int     getMaxCluster() { return max_cluster; };
    int     getMaxJob()     { return max_jobs; };
};
