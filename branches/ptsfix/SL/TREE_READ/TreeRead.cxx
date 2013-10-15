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

#include <arb_msg_fwd.h>
#include <arb_strbuf.h>
#include <arb_file.h>
#include <arb_defs.h>
#include <arbdbt.h>
#include <algorithm>

#define tree_assert(cond) arb_assert(cond)

/*!******************************************************************************************
                    load a tree from file system
********************************************************************************************/


// --------------------
//      TreeReader

class TreeReader : virtual Noncopyable {
    enum tr_lfmode { LF_UNKNOWN, LF_N, LF_R, LF_NR, LF_RN, };

    int   unnamed_counter;
    char *tree_file_name;
    FILE *in;
    int   last_character;          // may be EOF
    int   line_cnt;

    GBS_strstruct tree_comment;
    GBT_LEN       max_found_branchlen;
    double        max_found_bootstrap;
    tr_lfmode     lfmode;

    char *warnings;

    void setError(const char *message);
    void setErrorAt(const char *message);
    void setExpectedError(const char *expected);

    int get_char();
    int read_char();
    int read_tree_char(); // extracts comments and ignores whitespace outside comments

    char *content_ahead(size_t how_many, bool show_eof);

    void drop_tree_char(char expected);

    void setBranchName_acceptingBootstrap(GBT_TREE *node, char*& name);

    // The eat-functions below assume that the "current" character
    // has already been read into 'last_character':
    void  eat_white();
    __ATTR__USERESULT bool eat_number(GBT_LEN& result);
    char *eat_quoted_string();
    bool  eat_and_set_name_and_length(GBT_TREE *node, GBT_LEN& len);

    char *unnamedNodeName() { return GBS_global_string_copy("unnamed%i", ++unnamed_counter); }

    GBT_TREE *load_subtree(int structuresize, GBT_LEN& nodeLen);
    GBT_TREE *load_named_node(int structuresize, GBT_LEN& nodeLen);

public:

    TreeReader(FILE *input, const char *file_name);
    ~TreeReader();

    GBT_TREE *load(int structuresize) {
        GBT_LEN   rootNodeLen = TREE_DEFLEN_MARKER; // ignored dummy
        GBT_TREE *tree        = load_named_node(structuresize, rootNodeLen);

        if (!error) {
            if (rootNodeLen != TREE_DEFLEN_MARKER && rootNodeLen != 0.0) {
                add_warning("Length specified for root-node has been ignored");
            }

            // check for unexpected input
            if (last_character == ';') read_tree_char(); // accepts ';'
            if (last_character != EOF) {
                char *unused_input = content_ahead(30, false);
                add_warningf("Unexpected input-data after tree: '%s'", unused_input);
                free(unused_input);
            }
        }
        return tree;
    }

    GB_ERROR error;

    void add_warning(const char *msg) {
        if (warnings) freeset(warnings, GBS_global_string_copy("%s\n%s", warnings, msg));
        else warnings = GBS_global_string_copy("Warning(s): %s", msg);
    }
    __ATTR__FORMAT(2) void add_warningf(const char *format, ...) { FORWARD_FORMATTED(add_warning, format); }

    GB_ERROR get_warnings() const { return warnings; } // valid until TreeReader is destroyed

    char *takeComment() {
        // can only be called once (further calls will return NULL)
        return tree_comment.release();
    }

    double get_max_found_bootstrap() const { return max_found_bootstrap; }
    GBT_LEN get_max_found_branchlen() const { return max_found_branchlen; }
};

TreeReader::TreeReader(FILE *input, const char *file_name)
    : unnamed_counter(0),
      tree_file_name(strdup(file_name)),
      in(input),
      last_character(0),
      line_cnt(1),
      tree_comment(2048),
      max_found_branchlen(-1),
      max_found_bootstrap(-1),
      lfmode(LF_UNKNOWN),
      warnings(NULL),
      error(NULL)
{
    read_tree_char();
}

TreeReader::~TreeReader() {
    free(warnings);
    free(tree_file_name);
}

void TreeReader::setError(const char *message) {
    tree_assert(!error);
    error = GBS_global_string("Error reading %s:%i: %s",
                              tree_file_name, line_cnt, message);
}
char *TreeReader::content_ahead(size_t how_many, bool show_eof) {
    char show[how_many+1+4]; // 4 = oversize of '<EOF>'
    size_t i;
    for (i = 0; i<how_many; ++i) {
        show[i] = last_character;
        if (show[i] == EOF) {
            if (show_eof) {
                strcpy(show+i, "<EOF>");
                i += 5;
            }
            break;
        }
        read_char();
    }
    show[i] = 0;
    return strdup(show);
}

void TreeReader::setErrorAt(const char *message) {
    if (last_character == EOF) {
        setError(GBS_global_string("%s while end-of-file was reached", message));
    }
    else {
        char *show = content_ahead(30, true);
        setError(GBS_global_string("%s while looking at '%s'", message, show));
        free(show);
    }
}

void TreeReader::setExpectedError(const char *expected) {
    setErrorAt(GBS_global_string("Expected %s", expected));
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
            if (tree_comment.get_position()) {
                tree_comment.put('\n'); // not first comment -> add new line
            }

            while (openBrackets && !error) {
                c = get_char();
                switch (c) {
                    case EOF:
                        setError("Reached end of file while reading comment");
                        break;
                    case ']':
                        openBrackets--;
                        if (openBrackets) tree_comment.put(c); // write all but last closing brackets
                        break;
                    case '[':
                        openBrackets++;
                        // fall-through
                    default:
                        tree_comment.put(c);
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

void TreeReader::eat_white() {
    int c = last_character;
    while ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')) {
        c = read_char();
    }
}

bool TreeReader::eat_number(GBT_LEN& result) {
    char    strng[256];
    char   *s = strng;
    int     c = last_character;

    while (((c<='9') && (c>='0')) || (c=='.') || (c=='-') ||  (c=='+') || (c=='e') || (c=='E')) {
        *(s++) = c;
        c = read_char();
    }
    *s     = 0;
    result = GB_atof(strng);
    eat_white();

    bool consumed_some_length = strng[0];
    return consumed_some_length;
}

char *TreeReader::eat_quoted_string() {
    /*! Read in a quoted or unquoted string.
     * in quoted strings double quotes ('') are replaced by (').
     *
     * @return
     *     NULL in case of error,
     *     "" if no string present,
     *     otherwise the found string.
     */

    const int MAX_NAME_LEN = 1000;

    char  buffer[MAX_NAME_LEN+2];
    char *s = buffer;
    int   c = last_character;

#define NAME_TOO_LONG ((s-buffer)>MAX_NAME_LEN)

    if (c == '\'' || c == '"') {
        char found_quote = c;

        c = read_char();
        while (c!=EOF && c!=found_quote) {
            *(s++) = c;
            if (NAME_TOO_LONG) { c = 0; break; }
            c = read_char();
        }
        if (c == found_quote) c = read_tree_char();
    }
    else {
#if 0
        // previous behavior: skip prefixes matching PRE '_* *'
        // (reason unknown; behavior exists since [2])
        // conflicts with replacement of problematic character done in ../TREE_WRITE/TreeWrite.cxx@replace_by_underscore
        // -> disabled
        while (c == '_') c = read_tree_char();
        while (c == ' ') c = read_tree_char();
#endif
        while (c!=':' && c!=EOF && c!=',' && c!=';' && c != ')') {
            *(s++) = c;
            if (NAME_TOO_LONG) break;
            c = read_tree_char();
        }
    }
    *s = 0;
    if (NAME_TOO_LONG) {
        setError(GBS_global_string("Name '%s' is longer than %i bytes", buffer, MAX_NAME_LEN));
        return NULL;
    }
    return strdup(buffer);
}

void TreeReader::setBranchName_acceptingBootstrap(GBT_TREE *node, char*& name) {
    // store groupname and/or bootstrap value.
    //
    // ARBs extended newick format allows 3 kinds of node-names:
    //     'groupname'
    //     'bootstrap'
    //     'bootstrap:groupname' (needs to be quoted)
    //
    // where
    //     'bootstrap' is sth interpretable as double (optionally followed by '%')
    //     'groupname' is sth not interpretable as double
    //
    // If a groupname is detected, it is stored in node->name
    // If a bootstrap is detected, it is stored in node->remark_branch
    //
    // Bootstrap values will be scaled up by factor 100.
    // Wrong scale-ups (to 10000) will be corrected by calling TREE_scale() after the whole tree has been loaded.

    char *new_name = NULL;
    {
        char   *end       = 0;
        double  bootstrap = strtod(name, &end);

        bool is_bootstrap = (end != name);
        if (is_bootstrap) {
            if (end[0] == '%') {
                ++end;
                bootstrap = bootstrap/100.0; // percent -> [0..1]
            }
            is_bootstrap = end[0] == ':' || !end[0]; // followed by ':' or at EOS
        }

        if (is_bootstrap) {
            bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 (downscaling is done later)
            if (bootstrap > max_found_bootstrap) { max_found_bootstrap = bootstrap; }

            if (node->remark_branch) {
                error = "Invalid duplicated bootstrap specification detected";
            }
            else {
                node->remark_branch = GBS_global_string_copy("%i%%", int(bootstrap));
            }

            if (end[0] != 0) {      // sth behind bootstrap value
                arb_assert(end[0] == ':');
                new_name = strdup(end+1);
            }
            free(name);
        }
        else {
            new_name = name; // use whole input as groupname
        }
        name = NULL;
    }
    if (new_name) {
        if (node->name) {
            if (node->is_leaf) {
                add_warningf("Dropped group name specified for a single-node-subtree ('%s')\n", new_name);
                freenull(new_name);
            }
            else {
                add_warningf("Duplicated group name specification detected: dropped inner ('%s'), kept outer group name ('%s')\n",
                             node->name, new_name);
                freeset(node->name, new_name);
            }
        }
        else {
            node->name = new_name;
        }
    }
}

void TreeReader::drop_tree_char(char expected) {
    if (last_character != expected) {
        setExpectedError(GBS_global_string("'%c'", expected));
    }
    read_tree_char();
}

bool TreeReader::eat_and_set_name_and_length(GBT_TREE *node, GBT_LEN& nodeLen) {
    // reads optional branch-length and -name
    //
    // if 'nodeLen' contains TREE_DEFLEN_MARKER, it gets overwritten with any found length-specification
    // otherwise found length is added to 'nodeLen'
    //
    // sets the branch-name of 'node', if a name is found (e.g. sth like "(...)'name':0.5")
    //
    // returns true if successful, false otherwise (TreeReader::error is set then)

    bool done            = false;
    bool length_consumed = false;

    while (!done && !error) {
        switch (last_character) {
            case ';':
            case ',':
            case ')':
                done = true;
                break;
            case ':':
                if (!error && length_consumed) setErrorAt("Unexpected ':' (already read a branchlength)");
                if (!error) drop_tree_char(':');
                if (!error) {
                    GBT_LEN foundlen;
                    if (eat_number(foundlen)) {
                        if (is_marked_as_default_len(nodeLen)) {
                            nodeLen = foundlen;
                        }
                        else {
                            tree_assert(node->is_leaf); // should only happen when a single leaf in parenthesis was read
                            nodeLen += foundlen;        // sum leaf and node lengths
                        }
                        max_found_branchlen = std::max(max_found_branchlen, nodeLen);
                    }
                    else {
                        setExpectedError("valid length");
                    }
                }
                length_consumed = true;
                break;

            case EOF:
                done = true;
                break;

            default: {
                char *branchName = eat_quoted_string();
                if (branchName) {
                    if (branchName[0]) setBranchName_acceptingBootstrap(node, branchName);
                }
                else {
                    UNCOVERED();
                    setExpectedError("branch-name or one of ':;,)'");
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

GBT_TREE *TreeReader::load_named_node(int structuresize, GBT_LEN& nodeLen) {
    // reads a node or subtree.
    // a single node is expected to have a name (or will be auto-named)
    // subtrees may have a name (groupname)
    GBT_TREE *node = NULL;

    if (last_character == '(') {
        node = load_subtree(structuresize, nodeLen);
    }
    else {                      // single node
        eat_white();
        char *name = eat_quoted_string();
        if (name) {
            if (!name[0]) freeset(name, unnamedNodeName());

            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = true;
            node->name    = name;
        }
        else {
            UNCOVERED();
            setExpectedError("(quoted) string");
        }
    }
    if (node && !error) {
        if (!eat_and_set_name_and_length(node, nodeLen)) {
            GBT_delete_tree(node);
        }
    }
    tree_assert(contradicted(node, error));
    tree_assert(!node || !node->is_leaf || node->name); // leafs need to be named here
    return node;
}


GBT_TREE *TreeReader::load_subtree(int structuresize, GBT_LEN& nodeLen) {
    // loads a subtree (i.e. expects parenthesis around one or several nodes)
    //
    // 'nodeLen' normally is set to TREE_DEFLEN_MARKER
    //           or to length of single node (if parenthesis contain only one node)
    //
    // length and/or name behind '(...)' are not parsed (has to be done by caller).
    //
    // if subtree contains a single node (or a single other subtree), 'name'+'remark_branch' are
    // already set, when load_subtree() returns - otherwise they are NULL.

    GBT_TREE *node = NULL;

    drop_tree_char('(');

    GBT_LEN   leftLen = TREE_DEFLEN_MARKER;
    GBT_TREE *left    = load_named_node(structuresize, leftLen);

    if (left) {
        switch (last_character) {
            case ')':               // single node
                nodeLen = leftLen;
                node    = left;
                left    = 0;
                break;

            case ',': {
                GBT_LEN   rightLen = TREE_DEFLEN_MARKER;
                GBT_TREE *right    = NULL;

                while (last_character == ',' && !error) {
                    if (right) { // multi-branch
                        GBT_TREE *pair = createLinkedTreeNode(left, leftLen, right, rightLen, structuresize);

                        left  = pair; leftLen = 0;
                        right = 0; rightLen = TREE_DEFLEN_MARKER;
                    }

                    drop_tree_char(',');
                    if (!error) {
                        right = load_named_node(structuresize, rightLen);
                    }
                }

                if (!error) {
                    if (last_character == ')') {
                        node    = createLinkedTreeNode(left, leftLen, right, rightLen, structuresize);
                        nodeLen = TREE_DEFLEN_MARKER;

                        left  = NULL;
                        right = NULL;
                    }
                    else {
                        setExpectedError("one of ',)'");
                    }
                }

                GBT_delete_tree(right);
                if (error) GBT_delete_tree(node);

                break;
            }

            default:
                setExpectedError("one of ',)'");
                break;
        }
        GBT_delete_tree(left);
    }

    if (!error) drop_tree_char(')');

    tree_assert(contradicted(node, error));
    return node;
}

GBT_TREE *TREE_load(const char *path, int structuresize, char **commentPtr, bool allow_length_scaling, char **warningPtr) {
    /* Load a newick compatible tree from file 'path',
       structure size should be >0, see GBT_read_tree for more information
       if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
       if warningPtr != NULL -> set it to a malloc copy of any warnings occurring during tree-load (e.g. autoscale- or informational warnings)
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
        if (!reader.error) tree = reader.load(structuresize);
        fclose(input);

        if      (reader.error)          error = reader.error;
        else if (tree && tree->is_leaf) error = "tree is too small (need at least 2 species)";

        if (error) GBT_delete_tree(tree);

        if (tree) {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (reader.get_max_found_bootstrap() >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                reader.add_warningf("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                    bootstrap_scale, reader.get_max_found_bootstrap());
            }
            if (reader.get_max_found_branchlen() >= 1.1) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    reader.add_warningf("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %.2f)\n"
                                        "(use ARB_NT/Tree/Modify branches/Scale branchlengths with factor %.2f to undo auto-scaling)",
                                        branchlen_scale, reader.get_max_found_branchlen(), 1.0/branchlen_scale);
                }
            }

            TREE_scale(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths

            if (warningPtr) {
                const char *wmsg      = reader.get_warnings();
                if (wmsg) *warningPtr = strdup(wmsg);
            }

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

static GBT_TREE *loadFromFileContaining(const char *treeString, char **warningsPtr) {
    const char *filename = "trees/tmp.tree";
    FILE       *out      = fopen(filename, "wt");
    GBT_TREE   *tree     = NULL;

    if (out) {
        fputs(treeString, out);
        fclose(out);
        tree = TREE_load(filename, sizeof(GBT_TREE), NULL, false, warningsPtr);
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
    expected.add(that(GB_get_error()).is_equal_to_NULL());
    if (!GB_have_error() && tree) {
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

#define TEST_EXPECT_TREEFILE_FAILS_WITH(name,errpart) do {                          \
        GBT_TREE *tree = TREE_load(name, sizeof(GBT_TREE), NULL, false, NULL);      \
        TEST_EXPECT_TREELOAD_FAILED_WITH(tree, errpart);                            \
    } while(0)

#define TEST_EXPECT_TREESTRING_FAILS_WITH(treeString,errpart) do {      \
        GBT_TREE *tree = loadFromFileContaining(treeString, NULL);      \
        TEST_EXPECT_TREELOAD_FAILED_WITH(tree, errpart);                \
    } while(0)

// arguments 'names,count' are vs regression only!
#define TEST_EXPECT_TREESTRING_FAILS_WITH__BROKEN(treeString,errpart,names,count) do {  \
        char     *warnings = NULL;                                                      \
        GBT_TREE *tree     = loadFromFileContaining(treeString, &warnings);             \
        TEST_EXPECT_TREELOAD_FAILED_WITH__BROKEN(tree, errpart);                        \
        TEST_EXPECT_TREELOAD(tree, names, count);                                       \
        TEST_EXPECT_NULL(warnings);                                                     \
        GBT_delete_tree(tree);                                                          \
        free(warnings);                                                                 \
    } while(0)

#define TEST_EXPECT_TREESTRING_OK(treeString,names,count) do {                  \
        char     *warnings = NULL;                                              \
        GBT_TREE *tree     = loadFromFileContaining(treeString, &warnings);     \
        TEST_EXPECT_TREELOAD(tree, names, count);                               \
        TEST_EXPECT_NULL(warnings);                                             \
        GBT_delete_tree(tree);                                                  \
        free(warnings);                                                         \
    } while(0)

#define TEST_EXPECT_TREESTRING_OK_WITH_WARNING(treeString,names,count,warnPart) do {    \
        char     *warnings = NULL;                                                      \
        GBT_TREE *tree     = loadFromFileContaining(treeString, &warnings);             \
        TEST_EXPECT_TREELOAD(tree, names, count);                                       \
        TEST_REJECT_NULL(warnings);                                                     \
        TEST_EXPECT_CONTAINS(warnings, warnPart);                                       \
        GBT_delete_tree(tree);                                                          \
        free(warnings);                                                                 \
    } while(0)

#define TEST_EXPECT_TREESTRING_OK__BROKEN(treeString,names,count) do {  \
        GBT_TREE *tree = loadFromFileContaining(treeString, NULL);      \
        TEST_EXPECT_TREELOAD__BROKEN(tree, names, count);               \
    } while(0)

inline size_t countCommas(const char *str) {
    size_t commas = 0;
    for (size_t i = 0; str[i]; ++i) {
        commas += (str[i] == ',');
    }
    return commas;
}

void TEST_load_tree() {
    // just are few tests covering most of this module.
    // more load tests are in ../../TOOLS/arb_test.cxx@TEST_SLOW_arb_read_tree

    // simple succeeding tree load
    {
        char     *comment = NULL;
        GBT_TREE *tree    = TREE_load("trees/test.tree", sizeof(GBT_TREE), &comment, false, NULL);
        // -> ../../UNIT_TESTER/run/trees/test.tree

        TEST_EXPECT_TREELOAD(tree, "s1,s2,s3,s 4,s5,s-6", 6);
        if (tree) {
            TEST_REJECT_NULL(comment);
            TEST_EXPECT_CONTAINS(comment,
                                 // comment part from treefile:
                                 "tree covering most of tree reader code\n"
                                 "comment contains [extra brackets] inside comment\n");
            TEST_EXPECT_CONTAINS(comment,
                                 // comment as appended by load:
                                 ": Loaded from trees/test.tree\n");
        }
        free(comment);
        GBT_delete_tree(tree);
    }

    // detailed load tests (checking branchlengths and nodenames)
    {
        const char *treestring[] = {
            "(node1,node2)rootgroup;",             // [0] tree with a named root
            "(node1:0.00,(node2, node3:0.57)):0;", // [1] test tree lengths (esp. length zero)
            "(((((a))single)), ((b, c)17%:0.2));", // [2] test single-node-subtree name-conflict

            "((a,b)17,(c,d)33.3,(e,f)12.5:0.2);",             // [3] test bootstraps
            "((a,b)G,(c,d)H,(e,f)I:0.2);",                    // [4] test groupnames w/o bootstraps
            "((a,b)'17:G',(c,d)'33.3:H',(e,f)'12.5:I':0.2);", // [5] test groupnames with bootstraps
            "((a,b)17G,(c,d)33.3H,(e,f)12.5I:0.2)",           // [6] test groupnames + bootstraps w/o separator

            "((a,b)'17%:G',(c,d)'33.3%:H',(e,f)'12.5%:I':0.2);",  // [7] test bootstraps with percent spec
            "((a,b)'0.17:G',(c,d)'0.333:H',(e,f)'0.125:I':0.2);", // [8] test bootstraps in range [0..1]
        };

        const char *expected_nodes[] = {
            "node1,node2",
            "node1,node2,node3",
            "a,b,c",

            "a,b,c,d,e,f",
            "a,b,c,d,e,f",
            "a,b,c,d,e,f",
            "a,b,c,d,e,f",

            "a,b,c,d,e,f",
            "a,b,c,d,e,f",
        };
        const char *expected_warnings[] = {
            NULL,
            NULL,
            "Dropped group name specified for a single-node-subtree",

            "Auto-scaling bootstrap values by factor 0.01",
            NULL,
            "Auto-scaling bootstrap values by factor 0.01",
            NULL,
            NULL, // no auto-scaling shall occur here (bootstraps are already specified as percent)
            NULL, // no auto-scaling shall occur here (bootstraps are in [0..1])
        };

        STATIC_ASSERT(ARRAY_ELEMS(expected_nodes) == ARRAY_ELEMS(treestring));
        STATIC_ASSERT(ARRAY_ELEMS(expected_warnings) == ARRAY_ELEMS(treestring));

        for (size_t i = 0; i<ARRAY_ELEMS(treestring); ++i) {
            TEST_ANNOTATE(GBS_global_string("for tree #%zu = '%s'", i, treestring[i]));
            char     *warnings = NULL;
            GBT_TREE *tree     = loadFromFileContaining(treestring[i], &warnings);
            TEST_EXPECT_TREELOAD(tree, expected_nodes[i], countCommas(expected_nodes[i])+1);
            switch (i) {
                case 0:
                    TEST_EXPECT_EQUAL(tree->name, "rootgroup");
                    break;
                case 1:
                    TEST_EXPECT_EQUAL(tree->leftlen, 0);
                    TEST_EXPECT_EQUAL(tree->rightlen, TREE_DEFLEN);
                    TEST_EXPECT_EQUAL(tree->rightson->rightlen, 0.57);
                    break;
                case 2:
                    // test bootstrap with percent-specification is parsed correctly
                    TEST_EXPECT_NULL(tree->rightson->name);
                    TEST_EXPECT_EQUAL(tree->rightson->remark_branch, "17%");
                    TEST_EXPECT_EQUAL(tree->rightlen, 0.2);
                    break;

                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    // check bootstraps
                    TEST_EXPECT_NULL(tree->leftson->remark_branch);
                    switch (i) {
                        case 4:
                        case 6:
                            TEST_EXPECT_NULL(tree->leftson->leftson->remark_branch);
                            TEST_EXPECT_NULL(tree->leftson->rightson->remark_branch);
                            TEST_EXPECT_NULL(tree->rightson->remark_branch);
                            break;
                        case 3:
                        case 5:
                        case 7:
                        case 8:
                            TEST_EXPECT_EQUAL(tree->leftson->leftson->remark_branch,  "17%");
                            TEST_EXPECT_EQUAL(tree->leftson->rightson->remark_branch, "33%");
                            TEST_EXPECT_EQUAL(tree->rightson->remark_branch,          "13%");
                            break;
                        default:
                            TEST_REJECT(true); // unhandled tree
                            break;
                    }

                    // check node-names
                    TEST_EXPECT_NULL(tree->name);
                    TEST_EXPECT_NULL(tree->leftson->name);
                    switch (i) {
                        case 6:
                            // check un-separated digits are treated as strange names
                            // (previously these were accepted as bootstraps)
                            TEST_EXPECT_EQUAL(tree->leftson->leftson->name,  "17G");
                            TEST_EXPECT_EQUAL(tree->leftson->rightson->name, "33.3H");
                            TEST_EXPECT_EQUAL(tree->rightson->name,          "12.5I");
                            break;
                        case 4:
                        case 5:
                        case 8:
                        case 7:
                            TEST_EXPECT_EQUAL(tree->leftson->leftson->name,  "G");
                            TEST_EXPECT_EQUAL(tree->leftson->rightson->name, "H");
                            TEST_EXPECT_EQUAL(tree->rightson->name,          "I");
                            break;
                        case 3:
                            TEST_EXPECT_NULL(tree->leftson->leftson->name);
                            TEST_EXPECT_NULL(tree->leftson->rightson->name);
                            TEST_EXPECT_NULL(tree->rightson->name);
                            break;
                        default:
                            TEST_REJECT(true); // unhandled tree
                            break;
                    }

                    // expect_no_lengths:
                    TEST_EXPECT_EQUAL(tree->leftlen,           0); // multifurcation
                    TEST_EXPECT_EQUAL(tree->leftson->leftlen,  TREE_DEFLEN);
                    TEST_EXPECT_EQUAL(tree->leftson->rightlen, TREE_DEFLEN);
                    TEST_EXPECT_EQUAL(tree->rightlen,          0.2);
                    break;

                default:
                    TEST_REJECT(true); // unhandled tree
                    break;
            }
            if (expected_warnings[i]) {
                TEST_REJECT_NULL(warnings);
                TEST_EXPECT_CONTAINS(warnings, expected_warnings[i]);
            }
            else                      {
                TEST_EXPECT_NULL(warnings);
            }
            free(warnings);
            GBT_delete_tree(tree);
        }

        TEST_ANNOTATE(NULL);
    }

    // test valid trees with strange or wrong behavior
    TEST_EXPECT_TREESTRING_OK("(,);", "unnamed1,unnamed2", 2); // tree with 2 unamed species (weird, but ok)
    TEST_EXPECT_TREESTRING_OK("( a, (b,(c),d), (e,(f)) );", "a,b,c,d,e,f", 6);
    TEST_EXPECT_TREESTRING_OK("(((((a)))), ((b, c)));", "a,b,c", 3);

    TEST_EXPECT_TREESTRING_OK_WITH_WARNING("( (a), (((b),(c),(d))group)dupgroup, ((e),(f)) );", "a,b,c,d,e,f", 6,
                                           "Duplicated group name specification detected");

    // test unacceptable trees
    {
        const char *tooSmallTree[] = {
            "();",
            "()",
            ";",
            "",
            "(one)",
            "((((()))));",
            "(((((one)))));",
        };

        for (size_t i = 0; i<ARRAY_ELEMS(tooSmallTree); ++i) {
            TEST_ANNOTATE(GBS_global_string("for tree #%zu = '%s'", i, tooSmallTree[i]));
            GBT_TREE *tree = loadFromFileContaining(tooSmallTree[i], NULL);
            TEST_EXPECT_TREELOAD_FAILED_WITH(tree, "tree is too small");
        }
        TEST_ANNOTATE(NULL);
    }
    {
        GBT_TREE *tree = loadFromFileContaining("((a, b)25)20;", NULL);
        TEST_EXPECT_TREELOAD_FAILED_WITH(tree, "Invalid duplicated bootstrap specification detected");
    }

    // test invalid trees
    TEST_EXPECT_TREESTRING_FAILS_WITH("(;);", "Expected one of ',)'");

    TEST_EXPECT_TREESTRING_FAILS_WITH("(17",    "Expected one of ',)' while end-of-file was reached");
    TEST_EXPECT_TREESTRING_FAILS_WITH("((((",   "Expected one of ',)' while end-of-file was reached");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, 'b", "Expected one of ',)' while end-of-file was reached");

    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5::::",  "Unexpected ':' (already read a branchlength) while looking at '::::<EOF>'");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5:c:d",  "Unexpected ':' (already read a branchlength) while looking at ':c:d<EOF>'");
    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:5:c:d)", "Unexpected ':' (already read a branchlength) while looking at ':c:d)<EOF>'");

    TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment",         "while reading comment");
    TEST_EXPECT_TREESTRING_FAILS_WITH("[unclosed\ncomment [ bla ]", "while reading comment");

    TEST_EXPECT_TREESTRING_FAILS_WITH("(a, b:d)", "Expected valid length while looking at 'd)<EOF>'");
    TEST_EXPECT_TREESTRING_OK("(a, b:5)", "a,b", 2);

    // questionable accepted trees / check warnings

    TEST_EXPECT_TREESTRING_OK_WITH_WARNING("(a,b):0.5", "a,b", 2, "Length specified for root-node has been ignored");
    TEST_EXPECT_TREESTRING_OK_WITH_WARNING("(a, b))", "a,b", 2, "Unexpected input-data after tree: ')'");

    TEST_EXPECT_TREESTRING_OK("(a*,b%);", "a*,b%", 2); // @@@ really accept such names?

    // check errors
    TEST_EXPECT_TREEFILE_FAILS_WITH("trees/nosuch.tree",    "No such file");
    TEST_EXPECT_TREEFILE_FAILS_WITH("trees/corrupted.tree", "Error reading");

    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink("trees/tmp.tree")); // cleanup
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
