#define MAX_TRY 10
		// Live time of a name server 
#define TIME_OUT (1000*60)

#define MSG 1
		// output debug messages
struct Hs_struct;
struct ORS_gl_struct {
	aisc_com	*cl_link;
	struct Hs_struct	*server_communication;
	T_ORS_MAIN	cl_main;
	char		*server_name;
	};
extern struct ORS_gl_struct ORS_global;


extern ORS_main *aisc_main;
