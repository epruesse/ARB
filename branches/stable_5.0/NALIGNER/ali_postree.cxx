
// #include <malloc.h>

#include "ali_misc.hxx"
#include "ali_tlist.hxx"
#include "ali_postree.hxx"



/*****************************************************************************
 *
 * STRUCT: ALI_POSTREE_NODE
 *
 *****************************************************************************/


ALI_POSTREE_NODE::ALI_POSTREE_NODE(ALI_POSTREE_NODE_TYPE t,
                                   unsigned long nochild_position)
{
    typ = t;
    if (typ == Node) {
        node.number_of_children = nochild_position;
        node.children = (ALI_POSTREE_NODE *(*) [])
            CALLOC((unsigned int) nochild_position,
                   sizeof(ALI_POSTREE_NODE *));
        if (node.children == 0)
            ali_fatal_error("Out of memory");
    }
    else {
        leaf.position = nochild_position;
        leaf.next = 0;
    }
}

ALI_POSTREE_NODE::~ALI_POSTREE_NODE(void)
{
    if (typ == Node) {
        while (node.number_of_children-- > 0) {
            if ((*node.children)[node.number_of_children])
                delete (*node.children)[node.number_of_children];
        }
        free((char *) node.children);
    }
}

ALI_POSTREE_NODE *ALI_POSTREE_NODE::leftmost_leaf(void)
{
    ALI_POSTREE_NODE *n = this;
    long i;

    while (n->typ == Node) {
        for (i = 0;
             i < n->node.number_of_children && !(*n->node.children)[i]; i++);
        if (i < n->node.number_of_children)
            n = (*n->node.children)[i];
        else
            ali_fatal_error("Found node without leaf",
                            "ALI_POSTREE_NODE::leftmost_leaf()");
    }
    return n;
}

ALI_POSTREE_NODE *ALI_POSTREE_NODE::rightmost_leaf(void)
{
    ALI_POSTREE_NODE *n = this;
    long i;

    while (n->typ == Node) {
        for (i = (long) n->node.number_of_children - 1;
             i >= 0 && !(*n->node.children)[i]; i--);
        if (i >= 0)
            n = (*n->node.children)[i];
        else
            ali_fatal_error("Found node without leaf",
                            "ALI_POSTREE_NODE::rightmost_leaf()");
    }
    return n;
}

ALI_POSTREE_NODE *ALI_POSTREE_NODE::link_leafs(ALI_POSTREE_NODE *last)
{
    long i;

    if (typ == Node) {
        for (i = (long) node.number_of_children - 1; i >= 0; i--)
            if ((*node.children)[i])
                last = (*node.children)[i]->link_leafs(last);
    }
    else {
        leaf.next = last;
        last = this;
    }
    return last;
}


void ALI_POSTREE_NODE::print(unsigned long depth)
{
    unsigned long l, nr;


    if (typ == Node) {
        for (nr = 0; nr < node.number_of_children; nr++) {
            for (l = 0; l < depth; l++)
                printf("    ");
            printf("%2d:\n",nr);
            if ((*node.children)[nr])
                (*node.children)[nr]->print(depth+1);
        }
    }
    else {
        for (l = 0; l < depth; l++)
            printf("    ");
        printf("<%d>\n",leaf.position);
    }
}


/*****************************************************************************
 *
 * CLASS: ALI_POSTREE  (PRIVAT)
 *
 * DESCRIPTION: Implementation of a position tree
 *
 *****************************************************************************/

unsigned char *ALI_POSTREE::make_postree_sequence(
                                                  unsigned char *seq, unsigned long seq_len, unsigned char terminal)
{
    unsigned char *buffer, *src, *dst;

    buffer = (unsigned char *) CALLOC((unsigned int) seq_len + 1,
                                      sizeof(unsigned char));
    if (buffer == 0)
        ali_fatal_error("Out of memory");

    for (src = seq, dst = buffer; src < seq + seq_len;)
        *dst++ = *src++;

    *dst = terminal;

    return buffer;
}

void ALI_POSTREE::insert(unsigned char *seq, unsigned char terminal)
{
    ALI_POSTREE_NODE *v;
    unsigned char *akt, *pos = seq;
    unsigned long position;

    for (position = 0; *pos != terminal && *pos < number_of_branches;
         position++, pos++) {
        akt = pos;
        v = root;
        /*
         * Find maximal equal path in POSTREE
         */
        while ((*v->node.children)[*akt] && (*v->node.children)[*akt]->is_node())
            v = (*v->node.children)[*akt++];
        /*
         * Make a unique leaf for the new prefix
         */
        if (!(*v->node.children)[*akt])
            (*v->node.children)[*akt] = new ALI_POSTREE_NODE(Leaf,position);
        else {
            ALI_POSTREE_NODE *old_leaf = (*v->node.children)[*akt];
            unsigned char *akt2 = (unsigned char *)
                ((int) seq + (int) old_leaf->leaf.position +
                 (int) akt - (int) pos);
            /*
             * Expande equal part of path in prefix tree
             */
            while (*akt == *akt2) {
                (*v->node.children)[*akt] = new ALI_POSTREE_NODE(Node,
                                                                 number_of_branches);
                v = (*v->node.children)[*akt];
                akt++;
                akt2++;
            }
            (*v->node.children)[*akt2] = old_leaf;
            (*v->node.children)[*akt] = new ALI_POSTREE_NODE(Leaf,position);
        }
    }

    if (*pos >= number_of_branches)
        ali_fatal_error("Unexpected value","ALI_POSTREE::insert()");
    if ((*root->node.children)[*pos])
        ali_fatal_error("Terminal occupied","ALI_POSTREE::insert()");
    else
        (*root->node.children)[*pos] = new ALI_POSTREE_NODE(Leaf,position);
}


void ALI_POSTREE::link_leafs(void)
{
    ALI_POSTREE_NODE *first;

    first = root->link_leafs(0);
}


ali_postree_sol *ALI_POSTREE::make_postree_solution(
                                                    ALI_POSTREE_NODE *first, ALI_POSTREE_NODE *last,
                                                    unsigned long min_pos, unsigned long max_pos,
                                                    unsigned long seq_len, unsigned long errors,
                                                    ALI_TSTACK<char> *stack)
{
    unsigned long i;
    ali_postree_sol *solution = 0;
    ALI_TLIST<unsigned long> *pos_list = 0;

    /*
     * Make list of positions and make _insertations_ for the errors
     */
    while (first != last) {
        if (first->leaf.position >= min_pos && first->leaf.position <= max_pos &&
            first->leaf.position + seq_len + errors <= length_of_sequence) {
            if (pos_list == 0)
                pos_list = new ALI_TLIST<unsigned long>(first->leaf.position);
            else
                pos_list->append_end(first->leaf.position);
        }
        first = first->leaf.next;
    }
    if (first->leaf.position >= min_pos && first->leaf.position <= max_pos &&
        first->leaf.position + seq_len + errors <= length_of_sequence) {
        if (pos_list == 0)
            pos_list = new ALI_TLIST<unsigned long>(first->leaf.position);
        else
            pos_list->append_end(first->leaf.position);
    }

    /*
     * Make solution with _expanded_ path
     */
    if (pos_list != 0) {
        for (i = 0; i < errors; i++)
            stack->push(ALI_POSTREE_STACK_INS);
        solution = new ali_postree_sol(stack,pos_list);
        for (i = 0; i < errors; i++)
            stack->pop();
    }

    return solution;
}


unsigned long ALI_POSTREE::maximal_position(ALI_POSTREE_NODE *first,
                                            ALI_POSTREE_NODE *last)
{
    unsigned long maximum;

    maximum = first->leaf.position;

    while (first != last) {
        first = first->leaf.next;
        if (first->leaf.position > maximum)
            maximum = first->leaf.position;
    }

    return maximum;
}


void ALI_POSTREE::handle_remaining_sequence(
                                            unsigned char *seq, unsigned long seq_len,
                                            unsigned long seq_pos, unsigned long im_seq_len,
                                            unsigned long min_pos, unsigned long max_pos,
                                            unsigned long ref_pos, unsigned long errors,
                                            ALI_TSTACK<char> *stack,
                                            ALI_TLIST<ali_postree_sol *> *sol_list)
{
    ali_postree_sol *solution, *new_solution;
    ALI_TSTACK<char> *new_stack;
    ALI_TLIST<unsigned long> *pos_list;
    ALI_TLIST<ali_postree_sol *> *new_sol_list;
    int ok_flag = 0;

    if (ref_pos < min_pos || ref_pos > max_pos)
        return;

    new_sol_list = new ALI_TLIST<ali_postree_sol *>;
    new_stack = new ALI_TSTACK<char>(seq_len - seq_pos + errors + 1);

    finder(root,seq + seq_pos,seq_len - seq_pos, 0, 0,
           ref_pos + im_seq_len,ref_pos + im_seq_len,errors,
           new_stack,new_sol_list);

    /*
     * Generate solutions
     */
    if (!new_sol_list->is_empty()) {
        new_solution = new_sol_list->first();
        pos_list = new ALI_TLIST<unsigned long>(ref_pos);
        solution = new ali_postree_sol(stack,pos_list,"",new_solution->path);
        delete new_solution;
        sol_list->append_end(solution);
        while (new_sol_list->is_next()) {
            new_solution = new_sol_list->next();
            pos_list = new ALI_TLIST<unsigned long>(ref_pos);
            solution = new ali_postree_sol(stack,pos_list,"",new_solution->path);
            delete new_solution;
            sol_list->append_end(solution);
        }
    }
    delete new_stack;
    delete new_sol_list;
}


void ALI_POSTREE::finder(ALI_POSTREE_NODE *n,
                         unsigned char *seq, unsigned long seq_len,
                         unsigned long seq_pos, unsigned long im_seq_len,
                         unsigned long min_pos, unsigned long max_pos,
                         unsigned long errors,
                         ALI_TSTACK<char> *stack,
                         ALI_TLIST<ali_postree_sol *> *sol_list)
{
    ALI_POSTREE_NODE *first, *last;
    ali_postree_sol *solution = 0;
    unsigned long i;

    /*
     * Found end of sequence
     */
    if (seq_pos >= seq_len) {
        if (n->is_leaf()) {
            first = n;
            last = n;
        }
        else {
            first = n->leftmost_leaf();
            last = n->rightmost_leaf();
        }
        solution = make_postree_solution(first,last,min_pos,max_pos,
                                         im_seq_len,errors,stack);
        if (solution)
            sol_list->append_end(solution);
    }
    else {
        /*
         * Found unique position
         */
        if (n->is_leaf()) {
            handle_remaining_sequence(seq,seq_len,seq_pos,im_seq_len,
                                      min_pos,max_pos,n->leaf.position,errors,
                                      stack,sol_list);
        }
        /*
         * Recursive search
         */
        else {
            if ((*n->node.children)[seq[seq_pos]]) {
                stack->push(ALI_POSTREE_STACK_SUB);
                finder((*n->node.children)[seq[seq_pos]],seq,seq_len,seq_pos + 1,
                       im_seq_len + 1,min_pos, max_pos, errors, stack, sol_list);
                stack->pop();
            }
            /*
             * Recursive search with errors
             */
            if (errors > 0) {
                /*
                 * Deletion (in seq)
                 */
                stack->push(ALI_POSTREE_STACK_DEL);
                finder(n,seq,seq_len,seq_pos + 1,im_seq_len,min_pos,max_pos,
                       errors - 1,stack,sol_list);
                stack->pop();

                for (i = 0; i < number_of_branches; i++) {
                    if ((*n->node.children)[i]) {
                        /*
                         * Insertion (in seq)
                         */
                        stack->push(ALI_POSTREE_STACK_INS);
                        finder((*n->node.children)[i],seq,seq_len,seq_pos,
                               im_seq_len + 1,min_pos, max_pos, errors - 1,
                               stack, sol_list);
                        stack->pop();
                        /*
                         * Substitution
                         */
                        if (i != seq[seq_pos]) {
                            stack->push(ALI_POSTREE_STACK_SUB);
                            finder((*n->node.children)[i],seq,seq_len,seq_pos + 1,
                                   im_seq_len + 1, min_pos, max_pos, errors - 1,
                                   stack, sol_list);
                            stack->pop();
                        }
                    }
                }
            }
        }
    }
}





/*****************************************************************************
 *
 * CLASS: ALI_POSTREE  (PUBLIC)
 *
 * DESCRIPTION: Implementation of a position tree
 *
 *****************************************************************************/

ALI_POSTREE::ALI_POSTREE(unsigned long branches,
                         unsigned char *seq, unsigned long seq_len,
                         unsigned char terminal)
{
    unsigned char *seq_buffer;

    length_of_sequence = seq_len;
    number_of_branches = branches + 1;
    root = new ALI_POSTREE_NODE(Node,number_of_branches);

    seq_buffer = make_postree_sequence(seq,seq_len,terminal);
    insert(seq_buffer,terminal);
    link_leafs();

    free((char *) seq_buffer);
}


ALI_TLIST<ali_postree_sol *> *ALI_POSTREE::find(
                                                unsigned char *seq, unsigned long seq_len,
                                                unsigned long min_pos, unsigned long max_pos,
                                                unsigned long errors)
{
    ALI_TLIST<ali_postree_sol *> *sol_list;
    ALI_TSTACK<char> *stack;

    sol_list = new ALI_TLIST<ali_postree_sol *>;
    stack = new ALI_TSTACK<char>(seq_len + errors + 1);

    finder(root,seq,seq_len,0,0,min_pos,max_pos,errors,stack,sol_list);

    delete stack;

    return sol_list;
}


ALI_TLIST<ali_postree_sol *> *ALI_POSTREE::find_complement(
                                                           unsigned char *seq, unsigned long seq_len,
                                                           unsigned long min_pos, unsigned long max_pos,
                                                           float max_costs)
{
    ALI_TLIST<ali_postree_sol *> *sol_list;
    ALI_TSTACK<char> *stack;

    sol_list = new ALI_TLIST<ali_postree_sol *>;
    stack = new ALI_TSTACK<char>(2 * seq_len);

    /*
      compl_finder(root,seq,seq_len,0,0,min_pos,max_pos,max_costs,stack,sol_list);
    */

    delete stack;

    return sol_list;
}


void ALI_POSTREE::print(void)
{
    root->print(0);
}





/**************************
 *
 * TESTPART
 *
 **************************

unsigned char seq[] = {1,1,0,1,1,3,1,1,5,1,1};
unsigned long seq_len = 11;

unsigned char fseq[] = {1,1,3,1,1};
unsigned long fseq_len = 5;

void print_sol(ALI_TLIST<ali_postree_sol *> *sol_list)
{
   int i;
   ALI_TLIST<unsigned long> *pos_list;
        ali_postree_sol *solution;

   printf("SOL:\n");
   i = 1;
        while (!sol_list->is_empty()) {
      solution = sol_list->first();
                pos_list = solution->position_list;
                sol_list->delete_element();
                if (!pos_list->is_empty()) {
                   printf("%2d <%s> : %d",i,solution->path,pos_list->first());
                        while (pos_list->is_next())
                                printf(", %d",pos_list->next());
                }
                else
                        printf("%2d : empty",i);
                printf("\n");
                delete solution;
                i++;
        }
        delete sol_list;
}


main()
{
   ALI_TLIST<ali_postree_sol *> *sol_list;
   ALI_POSTREE pos_tree(5,seq,seq_len,4);

        pos_tree.print();

   printf("0 Fehler\n");
   sol_list = pos_tree.find(fseq,fseq_len,0,10,0);
        print_sol(sol_list);
        printf("1 Fehler\n");
   sol_list = pos_tree.find(fseq,fseq_len,0,10,1);
        print_sol(sol_list);
        printf("2 Fehler\n");
   sol_list = pos_tree.find(fseq,fseq_len,0,10,2);
        print_sol(sol_list);
        printf("3 Fehler\n");
   sol_list = pos_tree.find(fseq,fseq_len,0,10,3);
        print_sol(sol_list);
}

***************************************/
