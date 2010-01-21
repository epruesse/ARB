#include <stdio.h>

#include <MultiProbe.hxx>

void MP_list::append_elem_backwards(void *elem)
{
    MP_list_elem       *new_list_elem;

    if (elem == NULL)
        return;

    new_list_elem = new MP_list_elem;

    new_list_elem->elem = elem;
    new_list_elem->next = NULL;

    if (this->first == NULL)
    {
        this->first = new_list_elem;
        this->last = new_list_elem;
        this->no_of_entries++;

        return;
    }

    new_list_elem->next = this->first;
    this->first = new_list_elem;
    this->no_of_entries++;

    return;
}


void MP_list::append_elem(void *elem)
{
    MP_list_elem       *new_list_elem;

    if (elem == NULL)
        return;

    new_list_elem = new MP_list_elem;

    new_list_elem->elem = elem;
    new_list_elem->next = NULL;

    if (this->first == NULL)
    {
        this->first = new_list_elem;
        this->last = new_list_elem;
        this->no_of_entries++;

        return;
    }

    this->last->next = new_list_elem;
    this->last = new_list_elem;
    this->no_of_entries++;

    return;
}



void MP_list::delete_elem(void *elem)
{
    MP_list_elem     *current_list_elem,
        *previous_list_elem;

    current_list_elem = this->first;
    previous_list_elem = NULL;

    while ((current_list_elem != NULL)  && (current_list_elem->elem != elem))
    {
        previous_list_elem = current_list_elem;
        current_list_elem  = current_list_elem->next;
    }

    if (current_list_elem == NULL)
        return;

    if (current_list_elem == this->first)
    {
        if (current_list_elem == this->last)
            this->last = NULL;

        this->first = current_list_elem->next;
    }
    else
    {
        previous_list_elem->next = current_list_elem->next;

        if (current_list_elem == this->last)
            this->last = previous_list_elem;
    }

    this->no_of_entries--;
    delete current_list_elem;
    return;
}



short MP_list::is_elem(void *elem)
{
    MP_list_elem    *current_list_elem = this->first;

    if (elem == NULL)
        return (FALSE);

    while (current_list_elem && current_list_elem->elem != elem)
        current_list_elem = current_list_elem->next;

    return (current_list_elem) ? TRUE : FALSE;
}


MP_list::MP_list()
{
    this->first = NULL;
    this->last  = NULL;
    this->no_of_entries = 0;
}


MP_list::~MP_list()
{
}

