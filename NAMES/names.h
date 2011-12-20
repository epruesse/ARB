
#define MAX_TRY     20
#define RETRY_SLEEP 3           // in seconds

// Lifetime of nameserver in seconds
#if defined(DEBUG)
#define NAME_SERVER_TIMEOUT (1*60) // 1 minute (for debugging)
#else
// #define NAME_SERVER_TIMEOUT (15*60) // 15 minutes
#define NAME_SERVER_TIMEOUT (2*60) // 2 minutes (see http://bugs.arb-home.de/ticket/192 why)
#endif // DEBUG

// Sleeptime of nameserver in seconds
#define NAME_SERVER_SLEEP 5

struct Hs_struct;
extern AN_main *aisc_main;
