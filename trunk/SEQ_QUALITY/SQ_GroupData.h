//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.h                                         //
//    Purpose   : We will see!                                           //
//    Time-stamp: <Wed Nov/26/2003 11:59 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - December 2003                       //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <iostream.h>

#ifndef SQ_GROUPADTA_H
#define SQ_GROUPDATA_H

#ifndef _CPP_CSTDDEF
#include <cstddef>
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)


class SQ_GroupData {
public:
    SQ_GroupData();
    virtual ~SQ_GroupData();
    //    SQ_GroupData(const SQ_GroupData& other) {}
    virtual SQ_GroupData& operator = (const SQ_GroupData& other) = 0;
    virtual SQ_GroupData *clone() const = 0;

    void         SQ_set_avg_bases(int bases) { avg_bases = bases; }
    int          SQ_get_avg_bases() const { return avg_bases; }
    bool         SQ_is_initialized() const { return initialized; }

    virtual void   SQ_init_consensus(int size)                     = 0;
    virtual int    SQ_print_on_screen()                            = 0;
    virtual double SQ_test_against_consensus(const char *sequence) = 0;
    virtual void   SQ_add_sequence(const char *sequence)           = 0;
    virtual void   SQ_add(const SQ_GroupData& other)               = 0;

protected:
    int  size;
    int  avg_bases;
    bool initialized;
};

template <int I>
class Int {
    Int(const Int& other);            // copying not allowed

public:
    int i[I];

    int size() const { return I; }

    Int() {
        for (int j=0; j<I; ++j)
            i[j] = 0;
    }

    Int& operator += (const Int& other) {
        for (int j = 0; j<I; ++j) {
            i[j] += other.i[j];
        }
        return *this;
    }
    Int& operator=(const Int& other) {
        for (int j = 0; j<I; ++j) {
            i[j] = other.i[j];
        }
        return *this;      
    }
};


template <int I>
class SQ_GroupData_Impl : public SQ_GroupData {
    SQ_GroupData_Impl(const SQ_GroupData_Impl& other);

public:
    SQ_GroupData_Impl() { consensus = 0; }
    virtual ~SQ_GroupData_Impl();

    SQ_GroupData_Impl& operator=(const SQ_GroupData_Impl& other) {
      seq_assert(other.size>0 && other.initialized);
      if (!initialized) SQ_init_consensus(other.size);
      seq_assert(size==other.size);

      avg_bases=other.avg_bases;

      for (int s=0; s<size; ++s) {
	consensus[s] = other.consensus[s];
      }
      return *this;
    }

    void SQ_init_consensus(int size);
    int  SQ_print_on_screen();
    void SQ_add_column(int col);

    void SQ_add(const SQ_GroupData& other); // add's other to this

protected:
    Int<I> *consensus;
};


class SQ_GroupData_RNA: public SQ_GroupData_Impl<7> {

    SQ_GroupData_RNA(const SQ_GroupData_RNA& other);            // copying not allowed
public:
    SQ_GroupData_RNA() {}

    SQ_GroupData_RNA *clone() const { return new SQ_GroupData_RNA; }
    SQ_GroupData_RNA& operator=(const SQ_GroupData& other) {
      return static_cast<SQ_GroupData_RNA&>(SQ_GroupData_Impl<7>::operator=(static_cast<const SQ_GroupData_Impl<7>& >(other)));
    }

    double SQ_test_against_consensus(const char *sequence);
    void SQ_add_sequence(const char *sequence);
};

class SQ_GroupData_PRO: public SQ_GroupData_Impl<20> {

    SQ_GroupData_PRO(const SQ_GroupData_PRO& other);            // copying not allowed
public:
    SQ_GroupData_PRO() {}

    SQ_GroupData_PRO *clone() const { return new SQ_GroupData_PRO; }
    SQ_GroupData_PRO& operator=(const SQ_GroupData& other) {
      return static_cast<SQ_GroupData_PRO&>(SQ_GroupData_Impl<20>::operator=(static_cast<const SQ_GroupData_Impl<20>& >(other)));
    }

    double SQ_test_against_consensus(const char *sequence);
    void   SQ_add_sequence(const char *sequence);
};



// -----------------------
//      implementation
// -----------------------

template <int I>
SQ_GroupData_Impl<I>::~SQ_GroupData_Impl(){
    delete [] consensus;
}

template <int I>
void SQ_GroupData_Impl<I>::SQ_init_consensus(int size_) {
    size        = size_;
    consensus   = new Int<I>[size];
    initialized = true;
}

template <int I>
void SQ_GroupData_Impl<I>::SQ_add(const SQ_GroupData& other_base) {
    const SQ_GroupData_Impl<I>& other = dynamic_cast<const SQ_GroupData_Impl<I>&>(other_base);
    seq_assert(size==other.size);
    for (int i = 0; i<size; ++i) {
        consensus[i] += other.consensus[i];
    }
}

template <int I>
int SQ_GroupData_Impl<I>::SQ_print_on_screen() {
    for (int i=0; i < size; i++ ){
	for (int j = 0; j<I; j++) {
	    cout << consensus[i].i[j];
//	    printf("%i",consensus[i].i[j]);
	}
    }
    return (0);
}


#else
#error SQ_GroupData.h included twice
#endif
