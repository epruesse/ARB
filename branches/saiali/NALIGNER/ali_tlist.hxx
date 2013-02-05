// =============================================================== //
//                                                                 //
//   File      : ali_tlist.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_TLIST_HXX
#define ALI_TLIST_HXX

#ifndef ALI_MISC_HXX
#include "ali_misc.hxx"
#endif


template<class T>
struct ALI_TLIST_ELEM {
    T info;
    ALI_TLIST_ELEM<T> *next_elem, *prev_elem;

    ALI_TLIST_ELEM<T>(T &a) : info(a)
    { prev_elem = next_elem = 0; }
    void print() {
        printf("<%8p (%8p) %8p> = %lx", prev_elem, this, next_elem, info);
    }
};

template<class T>
class ALI_TLIST : virtual Noncopyable {
    ALI_TLIST_ELEM<T> *first_elem, *last_elem;
    ALI_TLIST_ELEM<T> *current_elem;
    ALI_TLIST_ELEM<T> *marked_elem;
    unsigned long cardinal;

public:
    int is_consistent() {
        if (!((current_elem == 0 && first_elem == 0 && last_elem == 0) ||
              (current_elem != 0 && first_elem != 0 && last_elem != 0))) {
            printf("List is inconsistent (1)\n");
            return 0;
        }
        if (first_elem != 0) {
            ALI_TLIST_ELEM<T> *pre = first_elem;

            int current_inside_flag = current_elem == pre;
            int marked_inside_flag  = marked_elem == pre;

            ALI_TLIST_ELEM<T> *akt = pre->next_elem;
            while (akt) {
                if (current_elem == akt)
                    current_inside_flag = 1;
                if (marked_elem == akt)
                    marked_inside_flag = 1;
                if (akt->prev_elem != pre) {
                    printf("List is inconsistent (2)\n");
                    return 0;
                }
                pre = akt;
                akt = akt->next_elem;
            }
            if (pre != last_elem) {
                printf("List is inconsistent (3)\n");
                return 0;
            }
            if (current_inside_flag == 0) {
                printf("List is inconsistent (4)\n");
                return 0;
            }
            if (marked_elem && marked_inside_flag == 0) {
                printf("List is inconsistent (5)\n");
                return 0;
            }
        }
        return 1;
    }

    ALI_TLIST() {
        first_elem = last_elem = current_elem = marked_elem = 0;
        cardinal = 0;
    }
    ALI_TLIST(T &a) {
        marked_elem = 0;
        first_elem = last_elem = current_elem = new ALI_TLIST_ELEM<T>(a);
        cardinal = 1;
    }
    ~ALI_TLIST() {
        while (first_elem != 0) {
            current_elem = first_elem;
            first_elem = current_elem->next_elem;
            delete current_elem;
        }
    }

    void print() {
        unsigned long l;
        ALI_TLIST_ELEM<T> *akt;

        printf("List (%ld):\n", cardinal);
        printf("first = %p  last = %p  current = %p  marked = %p\n",
               first_elem, last_elem, current_elem, marked_elem);
        for (akt = first_elem, l = 0; akt != 0 && akt != last_elem;
             l++, akt = akt->next_elem) {
            printf("%2ld ", l);
            akt->print();
            printf("\n");
        }
        if (akt != 0)
            akt->print();
        printf("\n\n");
    }
    // clear the list

    void make_empty() {
        while (first_elem != 0) {
            current_elem = first_elem;
            first_elem = current_elem->next_elem;
            delete current_elem;
        }
        first_elem = last_elem = current_elem = marked_elem = 0;
        cardinal = 0;
    }

    // append at end or front of _list_

    void append_end(T &a);
    void append_end(ALI_TLIST<T> &a);
    void append_front(T &a);
    void append_front(ALI_TLIST<T> &a);

    // insert after or bevore _current_ element

    void insert_after(T &a);
    void insert_after(ALI_TLIST<T> &a);
    void insert_bevor(T &a);
    void insert_bevor(ALI_TLIST<T> &a);

    // delete _current_ element and goto _next_ element

    void delete_element();

    // Mark a unique element

    void mark_element() {
        marked_elem = current_elem;
    }
    void marked() {
        if (marked_elem == 0)
            ali_fatal_error("No marked element in list", "ALI_TLIST<T>::marked()");
        current_elem = marked_elem;
        marked_elem = 0;
    }

    // Overwrite
    void overwrite_element(T new_elem) {
        if (current_elem != 0)
            (current_elem->info) = new_elem;
    }
    // For navigation through the list
    int cardinality() {
        return cardinal;
    }
    int is_empty() {
        return (current_elem == 0);
    }
    int is_next() {
        return (current_elem != 0 && current_elem->next_elem != 0);
    }
    int is_prev() {
        return (current_elem != 0 && current_elem->prev_elem != 0);
    }
    T current() {
        return current_elem->info;
    }
    T first() {
        current_elem = first_elem;
        return current_elem->info;
    }
    T last() {
        current_elem = last_elem;
        return current_elem->info;
    }
    T next() {
        if (current_elem->next_elem != 0)
            current_elem = current_elem->next_elem;
        return current_elem->info;
    }
    T prev() {
        if (current_elem->prev_elem != 0)
            current_elem = current_elem->prev_elem;
        return current_elem->info;
    }
};

template <class T>
void ALI_TLIST<T>::append_end(T &a)
{
    ALI_TLIST_ELEM<T> *elem = new ALI_TLIST_ELEM<T>(a);

    if (last_elem != 0) {
        last_elem->next_elem = elem;
        elem->prev_elem = last_elem;
        last_elem = elem;
    }

    else {
        last_elem = first_elem = current_elem = elem;
    }
    cardinal++;
}

template <class T>
void ALI_TLIST<T>::append_end(ALI_TLIST<T> &a)
{
    ALI_TLIST_ELEM<T> *elem, *akt_elem;

    for (akt_elem = a.first_elem; akt_elem != 0;
         akt_elem = akt_elem->next_elem) {
        elem = new ALI_TLIST_ELEM<T>(akt_elem->info);
        if (last_elem != 0) {
            last_elem->next_elem = elem;
            elem->prev_elem = last_elem;
            last_elem = elem;
        }
        else {
            last_elem = first_elem = current_elem = elem;
        }
        cardinal++;
    }
}

template <class T>
void ALI_TLIST<T>::append_front(T &a)
{
    ALI_TLIST_ELEM<T> *elem = new ALI_TLIST_ELEM<T>(a);

    if (first_elem != 0) {
        first_elem->prev_elem = elem;
        elem->next_elem = first_elem;
        first_elem = elem;
    }
    else {
        last_elem = first_elem = current_elem = elem;
    }
    cardinal++;
}

template <class T>
void ALI_TLIST<T>::append_front(ALI_TLIST<T> &a)
{
    ALI_TLIST_ELEM<T> *elem, *akt_elem;

    for (akt_elem = a.last_elem; akt_elem != 0;
         akt_elem = akt_elem->prev_elem) {
        elem = new ALI_TLIST_ELEM<T>(akt_elem->info);
        if (first_elem != 0) {
            first_elem->prev_elem = elem;
            elem->next_elem = first_elem;
            first_elem = elem;
        }
        else {
            last_elem = first_elem = current_elem = elem;
        }
        cardinal++;
    }
}

template <class T>
void ALI_TLIST<T>::insert_after(T &a)
{
    ALI_TLIST_ELEM<T> *elem = new ALI_TLIST_ELEM<T>(a);

    if (current_elem != 0) {
        if (current_elem->next_elem != 0) {
            elem->next_elem = current_elem->next_elem;
            current_elem->next_elem->prev_elem = elem;
        }
        current_elem->next_elem = elem;
        elem->prev_elem = current_elem;
        if (current_elem == last_elem)
            last_elem = elem;
    }
    else {
        last_elem = first_elem = current_elem = elem;
    }
    cardinal++;
}

template <class T>
void ALI_TLIST<T>::insert_after(ALI_TLIST<T> &a)
{
    ALI_TLIST_ELEM<T> *elem, *akt_elem;

    for (akt_elem = a.first_elem; akt_elem != 0;
         akt_elem = akt_elem->next_elem) {
        elem = new ALI_TLIST_ELEM<T>(akt_elem->info);
        if (current_elem != 0) {
            if (current_elem->next_elem != 0) {
                elem->next_elem = current_elem->next_elem;
                current_elem->next_elem->prev_elem = elem;
            }
            current_elem->next_elem = elem;
            elem->prev_elem = current_elem;
            if (current_elem == last_elem)
                last_elem = elem;
        }
        else {
            last_elem = first_elem = current_elem = elem;
        }
        cardinal++;
    }
}

template <class T>
void ALI_TLIST<T>::insert_bevor(T &a)
{
    ALI_TLIST_ELEM<T> *elem = new ALI_TLIST_ELEM<T>(a);

    if (current_elem != 0) {
        if (current_elem->prev_elem != 0) {
            elem->prev_elem = current_elem->prev_elem;
            current_elem->prev_elem->next_elem = elem;
        }
        current_elem->prev_elem = elem;
        elem->next_elem = current_elem;
        if (current_elem == first_elem)
            first_elem = elem;
    }
    else {
        last_elem = first_elem = current_elem = elem;
    }
    cardinal++;
}

template <class T>
void ALI_TLIST<T>::insert_bevor(ALI_TLIST<T> &a)
{
    ALI_TLIST_ELEM<T> *elem, *akt_elem;

    for (akt_elem = a.last_elem; akt_elem != 0;
         akt_elem = akt_elem->prev_elem) {
        elem = new ALI_TLIST_ELEM<T>(akt_elem->info);
        if (current_elem != 0) {
            if (current_elem->prev_elem != 0) {
                elem->prev_elem = current_elem->prev_elem;
                current_elem->prev_elem->next_elem = elem;
            }
            current_elem->prev_elem = elem;
            elem->next_elem = current_elem;
            if (current_elem == first_elem)
                first_elem = elem;
        }
        else {
            last_elem = first_elem = current_elem = elem;
        }
        cardinal++;
    }
}

template<class T>
void ALI_TLIST<T>::delete_element() {
    if (current_elem != 0) {
        if (current_elem == marked_elem) {
            ali_warning("Delete marked element");
            marked_elem = 0;
        }
        //   prev_elem <--> current <--> next_elem
        if (current_elem->prev_elem != 0 && current_elem->next_elem != 0) {
            ALI_TLIST_ELEM<T> *elem = current_elem;
            current_elem->prev_elem->next_elem = current_elem->next_elem;
            current_elem->next_elem->prev_elem = current_elem->prev_elem;
            current_elem = current_elem->next_elem;
            delete elem;
        }
        else {
            //   prev_elem <--> current -|
            if (current_elem->prev_elem != 0) {
                ALI_TLIST_ELEM<T> *elem = current_elem;
                current_elem->prev_elem->next_elem = 0;
                current_elem = current_elem->prev_elem;
                last_elem = current_elem;
                delete elem;
            }
            else {
                //   |- current <--> next_elem
                if (current_elem->next_elem != 0) {
                    ALI_TLIST_ELEM<T> *elem = current_elem;
                    current_elem->next_elem->prev_elem = 0;
                    current_elem = current_elem->next_elem;
                    first_elem = current_elem;
                    delete elem;
                }
                else {
                    //   |- current -|
                    ALI_TLIST_ELEM<T> *elem = current_elem;
                    delete elem;
                    first_elem = last_elem = current_elem = 0;
                }
            }
        }
        cardinal--;
    }
}

#else
#error ali_tlist.hxx included twice
#endif // ALI_TLIST_HXX
