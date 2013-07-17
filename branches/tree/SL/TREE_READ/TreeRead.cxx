// ============================================================ //
//                                                              //
//   File      : TreeRead.cxx                                   //
//   Purpose   : load tree from file                            //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#include "TreeRead.h"

#include <arbdbt.h>
#include <arb_strbuf.h>
#include <arb_file.h>

#define tree_assert(cond) arb_assert(cond)

/*!******************************************************************************************
                    load a tree from file system
********************************************************************************************/


// --------------------
//      TreeReader

enum tr_lfmode { LF_UNKNOWN, LF_N, LF_R, LF_NR, LF_RN, };

struct TreeReader {
    char          *tree_file_name;
    FILE          *in;
    int            last_character;                  // may be EOF
    int            line_cnt;
    GBS_strstruct *tree_comment;
    double         max_found_branchlen;
    double         max_found_bootstrap;
    GB_ERROR       error;
    tr_lfmode      lfmode;
};

static void setReaderError(TreeReader *reader, const char *message) {
    tree_assert(!reader->error);
    reader->error = GBS_global_string("Error reading %s:%i: %s",
                                      reader->tree_file_name,
                                      reader->line_cnt,
                                      message);
}

static char gbt_getc(TreeReader *reader) {
    // reads character from stream
    // - converts linefeeds for DOS- and MAC-textfiles
    // - increments line_cnt
    char c   = getc(reader->in);
    int  inc = 0;

    if (c == '\n') {
        switch (reader->lfmode) {
            case LF_UNKNOWN: reader->lfmode = LF_N; inc = 1; break;
            case LF_N:       inc = 1; break;
            case LF_R:       reader->lfmode = LF_RN; c = gbt_getc(reader); break;
            case LF_NR:      c = gbt_getc(reader); break;
            case LF_RN:      inc = 1; break;
        }
    }
    else if (c == '\r') {
        switch (reader->lfmode) {
            case LF_UNKNOWN: reader->lfmode = LF_R; inc = 1; break;
            case LF_R:       inc = 1; break;
            case LF_N:       reader->lfmode = LF_NR; c = gbt_getc(reader); break;
            case LF_RN:      c = gbt_getc(reader); break;
            case LF_NR:      inc = 1; break;
        }
        if (c == '\r') c = '\n';     // never report '\r'
    }
    if (inc) reader->line_cnt++;

    return c;
}

static char gbt_read_char(TreeReader *reader) {
    bool done = false;
    int  c    = ' ';

    while (!done) {
        c = gbt_getc(reader);
        if (c == ' ' || c == '\t' || c == '\n') ; // skip
        else if (c == '[') {    // collect tree comment(s)
            int openBrackets = 1;
            if (GBS_memoffset(reader->tree_comment)) {
                // not first comment -> do new line
                GBS_chrcat(reader->tree_comment, '\n');
            }

            while (openBrackets && !reader->error) {
                c = gbt_getc(reader);
                switch (c) {
                    case EOF:
                        setReaderError(reader, "Reached end of file while reading comment");
                        break;
                    case ']':
                        openBrackets--;
                        if (openBrackets) GBS_chrcat(reader->tree_comment, c); // write all but last closing brackets
                        break;
                    case '[':
                        openBrackets++;
                        // fall-through
                    default:
                        GBS_chrcat(reader->tree_comment, c);
                        break;
                }
            }
        }
        else done = true;
    }

    reader->last_character = c;
    return c;
}

static char gbt_get_char(TreeReader *reader) {
    int c = gbt_getc(reader);
    reader->last_character = c;
    return c;
}

static TreeReader *newTreeReader(FILE *input, const char *file_name) {
    TreeReader *reader = (TreeReader*)GB_calloc(1, sizeof(*reader));

    reader->tree_file_name      = strdup(file_name);
    reader->in                  = input;
    reader->tree_comment        = GBS_stropen(2048);
    reader->max_found_branchlen = -1;
    reader->max_found_bootstrap = -1;
    reader->line_cnt            = 1;
    reader->lfmode              = LF_UNKNOWN;

    gbt_read_char(reader);

    return reader;
}

static void freeTreeReader(TreeReader *reader) {
    free(reader->tree_file_name);
    if (reader->tree_comment) GBS_strforget(reader->tree_comment);
    free(reader);
}

static char *getTreeComment(TreeReader *reader) {
    // can only be called once. Deletes the comment from TreeReader!
    char *comment = 0;
    if (reader->tree_comment) {
        comment = GBS_strclose(reader->tree_comment);
        reader->tree_comment = 0;
    }
    return comment;
}


/* ------------------------------------------------------------
 * The following functions assume that the "current" character
 * has already been read into 'TreeReader->last_character'
 */

static void gbt_eat_white(TreeReader *reader) {
    int c = reader->last_character;
    while ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')) {
        c = gbt_get_char(reader);
    }
}

static double gbt_read_number(TreeReader *reader) {
    char    strng[256];
    char   *s = strng;
    int     c = reader->last_character;
    double  fl;

    while (((c<='9') && (c>='0')) || (c=='.') || (c=='-') || (c=='e') || (c=='E')) {
        *(s++) = c;
        c = gbt_get_char(reader);
    }
    *s = 0;
    fl = GB_atof(strng);

    gbt_eat_white(reader);

    return fl;
}

static char *gbt_read_quoted_string(TreeReader *reader) {
    /* Read in a quoted or unquoted string.
     * in quoted strings double quotes ('') are replaced by (')
     */

    char  buffer[1024];
    char *s = buffer;
    int   c = reader->last_character;

    if (c == '\'') {
        c = gbt_get_char(reader);
        while ((c != EOF) && (c!='\'')) {
        gbt_lt_double_quot :
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_get_char(reader);
        }
        if (c == '\'') {
            c = gbt_read_char(reader);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }
    else {
        while (c == '_') c = gbt_read_char(reader);
        while (c == ' ') c = gbt_read_char(reader);
        while ((c != ':') && (c != EOF) && (c!=',') &&
                (c!=';') && (c != ')'))
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_read_char(reader);
        }
    }
    *s = 0;
    return strdup(buffer);
}

static void setBranchName(TreeReader *reader, GBT_TREE *node, char *name) {
    // detect bootstrap values
    // name has to be stored in node or must be freed

    char   *end       = 0;
    double  bootstrap = strtod(name, &end);

    if (end == name) {          // no digits -> no bootstrap
        node->name = name;
    }
    else {
        bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 */
        // downscaling in done later!

        if (bootstrap>reader->max_found_bootstrap) {
            reader->max_found_bootstrap = bootstrap;
        }

        tree_assert(node->remark_branch == 0);
        node->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);

        if (end[0] != 0) {      // sth behind bootstrap value
            if (end[0] == ':') ++end; // ARB format for nodes with bootstraps AND node name is 'bootstrap:nodename'
            node->name = strdup(end);
        }
        free(name);
    }
}

static bool gbt_readNameAndLength(TreeReader *reader, GBT_TREE *node, GBT_LEN *len) {
    /* reads the branch-length and -name
       '*len' should normally be initialized with TREE_DEFLEN_MARKER
     * returns the branch-length in 'len' and sets the branch-name of 'node'
     * returns true if successful, otherwise reader->error gets set
     */

    bool done = false;
    while (!done && !reader->error) {
        switch (reader->last_character) {
            case ';':
            case ',':
            case ')':
                done = true;
                break;
            case ':':
                gbt_read_char(reader);      // drop ':'
                *len = gbt_read_number(reader);
                if (*len>reader->max_found_branchlen) {
                    reader->max_found_branchlen = *len;
                }
                break;
            default: {
                char *branchName = gbt_read_quoted_string(reader);
                if (branchName) {
                    setBranchName(reader, node, branchName);
                }
                else {
                    UNCOVERED();
                    setReaderError(reader, "Expected branch-name or one of ':;,)'");
                }
                break;
            }
        }
    }

    return !reader->error;
}

static GBT_TREE *gbt_linkedTreeNode(GBT_TREE *left, GBT_LEN leftlen, GBT_TREE *right, GBT_LEN rightlen, int structuresize) {
    GBT_TREE *node = (GBT_TREE*)GB_calloc(1, structuresize);

    node->leftson  = left;
    node->leftlen  = leftlen;
    node->rightson = right;
    node->rightlen = rightlen;

    left->father  = node;
    right->father = node;

    return node;
}

static GBT_TREE *gbt_load_tree_rek(TreeReader *reader, int structuresize, GBT_LEN *nodeLen) {
    GBT_TREE *node = 0;

    if (reader->last_character == '(') {
        gbt_read_char(reader);  // drop the '('

        GBT_LEN   leftLen = TREE_DEFLEN_MARKER;
        GBT_TREE *left    = gbt_load_tree_rek(reader, structuresize, &leftLen);

        tree_assert(contradicted(left, reader->error));

        if (left) {
            if (gbt_readNameAndLength(reader, left, &leftLen)) {
                switch (reader->last_character) {
                    case ')':               // single node
                        *nodeLen = leftLen;
                        node     = left;
                        left     = 0;
                        break;

                    case ',': {
                        GBT_LEN   rightLen = TREE_DEFLEN_MARKER;
                        GBT_TREE *right    = 0;

                        while (reader->last_character == ',' && !reader->error) {
                            if (right) { // multi-branch
                                GBT_TREE *pair = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);

                                left  = pair; leftLen = 0;
                                right = 0; rightLen = TREE_DEFLEN_MARKER;
                            }

                            gbt_read_char(reader); // drop ','
                            right = gbt_load_tree_rek(reader, structuresize, &rightLen);
                            if (right) gbt_readNameAndLength(reader, right, &rightLen);
                        }

                        if (!reader->error) {
                            if (reader->last_character == ')') {
                                node     = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                                *nodeLen = TREE_DEFLEN_MARKER;

                                left = 0;
                                right  = 0;

                                gbt_read_char(reader); // drop ')'
                            }
                            else {
                                setReaderError(reader, "Expected one of ',)'");
                            }
                        }

                        GBT_delete_tree(right);

                        break;
                    }

                    default:
                        setReaderError(reader, "Expected one of ',)'");
                        break;
                }
            }
            else {
                UNCOVERED();
                tree_assert(reader->error);
            }

            GBT_delete_tree(left);
        }
    }
    else {                      // single node
        gbt_eat_white(reader);
        char *name = gbt_read_quoted_string(reader);
        if (name) {
            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = true;
            node->name    = name;
        }
        else {
            UNCOVERED();
            setReaderError(reader, "Expected quoted string");
        }
    }

    tree_assert(contradicted(node, reader->error));
    return node;
}



GBT_TREE *TREE_load(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr) {
    /* Load a newick compatible tree from file 'path',
       structure size should be >0, see GBT_read_tree for more information
       if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
       if warningPtr != NULL -> set it to a malloc copy auto-scale-warning (if autoscaling happens)
    */

    GBT_TREE *tree  = 0;
    FILE     *input = fopen(path, "rt");
    GB_ERROR  error = 0;

    if (!input) {
        error = GBS_global_string("No such file: %s", path);
    }
    else {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else        name_only = path;

        TreeReader *reader      = newTreeReader(input, name_only);
        GBT_LEN     rootNodeLen = TREE_DEFLEN_MARKER; // root node has no length. only used as input to gbt_load_tree_rek
        tree                    = gbt_load_tree_rek(reader, structuresize, &rootNodeLen);
        fclose(input);

        if (reader->error) {
            GBT_delete_tree(tree);
            error = reader->error;
        }

        if (tree) {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (reader->max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                if (warningPtr) {
                    *warningPtr = GBS_global_string_copy("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                                         bootstrap_scale, reader->max_found_bootstrap);
                }
            }
            if (reader->max_found_branchlen >= 1.1) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %.2f)\n"
                                                         "(use ARB_NT/Tree/Modify branches/Scale branchlengths with factor %.2f to undo auto-scaling)",
                                                         branchlen_scale, reader->max_found_branchlen, 1.0/branchlen_scale);
                        if (*warningPtr) {
                            char *w2 = GBS_global_string_copy("%s\n%s", *warningPtr, w);

                            free(*warningPtr);
                            free(w);
                            *warningPtr = w2;
                        }
                        else {
                            *warningPtr = w;
                        }
                    }
                }
            }

            TREE_scale(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths

            if (commentPtr) {
                char *comment = getTreeComment(reader);

                const char *loaded_from = GBS_global_string("Loaded from %s", path);
                freeset(comment, GBS_log_dated_action_to(comment, loaded_from));

                tree_assert(*commentPtr == 0);
                *commentPtr = comment;
            }
        }

        freeTreeReader(reader);
    }

    tree_assert(tree||error);
    if (error) {
        GB_export_errorf("Import tree: %s", error);
        tree_assert(!tree);
    }

    return tree;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static char *leafNames(GBT_TREE *tree, size_t& count) {
    GB_CSTR       *name_array = GBT_get_names_of_species_in_tree(tree, &count);
    GBS_strstruct  names(1024);

    for (size_t n = 0; n<count; ++n) {
        names.cat(name_array[n]);
        names.put(',');
    }
    names.cut_tail(1);
    free(name_array);

    return names.release();
}

static GBT_TREE *loadFromFileContaining(const char *treeString) {
    const char *filename = "trees/tmp.tree";
    FILE       *out      = fopen(filename, "wt");
    GBT_TREE   *tree     = NULL;

    if (out) {
        fputs(treeString, out);
        fclose(out);
        tree = TREE_load(filename, sizeof(GBT_TREE), NULL, 0, NULL);
    }
    else {
        GB_export_IO_error("save tree", filename);
    }

    return tree;
}

static arb_test::match_expectation loading_tree_failed_with(GBT_TREE *tree, const char *errpart) {
    using namespace   arb_test;
    expectation_group expected;

    expected.add(that(tree).is_equal_to_NULL());
    expected.add(that(GB_have_error()).is_equal_to(true));
    if (GB_have_error()) {
        expected.add(that(GB_await_error()).does_contain(errpart));
    }
    return all().ofgroup(expected);
}

static arb_test::match_expectation loading_tree_succeeds(GBT_TREE *tree, const char *names_expected, size_t count_expected) {
    using namespace   arb_test;
    expectation_group expected;

    expected.add(that(tree).does_differ_from_NULL());
    expected.add(that(GB_have_error()).is_equal_to(false));
    if (!GB_have_error()) {
        size_t  leafcount;
        char   *names = leafNames(tree, leafcount);
        expected.add(that(names).is_equal_to(names_expected));
        expected.add(that(leafcount).is_equal_to(count_expected));
        free(names);
    }
    return all().ofgroup(expected);
}

#define TEST_EXPECT_TREELOAD_FAILED_WITH(tree,errpart)         TEST_EXPECTATION(loading_tree_failed_with(tree, errpart))
#define TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree,errpart) TEST_EXPECTATION__BROKEN(loading_tree_failed_with(tree, errpart))

#define TEST_EXPECT_TREELOAD(tree,names,count)         TEST_EXPECTATION(loading_tree_succeeds(tree,names,count))
#define TEST_EXPECT_TREELOAD__BROKEN(tree,names,count) TEST_EXPECTATION__BROKEN(loading_tree_succeeds(tree,names,count))

#define TEST_EXPECT_TREEFILE_FAILS_WITH(name,errpart) do {                      \
        GBT_TREE *tree = TREE_load(name, sizeof(GBT_TREE), NULL, 0, NULL);      \
        TEST_EXPECT_TREELOAD_FAILED_WITH(tree, errpart);                        \
    } while(0)

#define TEST_EXPECT_TREESTRING_FAILS_WITH(treeString,errpart) do {      \
        GBT_TREE *tree = loadFromFileContaining(treeString);            \
        TEST_EXPECT_TREELOAD_FAILED_WITH(tree, errpart);                \
    } while(0)

// arguments 'names,count' are vs regression only!
#define TEST_EXPECT_TREESTRING_FAILS_WITH__BROKEN(treeString,errpart,names,count) do {  \
        GBT_TREE *tree = loadFromFileContaining(treeString);                            \
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, errpart);                        \
        TEST_EXPECT_TREELOAD(tree, names, count);                                       \
    } while(0)

void TEST_load_tree() {
    // just are few tests covering most of this module.
    // more load tests are in ../../TOOLS/arb_test.cxx@TEST_SLOW_arb_read_tree

    // simple succeeding tree load
    {
        char     *comment = NULL;
        GBT_TREE *tree    = TREE_load("trees/test.tree", sizeof(GBT_TREE), &comment, 0, NULL);
        // -> ../../UNIT_TESTER/run/trees/test.tree

        TEST_EXPECT_TREELOAD(tree, "s1,s2,s3,s 4,s5,\"s-6\"", 6); // @@@ remove double-quotes from name?
        TEST_EXPECT_CONTAINS(comment,
                             // comment part from treefile:
                             "tree covering most of tree reader code\n"
                             "comment contains [extra brackets] inside comment\n");
        TEST_EXPECT_CONTAINS(comment,
                             // comment as appended by load:
                             ": Loaded from trees/test.tree\n");

        free(comment);
        GBT_delete_tree(tree);
    }

    // test valid trees with strange or wrong behavior
    {
        GBT_TREE *tree = loadFromFileContaining("(,);");
        TEST_EXPECT_TREELOAD(tree, ",", 2); // tree with 2 unamed species (weird, but ok)
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("( a, (b,(c),d), (e,(f)) );");
        TEST_EXPECT_TREELOAD__BROKEN(tree, "a,b,c,d,e,f", 6); // @@@ superfluous parenthesis confuse reader
        TEST_EXPECT_TREELOAD(tree, "a,b,c,d", 4); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("( (a), ((b),(c),(d)), ((e),(f)) );");
        TEST_EXPECT_TREELOAD__BROKEN(tree, "a,b,c,d,e,f", 6); // @@@ superfluous parenthesis confuse reader
        TEST_EXPECT_TREELOAD(tree, "a", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("(((((a)))), ((b, c)));");
        TEST_EXPECT_TREELOAD__BROKEN(tree, "a,b,c", 3); // @@@ superfluous parenthesis confuse reader
        TEST_EXPECT_TREELOAD(tree, "a", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }

    // test forbidden trees
    {
        GBT_TREE *tree = loadFromFileContaining("();");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "empty");
        TEST_EXPECT_TREELOAD(tree, "", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining(";");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "empty");
        TEST_EXPECT_TREELOAD(tree, "", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "empty");
        TEST_EXPECT_TREELOAD(tree, "", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("(one);");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "???"); // @@@ tree with 1 leaf should not load
        TEST_EXPECT_TREELOAD(tree, "one", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("((((()))));");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "empty");
        TEST_EXPECT_TREELOAD(tree, "", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("(((((one)))));");
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, "???");
        TEST_EXPECT_TREELOAD(tree, "one", 1); // unwanted - just protect vs regression
        GBT_delete_tree(tree);
    }

    // test invalid trees
    TEST_EXPECT_TREESTRING_FAILS_WITH("(;);", "Expected one of ',)'");
    // TEST_EXPECT_TREESTRING_FAILS_WITH("(17", "xxx"); // @@@ deadlocks
    // TEST_EXPECT_TREESTRING_FAILS_WITH("((((", "xxx"); // @@@ deadlocks
    // TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment", "while reading comment"); // @@@ fails an assertion
    // TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment [ bla ]", "while reading comment"); // @@@ fails an assertion

    // TEST_EXPECT_TREESTRING_FAILS_WITH("(a, 'b", "akjhsd"); // @@@ deadlocks
    // TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5::::", "akjhsd"); // @@@ deadlocks
    // TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5:c:d", "akjhsd"); // @@@ deadlocks
    TEST_EXPECT_TREESTRING_FAILS_WITH__BROKEN("(a, b:5:c:d)", "akjhsd", "a,d", 2);

    // questionable accepted trees
    TEST_EXPECT_TREESTRING_FAILS_WITH__BROKEN("())", "xxx", "", 1); // @@@ at least warn!
    TEST_EXPECT_TREESTRING_FAILS_WITH__BROKEN("(a*,b%);", "xxx", "a*,b%", 2); // @@@ really accept such names?

    // check errors
    TEST_EXPECT_TREEFILE_FAILS_WITH("trees/nosuch.tree",    "No such file");
    TEST_EXPECT_TREEFILE_FAILS_WITH("trees/corrupted.tree", "Error reading");

    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink("trees/tmp.tree")); // cleanup
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
