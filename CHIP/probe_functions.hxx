
#ifndef PROBE_FUNCTIONS_HXX
#define PROBE_FUNCTIONS_HXX

// initialization (needed for all functions below):
GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername, void (*print_function)(const char *format, ...));
void 	 PG_exit_pt_server(void);

// probe match:

struct PG_probe_match_para {
    // expert window
    double	bondval[16];
    double	split;		// should be 0.5
    double 	dtedge;		// should be 0.5
    double 	dt;		// should be 0.5
};

typedef int euer_container;

GB_ERROR PG_probe_match(euer_container& g, const PG_probe_match_para& para, const char *for_probe);



#else
#error probe_functions.hxx included twice
#endif // PROBE_FUNCTIONS_HXX

