#include "ed4_class.hxx"

ED4_returncode ED4_list::append_elem_backwards(void *elem)
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


ED4_returncode ED4_list::append_elem(void *elem)
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



ED4_returncode ED4_list::delete_elem(void *elem)
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

short ED4_list::is_elem(void *elem)
{
    ED4_list_elem       *current_list_elem = first();

    if (elem == NULL) return (0);

    while (current_list_elem && current_list_elem->elem()!=elem) {
        current_list_elem = current_list_elem->next();
    }

    return current_list_elem!=0;
}


ED4_list::ED4_list()
{
    my_first = NULL;
    my_last  = NULL;
    my_no_of_entries = 0;
}


ED4_list::~ED4_list()
{
}

