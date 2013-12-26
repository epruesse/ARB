// =============================================================== //
//                                                                 //
//   File      : TreeWrite.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <TreeWrite.h>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <xml.hxx>

using namespace std;

#define tree_assert(cond) arb_assert(cond)

inline void replace_by_underscore(char *str, const char *toReplace) {
    char *unwanted = strpbrk(str, toReplace);
    while (unwanted) {
        unwanted[0] = '_';
        unwanted    = strpbrk(unwanted+1, toReplace);
    }
}

inline bool isQuoteChar(char c) { return c == '"' || c == '\''; }
inline bool whole_label_quoted(const char *label, size_t length) { return isQuoteChar(label[0]) && label[0] == label[length-1]; }

static void export_tree_label(const char *label, FILE *out, TREE_node_quoting qmode) {
    // writes a label into the Newick file
    // label is quoted if necessary
    // label may be an internal_node_label, a leaf_label or a root_label
    tree_assert(label);

    const char *disallowed_chars = " \t\'\"()[]:;,"; // '(' is first problem_char
    const char *problem_chars    = disallowed_chars+4;
    tree_assert(problem_chars[0] == '(');

    bool need_quotes  = strpbrk(label, disallowed_chars) != NULL;
    char used_quote   = 0;
    bool force_quotes = (qmode & TREE_FORCE_QUOTES);

    if (force_quotes || need_quotes) {
        if      (qmode&TREE_SINGLE_QUOTES) used_quote = '\'';
        else if (qmode&TREE_DOUBLE_QUOTES) used_quote = '\"';
    }

    char *fixed_label;
    {
        size_t label_length = strlen(label);
        fixed_label         = GB_strduplen(label, label_length);

        if (whole_label_quoted(fixed_label, label_length)) {
            // if whole label is quoted -> remove quotes
            memmove(fixed_label, fixed_label+1, label_length-2);
            fixed_label[label_length-2] = 0;
        }
    }

    if (used_quote) {
        // replace all problematic characters if requested
        bool force_replace = (qmode & TREE_FORCE_REPLACE);
        if (force_replace) replace_by_underscore(fixed_label, problem_chars);

        // replace used quote by an '_' if it appears inside label
        char used_quote_as_string[] = { used_quote, 0 };
        replace_by_underscore(fixed_label, used_quote_as_string);

        if (!force_quotes) {
            need_quotes = strpbrk(fixed_label, disallowed_chars) != NULL;
            if (!need_quotes) used_quote = 0; // @@@ fails if both quote-types are used in one name
        }
    }
    else {
        // unquoted label - always replace all problematic characters by '_'
        replace_by_underscore(fixed_label, disallowed_chars);
    }

    if (used_quote) fputc(used_quote, out);
    fputs(fixed_label, out);
    if (used_quote) fputc(used_quote, out);

    free(fixed_label);
}



// documentation of the Newick Format is in ../SOURCE_TOOLS/docs/newick_doc.html

inline void indentTo(int indent, FILE *out) {
    for (int i = 0; i < indent; i++) {
        putc(' ', out);
        putc(' ', out);
    }
}

static const char *export_tree_node_print(GBDATA *gb_main, FILE *out, GBT_TREE *tree, const char *tree_name,
                                          bool pretty, int indent,
                                          const TREE_node_text_gen *node_gen, bool save_branchlengths,
                                          bool save_bootstraps, bool save_groupnames, TREE_node_quoting qmode)
{
    const char *error = 0;
    const char *buf;

    if (pretty) indentTo(indent, out);

    if (tree->is_leaf) {
        if (node_gen) buf = node_gen->gen(gb_main, tree->gb_node, NDS_OUTPUT_LEAFTEXT, tree, tree_name);
        else          buf = tree->name;

        export_tree_label(buf, out, qmode);
    }
    else {
        if (pretty) fputs("(\n", out);
        else        putc('(', out);

        error = export_tree_node_print(gb_main, out, tree->leftson, tree_name, pretty, indent+1, node_gen, save_branchlengths, save_bootstraps, save_groupnames, qmode);
        if (save_branchlengths) fprintf(out, ":%.5f", tree->leftlen);
        fputs(",\n", out);

        if (error) return error;

        error = export_tree_node_print(gb_main, out, tree->rightson, tree_name, pretty, indent+1, node_gen, save_branchlengths, save_bootstraps, save_groupnames, qmode);
        if (save_branchlengths) fprintf(out, ":%.5f", tree->rightlen);
        fputc('\n', out);

        if (pretty) indentTo(indent, out);
        fputc(')', out);

        buf             = 0;
        char *bootstrap = 0;

        if (tree->remark_branch && save_bootstraps) {
            const char *boot = tree->remark_branch;
            if (boot[strlen(boot)-1] == '%') { // does remark_branch contain a bootstrap value ?
                char   *end = 0;
                double  val = strtod(boot, &end);
                tree_assert(end[0] == '%');        // otherwise sth strange is contained in remark_branch

                boot = GBS_global_string("%i", int(val+0.5));
            }
            bootstrap = strdup(boot);
        }

        if (tree->name && save_groupnames) buf = tree->name;

        const char *print = 0;
        if (buf) {
            if (bootstrap) print = GBS_global_string("%s:%s", bootstrap, buf);
            else           print = buf;
        }
        else if (bootstrap) print = bootstrap;

        if (print) export_tree_label(print, out, qmode);

        free(bootstrap);
    }

    return error;
}

inline string buildNodeIdentifier(const string& parent_id, int& son_counter) {
    ++son_counter;
    if (parent_id.empty()) return GBS_global_string("n_%i", son_counter);
    return GBS_global_string("%s.%i", parent_id.c_str(), son_counter);
}

static const char *export_tree_node_print_xml(GBDATA *gb_main, GBT_TREE *tree, double my_length, const char *tree_name,
                                              const TREE_node_text_gen *node_gen, bool skip_folded, const string& parent_id, int& parent_son_counter) {
    const char *error = 0;

    if (tree->is_leaf) {
        XML_Tag item_tag("ITEM");

        item_tag.add_attribute("name", buildNodeIdentifier(parent_id, parent_son_counter));

        item_tag.add_attribute("itemname",
                               node_gen
                               ? node_gen->gen(gb_main, tree->gb_node, NDS_OUTPUT_LEAFTEXT, tree, tree_name)
                               : tree->name);

        item_tag.add_attribute("length", GBS_global_string("%.5f", my_length));
    }
    else {
        char *groupname = 0;
        char *bootstrap = 0;

        if (tree->remark_branch) {
            const char *boot = tree->remark_branch;
            if (boot[0] && boot[strlen(boot)-1] == '%') { // does remark_branch contain a bootstrap value ?
                char   *end = 0;
                double  val = strtod(boot, &end);

                tree_assert(end[0] == '%');          // otherwise sth strange is contained in remark_branch
                bootstrap = GBS_global_string_copy("%i", int(val+0.5));
            }
        }
        bool folded = false;
        if (tree->name) {
            const char *buf;

            if (node_gen) buf = node_gen->gen(gb_main, tree->gb_node, NDS_OUTPUT_LEAFTEXT, tree, tree_name);
            else          buf = tree->name;

            tree_assert(buf);
            groupname = strdup(buf);

            GBDATA *gb_grouped = GB_entry(tree->gb_node, "grouped");
            if (gb_grouped) {
                folded = GB_read_byte(gb_grouped);
            }
        }

        if (my_length || bootstrap || groupname) {
            bool hide_this_group = skip_folded && folded; // hide folded groups only if skip_folded is true

            XML_Tag branch_tag(hide_this_group ? "FOLDED_GROUP" : "BRANCH");
            string  my_id = buildNodeIdentifier(parent_id, parent_son_counter);

            branch_tag.add_attribute("name", my_id);

            if (my_length) {
                branch_tag.add_attribute("length", GBS_global_string("%.5f", my_length));
            }
            if (bootstrap) {
                branch_tag.add_attribute("bootstrap", bootstrap);
                freenull(bootstrap);
            }
            if (groupname) {
                branch_tag.add_attribute("groupname", groupname);
                freenull(groupname);
                if (folded) branch_tag.add_attribute("folded", "1");
            }
            else {
                tree_assert(!folded);
            }

            if (hide_this_group) {
                branch_tag.add_attribute("items_in_group", GBT_count_leafs(tree));
            }
            else {
                int my_son_counter = 0;
                if (!error) error  = export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, tree_name, node_gen, skip_folded, my_id, my_son_counter);
                if (!error) error  = export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, tree_name, node_gen, skip_folded, my_id, my_son_counter);
            }
        }
        else {
            if (!error) error = export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, tree_name, node_gen, skip_folded, parent_id, parent_son_counter);
            if (!error) error = export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, tree_name, node_gen, skip_folded, parent_id, parent_son_counter);
        }
    }

    return error;
}

GB_ERROR TREE_write_XML(GBDATA *gb_main, const char *db_name, const char *tree_name, const TREE_node_text_gen *node_gen, bool skip_folded, const char *path) {
    GB_ERROR  error  = 0;
    FILE     *output = fopen(path, "w");

    if (!output) error = GB_export_errorf("file '%s' could not be opened for writing", path);
    else {
        GB_transaction gb_dummy(gb_main);

        GBT_TREE *tree   = GBT_read_tree(gb_main, tree_name, GBT_TREE_NodeFactory());
        if (!tree) error = GB_await_error();
        else {
            error = GBT_link_tree(tree, gb_main, true, 0, 0);
            if (!error && node_gen) node_gen->init(gb_main);

            if (!error) {
                GBDATA *tree_cont   = GBT_find_tree(gb_main, tree_name);
                GBDATA *tree_remark = GB_entry(tree_cont, "remark");

                XML_Document xml_doc("ARB_TREE", "arb_tree.dtd", output);

                xml_doc.add_attribute("database", db_name);
                xml_doc.add_attribute("treename", tree_name);
                xml_doc.add_attribute("export_date", GB_date_string());

                if (tree_remark) {
                    char *remark = GB_read_string(tree_remark);
                    XML_Tag  remark_tag("COMMENT");
                    XML_Text remark_text(remark);
                    free(remark);
                }

                int my_son_counter = 0;
                error              = export_tree_node_print_xml(gb_main, tree, 0.0, tree_name, node_gen, skip_folded, "", my_son_counter);
            }
        }
        fclose(output);
    }

    return error;
}

static char *complete_newick_comment(const char *comment) {
    // ensure that all '[' in 'comment' are closed by corresponding ']' by inserting additional brackets

    int            openBrackets = 0;
    GBS_strstruct *out          = GBS_stropen(strlen(comment)*1.1);

    for (int o = 0; comment[o]; ++o) {
        switch (comment[o]) {
            case '[':
                openBrackets++;
                break;
            case ']':
                if (openBrackets == 0) {
                    GBS_chrcat(out, '[');           // insert one
                }
                else {
                    openBrackets--;
                }
                break;

            default:
                break;
        }
        GBS_chrcat(out, comment[o]);
    }

    while (openBrackets>0) {
        GBS_chrcat(out, ']');                       // insert one
        openBrackets--;
    }

    tree_assert(openBrackets == 0);

    return GBS_strclose(out);
}

GB_ERROR TREE_write_Newick(GBDATA *gb_main, const char *tree_name, const TREE_node_text_gen *node_gen, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, TREE_node_quoting quoteMode, const char *path)
{
    GB_ERROR  error  = 0;
    FILE     *output = fopen(path, "w");

    if (!output) error = GB_export_errorf("file '%s' could not be opened for writing", path);
    else {
        GB_transaction gb_dummy(gb_main);

        GBT_TREE *tree   = GBT_read_tree(gb_main, tree_name, GBT_TREE_NodeFactory());
        if (!tree) error = GB_await_error();
        else {
            error = GBT_link_tree(tree, gb_main, true, 0, 0);
            if (!error && node_gen) node_gen->init(gb_main);

            if (!error) {
                char   *remark      = 0;
                GBDATA *tree_cont   = GBT_find_tree(gb_main, tree_name);
                GBDATA *tree_remark = GB_entry(tree_cont, "remark");

                if (tree_remark) {
                    remark = GB_read_string(tree_remark);
                }
                {
                    const char *saved_to = GBS_global_string("%s saved to %s", tree_name, path);
                    freeset(remark, GBS_log_dated_action_to(remark, saved_to));
                }

                if (remark) {
                    char *wellformed = complete_newick_comment(remark);

                    tree_assert(wellformed);

                    fputc('[', output); fputs(wellformed, output); fputs("]\n", output);
                    free(wellformed);
                }
                free(remark);
                if (!error) {
                    error = export_tree_node_print(gb_main, output, tree, tree_name, pretty, 0, node_gen,  save_branchlengths,  save_bootstraps,  save_groupnames, quoteMode);
                }
            }

            delete tree;
        }

        fprintf(output, ";\n");
        fclose(output);
    }

    return error;
}

// --------------------------------------------------------------------------------

static void export_tree_node_print_remove(char *str) {
    int i = 0;
    while (char c = str[i]) {
        if (c == '\'' || c == '\"') str[i] = '.';
        i++;
    }
}

static void export_tree_rek(GBT_TREE *tree, FILE *out, bool export_branchlens, bool dquot) {
    if (tree->is_leaf) {
        export_tree_node_print_remove(tree->name);
        fprintf(out,
                dquot ? " \"%s\" " : " '%s' ",
                tree->name);
    }
    else {
        fputc('(', out);
        export_tree_rek(tree->leftson,  out, export_branchlens, dquot); if (export_branchlens) fprintf(out, ":%.5f,", tree->leftlen);
        export_tree_rek(tree->rightson, out, export_branchlens, dquot); if (export_branchlens) fprintf(out, ":%.5f",  tree->rightlen);
        fputc(')', out);

        if (tree->name) {
            export_tree_node_print_remove(tree->name);
            fprintf(out,
                    dquot ? "\"%s\"" : "'%s'",
                    tree->name);
        }
    }
}

#if defined(WARN_TODO)
#warning maybe replace TREE_export_tree by TREE_write_Newick
// need some additional parameters (no comment, trifurcation)
#endif

GB_ERROR TREE_export_tree(GBDATA *, FILE *out, GBT_TREE *tree, bool triple_root, bool export_branchlens, bool dquot) {
    if (triple_root) {
        GBT_TREE *one, *two, *three;
        if (tree->is_leaf) {
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf && tree->rightson->is_leaf) {
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf) {
            one = tree->leftson;
            two = tree->rightson->leftson;
            three = tree->rightson->rightson;
        }
        else {
            one = tree->leftson->leftson;
            two = tree->leftson->rightson;
            three = tree->rightson;
        }
        fputc('(', out);
        export_tree_rek(one,   out, export_branchlens, dquot); if (export_branchlens) fprintf(out, ":%.5f", 1.0); fputc(',', out);
        export_tree_rek(two,   out, export_branchlens, dquot); if (export_branchlens) fprintf(out, ":%.5f", 1.0); fputc(',', out);
        export_tree_rek(three, out, export_branchlens, dquot); if (export_branchlens) fprintf(out, ":%.5f", 1.0);
        fputc(')', out);
    }
    else {
        export_tree_rek(tree, out, export_branchlens, dquot);
    }
    return 0;
}
