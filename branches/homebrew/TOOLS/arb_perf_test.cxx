// =============================================================== //
//                                                                 //
//   File      : arb_perf_test.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>

#include <climits>
#include <ctime>
#include <sys/time.h>

// --------------------------------------------------------------------------------
// data used for tests

static GBDATA *gb_species_data = 0;
static GBDATA *gb_main         = 0;

// --------------------------------------------------------------------------------
// Test functions

static void iterate(GBDATA *gbd) {
    for (GBDATA *gb_child = GB_child(gbd); gb_child; gb_child = GB_nextChild(gb_child)) {
        iterate(gb_child);
    }
}

static void test_GB_iterate_DB() { // iterate through all DB elements
    iterate(gb_main);
}
static void test_GB_find_string() {
    GB_find_string(gb_species_data, "full_name", "asdfasdf", GB_IGNORE_CASE, SEARCH_GRANDCHILD);
}
static void test_GB_find_string_indexed() {
    // field 'name' is indexed by GBT_open!
    GB_find_string(gb_species_data, "name", "asdfasdf", GB_IGNORE_CASE, SEARCH_GRANDCHILD);
}

// --------------------------------------------------------------------------------

static void noop() {
}

typedef void (*test_fun)();

struct Test {
    const char  *name;
    test_fun     fun;
};

#define TEST(fun) { #fun, test_##fun }

static Test Test[] = {
    TEST(GB_iterate_DB),
    TEST(GB_find_string),
    TEST(GB_find_string_indexed),

    { NULL, NULL },
};

// --------------------------------------------------------------------------------

static long callDelay(long loops);

#define SECOND      1000000
#define WANTED_TIME 5*SECOND // time reserved for each test

static long run_test(test_fun fun, long loops, double *perCall) {
    // returns time for test 'fun' in microseconds
    struct timeval t1;
    struct timeval t2;

    gettimeofday(&t1, 0);
    for (int i = 0; i<loops; ++i) {
        fun();
    }
    gettimeofday(&t2, 0);

    long usecs  = t2.tv_sec - t1.tv_sec;
    usecs      *= SECOND;
    usecs      += t2.tv_usec - t1.tv_usec;

    static bool recurse = true;
    if (recurse) {
        recurse  = false;
        usecs   -= callDelay(loops);
        recurse  = true;
    }

    *perCall = double(usecs)/loops;

    return usecs;
}

static long callDelay(long loops) {
    static GB_HASH *call_delay_hash        = 0;
    if (!call_delay_hash) call_delay_hash = GBS_create_hash(100, GB_MIND_CASE);

    const char *loopKey   = GBS_global_string("%li", loops);
    long        delay = GBS_read_hash(call_delay_hash, loopKey);
    if (!delay) {
        double perCall;
        delay = run_test(noop, loops, &perCall);
        GBS_write_hash(call_delay_hash, loopKey, delay);
    }
    return delay;
}

static long estimate_loops(test_fun fun) {
    long   loops    = 1;
    double perCall;
    long   duration = run_test(fun, loops, &perCall);

    while (duration<1000) {
        loops = (loops*10000.0)/duration+1;
        if (loops>0) {
            duration = run_test(fun, loops, &perCall);
        }
    }

    loops = (loops*double(WANTED_TIME))/duration+1;
    if (loops <= 0) loops = LONG_MAX;
    return loops;
}

static long count_elements(GBDATA *gbd) {
    long count = 0;
    for (GBDATA *gb_child = GB_child(gbd); gb_child; gb_child = GB_nextChild(gb_child)) {
        count += count_elements(gb_child);
    }
    return count+1; // self
}

int ARB_main(int argc, char *argv[])
{
    GB_ERROR error = 0;

    if (argc == 0) {
        fprintf(stderr, "arb_perf_test source.arb\n");
        fprintf(stderr, "Test performance of some commands - see source code\n");

        error = "Missing arguments";
    }
    else {
        const char *in = argv[1];
        gb_main  = GBT_open(in, "rw");

        if (!gb_main) {
            error = GB_await_error();
        }
        else {
            // init test data
            {
                GB_transaction ta(gb_main);
                gb_species_data = GBT_get_species_data(gb_main);
            }

            printf("Loaded DB '%s'\n", in);
            printf(" * contains %li elements\n", count_elements(gb_main));
            printf(" * contains %li species\n", GBT_get_species_count(gb_main));

            printf("Running tests:\n");
            for (int test = 0; Test[test].name; ++test) {
                GB_transaction ta(gb_main);

                double perCall;
                long   esti_loops = estimate_loops(Test[test].fun);
                long   usecs      = run_test(Test[test].fun, esti_loops, &perCall);

                printf("Test #%i: %-25s %10li loops = %10li us (%10.2f us/call, %10.2f calls/sec)\n",
                       test+1,
                       Test[test].name,
                       esti_loops,
                       usecs,
                       perCall,
                       SECOND/perCall);
            }

            GB_close(gb_main);
        }
    }

    if (error) {
        fprintf(stderr, "arb_perf_test: Error: %s\n", error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
