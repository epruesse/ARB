#include <cstdio>
#include <cmath>
#include <cstring>
#include <memory.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>

#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "AP_error.hxx"

using namespace std;

/*********************************************
AP_ERR
*************************/

int AP_ERR::modus = 0;

AP_ERR::~AP_ERR()
{
    delete text;
}


AP_ERR::AP_ERR (const char *pntr)
    // setzt den Fehlertext und zeigt ihn an
{
    text = pntr;
    if (modus == 0) {
        cout << "\n*** WARNING *** \n" << text <<"\n";
        cout.flush();
    }
}

AP_ERR::AP_ERR (const char *pntr,const char *pntr2)
{
    text = pntr2;
    if (modus == 0) {
        cout << "\n***** WARNING  in " << pntr << "\n" << text <<"\n";
        cout.flush();
    }
}

AP_ERR::AP_ERR (const char *pntr,const char *pntr2,const int core)
{
    text = pntr2;
    cout << "\n*** FATAL ERROR *** " << core << " in " <<pntr << "\n" << text <<"\n";
    cout.flush();
    GB_CORE;
}

AP_ERR::AP_ERR (const char *pntr, const int core)
    // setzt den Fehlertext
    // bricht ab
{
    text = pntr;
    cout << "\n*** FATAL ERROR *** " << core << "\n" << text << "\n";
    cout.flush();
    GB_CORE;	// -segmantation Fault
}

const char *AP_ERR::show()
{
    return text;
}

void AP_ERR::set_mode(int i) {
    modus = i;
}

/**************

AP_main

***************/

AP_main::AP_main(void) {
    memset((char *)this,0,sizeof(AP_main));
}

AP_main::~AP_main(void) {
    if (use) delete use;
    if (stack) delete stack;
}

char *AP_main::open(char *db_server)
{
    char *error = 0 ;

    gb_main = GB_open(db_server,"rwt");
    if (!gb_main) return (char *)GB_get_error();
    if (error) return error;

    return 0;
}

void AP_main::user_push(void) {
    this->user_push_counter = stack_level + 1;
    this->push();
}

void AP_main::user_pop(void) {
    // checks if user_pop possible
    if (user_push_counter == stack_level) {
        this->pop();	// changes user_push_counter if user pop
    } else {
        new AP_ERR("AP_main::user_pop()","No user pop possible");
    }
    return;
}

void AP_main::push(void) {
    // if count > 1 the nodes are buffered more than once
    // WARNING:: node only has to be buffered once in the stack
    //
    //
    stack_level ++;
    if (stack) list.push(stack);
    stack = new AP_main_stack;
    stack->last_user_buffer = this->user_push_counter;
}

void AP_main::pop(void) {
    AP_tree *knoten;
    if (!stack) {
        new AP_ERR("AP_main::pop()","Stack underflow !");
        return;
    }
    while ( (knoten = stack->pop()) ) {
        if (stack_level != knoten->stack_level) {
            GB_internal_error("AP_main::pop: Error in stack_level");
            cout << "Main UPD - node UPD : " << stack_level << " -- " << knoten->stack_level << " \n";
            return;
        }
        knoten->pop(stack_level);
    }
    delete stack;
    stack_level --;
    stack = list.pop();

    if (stack) {
        user_push_counter = stack->last_user_buffer;
    }else{
        user_push_counter = 0;
    }
    return;
}

void AP_main::clear(void) {
    // removes count elements from the list
    // because the current tree is used
    //
    // if stack_counter greater than last user_push then
    // moves all not previous buffered nodes in the
    // previous stack

    AP_tree * knoten;
    AP_main_stack * new_stack;
    if (!stack) {
        new AP_ERR("AP_main::clear","Stack underflow !");
        return;
    }
    if (user_push_counter >= stack_level) {
        if (stack != 0) {
            if (stack->size() > 0) {
                while (stack->size() > 0) {
                    knoten = stack->pop();
                    //if (buffer_cout == AP_TRUE) knoten->printl();
                    knoten->clear(stack_level,user_push_counter);
                }
            }
            delete stack;
            stack = list.pop();
        }
    } else {
        if (stack) {
            new_stack = list.pop();
            while ( (knoten = stack->pop()) ) {
                if (knoten->clear(stack_level,user_push_counter) != AP_TRUE) {
                    // node is not cleared because buffered in previous node stack
                    // node is instead copied in previous level
                    if (new_stack) new_stack->push(knoten);
                }
            }
            delete stack;
            stack = new_stack;
        } else {
            new AP_ERR("AP_main::clear");
        }
    }
    stack_level --;
    if (stack) user_push_counter = stack->last_user_buffer;
    else user_push_counter = 0;

}

void AP_main::push_node(AP_tree * node,AP_STACK_MODE mode) {
    //
    //	stores node
    //
    if (!stack) {
        if (mode & SEQUENCE) 	node->unhash_sequence();
        return;
    }

    if ( stack_level < node->stack_level) {
        GB_warning("AP_main::push_node: stack_level < node->stack_level");
        return;
    }

    if (node->push(mode,stack_level)) 	stack->push(node);
}


void AP_main::set_tree_root(AP_tree *new_root) {
    // removes old root and sets it
    // to the father of the new_root
    *ap_main->tree_root = new_root;
}

