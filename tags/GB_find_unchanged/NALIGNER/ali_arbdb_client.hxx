


class ALI_ARBDB {
private:
    char *alignment;

public:
    GBDATA *gb_main;
    int open(char *name, char *use_alignment = 0);
    int close(void);
    void begin_transaction(void);
    void commit_transaction(void);
    char *get_sequence(char *name, int and_mark = 0);
    char *get_extended(char *name);
    int put_sequence(char *name, char *sequence, char *info = 0);
    int put_extended(const char *name, char *sequence, char *info = 0);
};

