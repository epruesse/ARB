// main-wrapper

#include <arb_main.h>

int ARB_main(int argc, char *argv[]);
int main(int argc, char **argv) {
    // this is the only main() we like in production code
    // other main()'s occur in test-code
    start_of_main();
    return ARB_main(argc, argv);
}
