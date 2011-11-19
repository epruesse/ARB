
#include <stdio.h>
#include "ali_misc.hxx"

#include "ali_tlist.hxx"



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
void ALI_TLIST<T>::delete_element(void)
{
    ALI_TLIST_ELEM<T> *elem;

    if (current_elem != 0) {
        if (current_elem == marked_elem) {
            ali_warning("Delete marked element");
            marked_elem = 0;
        }
        /*   prev_elem <--> current <--> next_elem   */
        if (current_elem->prev_elem != 0 && current_elem->next_elem != 0) {
            elem = current_elem;
            current_elem->prev_elem->next_elem = current_elem->next_elem;
            current_elem->next_elem->prev_elem = current_elem->prev_elem;
            current_elem = current_elem->next_elem;
            delete elem;
        }
        else {
            /*   prev_elem <--> current -|   */
            if (current_elem->prev_elem != 0) {
                elem = current_elem;
                current_elem->prev_elem->next_elem = 0;
                current_elem = current_elem->prev_elem;
                last_elem = current_elem;
                delete elem;
            }
            else {
                /*   |- current <--> next_elem   */
                if (current_elem->next_elem != 0) {
                    elem = current_elem;
                    current_elem->next_elem->prev_elem = 0;
                    current_elem = current_elem->next_elem;
                    first_elem = current_elem;
                    delete elem;
                }
                else {
                    /*   |- current -|   */
                    elem = current_elem;
                    delete elem;
                    first_elem = last_elem = current_elem = 0;
                }
            }
        }
        cardinal--;
    }
}


