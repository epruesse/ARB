#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <awt.hxx>

#define _NO_AWT_NDS_WINDOW_FUNCTIONS
#include <awt_nds.hxx>

#include <xml.hxx>

using namespace std;

// static void awt_export_tree_node_print_remove(char *str)
// {
//     int i,len;
//     len = strlen (str);
//     for(i=0;i<len;i++) {
//         if (str[i] =='\'') str[i] ='.';
//         if (str[i] =='\"') str[i] ='.';
//     }
// }

// -------------------------------------------------------------------------
//      static void awt_export_tree_label(const char *label, FILE *out)
// -------------------------------------------------------------------------
// writes a label into the Newick file
// label is quoted if necessary
// label may be an internal_node_label, a leaf_label or a root_label
static void awt_export_tree_label(const char *label, FILE *out) {
    awt_assert(label);
    char *need_quotes = strpbrk(label, " \t()[]\'\":;,");

    if (need_quotes) {
        fputc('\'', out);
        while (*label) {
            char c = *label++;
            if (c == '\'') fputc('\'', out); // ' in quoted labels as ''
            fputc(c, out);
        }
        fputc('\'', out);
    }
    else {
        fputs(label, out);
    }
}



// documentation of the Newick Format is in ../SOURCE_TOOLS/docs/newick_doc.html

inline void indentTo(int indent, FILE *out) {
    for (int i = 0; i < indent; i++) {
        putc(' ',out);
        putc(' ',out);
    }
}

static const char *awt_export_tree_node_print(GBDATA *gb_main, FILE *out, GBT_TREE *tree, const char *tree_name,
                                              AW_BOOL pretty, int indent,
                                              AW_BOOL use_NDS, AW_BOOL save_branchlengths,
                                              AW_BOOL save_bootstraps, AW_BOOL save_groupnames)
{
    const char *error = 0;
    const char *buf;

    if (pretty) indentTo(indent, out);
    
    if (tree->is_leaf) {
        if (use_NDS)    buf = make_node_text_nds(gb_main, tree->gb_node,0,tree, tree_name);
        else            buf = tree->name;

        awt_export_tree_label(buf, out);
    }
    else {
        if (pretty) fputs("(\n", out);
        else        putc('(', out);

        error = awt_export_tree_node_print(gb_main, out, tree->leftson, tree_name, pretty, indent+1, use_NDS, save_branchlengths, save_bootstraps, save_groupnames);
        if (save_branchlengths) fprintf(out, ":%.5f", tree->leftlen);
        fputs(",\n", out);

        if (error) return error;

        error = awt_export_tree_node_print(gb_main, out, tree->rightson, tree_name, pretty, indent+1, use_NDS, save_branchlengths, save_bootstraps, save_groupnames);
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
                awt_assert(end[0] == '%');        // otherwise sth strange is contained in remark_branch
                
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

        if (print) awt_export_tree_label(print, out);

        free(bootstrap);
    }

    return error;
}

inline string buildNodeIdentifier(const string& parent_id, int& son_counter) {
    ++son_counter;
    if (parent_id.empty()) return GBS_global_string("n_%i", son_counter);
    return GBS_global_string("%s.%i", parent_id.c_str(), son_counter);
}

static const char *awt_export_tree_node_print_xml(GBDATA *gb_main, GBT_TREE *tree, double my_length, const char *tree_name,
                                                  AW_BOOL use_NDS, AW_BOOL skip_folded, const string& parent_id, int& parent_son_counter) {
    const char *error = 0;

    if (tree->is_leaf) {
        XML_Tag item_tag("ITEM");

        item_tag.add_attribute("name", buildNodeIdentifier(parent_id, parent_son_counter));

        item_tag.add_attribute("itemname",
                               use_NDS
                               ? make_node_text_nds(gb_main, tree->gb_node, 0, tree, tree_name)
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
                
                awt_assert(end[0] == '%');          // otherwise sth strange is contained in remark_branch
                bootstrap = GBS_global_string_copy("%i", int(val+0.5));
            }
        }
        bool folded = false;
        if (tree->name) {
            const char *buf;

            if (use_NDS) buf = make_node_text_nds(gb_main, tree->gb_node,0,tree, tree_name);
            else buf         = tree->name;

            awt_assert(buf);
            groupname = strdup(buf);

            GBDATA *gb_grouped = GB_entry(tree->gb_node, "grouped");
            if (gb_grouped) {
                folded = GB_read_byte(gb_grouped);
            }
        }

        if (my_length || bootstrap || groupname ) {
            bool hide_this_group = skip_folded && folded; // hide folded groups only if skip_folded is true

            XML_Tag branch_tag(hide_this_group ? "FOLDED_GROUP" : "BRANCH");
            string  my_id = buildNodeIdentifier(parent_id, parent_son_counter);

            branch_tag.add_attribute("name", my_id);

            if (my_length) {
                branch_tag.add_attribute("length", GBS_global_string("%.5f", my_length));
            }
            if (bootstrap) {
                branch_tag.add_attribute("bootstrap", bootstrap);
                freeset(bootstrap, 0);
            }
            if (groupname) {
                branch_tag.add_attribute("groupname", groupname);
                freeset(groupname, 0);
                if (folded) branch_tag.add_attribute("folded", "1");
            }
            else {
                awt_assert(!folded);
            }

            int my_son_counter = 0;
            if (hide_this_group) {
                branch_tag.add_attribute("items_in_group", GBT_count_nodes(tree));
            }
            else {
                if (!error) error  = awt_export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, tree_name, use_NDS, skip_folded, my_id, my_son_counter);
                if (!error) error  = awt_export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, tree_name, use_NDS, skip_folded, my_id, my_son_counter);
            }
        }
        else {
            if (!error) error = awt_export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, tree_name, use_NDS, skip_folded, parent_id, parent_son_counter);
            if (!error) error = awt_export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, tree_name, use_NDS, skip_folded, parent_id, parent_son_counter);
        }
    }

    return error;
}

// --------------------------------------------
//      const char * AWT_date_string(void)
// --------------------------------------------
const char * AWT_date_string(void) {
    struct timeval date;
    struct tm *p;

    gettimeofday(&date, 0);

#if defined(DARWIN)
    struct timespec local;
    TIMEVAL_TO_TIMESPEC(&date, &local); // not avail in time.h of Linux gcc 2.95.3
    p = localtime(&local.tv_sec);
#else
    p = localtime(&date.tv_sec);
#endif // DARWIN

    char *readable = asctime(p); // points to a static buffer
    char *cr       = strchr(readable, '\n');
    awt_assert(cr);
    cr[0]          = 0;         // cut of \n

    return readable;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, AW_BOOL use_NDS, AW_BOOL skip_folded, const char *path)
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *db_name, const char *tree_name, AW_BOOL use_NDS, AW_BOOL skip_folded, const char *path) {
    GB_ERROR  error  = 0;
    FILE     *output = fopen(path, "w");

    if (!output) error = GB_export_error("file '%s' could not be opened for writing", path);
    else {
        GB_transaction gb_dummy(gb_main);

        GBT_TREE *tree = GBT_read_tree(gb_main,tree_name,sizeof(GBT_TREE));
        if (!tree) {
            error = GB_get_error();
        }
        else {
            error = GBT_link_tree(tree,gb_main,GB_TRUE, 0, 0);
            if (!error && use_NDS) make_node_text_init(gb_main);

            if (!error) {
                GBDATA *tree_cont   = GBT_get_tree(gb_main,tree_name);
                GBDATA *tree_remark = GB_entry(tree_cont, "remark");

                XML_Document xml_doc("ARB_TREE", "arb_tree.dtd", output);

                xml_doc.add_attribute("database", db_name);
                xml_doc.add_attribute("treename", tree_name);
                xml_doc.add_attribute("export_date", AWT_date_string());

                if (tree_remark) {
                    char *remark = GB_read_string(tree_remark);
                    XML_Tag  remark_tag("COMMENT");
                    XML_Text remark_text(remark);
                    free(remark);
                }

                int my_son_counter = 0;
                error              = awt_export_tree_node_print_xml(gb_main,tree,0.0, tree_name, use_NDS, skip_folded, "", my_son_counter);
            }
        }
        fclose(output);
    }

    return error;
}

GB_ERROR AWT_export_Newick_tree(GBDATA *gb_main, char *tree_name, AW_BOOL use_NDS, AW_BOOL save_branchlengths, AW_BOOL save_bootstraps, AW_BOOL save_groupnames, AW_BOOL pretty, char *path)
{
    GB_ERROR  error  = 0;
    FILE     *output = fopen(path, "w");

    if (!output) error = GB_export_error("file '%s' could not be opened for writing", path);
    else {
        GB_transaction gb_dummy(gb_main);

        GBT_TREE *tree = GBT_read_tree(gb_main,tree_name,sizeof(GBT_TREE));
        if (!tree) {
            error = GB_get_error();
        }
        else {
            error = GBT_link_tree(tree,gb_main,GB_TRUE, 0, 0);
            if (!error && use_NDS) make_node_text_init(gb_main);

            if (!error) {
                char   *remark      = 0;
                GBDATA *tree_cont   = GBT_get_tree(gb_main,tree_name);
                GBDATA *tree_remark = GB_entry(tree_cont, "remark");

                if (tree_remark) remark = GB_read_string(tree_remark);
                else remark             = GBS_global_string_copy("ARB-tree '%s'", tree_name);

                {
                    char *escaped_remark = GBT_newick_comment(remark, GB_TRUE);

                    if (escaped_remark) {
                        fprintf(output, "[%s]\n", escaped_remark);
                        free(escaped_remark);
                    }
                    else {
                        error = GB_get_error();
                    }
                }
                free(remark);
                if (!error) {
                    error = awt_export_tree_node_print(gb_main, output, tree, tree_name, pretty, 0, use_NDS,  save_branchlengths,  save_bootstraps,  save_groupnames);
                }
            }

            GBT_delete_tree(tree);
        }

        fprintf(output, ";\n");
        fclose(output);
    }

    return error;
}
