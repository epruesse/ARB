// main-wrapper

int ARB_main(int argc, char *argv[]);
int main(int argc, char **argv) {
    // this is the only main() we like in production code
    // other main()'s occur in test-code

    return ARB_main(argc, argv);
}
