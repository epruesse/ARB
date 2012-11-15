typedef struct hnode {
    PART *part;
    struct hnode *next;
} HNODE;

struct gbs_hash_struct;
extern int Tree_count;
extern gbs_hash_struct *Name_hash;
#define HASH_MAX 123

void hash_print(void);
void hash_init(void);
void hash_settreecount(int tree_count);
void hash_free(void);
PART *hash_getpart(void);
void hash_insert(PART *part, int weight);
void build_sorted_list(void);
