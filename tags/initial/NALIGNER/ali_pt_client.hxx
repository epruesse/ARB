

class ALI_PT {
private:
   aisc_com *link;
   T_PT_LOCS locs;
   T_PT_MAIN com;
   T_PT_FAMILYLIST f_list;

   init_communication();

public:
   int open(char *servername,GBDATA *gb_main);
   int close(void);
   int find_family(char *string, int find_type = 0);
   int first_family(char **seq_name, int *matches);
   int next_family(char **seq_name, int *matches);
};

