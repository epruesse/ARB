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

#define tree_assert(cond) arb_assert(cond)

/*!******************************************************************************************
                    load a tree from file system
********************************************************************************************/


/* -------------------- */
/*      TreeReader      */
/* -------------------- */

enum tr_lfmode { LF_UNKNOWN, LF_N, LF_R, LF_NR, LF_RN, };

typedef struct {
    char                 *tree_file_name;
    FILE                 *in;
    int                   last_character; // may be EOF
    int                   line_cnt;
    struct GBS_strstruct *tree_comment;
    double                max_found_branchlen;
    double                max_found_bootstrap;
    GB_ERROR              error;
    enum tr_lfmode        lfmode;
} TreeReader;

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
    /* can only be called once. Deletes the comment from TreeReader! */
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
    /* detect bootstrap values */
    /* name has to be stored in node or must be freed */

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
                gbt_read_char(reader);      /* drop ':' */
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

        tree_assert(left || reader->error);

        if (left) {
            if (gbt_readNameAndLength(reader, left, &leftLen)) {
                switch (reader->last_character) {
                    case ')':               /* single node */
                        *nodeLen = leftLen;
                        node     = left;
                        left     = 0;
                        break;

                    case ',': {
                        GBT_LEN   rightLen = TREE_DEFLEN_MARKER;
                        GBT_TREE *right    = 0;

                        while (reader->last_character == ',' && !reader->error) {
                            if (right) { /* multi-branch */
                                GBT_TREE *pair = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);

                                left  = pair; leftLen = 0;
                                right = 0; rightLen = TREE_DEFLEN_MARKER;
                            }

                            gbt_read_char(reader); /* drop ',' */
                            right = gbt_load_tree_rek(reader, structuresize, &rightLen);
                            if (right) gbt_readNameAndLength(reader, right, &rightLen);
                        }

                        if (reader->last_character == ')') {
                            node     = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                            *nodeLen = TREE_DEFLEN_MARKER;

                            left = 0;
                            right  = 0;

                            gbt_read_char(reader); /* drop ')' */
                        }
                        else {
                            setReaderError(reader, "Expected one of ',)'");
                        }

                        free(right);

                        break;
                    }

                    default:
                        setReaderError(reader, "Expected one of ',)'");
                        break;
                }
            }
            else {
                tree_assert(reader->error);
            }

            free(left);
        }
    }
    else {                      /* single node */
        gbt_eat_white(reader);
        char *name = gbt_read_quoted_string(reader);
        if (name) {
            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = true;
            node->name    = name;
        }
        else {
            setReaderError(reader, "Expected quoted string");
        }
    }

    tree_assert(node || reader->error);
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
        GBT_LEN     rootNodeLen = TREE_DEFLEN_MARKER; /* root node has no length. only used as input to gbt_load_tree_rek */
        tree                    = gbt_load_tree_rek(reader, structuresize, &rootNodeLen);
        fclose(input);

        if (reader->error) {
            GBT_delete_tree(tree);
            tree  = 0;
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
            if (reader->max_found_branchlen >= 1.01) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %5.2f)",
                                                         branchlen_scale, reader->max_found_branchlen);
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
                freeset(comment, TREE_log_action_to_tree_comment(comment, loaded_from));

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

