
#define MAX_TRY 10

// Live time of a name server in milliseconds:
#if defined(DEBUG)
// debug: 30 seconds 
#define TIME_OUT (1000*30)
#else
// release: 4 minutes
#define TIME_OUT (1000*60*4)
#endif // DEBUG

struct Hs_struct;
struct AN_gl_struct {
    aisc_com         *cl_link;
    struct Hs_struct *server_communication;
    T_AN_MAIN         cl_main;
    char             *server_name;
};
extern struct AN_gl_struct AN_global;


extern AN_main *aisc_main;
