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

class TreeReader : virtual Noncopyable {

    int get_char();
    int read_tree_char();
    int read_char();


    void setError(const char *message);
    void drop_tree_char(char expected) {
        if (last_character != expected) {
            setError(GBS_global_string("Expected '%c', got '%c'", expected, last_character));
        }
        read_tree_char();
    }

    void setBranchName(GBT_TREE *node, char *name);

    // The eat-functions below assume that the "current" character
    // has already been read into 'last_character':
    void    eat_white();
    double  eat_number();
    char   *eat_quoted_string();
    bool    eat_and_set_name_and_length(GBT_TREE *node, GBT_LEN *len);

public:
    char          *tree_file_name;
    FILE          *in;
    int            last_character;                  // may be EOF
    int            line_cnt;
    GBS_strstruct *tree_comment;
    double         max_found_branchlen;
    double         max_found_bootstrap;
    GB_ERROR       error;
    tr_lfmode      lfmode;

    TreeReader(FILE *input, const char *file_name);
    ~TreeReader();

    GBT_TREE *load_tree(int structuresize, GBT_LEN *nodeLen);
    char *takeComment();
};

TreeReader::TreeReader(FILE *input, const char *file_name)
    : tree_file_name(strdup(file_name)),
      in(input),
      last_character(0),
      line_cnt(1),
      tree_comment(GBS_stropen(2048)),
      max_found_branchlen(-1),
      max_found_bootstrap(-1),
      error(NULL),
      lfmode(LF_UNKNOWN)
{
    read_tree_char();
}

TreeReader::~TreeReader() {
    free(tree_file_name);
    if (tree_comment) GBS_strforget(tree_comment);
}

void TreeReader::setError(const char *message) {
    tree_assert(!error);
    error = GBS_global_string("Error reading %s:%i: %s",
                              tree_file_name, line_cnt, message);
}

int TreeReader::get_char() {
    // reads character from stream
    // - converts linefeeds for DOS- and MAC-textfiles
    // - increments line_cnt

    int c   = getc(in);
    int inc = 0;

    if (c == '\n') {
        switch (lfmode) {
            case LF_UNKNOWN: lfmode = LF_N; inc = 1; break;
            case LF_N:       inc = 1; break;
            case LF_R:       lfmode = LF_RN; c = get_char(); break;
            case LF_NR:      c = get_char(); break;
            case LF_RN:      inc = 1; break;
        }
    }
    else if (c == '\r') {
        switch (lfmode) {
            case LF_UNKNOWN: lfmode = LF_R; inc = 1; break;
            case LF_R:       inc = 1; break;
            case LF_N:       lfmode = LF_NR; c = get_char(); break;
            case LF_RN:      c = get_char(); break;
            case LF_NR:      inc = 1; break;
        }
        if (c == '\r') c = '\n';     // never report '\r'
    }
    if (inc) line_cnt++;

    return c;
}

int TreeReader::read_tree_char() {
    // reads over tree comment(s) and whitespace.
    // tree comments are stored inside TreeReader

    bool done = false;
    int  c    = ' ';

    while (!done && !error) {
        c = get_char();
        if (c == ' ' || c == '\t' || c == '\n') ; // skip
        else if (c == '[') {    // collect tree comment(s)
            int openBrackets = 1;
            if (GBS_memoffset(tree_comment)) {
                // not first comment -> do new line
                GBS_chrcat(tree_comment, '\n');
            }

            while (openBrackets && !error) {
                c = get_char();
                switch (c) {
                    case EOF:
                        setError("Reached end of file while reading comment");
                        break;
                    case ']':
                        openBrackets--;
                        if (openBrackets) GBS_chrcat(tree_comment, c); // write all but last closing brackets
                        break;
                    case '[':
                        openBrackets++;
                        // fall-through
                    default:
                        GBS_chrcat(tree_comment, c);
                        break;
                }
            }
        }
        else done = true;
    }

    last_character = c;
    return c;
}

int TreeReader::read_char() {
    int c = get_char();
    last_character = c;
    return c;
}

char *TreeReader::takeComment() {
    // can only be called once. Deletes the comment from TreeReader!
    char *comment = 0;
    if (tree_comment) {
        comment = GBS_strclose(tree_comment);
        tree_comment = 0;
    }
    return comment;
}

void TreeReader::eat_white() {
    int c = last_character;
    while ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')) {
        c = read_char();
    }
}

double TreeReader::eat_number() {
    char    strng[256];
    char   *s = strng;
    int     c = last_character;
    double  fl;

    while (((c<='9') && (c>='0')) || (c=='.') || (c=='-') || (c=='e') || (c=='E')) {
        *(s++) = c;
        c = read_char();
    }
    *s = 0;
    fl = GB_atof(strng);

    eat_white();
    return fl;
}

char *TreeReader::eat_quoted_string() {
    /*! Read in a quoted or unquoted string.
     * in quoted strings double quotes ('') are replaced by (')
     *
     * @return
     *     NULL in case of error,
     *     "" if no string present,
     *     otherwise the found string.
     */

    char  buffer[1024];
    char *s = buffer;
    int   c = last_character;

    if (c == '\'') {
        c = read_char();
        while ((c != EOF) && (c!='\'')) {
          gbt_lt_double_quot :
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setError(GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = read_char();
        }
        if (c == '\'') {
            c = read_tree_char();
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }
    else {
        while (c == '_') c = read_tree_char();
        while (c == ' ') c = read_tree_char();
        while ((c != ':') && (c != EOF) && (c!=',') &&
                (c!=';') && (c != ')'))
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setError(GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = read_tree_char();
        }
    }
    *s = 0;
    return strdup(buffer);
}

void TreeReader::setBranchName(GBT_TREE *node, char *name) {
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

        if (bootstrap > max_found_bootstrap) {
            max_found_bootstrap = bootstrap;
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

static char *unnamedNodeName() {
    static int i = 0;
    return GBS_global_string_copy("unnamed%i", ++i);
}

bool TreeReader::eat_and_set_name_and_length(GBT_TREE *node, GBT_LEN *len) {
    /* reads the branch-length and -name
       '*len' should normally be initialized with TREE_DEFLEN_MARKER
       * returns the branch-length in 'len' and sets the branch-name of 'node'
     * returns true if successful, otherwise reader->error gets set
     */

    bool done = false;
    while (!done && !error) {
        switch (last_character) {
            case ';':
            case ',':
            case ')':
                done = true;
                break;
            case ':':
                drop_tree_char(':');
                if (!error) {
                    *len = eat_number();
                    if (*len>max_found_branchlen) {
                        max_found_branchlen = *len;
                    }
                }
                break;
            case EOF:
                setError("Unexpected end of file");
                break;

            default: {
                char *branchName = eat_quoted_string();
                if (branchName) {
                    // if (branchName[0] == 0) freeset(branchName, unnamedNodeName());
                    setBranchName(node, branchName);
                }
                else {
                    UNCOVERED();
                    setError("Expected branch-name or one of ':;,)'");
                }
                break;
            }
        }
    }

    return !error;
}

static GBT_TREE *createLinkedTreeNode(GBT_TREE *left, GBT_LEN leftlen, GBT_TREE *right, GBT_LEN rightlen, int structuresize) {
    GBT_TREE *node = (GBT_TREE*)GB_calloc(1, structuresize);

    node->leftson  = left;
    node->leftlen  = leftlen;
    node->rightson = right;
    node->rightlen = rightlen;

    left->father  = node;
    right->father = node;

    return node;
}

GBT_TREE *TreeReader::load_tree(int structuresize, GBT_LEN *nodeLen) {
    GBT_TREE *node = 0;

    if (last_character == '(') {
        drop_tree_char('(');

        if (!error) {
            GBT_LEN   leftLen = TREE_DEFLEN_MARKER;
            GBT_TREE *left    = load_tree(structuresize, &leftLen);

            tree_assert(contradicted(left, error));

            if (left) {
                if (eat_and_set_name_and_length(left, &leftLen)) {
                    switch (last_character) {
                        case ')':               // single node
                            *nodeLen = leftLen;
                            node     = left;
                            left     = 0;
                            break;

                        case ',': {
                            GBT_LEN   rightLen = TREE_DEFLEN_MARKER;
                            GBT_TREE *right    = 0;

                            while (last_character == ',' && !error) {
                                if (right) { // multi-branch
                                    GBT_TREE *pair = createLinkedTreeNode(left, leftLen, right, rightLen, structuresize);

                                    left  = pair; leftLen = 0;
                                    right = 0; rightLen = TREE_DEFLEN_MARKER;
                                }

                                drop_tree_char(',');
                                if (!error) {
                                    right = load_tree(structuresize, &rightLen);
                                    if (right) eat_and_set_name_and_length(right, &rightLen);
                                }
                            }

                            if (!error) {
                                if (last_character == ')') {
                                    node     = createLinkedTreeNode(left, leftLen, right, rightLen, structuresize);
                                    *nodeLen = TREE_DEFLEN_MARKER;

                                    left = 0;
                                    right  = 0;

                                    drop_tree_char(')');
                                    if (error) GBT_delete_tree(node);
                                }
                                else {
                                    setError("Expected one of ',)'");
                                }
                            }

                            GBT_delete_tree(right);

                            break;
                        }

                        default:
                            setError("Expected one of ',)'");
                            break;
                    }
                }
                else {
                    tree_assert(error);
                }

                GBT_delete_tree(left);
            }
        }
    }
    else {                      // single node
        eat_white();
        char *name = eat_quoted_string();
        if (name) {
            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = true;
            node->name    = name;
        }
        else {
            UNCOVERED();
            setError("Expected (quoted) string");
        }
    }

    tree_assert(contradicted(node, error));
    return node;
}


GBT_TREE *TREE_load(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr) {
    /* Load a newick compatible tree from file 'path',
       structure size should be >0, see GBT_read_tree for more information
       if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
       if warningPtr != NULL -> set it to a malloc copy auto-scale-warning (if autoscaling happens)
    */

    GBT_TREE *tree  = NULL;
    FILE     *input = fopen(path, "rt");
    GB_ERROR  error = NULL;

    if (!input) {
        error = GBS_global_string("No such file: %s", path);
    }
    else {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else        name_only = path;

        TreeReader reader(input, name_only);
        if (!reader.error) {
            GBT_LEN rootNodeLen = TREE_DEFLEN_MARKER;     // root node has no length. only used as input to gbt_load_tree_rek
            tree                = reader.load_tree(structuresize, &rootNodeLen);
        }
        fclose(input);

        if (reader.error) {
            GBT_delete_tree(tree);
            error = reader.error;
        }

        if (tree) {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (reader.max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                if (warningPtr) {
                    *warningPtr = GBS_global_string_copy("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                                         bootstrap_scale, reader.max_found_bootstrap);
                }
            }
            if (reader.max_found_branchlen >= 1.1) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %.2f)\n"
                                                         "(use ARB_NT/Tree/Modify branches/Scale branchlengths with factor %.2f to undo auto-scaling)",
                                                         branchlen_scale, reader.max_found_branchlen, 1.0/branchlen_scale);
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
                char *comment = reader.takeComment();

                const char *loaded_from = GBS_global_string("Loaded from %s", path);
                freeset(comment, GBS_log_dated_action_to(comment, loaded_from));

                tree_assert(*commentPtr == 0);
                *commentPtr = comment;
            }
        }
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
        GBT_delete_tree(tree);                                                          \
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
        GBT_TREE *tree = loadFromFileContaining("()");
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

    TEST_EXPECT_TREESTRING_FAILS_WITH("(17",         "Unexpected end of file");
    TEST_EXPECT_TREESTRING_FAILS_WITH("((((",        "Unexpected end of file");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, 'b",      "Unexpected end of file");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5::::", "Unexpected end of file");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5:c:d", "Unexpected end of file");

    TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment",         "while reading comment");
    TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment [ bla ]", "while reading comment");

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
