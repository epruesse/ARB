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
    T             *m_elem;
    ED4_list_elem *m_next;
public:
    ED4_list_elem(T *element)
        : m_elem(element),
          m_next(NULL)
    { e4_assert(element); }

    ED4_list_elem *next() const { return m_next; }
    T *elem() const { return m_elem; }

    void set_next(ED4_list_elem *the_next) { m_next = the_next; }
};

template <class T>
class ED4_list : virtual Noncopyable {
    ED4_list_elem<T> *first;
    ED4_list_elem<T> *last;

    int count;

public:

    ED4_list()
        : first(NULL),
          last(NULL),
          count(0)
    {}

    ED4_list_elem<T> *head() const { return first; }
    ED4_list_elem<T> *tail() const { return last; }
    int size() const { return count; }

    void append_elem(T *elem) {
        ED4_list_elem<T> *new_list_elem = new ED4_list_elem<T>(elem);
        if (!first) {
            first = new_list_elem;
            last  = new_list_elem;
        }
        else {
            last->set_next(new_list_elem);
            last = new_list_elem;
        }
        count++;
    }

    void prepend_elem(T *elem) {
        ED4_list_elem<T> *new_list_elem = new ED4_list_elem<T>(elem);
        if (!first) {
            first = new_list_elem;
            last  = new_list_elem;
        }
        else {
            new_list_elem->set_next(first);
            first = new_list_elem;
        }
        count++;
    }

    void remove_elem(const T *elem) {
        ED4_list_elem<T> *curr  = first;
        ED4_list_elem<T> *pred = NULL;

        while (curr && curr->elem() != elem) {
            pred = curr;
            curr  = curr->next();
        }

        if (curr) {
            if (curr == first) {
                if (curr == last) last = NULL;
                first = curr->next();
            }
            else {
                pred->set_next(curr->next());
                if (curr == last) last = pred;
            }
            count--;
            delete curr;
        }
    }
    
    bool has_elem(const T *elem) const {
        if (!elem || !count) return false;
        
        ED4_list_elem<T> *curr = first;
        while (curr && curr->elem()!=elem) {
            curr = curr->next();
        }
        return curr != 0;
    }

};

#else
#error ed4_list.hxx included twice
#endif // ED4_LIST_HXX
