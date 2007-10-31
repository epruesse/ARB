

#ifndef _ALI_POSTREE_INC_
#define _ALI_POSTREE_INC_

#include <string.h>
// #include <malloc.h>

#include "ali_misc.hxx"
#include "ali_tlist.hxx"
#include "ali_tstack.hxx"

/*****************************************************************************
 *
 * STRUCT: ALI_POSTREE_NODE
 *
 *****************************************************************************/

enum ALI_POSTREE_NODE_TYPE {Node, Leaf};

struct ALI_POSTREE_NODE {
    ALI_POSTREE_NODE_TYPE typ;
    union {
        struct ALI_POSTREE_LEAF_PART {                          /* leaf part */
            unsigned long position;
            struct ALI_POSTREE_NODE *next;
        } leaf;
        struct ALI_POSTREE_NODE_PART {                          /* node part */
            unsigned long number_of_children;
            struct ALI_POSTREE_NODE *(*children)[];
        } node;
    };

    ALI_POSTREE_NODE(ALI_POSTREE_NODE_TYPE t,
                     unsigned long nochild_position = 0);
    ~ALI_POSTREE_NODE(void);

    int is_leaf(void) {
        return (typ == Leaf);
    }
    int is_node(void) {
        return (typ == Node);
    }

    ALI_POSTREE_NODE *leftmost_leaf(void);
    ALI_POSTREE_NODE *rightmost_leaf(void);
    ALI_POSTREE_NODE *link_leafs(ALI_POSTREE_NODE *last);
    void print(unsigned long depth);
};



/*****************************************************************************
 *
 * CLASS: ALI_POSTREE
 *
 *****************************************************************************/

struct ali_postree_sol {
    char *path;
    ALI_TLIST<unsigned long> *position_list;

    ali_postree_sol(char *p, ALI_TLIST<unsigned long> *pos) {
        path = p;
        position_list = pos;
    }
    ali_postree_sol(ALI_TSTACK<char> *stack, ALI_TLIST<unsigned long> *pos,
                    char *prefix = "", char *postfix = "") {
        unsigned long i, akt;
        position_list = pos;

        path = (char *) calloc(strlen(prefix) + (unsigned int)stack->akt_size() +
                               strlen(postfix) + 1, sizeof(char));
        if (path == 0)
            ali_fatal_error("Out of memory");

        for (akt = 0; akt < strlen(prefix); akt++)
            path[akt] = prefix[akt];
        for (i = 0; i < stack->akt_size(); i++, akt++)
            path[akt] = stack->get(i);
        for (i = 0; i < strlen(postfix); i++, akt++)
            path[akt] = postfix[i];

        path[akt] = '\0';
    }
    ~ali_postree_sol(void) {
        if (path)
            free((char *) path);
        if (position_list)
            delete position_list;
    }
};



#define ALI_POSTREE_STACK_INS 'I'
#define ALI_POSTREE_STACK_DEL 'D'
#define ALI_POSTREE_STACK_SUB 'S'



class ALI_POSTREE {
    unsigned long length_of_sequence;
    unsigned long number_of_branches;
    ALI_POSTREE_NODE *root;

    unsigned char *make_postree_sequence(
                                         unsigned char *seq, unsigned long seq_len,
                                         unsigned char terminal);
    void insert(unsigned char *seq, unsigned char terminal);
    void link_leafs(void);
    ali_postree_sol *make_postree_solution(
                                           ALI_POSTREE_NODE *first, ALI_POSTREE_NODE *last,
                                           unsigned long min_pos, unsigned long max_pos,
                                           unsigned long seq_len, unsigned long errors,
                                           ALI_TSTACK<char> *stack);
    unsigned long maximal_position(ALI_POSTREE_NODE *first,
                                   ALI_POSTREE_NODE *last);
    void handle_remaining_sequence(
                                   unsigned char *seq, unsigned long seq_len,
                                   unsigned long seq_pos, unsigned long im_seq_len,
                                   unsigned long min_pos, unsigned long max_pos,
                                   unsigned long ref_pos, unsigned long errors,
                                   ALI_TSTACK<char> *stack,
                                   ALI_TLIST<ali_postree_sol *> *sol_list);
    void finder(ALI_POSTREE_NODE *n,
                unsigned char *seq, unsigned long seq_len,
                unsigned long seq_pos, unsigned long im_seq_len,
                unsigned long min_pos, unsigned long max_pos,
                unsigned long errors,
                ALI_TSTACK<char> *stack,
                ALI_TLIST<ali_postree_sol *> *sol_list);

public:
    ALI_POSTREE(unsigned long branches,
                unsigned char *seq, unsigned long seq_len,
                unsigned char terminal = 4);
    ~ALI_POSTREE(void) {
        delete root;
    }
    ALI_TLIST<ali_postree_sol *> *find(
                                       unsigned char *seq, unsigned long seq_len,
                                       unsigned long min_pos, unsigned long max_pos,
                                       unsigned long errors = 0);
    ALI_TLIST<ali_postree_sol *> *find_complement(
                                                  unsigned char *seq, unsigned long seq_len,
                                                  unsigned long min_pos, unsigned long max_pos,
                                                  float costs = 0.0);
    void print(void);
};


#endif
