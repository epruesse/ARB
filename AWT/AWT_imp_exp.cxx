#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <awt.hxx>

#define _NO_AWT_NDS_WINDOW_FUNCTIONS
#include <awt_nds.hxx>

#include <xml.hxx>

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



// documentation of the Newick Format is in "./newick_doc.html"

static const char *awt_export_tree_node_print(GBDATA *gb_main, FILE *out, GBT_TREE *tree, int deep, AW_BOOL use_NDS, AW_BOOL save_branchlengths, AW_BOOL save_bootstraps, AW_BOOL save_groupnames)
{
    int             i;
    const char *error = 0;
    char *buf;
    for (i = 0; i < deep; i++) {
        putc(' ',out);
        putc(' ',out);
    }
    if (tree->is_leaf) {
        if (use_NDS) buf = make_node_text_nds(gb_main, tree->gb_node,0,tree);
        else buf         = tree->name;

        //         awt_export_tree_node_print_remove(buf);
        //         fprintf(out," '%s' ",buf);
        awt_export_tree_label(buf, out);
    }
    else {
        fprintf(out, "(\n");

        error = awt_export_tree_node_print(gb_main, out,tree->leftson, deep+1, use_NDS, save_branchlengths, save_bootstraps, save_groupnames);
        if (save_branchlengths) fprintf(out, ":%.5f", tree->leftlen);
        fputs(",\n", out);

        if (error) return error;

        error = awt_export_tree_node_print(gb_main, out, tree->rightson, deep+1, use_NDS, save_branchlengths, save_bootstraps, save_groupnames);
        if (save_branchlengths) fprintf(out, ":%.5f", tree->rightlen);
        fputc('\n', out);

        for (i = 0; i < deep; i++) fputs("  ", out);
        fputc(')', out);

        buf                   = 0;
        char *bootstrap = 0;

        if (tree->remark_branch && save_bootstraps) {
            const char *boot = tree->remark_branch;
            if (boot[strlen(boot)-1] == '%') { // does remark_branch contain a bootstrap value ?
                char   *end = 0;
                double  val = strtod(boot, &end);
                awt_assert(end[0] == '%'); // otherwise sth strange is contained in remark_branch
                boot        = GBS_global_string("%i", int(val+0.5));
            }
            bootstrap = GB_strdup(boot);
        }

        if (tree->name && save_groupnames) {
            if (use_NDS) buf = make_node_text_nds(gb_main, tree->gb_node,0,tree);
            else buf         = tree->name;
        }

        const char *print = 0;
        if (buf) {
            if (bootstrap) print = GBS_global_string("%s:%s", bootstrap, buf);
            else print           = buf;
        }
        else if (bootstrap) print = bootstrap;

        if (print) awt_export_tree_label(print, out);

        free(bootstrap);
    }

    return error;
}
// -----------------------------------------------------------------------------------------------------------------------------------------
//      static const char *awt_export_tree_node_print_xml(GBDATA *gb_main, GBT_TREE *tree, int deep, double my_length, AW_BOOL use_NDS)
// -----------------------------------------------------------------------------------------------------------------------------------------
static const char *awt_export_tree_node_print_xml(GBDATA *gb_main, GBT_TREE *tree, double my_length, AW_BOOL use_NDS) {
    const char *error = 0;

    if (tree->is_leaf) {
        XML_Tag species_tag("SPECIES");

        if (use_NDS) species_tag.add_attribute("name", make_node_text_nds(gb_main, tree->gb_node, 0, tree));
        else         species_tag.add_attribute("name", tree->name);

        species_tag.add_attribute("length", GBS_global_string("%.5f", my_length));
    }
    else {
        char *groupname = 0;
        char *bootstrap = 0;

        if (tree->remark_branch) {
            const char *boot = tree->remark_branch;
            if (boot[0] && boot[strlen(boot)-1] == '%') { // does remark_branch contain a bootstrap value ?
                char   *end = 0;
                double  val = strtod(boot, &end);
                awt_assert(end[0] == '%'); // otherwise sth strange is contained in remark_branch
                boot        = GBS_global_string("%i", int(val+0.5));

                bootstrap = GB_strdup(boot);
            }
        }
        if (tree->name) {
            char *buf;

            if (use_NDS) buf = make_node_text_nds(gb_main, tree->gb_node,0,tree);
            else buf         = tree->name;

            awt_assert(buf);
            groupname = GB_strdup(buf);
        }

        if (my_length || bootstrap || groupname ) {
            XML_Tag node_tag("NODE");

            if (my_length) node_tag.add_attribute("length", GBS_global_string("%.5f", my_length));
            if (bootstrap) {
                node_tag.add_attribute("bootstrap", bootstrap);
                free(bootstrap);
                bootstrap = 0;
            }
            if (groupname) {
                node_tag.add_attribute("groupname", groupname);
                free(groupname);
                groupname = 0;
            }

            error = awt_export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, use_NDS);
            error = awt_export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, use_NDS);
        }
        else {
            error = awt_export_tree_node_print_xml(gb_main, tree->leftson, tree->leftlen, use_NDS);
            error = awt_export_tree_node_print_xml(gb_main, tree->rightson, tree->rightlen, use_NDS);
        }
    }

    return error;
}

// -----------------------------------------------------------------------------------------------------------------
//      GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *tree_name, AW_BOOL use_NDS, const char *path)
// -----------------------------------------------------------------------------------------------------------------
GB_ERROR AWT_export_XML_tree(GBDATA *gb_main, const char *tree_name, AW_BOOL use_NDS, const char *path) {
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
            error = GBT_link_tree(tree,gb_main,GB_TRUE);
            if (!error && use_NDS) make_node_text_init(gb_main);

            if (!error) {
                GBDATA *tree_cont   = GBT_get_tree(gb_main,tree_name);
                GBDATA *tree_remark = GB_find(tree_cont, "remark", 0, down_level);

                XML_Document xml_doc("ARB_TREE", "arb_tree.dtd", output);

                {
                    char *remark            = 0;
                    if (tree_remark) remark = GB_read_string(tree_remark);
                    else  remark            = GB_strdup(GBS_global_string("ARB-tree '%s'", tree_name));

                    XML_Tag  remark_tag("COMMENT");
                    XML_Text remark_text(remark);
                    free(remark);
                }

                error = awt_export_tree_node_print_xml(gb_main,tree,0.0, use_NDS);
            }
        }
        fclose(output);
    }

    return error;
}

GB_ERROR AWT_export_tree(GBDATA *gb_main, char *tree_name, AW_BOOL use_NDS, AW_BOOL save_branchlengths, AW_BOOL save_bootstraps, AW_BOOL save_groupnames, char *path)
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
            error = GBT_link_tree(tree,gb_main,GB_TRUE);
            if (!error && use_NDS) make_node_text_init(gb_main);

            if (!error) {
                char   *remark      = 0;
                int     i;
                GBDATA *tree_cont   = GBT_get_tree(gb_main,tree_name);
                GBDATA *tree_remark = GB_find(tree_cont, "remark", 0, down_level);

                if (tree_remark) remark = GB_read_string(tree_remark);
                else remark             = GB_strdup(GBS_global_string("ARB-tree '%s'", tree_name));

                for (i = 0; remark[i]; ++i) if (remark[i] == ']') remark[i] = '_'; // replace ']' by '_'

                fprintf(output, "[%s]\n", remark);
                free(remark);
                error = awt_export_tree_node_print(gb_main,output,tree,0,use_NDS, save_branchlengths, save_bootstraps, save_groupnames);
            }

            GBT_delete_tree(tree);
        }

        fprintf(output, ";\n");
        fclose(output);
    }

    return error;
}
