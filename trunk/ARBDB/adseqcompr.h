

typedef struct {
    int len;
    char    used[256];
    unsigned char *con[256];
} GB_Consensus;



typedef struct {
    GBDATA *gbd;
    int master;
}GB_Sequence;

typedef struct {
    GBDATA *gbd;
    int master;
}GB_Master;
