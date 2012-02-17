// ================================================================ //
//                                                                  //
//   File      : ed4_list.hxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ED4_LIST_HXX
#define ED4_LIST_HXX

class ED4_list_elem : virtual Noncopyable {
    void          *my_elem;
    ED4_list_elem *my_next;
public:
    ED4_list_elem(void *element) { my_elem = element; my_next = 0; }
    ~ED4_list_elem() {}

    ED4_list_elem *next() const { return my_next; }
    void *elem() const { return my_elem; }

    void set_next(ED4_list_elem *the_next) { my_next = the_next; }
};

class ED4_list : virtual Noncopyable {
    // class which implements a general purpose linked list of void*

    ED4_list_elem *my_first;
    ED4_list_elem *my_last;
    ED4_index      my_no_of_entries;

public:

    ED4_list_elem *first() const { return my_first; }
    ED4_list_elem *last() const { return my_last; }
    ED4_index no_of_entries() const { return my_no_of_entries; }

    ED4_returncode  append_elem(void *elem);
    ED4_returncode  delete_elem(void *elem);
    ED4_returncode  append_elem_backwards(void *elem);
    short has_elem(void *elem);

    ED4_list();
};

inline ED4_returncode ED4_list::append_elem_backwards(void *elem)
{
    ED4_list_elem *new_list_elem;

    if (elem == NULL) return (ED4_R_IMPOSSIBLE);

    new_list_elem = new ED4_list_elem(elem);

    if (!first()) {
        my_first = new_list_elem;
        my_last = new_list_elem;
    }
    else {
        new_list_elem->set_next(first());
        my_first = new_list_elem;
    }

    my_no_of_entries++;

    return (ED4_R_OK);
}


inline ED4_returncode ED4_list::append_elem(void *elem)
{
    ED4_list_elem       *new_list_elem;

    if (elem == NULL)
        return (ED4_R_IMPOSSIBLE);

    new_list_elem = new ED4_list_elem(elem);

    if (my_first == NULL) {
        my_first = new_list_elem;
        my_last = new_list_elem;
    }
    else {
        my_last->set_next(new_list_elem);
        my_last = new_list_elem;
    }

    my_no_of_entries++;
    return (ED4_R_OK);
}



inline ED4_returncode ED4_list::delete_elem(void *elem)
{
    ED4_list_elem    *current_list_elem,
        *previous_list_elem;

    current_list_elem = first();
    previous_list_elem = NULL;

    while (current_list_elem && current_list_elem->elem()!=elem) {
        previous_list_elem = current_list_elem;
        current_list_elem  = current_list_elem->next();
    }

    if (current_list_elem == NULL) {
        return (ED4_R_IMPOSSIBLE);
    }

    if (current_list_elem == first()) {
        if (current_list_elem == last()) {
            my_last = NULL;
        }
        my_first = current_list_elem->next();
    }
    else
    {
        previous_list_elem->set_next(current_list_elem->next());

        if (current_list_elem == last()) {
            my_last = previous_list_elem;
        }
    }

    my_no_of_entries--;
    delete current_list_elem;
    return (ED4_R_OK);
}

inline short ED4_list::has_elem(void *elem)
{
    ED4_list_elem       *current_list_elem = first();

    if (elem == NULL) return (0);

    while (current_list_elem && current_list_elem->elem()!=elem) {
        current_list_elem = current_list_elem->next();
    }

    return current_list_elem!=0;
}


inline ED4_list::ED4_list()
{
    my_first = NULL;
    my_last  = NULL;
    my_no_of_entries = 0;
}


#else
#error ed4_list.hxx included twice
#endif // ED4_LIST_HXX
