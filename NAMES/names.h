
#define MAX_TRY 20
#define RETRY_SLEEP 3 // in seconds

// Lifetime of nameserver in seconds
#define NAME_SERVER_TIMEOUT (15*60) // 15 minutes

// Sleeptime of nameserver in seconds
#define NAME_SERVER_SLEEP 5

struct Hs_struct;
struct AN_gl_struct {
    aisc_com         *cl_link;
    struct Hs_struct *server_communication;
    T_AN_MAIN         cl_main;
    char             *server_name;
};
extern struct AN_gl_struct AN_global;


extern AN_main *aisc_main;
