/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000
// Ralf Westram
// (Coded@ReallySoft.de)
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Ralf Westram makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
// This code is part of my library.
// You may find a more recent version at http://www.reallysoft.de/
//
/////////////////////////////////////////////////////////////////////////////

#ifndef SMARTPTR_H
#define SMARTPTR_H

#ifndef NDEBUG
# define tpl_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define tpl_assert(bed)
#endif

// --------------------------------------------------------------------------------
//     SmartPointers
// --------------------------------------------------------------------------------

template <class T> class SmartPtr;

template <class T>
// --------------------------------------------------------------------------------
//     class Counted
// --------------------------------------------------------------------------------
class Counted {
private:
    unsigned counter;
    T *const pointer;

    Counted(T *p) : counter(0), pointer(p) { tpl_assert(p); }
    ~Counted() { tpl_assert(counter==0); delete pointer; }

    unsigned new_reference() {
        //cout << "new reference to pointer" << int(pointer) << " (now there are " << counter+1 << " references)" << "\n";
        return ++counter;
    }
    unsigned free_reference() {
        //cout << "removing reference to pointer " << int(pointer) << " (now there are " << counter-1 << " references)\n";
        tpl_assert(counter!=0);
        return --counter;
    }

    friend class SmartPtr<T>;
};

template <class T>
// --------------------------------------------------------------------------------
//     class SmartPtr
// --------------------------------------------------------------------------------
/** @memo Smart pointer class
     */
class SmartPtr {
private:
    Counted<T> *object;

    void Unbind() {
        if (object && object->free_reference()==0) delete object;
        object = 0;
    }
public:
    /** build Smart-NULL-Ptr */
    SmartPtr() : object(0) {}

    /** build normal SmartPtr

        by passing an object to a SmartPtr you loose the responsibility over the object
        to the SmartPtr.
        So you NEVER should free such an object manually.

        @param p Pointer to any dynamically allocated object
    */
    SmartPtr(T *p) {
        object = new Counted<T>(p);
        object->new_reference();
    }

    /** destroy SmartPtr

        object will not be destroyed as long as any other SmartPtr points to it
    */
    ~SmartPtr() {Unbind(); }

    SmartPtr(const SmartPtr<T>& other) {
        object = other.object;
        if (object) object->new_reference();
    }
    SmartPtr<T>& operator=(const SmartPtr<T>& other) {
        if (other.object) other.object->new_reference();
        Unbind();
        object = other.object;
        return *this;
    }

    T *operator->() { tpl_assert(object); return object->pointer; }
    const T *operator->() const { tpl_assert(object); return object->pointer; }

    T& operator*() { tpl_assert(object); return *(object->pointer); }
    const T& operator*() const { tpl_assert(object); return *(object->pointer); }

    /** test if SmartPtr is 0 */
    bool Null() const { return object==0; }

    /** set SmartPtr to 0 */
    void SetNull() { Unbind(); }

    /** create a deep copy of the object pointed to by the smart pointer.

        Afterwards there exist two equal copies of the object.

        @return SmartPtr to the new copy.
    */
    SmartPtr<T> deep_copy() const { return SmartPtr<T>(new T(**this)); }

    /** test if two SmartPtrs point to the same object
        (this is used for operators == and !=).

        if you like to compare the objects themselves use
        (*smart_ptr1 == *smart_ptr2)

        @return true if the SmartPtrs point to the same object
    */
    bool sameObject(const SmartPtr<T>& other) const {
        tpl_assert(object);
        tpl_assert(other.object);
        return object==other.object;
    }
};

template <class T> bool operator==(const SmartPtr<T>& s1, const SmartPtr<T>& s2) {
    return s1.sameObject(s2);
}

template <class T> bool operator!=(const SmartPtr<T>& s1, const SmartPtr<T>& s2) {
    return !s1.sameObject(s2);
}


#else
#error smartptr.h included twice
#endif // SMARTPTR_H
