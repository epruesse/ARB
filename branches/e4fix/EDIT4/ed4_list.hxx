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

template <class T>
class ED4_list_elem : virtual Noncopyable {
    T             *my_elem;
    ED4_list_elem *my_next;
public:
    ED4_list_elem(T *element) { my_elem = element; my_next = 0; }
    ~ED4_list_elem() {}

    ED4_list_elem *next() const { return my_next; }
    T *elem() const { return my_elem; }

    void set_next(ED4_list_elem *the_next) { my_next = the_next; }
};

template <class T>
class ED4_list : virtual Noncopyable {
    ED4_list_elem<T> *my_first;
    ED4_list_elem<T> *my_last;

    ED4_index my_no_of_entries;

public:

    ED4_list() {
        my_first = NULL;
        my_last  = NULL;
        my_no_of_entries = 0;
    }

    ED4_list_elem<T> *first() const { return my_first; }
    ED4_list_elem<T> *last() const { return my_last; }
    ED4_index no_of_entries() const { return my_no_of_entries; }

    ED4_returncode append_elem(T *elem) {
        if (!elem) return (ED4_R_IMPOSSIBLE);

        ED4_list_elem<T> *new_list_elem = new ED4_list_elem<T>(elem);

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

    ED4_returncode  delete_elem(T *elem) {
        ED4_list_elem<T> *current_list_elem  = first();
        ED4_list_elem<T> *previous_list_elem = NULL;

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
    
    ED4_returncode  append_elem_backwards(T *elem) {
        ED4_list_elem<T> *new_list_elem;

        if (elem == NULL) return (ED4_R_IMPOSSIBLE);

        new_list_elem = new ED4_list_elem<T>(elem);

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
    
    short has_elem(T *elem) {
        ED4_list_elem<T> *current_list_elem = first();

        if (elem == NULL) return (0);

        while (current_list_elem && current_list_elem->elem()!=elem) {
            current_list_elem = current_list_elem->next();
        }

        return current_list_elem!=0;
    }
    
};

#else
#error ed4_list.hxx included twice
#endif // ED4_LIST_HXX
