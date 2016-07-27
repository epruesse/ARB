//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.h                                         //
//    Purpose   : Classes to store global information about sequences    //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#ifndef SQ_GROUPDATA_H
#define SQ_GROUPDATA_H

#ifndef _GLIBCXX_CSTDDEF
# include <cstddef>
#endif
#ifndef _GLIBCXX_IOSTREAM
# include <iostream>
#endif
#ifndef ARB_ASSERT_H
# include <arb_assert.h>
#endif
#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif

#define seq_assert(bed) arb_assert(bed)

struct consensus_result {
    double conformity;
    double deviation;
};

class SQ_GroupData {
protected:
    int    size;
    int    avg_bases;
    int    nr_sequences;
    double gc_prop;
    bool   initialized;

public:
    SQ_GroupData();
    virtual ~SQ_GroupData();
    virtual SQ_GroupData& operator      = (const SQ_GroupData& other) = 0;
    virtual SQ_GroupData *clone() const = 0;

    void SQ_set_avg_bases(int bases) {
        avg_bases += bases;
    }
    int SQ_get_avg_bases() const {
        return (avg_bases/nr_sequences);
    }
    void SQ_set_avg_gc(double gc) {
        gc_prop += gc;
    }
    double SQ_get_avg_gc() const {
        return (gc_prop/nr_sequences);
    }
    void SQ_count_sequences() {
        nr_sequences++;
    }
    int SQ_get_nr_sequences() const {
        return nr_sequences;
    }
    bool SQ_is_initialized() const {
        return initialized;
    }

    virtual void SQ_init_consensus(int size)                               = 0;
    virtual int SQ_print_on_screen()                                       = 0;
    virtual consensus_result SQ_calc_consensus(const char *sequence) const = 0;
    virtual void SQ_add_sequence(const char *sequence)                     = 0;
    virtual void SQ_add(const SQ_GroupData& other)                         = 0;

    int getSize() const {
        return size;
    }

};

template <int I> class Int {
    Int(const Int& other); // copying not allowed

public:
    int i[I];

    int size() const {
        return I;
    }

    Int() {
        memset(i, 0, I*sizeof(int));
    }

    Int& operator += (const Int& other) {
        const int *otheri = other.i;
        for (int j = 0; j<I; ++j) {
            i[j] += otheri[j];
        }
        return *this;
    }

    Int& operator = (const Int& other) {
        memcpy(i, other.i, I*sizeof(int));
        return *this;
    }
};

template <int I> class SQ_GroupData_Impl : public SQ_GroupData {
    SQ_GroupData_Impl(const SQ_GroupData_Impl& other);
    SQ_GroupData_Impl& operator = (const SQ_GroupData_Impl& Other);

public:
    SQ_GroupData_Impl() {
        consensus = 0;
    }
    virtual ~SQ_GroupData_Impl() OVERRIDE;

    SQ_GroupData_Impl& operator = (const SQ_GroupData& Other) OVERRIDE {
        const SQ_GroupData_Impl& other = dynamic_cast<const SQ_GroupData_Impl&>(Other);
        seq_assert(other.size>0 && other.initialized);

        if (!initialized) SQ_init_consensus(other.size);
        seq_assert(size==other.size);

        avg_bases = other.avg_bases;
        gc_prop   = other.gc_prop;
        
        for (int s=0; s<size; ++s) {
            consensus[s] = other.consensus[s];
        }
        nr_sequences = other.nr_sequences;
        return *this;
    }

    void SQ_init_consensus(int size) OVERRIDE;
    int SQ_print_on_screen() OVERRIDE;
    void SQ_add(const SQ_GroupData& other) OVERRIDE; // add's other to this

protected:
    Int<I> *consensus;
};

class SQ_GroupData_RNA : public SQ_GroupData_Impl<6> {
    typedef SQ_GroupData_Impl<6> Base;
    SQ_GroupData_RNA(const SQ_GroupData_RNA& other); // copying not allowed
public:
    SQ_GroupData_RNA() {
    }

    SQ_GroupData_RNA *clone() const OVERRIDE {
        return new SQ_GroupData_RNA;
    }
    SQ_GroupData_RNA& operator = (const SQ_GroupData& other) OVERRIDE {
        Base::operator=(other);
        return *this;
    }

    consensus_result SQ_calc_consensus (const char *sequence) const OVERRIDE;
    void SQ_add_sequence (const char *sequence) OVERRIDE;
protected:
    static int class_counter;
};

class SQ_GroupData_PRO : public SQ_GroupData_Impl<20> {
    typedef SQ_GroupData_Impl<20> Base;
    SQ_GroupData_PRO(const SQ_GroupData_PRO& other); // copying not allowed
public:
    SQ_GroupData_PRO() {
    }

    SQ_GroupData_PRO *clone() const OVERRIDE {
        return new SQ_GroupData_PRO;
    }
    SQ_GroupData_PRO& operator = (const SQ_GroupData& other) OVERRIDE {
        Base::operator=(other);
        return *this;
    }

    consensus_result SQ_calc_consensus (const char *sequence) const OVERRIDE;
    void SQ_add_sequence (const char *sequence) OVERRIDE;
};

// -----------------------
//      implementation

template <int I> SQ_GroupData_Impl<I>::~SQ_GroupData_Impl() {
    delete [] consensus;
}

template <int I> void SQ_GroupData_Impl<I>::SQ_init_consensus(int size_) {
    seq_assert (!initialized);

    size = size_;
    consensus = new Int<I>[size];
    initialized = true;
}

template <int I> void SQ_GroupData_Impl<I>::SQ_add(
        const SQ_GroupData& other_base) {
    const SQ_GroupData_Impl<I>& other =
            dynamic_cast<const SQ_GroupData_Impl<I>&> (other_base);
    seq_assert (size==other.size);
    for (int i = 0; i<size; ++i) {
        consensus[i] += other.consensus[i];
    }
    nr_sequences+=other.nr_sequences;
    avg_bases+=other.avg_bases;
    gc_prop += other.gc_prop;
}

template <int I> int SQ_GroupData_Impl<I>::SQ_print_on_screen() {
    for (int i=0; i < size; i++) {
        for (int j = 0; j<I; j++) {
            std::cout << consensus[i].i[j];
        }
    }
    return (0);
}

#else
#error SQ_GroupData.h included twice
#endif
