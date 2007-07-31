#ifndef SOTL_HXX
#define SOTL_HXX

// SOTL = SelfOrganising TemplateList

/*
  Copyright by Andrej Konkow 1996

  User has to take care by his own on calling update_pos_no, when inserting
  elements somewhere in the middle. If elements are inserted only at the last
  position the pos is incremented correctly. By default the first element in the list
  has position 1.
  no_of_members is counted and updated automatically.

  Caution:
  This List by default is NOT a selforganizing list. This means that by default an element
  that is being asked for is NOT put at front of the list. This only will be done while the
  flag sotl is set to FALSE(this is default). You can change this flag with the methods
  sotl_list() (sets the flag TRUE) and no_sotl_list() (sets the flag FALSE).

  The List is created by
  List<Typename> *instance_variable = new List<Typename>(sotl_flag);
  ^^^^^^^
  sotl_flag is optional.
  For a normal double linked list
  this flag has not to be set. For
  a selforganizing list : TRUE

  Afterwards the elements can be inserted.
  The User of this list has to delete the elements which are inserted to the list by
  its own.
  The User has only have to work with the List class.
*/

#include <stdio.h>

typedef char BOOL;
typedef unsigned long positiontype;
#define TRUE	1
#define FALSE	0

#define RELATION_GREATER	1
#define RELATION_LESS		2


template <class Type> class list_elem
{
private:
    positiontype		pos;


public:
    list_elem<Type>		*next;
    list_elem<Type>		*prev;
    Type 			*elem;
    BOOL			isolate_list_elem();		// set next and prev links to NULL
    // TRUE if isolation has taken place,
    // else FALSE(for example if we're the
    // only element

    Type			*get_elem()			{ return elem; };
    void			set_elem( Type *el )		{ elem = el; };
    positiontype		get_pos()			{ return pos; };
    void			set_pos(positiontype p)		{ pos = p; };
    list_elem<Type>		*get_next()			{ return next; };
    void			set_next(list_elem<Type> *n)	{ next = n; };
    list_elem<Type>		*get_prev()			{ return prev; };
    void			set_prev(list_elem<Type> *p)	{ prev = p; };

    list_elem();
    list_elem(Type *el);
    ~list_elem();
};


template <class Type> class List
{
private:
    list_elem<Type>	*first;
    list_elem<Type>	*last;
    list_elem<Type>	*last_asked_list_elem;
    list_elem<Type>	*remembered_elem;

    positiontype	no_of_members;
    BOOL		sotl;


    list_elem<Type>	*get_list_elem_with_member	( Type *object );
    list_elem<Type>	*get_list_elem_at_pos		( positiontype pos );
    list_elem<Type>	*get_list_elem_at_pos_simple	( positiontype pos );
public:
    // general List functions


    //only use these functions if you know what you are doing !!!  BEGINNING
    list_elem<Type>	*get_first_list_elem()	{ return first; };	// do not use !!!
    list_elem<Type>	*get_last_list_elem()	{ return last; };	// do not use !!!
    list_elem<Type>	*get_current_list_elem(){ return last_asked_list_elem; };// do not use !!!
    void		remember_current()	{ remembered_elem = last_asked_list_elem; };
    void		set_current_ARC(list_elem<Type> *t);		//ARC = and remember current
    void		set_remembered_as_current_ARC();		//ARC = and remember current
    //only use these functions if you know what you are doing !!!  END

    void		sotl_list()		{ sotl = TRUE; };
    void		no_sotl_list()		{ sotl = FALSE; };
    positiontype	get_no_of_members()	{ return no_of_members; };

    positiontype	insert_as_first		( Type *object );
    positiontype 	insert_as_last		( Type *object );	//returns pos_no
    positiontype	insert_after_current	( Type *object );
    positiontype	insert_before_current	( Type *object );
    positiontype	insert			( Type *object );	//returns pos_no
    Type 		*get_first		();
    Type		*get_last		();
    Type		*get_prev		();
    Type		*get_next		();

    void 		remove_member_from_list ( Type *object );	//object won't be deleted
    void		remove_first();
    void		remove_last();
    List<Type>	*duplicate_list		( Type *object );	// the list is duplicated
    // from the element given til the end.
    // For duplicating the whole list
    // object = get_first(). Only the list is
    // duplicated, not the elems. Caution:
    // In a sotl list the asked element is put
    // to the front. So first make a no_sotl_list()
    // ask and duplicate the list, and then make a
    // sotl_list() again.

    BOOL		exchange_members	( Type *ex, Type *change );

    /*	following functions only make sense if our list is sorted by the address of
        our item inserted to the list.
        Caution:
		The list is only sorted if it's NOT a sotl list. So take care to
		create or set it to a no_sotl_list().
    */

    // Comment to insert_sorted_by_address_of_object:
    // The function returns the no_of_members in the list.
    // By default an object(the meaning here is that one object equals another,
    // if the address of both objects matches) can be inserted to the list
    // several times. If there is the need to insert an object only once
    // in the list, the flag duplicates has to be set to FALSE.
    // Finally the relation has to be set, by which the list is sorted.
    // The possiblilities are : RELATION_GREATER and RELATION_LESS
    positiontype	insert_sorted_by_address_of_object(	Type *object,
                                                        int relation=RELATION_LESS,
                                                        BOOL duplicates=TRUE );

    // Comment to sort_list_join:
    // if an object found in List l is found in this list it won't be inserted
    // a second time.
    void	 	sort_list_join		(	List<Type> *l,
                                        int relation=RELATION_LESS);	//Lists given as parameter
    //won't be affected
    // Comment to sort_list_subtract:
    // this function call only makes sense if the same element can be found
    // in both lists. The flag relation tells the method how this list(not list l)
    // is sorted. Both lists have to be sorted by the same relation.
    void		sort_list_subtract	(	List<Type> *l,
                                        int relation=RELATION_LESS);



    positiontype	insert_at_pos_simple	( Type *object, positiontype pos );	//returns pos inserted to
    //doesn't refer to get_pos()
    Type		*get_member_at_pos_simple( positiontype pos );

    /*
       Following functions only make sense if the user takes care of the list as
       a numbered list.
    */
    // Comment to insert_at_pos :
    // user does not have to call update_pos_no after insert_at_pos()
    positiontype	insert_at_pos	 	( Type *object, positiontype pos );	//returns pos inserted to
    positiontype	get_pos_of_member	( Type *object );
    Type		*get_member_at_pos	( positiontype pos );
    BOOL 		remove_pos_from_list	( positiontype pos );		//element won't be deleted
    BOOL 		exchange_positions	( positiontype ex, positiontype change );	//exchange elems in list
    void		update_pos_no( list_elem<Type> *elem, positiontype nr);	//updates no from
    // the given elem with nr til last
    void		update_pos_no();		//updates pos number from first to last

    List(BOOL so=FALSE);
    ~List();
};

/**************************
Beginn list_elem
***************************/

template <class Type> inline list_elem<Type>::list_elem()
{
	pos = 0;
	next = NULL;
	prev = NULL;
	elem = NULL;
}

template <class Type> inline list_elem<Type>::list_elem(Type *el)
{
	pos = 0;
	next = NULL;
	prev = NULL;
	elem = el;
}

template <class Type> inline list_elem<Type>::~list_elem()
{
	isolate_list_elem();
}

template <class Type> inline  BOOL list_elem<Type>::isolate_list_elem()
{
	if (prev && next)		//somewhere in the middle
	{
		prev->next	= next;
		next->prev	= prev;
		next		= NULL;
		prev		= NULL;
	}
	else if (prev)			//we're the last
	{
		prev->next	= NULL;
		prev		= NULL;
	}
	else if (next)			//we're the first
	{
		next->prev	= NULL;
		next		= NULL;
	}
	else
		return FALSE;

	return TRUE;
}

/**************************
Ende list_elem
**************************/

/**************************
Beginn List
***************************/

template <class Type> inline List<Type>::List(BOOL so)
{
	first = last = last_asked_list_elem = remembered_elem = NULL;
	no_of_members = 0;
	sotl = so;
}

template <class Type> inline List<Type>::~List()
{
	list_elem<Type> *elem, *help;

	elem = first;
	while (elem)				//delete every object in list
	{
		help = elem->next;
		delete elem;
		elem = help;
	}
}

template <class Type> inline void List<Type>::set_current_ARC(list_elem<Type> *t)	//ARC = and remember current
{
	remembered_elem = last_asked_list_elem;
	last_asked_list_elem = t;
}

template <class Type> inline void List<Type>::set_remembered_as_current_ARC()	//ARC = and remember current
{
	list_elem<Type> *mark;

	mark = last_asked_list_elem;
	last_asked_list_elem = remembered_elem;
	remembered_elem = mark;
}

template <class Type> inline list_elem<Type> *List<Type>::get_list_elem_with_member( Type *object )
{
	list_elem<Type>	*loc_elem;

	loc_elem = first;
	while (loc_elem && loc_elem->elem!=object)
		loc_elem = loc_elem->next;

	return loc_elem;
}

template <class Type> inline list_elem<Type> *List<Type>::get_list_elem_at_pos( positiontype pos )
{
	list_elem<Type>	*elem;

	if (pos < 1 || pos > no_of_members)
		return NULL;

	if (! last_asked_list_elem)
	{
		if (pos > no_of_members/2)
		{
			elem = last;
			while (elem && elem->get_pos() != pos)
				elem = elem->get_prev();
		}
		else
		{
			elem = first;
			while (elem && elem->get_pos() != pos)
				elem = elem->next;
		}
	}
	else
	{
		elem = last_asked_list_elem;

		if (pos >= last_asked_list_elem->get_pos())
		{
			while (elem && elem->get_pos() != pos)
				elem = elem->next;
		}
		else	// pos < last_asked_list_elem->pos
		{
			while (elem && elem->get_pos() != pos)
				elem = elem->get_prev();
		}
	}

	return elem;
}

template <class Type> inline list_elem<Type> *List<Type>::get_list_elem_at_pos_simple( positiontype pos )
{
	list_elem<Type>		*elem;
	register positiontype	counter = 1;

	if (pos < 1 || pos > no_of_members)
		return NULL;

	if (pos > no_of_members/2)
	{
		elem = last;
		counter = no_of_members;
		while (elem && counter != pos)
		{
			elem = elem->get_prev();
			counter --;
		}
	}
	else
	{
		elem = first;
		while (elem && counter != pos)
		{
			elem = elem->next;
			counter ++;
		}
	}

	return elem;
}

template <class Type> inline Type *List<Type>::get_first()
{
	if (first)
	{
		last_asked_list_elem = first;
		return first->elem;
	}
	else
		return NULL;
}

template <class Type> inline Type *List<Type>::get_last()
{
	if (last && ! sotl)			//behaviour of a normal linked list
	{
		last_asked_list_elem = last;
		return last->elem;
	}
	else if (last && sotl)
	{
		last_asked_list_elem = last->get_prev();
		insert_as_first(last->elem);
		remove_last();
		return first->elem;
	}
	else
		return NULL;
}

template <class Type> inline Type *List<Type>::get_prev()
{
	Type		*result = NULL;
	list_elem<Type>	*mark_prev;

	if (last_asked_list_elem){

        if (!sotl)		//behaviour of a normal linked list
        {
            last_asked_list_elem = last_asked_list_elem->get_prev();

            if (last_asked_list_elem)
                result = last_asked_list_elem->elem;
        }
        else if (sotl)
        {
            if (last_asked_list_elem)
            {
                result		= last_asked_list_elem->elem;
                mark_prev	= last_asked_list_elem->get_prev();

                remove_member_from_list( result );
                insert_as_first( result );
                last_asked_list_elem = mark_prev;
            }
        }
	}
	return result;
}

template <class Type> inline Type *List<Type>::get_next()
{
	Type		*result = NULL;
	list_elem<Type>	*mark_next;

	if (last_asked_list_elem){

        if (!sotl)		//behaviour of a normal linked list
        {
            last_asked_list_elem = last_asked_list_elem->next;

            if (last_asked_list_elem)
                result = last_asked_list_elem->elem;
        }
        else if (sotl)
        {
            if (last_asked_list_elem)
            {
                result		= last_asked_list_elem->elem;
                mark_next	= last_asked_list_elem->next;

                remove_member_from_list( result );
                insert_as_first( result );
                last_asked_list_elem = mark_next;
            }
        }
	}

	return result;
}


template <class Type> inline positiontype List<Type>::insert_as_first( Type *object )
{
	list_elem<Type>	*help = NULL;

	if (! first)					// create first element
	{						// in list
		first = new list_elem<Type>( object );
		first->set_pos(1);
		last = first;
	}
	else
	{
		help = new list_elem<Type>( object );
		help->set_pos(1);				//update by USER !!!
		help->set_next(first);
		first->set_prev(help);
		first = help;
	}

	no_of_members ++;
	return 1;
}

template <class Type> inline  positiontype List<Type>::insert_as_last( Type *object )
{
	list_elem<Type>	*help = NULL;

	if (! first)					// create first element
	{						// in list
		first = new list_elem<Type>( object );
		first->set_pos(1);
		last = first;
	}
	else
	{
		help = new list_elem<Type>( object );
		help->set_pos( no_of_members+1 );
		help->set_prev(last);
		last->set_next(help);
		last = help;
	}

	no_of_members ++;
	return no_of_members;
}

template <class Type> inline positiontype List<Type>::insert_after_current( Type *object )
{
    list_elem<Type>	*help = NULL;
    positiontype result = 0;

    if (last_asked_list_elem){

        if (!last_asked_list_elem->get_next()){
            result = insert_as_last( object );
        }else{

            help = new list_elem<Type>( object );
            help->set_pos( last_asked_list_elem->get_pos() + 1 );
            help->set_prev(last_asked_list_elem);
            help->set_next( last_asked_list_elem->get_next() );
            help->get_next()->set_prev(help);
            last_asked_list_elem->set_next( help );
            last_asked_list_elem = help;

            no_of_members ++;
            result =  no_of_members;
        }
    }
    return result;
}

template <class Type> inline positiontype List<Type>::insert_before_current( Type *object )
{
    list_elem<Type>	*help = NULL;
	positiontype result = 0;
    if (last_asked_list_elem){

        if (!last_asked_list_elem->get_prev()){
            result=  insert_as_first( object );
        }else{

            help = new list_elem<Type>( object );
            help->set_pos( last_asked_list_elem->get_pos() - 1 );
            help->set_next(last_asked_list_elem);
            help->set_prev( last_asked_list_elem->get_prev() );
            help->get_prev()->set_next(help);
            last_asked_list_elem->set_prev( help );
            last_asked_list_elem = help;

            no_of_members ++;
            result = no_of_members;
        }
    }
    return result;
}

template <class Type> inline  positiontype List<Type>::insert( Type *object )
{
	return insert_as_first( object );
}

template <class Type> inline positiontype List<Type>::insert_at_pos_simple( Type *object, positiontype temp_pos )	//returns pos inserted to
{
	list_elem<Type>	*elem,
        *new_elem;

	if (temp_pos<=1)
	{
		insert_as_first( object );
		return 1;
	}
	else if (temp_pos>no_of_members)
	{
		insert_as_last( object );
		return no_of_members;
	}
	else
	{
		elem = get_list_elem_at_pos_simple( temp_pos );

		new_elem = new list_elem<Type>;
		new_elem->elem = object;
		new_elem->set_pos(temp_pos);
		new_elem->prev = elem->prev;
		new_elem->next = elem;
		elem->prev->next = new_elem;
		elem->prev = new_elem;

		no_of_members ++;
		return temp_pos;
	}
}

template <class Type> inline void List<Type>::remove_member_from_list( Type *object )
{
	list_elem<Type>	*loc_elem;

	if (last_asked_list_elem &&
		last_asked_list_elem->elem==object)
	{
		if (! last_asked_list_elem->next )	//we're the last
			last = last_asked_list_elem->get_prev();

		if (! last_asked_list_elem->get_prev() )	//we're the first
			first = last_asked_list_elem->next;

		delete last_asked_list_elem;

		if (no_of_members == 1)
			first = last = NULL;

		last_asked_list_elem = NULL;
	}
	else
	{
		if (last_asked_list_elem &&
			last_asked_list_elem->next &&
			last_asked_list_elem->next->elem == object )
			loc_elem = last_asked_list_elem->next;
		else if (last_asked_list_elem &&
                 last_asked_list_elem->get_prev() &&
                 last_asked_list_elem->get_prev()->elem == object )
			loc_elem = last_asked_list_elem->get_prev();
		else
			loc_elem = get_list_elem_with_member( object );

		if (! loc_elem)
			return;

		if (! loc_elem->next )	//we're the last
			last = loc_elem->get_prev();

		if (! loc_elem->get_prev() )	//we're the first
			first = loc_elem->next;

		delete loc_elem;
	}

	no_of_members --;
}

template <class Type> inline void List<Type>::remove_first()
{
	list_elem<Type> *new_first;

	if (no_of_members <= 1)			//case 0 or 1
	{
		delete first;
		first = last = last_asked_list_elem = NULL;
		no_of_members = 0;
	}
	else
	{
		new_first = first->next;
		delete first;
		first = new_first;
		no_of_members --;
	}
}

template <class Type> inline void List<Type>::remove_last()
{
	list_elem<Type> *new_last;

	if (no_of_members <= 1)			//case 0 or 1
	{
		delete last;
		first = last = last_asked_list_elem = NULL;
		no_of_members = 0;
	}
	else
	{
		new_last = last->get_prev();
		delete last;
		last = new_last;
		no_of_members --;
	}
}

template <class Type> inline positiontype List<Type>::insert_sorted_by_address_of_object( Type *object,
                                                                                          int relation,
                                                                                          BOOL duplicates )		//falls object schon vorhanden, dann
{
	list_elem<Type>	*help = NULL,
        *l_help;

	if (! first)				// create first element
	{					// in list
		first = new list_elem<Type>( object );
		first->set_pos(1);
		last = first;
	}
	else
	{
		if (relation == RELATION_GREATER)	// search for a place to insert to
		{					// objects which are created later,
			l_help = first;			// have a greater address !!!
			while (l_help && object < l_help->elem)
				l_help = l_help->next;
		}
		else		//RELATION_LESS
		{
			l_help = last;
			while (l_help && object < l_help->elem)
				l_help = l_help->get_prev();
		}

		if (l_help && object == l_help->elem && ! duplicates )	//Element already is in the list
			return no_of_members;

		help = new list_elem<Type>( object );	// generate new element

		if (! l_help)				//new element in front/at end
		{
			if (relation == RELATION_GREATER)
			{
				last->set_next(help);
				help->set_prev(last);
				last = help;
			}
			else		//RELATION_LESS
			{
				first->set_prev(help);
				help->set_next(first);
				first = help;
			}
		}
		else
		{
			if (relation == RELATION_GREATER)
			{
				if (first == l_help)
					first = help;

				help->set_next(l_help);
				help->set_prev(l_help->get_prev());
			}
			else		//RELATION_LESS
			{
				if (last == l_help)
					last = help;

				help->set_prev(l_help);
				help->set_next(l_help->next);
			}

			if (help->next)
				help->next->set_prev(help);

			if (help->get_prev())
				help->get_prev()->set_next(help);
		}

	}

	no_of_members ++;

	return no_of_members;
}

template <class Type> inline void List<Type>::sort_list_join( List<Type> *l,
                                                              int relation)
{
	list_elem<Type>	*this_list = first,
        *join_list,
        *new_elem;

	if (! l || ! l->get_no_of_members())
		return;

	join_list = l->get_first_list_elem();

	while (this_list && join_list)
	{
		if ( this_list->elem == join_list->elem )
		{
			this_list = this_list->next;
			join_list = join_list->next;
		}
		else
		{
			if (	( relation == RELATION_GREATER && this_list->elem > join_list->elem) ||
                    ( relation == RELATION_LESS    && this_list->elem < join_list->elem) )
				this_list = this_list->next;
			else
			{
				new_elem = new list_elem<Type>( join_list->elem );
				new_elem->set_prev( this_list->get_prev() );
				new_elem->set_next( this_list );
				this_list->set_prev( new_elem );

				if (new_elem->get_prev())
					new_elem->get_prev()->set_next( new_elem );
				else
					first = new_elem;

				join_list = join_list->next;

				no_of_members ++;
			}
		}
	}

	if (! join_list || (!this_list && !join_list))
		return;

	if (!this_list)
	{
		while(join_list)
		{
			new_elem = new list_elem<Type>( join_list->elem );
			new_elem->set_prev( last );

			if (last)
				last->set_next( new_elem );

			if (!first)
				first = new_elem;

			last = new_elem;
			no_of_members ++;
			join_list = join_list->next;
		}
	}
}

template <class Type> inline  void List<Type>::sort_list_subtract( List<Type> *l, int relation )
{
	list_elem<Type>	*this_list = first,
        *join_list = l->get_first_list_elem(),
        *mark;

    while (this_list && join_list)
    {
        if ( (	relation == RELATION_GREATER && this_list->elem > join_list->elem) ||
		     (	relation == RELATION_LESS    && this_list->elem < join_list->elem) )
            this_list = this_list->next;
		else if ( (	relation == RELATION_GREATER && this_list->elem < join_list->elem) ||
                  (		relation == RELATION_LESS    && this_list->elem > join_list->elem) )
            join_list = join_list->next;
        else if (this_list->elem == join_list->elem) //same element => delete from this_list
        {
			if (this_list == first)
				first = this_list->next;

			if (this_list == last)
				last = this_list->get_prev();

			mark = this_list->next;
			delete this_list;
			this_list = mark;

			join_list = join_list->next;
			no_of_members --;

			if (! no_of_members)
			{
				last_asked_list_elem = NULL;
				return;
			}
		}
    }
}

template <class Type> inline List<Type> *List<Type>::duplicate_list( Type *object )
{
	list_elem<Type>	*help_l = first;
	List<Type>	*new_list = NULL;

	if (last_asked_list_elem->elem == object)
		help_l = last_asked_list_elem;
	else
		help_l = get_list_elem_with_member( object );

	if (help_l)
		new_list = new List<Type>;

	while (help_l)
	{
		new_list->insert_as_last(help_l->elem);

		help_l = help_l->next;
	}

	return new_list;
}

template <class Type>  inline void List<Type>::update_pos_no( list_elem<Type> *elem, positiontype nr )
{
	list_elem<Type> *mark;

	if (!elem)
		return;

	elem->set_pos(nr++);
	mark = elem->next;
	while (mark)
	{
		mark->set_pos(nr++);
		mark = mark->next;
	}
}

template <class Type> inline void List<Type>::update_pos_no()
{
	update_pos_no( first, 1);
}

template <class Type> inline BOOL List<Type>::exchange_members( Type *ex, Type *change )
{
	list_elem<Type>	*one, *two;
	BOOL result = FALSE;
	while (1){
        if (!ex || !change)
            break;

        if (last_asked_list_elem && last_asked_list_elem->elem == ex)
            one = last_asked_list_elem;
        else
            one = get_list_elem_with_member( ex );

        if (! one)
            break;

        if (last_asked_list_elem && last_asked_list_elem->elem == change)
            two = last_asked_list_elem;
        else
            two = get_list_elem_with_member( change );

        if (!two)
            break;

        one->set_elem(change);
        two->set_elem(ex);
        result = TRUE;
        break;
	}

	return result;
}

template <class Type> inline BOOL List<Type>::exchange_positions( positiontype ex, positiontype change )
{
	list_elem<Type>	*one, *two;
	Type		*dummy;
	BOOL 	result = FALSE;
	while(1){
        if (	ex < 1 || ex > no_of_members ||
                change < 1 || change > no_of_members)
            break;

        one = get_list_elem_at_pos( ex );
        if (! one)
            break;

        two = get_list_elem_at_pos( change );
        if (!two)
            break;

        dummy = one->elem;
        one->elem = two->elem;
        two->elem = dummy;
        result = TRUE;
        break;
	}
	return result;
}

template <class Type> inline positiontype List<Type>::get_pos_of_member( Type *object )
{
	list_elem<Type>	*elem;

	if (!object)
		return 0;

	if (last_asked_list_elem && last_asked_list_elem->elem == object)
		return last_asked_list_elem->get_pos();
	else
	{
		elem = first;
		while (elem && elem->elem != object)
			elem = elem->next;

		if (elem)
			return elem->get_pos();
		else
			return 0;
	}
}


template <class Type> inline positiontype List<Type>::insert_at_pos( Type *object, positiontype pos ) 	//returns pos inserted to
{
	list_elem<Type>	*elem,
        *new_elem;
    positiontype result ;

	elem = get_list_elem_at_pos( pos );

	if (! elem)
	{
		insert_as_last( object );
		result = last->get_pos();
	}
	else
	{
		new_elem = new list_elem<Type>( object );
		new_elem->set_prev(elem->get_prev());
		new_elem->set_next(elem);

		if (elem->get_prev())
			elem->get_prev()->set_next(new_elem);

		elem->set_prev(new_elem);

		if (first == elem)
			first = new_elem;

		update_pos_no ( new_elem, pos );

		result =  pos;
	}
	return result;
}

template <class Type> inline Type *List<Type>::get_member_at_pos( positiontype pos )
{
	list_elem<Type>	*elem;

	elem = get_list_elem_at_pos( pos );

	if (elem)
	{
		last_asked_list_elem = elem;
		return elem->elem;
	}
	else
		return NULL;
}

template <class Type> inline Type *List<Type>::get_member_at_pos_simple( positiontype pos )
{
	list_elem<Type>	*elem;

	elem = get_list_elem_at_pos_simple( pos );

	if (elem)
	{
		last_asked_list_elem = elem;
		return elem->elem;
	}
	else
		return NULL;
}

template <class Type> inline BOOL List<Type>::remove_pos_from_list( positiontype pos )
{
	list_elem<Type>	*loc_elem;
	BOOL result = FALSE;
	while(1){
        if (pos < 1 || pos > no_of_members)
            break;

        loc_elem = last_asked_list_elem;

        if (loc_elem && loc_elem->get_pos() == pos)
            last_asked_list_elem = NULL;
        else
        {
            if (! (loc_elem = get_list_elem_at_pos( pos )))
                break;
        }

        if (last == loc_elem)
            last = last->get_prev();

        if (first == loc_elem)


            first = first->next;

        delete loc_elem;
        no_of_members --;
        result = TRUE;
        break;
	}

	return result;
}

/**************************
Ende List
**************************/

#else
#error SoTl.hxx included twice
#endif
