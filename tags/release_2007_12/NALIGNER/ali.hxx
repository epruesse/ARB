#include "ali_preali.hxx"
#include "ali_arbdb_client.hxx"
#include "ali_pt_client.hxx"


struct ali_global_struct {
    char *server_name;
    ALI_ARBDB arbdb;     
    ALI_PT pt;
};

extern struct ali_global_struct aligs;

void message(char *errortext);
