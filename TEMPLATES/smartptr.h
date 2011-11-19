/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2003
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

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define tpl_assert(bed) arb_assert(bed)

// --------------------------------------------------------------------------------
//     SmartPointers
// --------------------------------------------------------------------------------
//
//  provides:
//
//  SmartPtr<type>                                                              uses delete
//  SmartPtr<type, Counted<type, auto_delete_array_ptr<type> > >                uses delete []
//  SmartPtr<type, Counted<type, auto_free_ptr<type> > >                        uses free
//  SmartPtr<type, Counted<type, custom_dealloc_ptr<type, deallocator> > >      uses custom deallocator
//
// --------------------------------------------------------------------------------
// macros for convinience:

#define SmartArrayPtr(type)               SmartPtr<type, Counted<type, auto_delete_array_ptr<type> > >
#define SmartMallocPtr(type)              SmartPtr<type, Counted<type, auto_free_ptr<type> > >
#define SmartCustomPtr(type, deallocator) SmartPtr<type, Counted<type, custom_dealloc_ptr<type, deallocator> > >

// --------------------------------------------------------------------------------
// examples:
//
// typedef SmartPtr<std::string> StringPtr;
// StringPtr s = new std::string("hello world");        // will be deallocated using delete
// 
// typedef SmartArrayPtr(std::string) StringArrayPtr;
// StringArrayPtr strings = new std::string[100];       // will be deallocated using delete []
// 
// typedef SmartMallocPtr(char) CharPtr;
// CharPtr cp = strdup("hello world");                  // will be deallocated using free()
// 
// typedef SmartCustomPtr(GEN_position, GEN_free_position) GEN_position_Ptr;
// GEN_position_Ptr gp = GEN_new_position(5, GB_FALSE); // will be deallocated using GEN_free_position()
// 
// --------------------------------------------------------------------------------


#ifdef NDEBUG
#ifdef DUMP_SMART_PTRS
#error Please do not define DUMP_SMART_PTRS in NDEBUG mode!
#endif
#endif

#ifdef DUMP_SMART_PTRS
#define DUMP_SMART_PTRS_DO(cmd) do { (cmd); } while(0)
#else
#define DUMP_SMART_PTRS_DO(cmd)
#endif

// -----------------------------------------------------------------
//      helper pointer classes
//
// used by Counted<> to use correct method to destroy the contents
// -----------------------------------------------------------------

template<class T, void (*DEALLOC)(T*)>
class custom_dealloc_ptr {
    T *const thePointer;
public:
    custom_dealloc_ptr(T *p) : thePointer(p) {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p now controlled by custom_dealloc_ptr\n", thePointer));
    }
    ~custom_dealloc_ptr() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets destroyed by custom_dealloc_ptr (using fun %p)\n", thePointer, DEALLOC));
        DEALLOC(thePointer);
    }

    const T* getPointer() const { return thePointer; }
    T* getPointer() { return thePointer; }
};

template <class T>
class auto_free_ptr {
    T *const thePointer;
public:
    auto_free_ptr(T *p) : thePointer(p) {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p now controlled by auto_free_ptr\n", thePointer));
    }
    ~auto_free_ptr() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets destroyed by auto_free_ptr\n", thePointer));
        free(thePointer);
    }

    const T* getPointer() const { return thePointer; }
    T* getPointer() { return thePointer; }
};

template <class T>
class auto_delete_ptr {
    T *const thePointer;
public:
    auto_delete_ptr(T *p) : thePointer(p) {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p now controlled by auto_delete_ptr\n", thePointer));
    }
    ~auto_delete_ptr() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets destroyed by auto_delete_ptr\n", thePointer));
        delete thePointer;
    }

    const T* getPointer() const { return thePointer; }
    T* getPointer() { return thePointer; }
};

template <class T>
class auto_delete_array_ptr {
    T *const thePointer;
public:
    auto_delete_array_ptr(T *p) : thePointer(p) {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p now controlled by auto_delete_array_ptr\n", thePointer));
    }
    ~auto_delete_array_ptr() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets destroyed by auto_delete_array_ptr\n", thePointer));
        delete [] thePointer;
    }

    const T* getPointer() const { return thePointer; }
    T* getPointer() { return thePointer; }
};

// -----------------------
//      class Counted
// -----------------------

template <class T, class C> class SmartPtr;

template <class T, class AP>
class Counted {
    unsigned counter;
    AP       pointer;

public:

    Counted(T *p) : counter(0), pointer(p) {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p now controlled by Counted\n", getPointer()));
        tpl_assert(p);
    }
#ifdef DEBUG
    ~Counted() {
        tpl_assert(counter==0);
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets destroyed by Counted\n", getPointer()));
    }
#endif

    unsigned new_reference() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p gets new reference (now there are %i references)\n", getPointer(), counter+1));
        return ++counter;
    }
    unsigned free_reference() {
        DUMP_SMART_PTRS_DO(fprintf(stderr, "pointer %p looses a reference (now there are %i references)\n", getPointer(), counter-1));
        tpl_assert(counter!=0);
        return --counter;
    }

    const T* getPointer() const { return pointer.getPointer(); }
    T* getPointer() { return pointer.getPointer(); }

    friend class SmartPtr<T, Counted<T, AP> >;
};


// --------------------------------------------------------------------------------
//     class SmartPtr
// --------------------------------------------------------------------------------
/** @memo Smart pointer class
     */

template <class T, class C = Counted<T, auto_delete_ptr<T> > >
class SmartPtr {
private:
    C *object;

    void Unbind() {
        if (object && object->free_reference()==0) {
            DUMP_SMART_PTRS_DO(fprintf(stderr, "Unbind() deletes Counted object %p (which hold pointer %p)\n", object, object->getPointer()));
            delete object;
        }
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
        object = new C(p);
        object->new_reference();
    }

    /** destroy SmartPtr

        object will not be destroyed as long as any other SmartPtr points to it
    */
    ~SmartPtr() { Unbind(); }

    SmartPtr(const SmartPtr<T, C>& other) {
        object = other.object;
        if (object) object->new_reference();
    }
    SmartPtr<T, C>& operator=(const SmartPtr<T, C>& other) {
        if (other.object) other.object->new_reference();
        Unbind();
        object = other.object;
        return *this;
    }

    const T *operator->() const { tpl_assert(object); return object->getPointer(); }
    T *operator->() { tpl_assert(object); return object->getPointer(); }

    const T& operator*() const { tpl_assert(object); return *(object->getPointer()); }
    T& operator*() { tpl_assert(object); return *(object->getPointer()); }

    /** test if SmartPtr is 0 */
    bool Null() const { return object==0; }

    /** set SmartPtr to 0 */
    void SetNull() { Unbind(); }

    /** create a deep copy of the object pointed to by the smart pointer.

        Afterwards there exist two equal copies of the object.

        @return SmartPtr to the new copy.
    */
    SmartPtr<T, C> deep_copy() const { return SmartPtr<T, C>(new T(**this)); }

    /** test if two SmartPtrs point to the same object
        (this is used for operators == and !=).

        if you like to compare the objects themselves use
        (*smart_ptr1 == *smart_ptr2)

        @return true if the SmartPtrs point to the same object
    */
    bool sameObject(const SmartPtr<T, C>& other) const {
        tpl_assert(object);
        tpl_assert(other.object);
        return object==other.object;
    }
};

template <class T, class C> bool operator==(const SmartPtr<T, C>& s1, const SmartPtr<T, C>& s2) {
    return s1.sameObject(s2);
}

template <class T, class C> bool operator!=(const SmartPtr<T, C>& s1, const SmartPtr<T, C>& s2) {
    return !s1.sameObject(s2);
}

#else
#error smartptr.h included twice
#endif // SMARTPTR_H
