
#ifndef CA_MARK_HXX
#define CA_MARK_HXX

// initialization (needed for all functions below):
GB_ERROR CHIP_init_pt_server(GBDATA *gb_main, const char *servername);
void 	 CHIP_exit_pt_server(void);

// probe match:

struct CHIP_probe_match_para {
    // expert window
    double	bondval[16];
    double	split;		// should be 0.5
    double 	dtedge;		// should be 0.5
    double 	dt;		// should be 0.5
};

struct probe_data {
  char name[255];
  char longname[255];
  char sequence[255];
};


typedef int euer_container;

//GB_ERROR CHIP_probe_match(euer_container& g, const CHIP_probe_match_para& para, const char *for_probe, GBDATA *gb_main);
GB_ERROR CHIP_probe_match(probe_data& pD, const CHIP_probe_match_para& para, char *fn, int numMismatches, int weightedMismatches);
GB_ERROR read_input_file(char *fn);
char *parse_match_info(const char *match_info);
GB_ERROR write_result_file(char *fn);


#ifdef NDEBUG
#define chip_assert(bed)
#else
#define chip_assert(bed) do { if ((bed)==false) { cerr << "Assertion '" << #bed << "' failed in " << __LINE__ << "\n"; exit(1); } } while(0)
// #define pg_assert(bed) do { if ((bed)==false) { cerr << "Assertion '" << #bed << "' failed in " << __LINE__ << "\n"; exit(1); } } while(0)
#endif

#else
#error ca_mark.hxx included twice
#endif // CA_MARK_HXX
